# GetSelfProfile native memory reader

Target: the locally loaded Windows x64 Weixin.dll 4.1.15.13.
PE timestamp: `0x6AB0A378`; SizeOfImage: `0xC0AE000`.

Pointer chain: `manager = *(module + 0xB912690)`;
`account = *(manager + 0x68)`.
Expected account vtable: `module + 0x8DFE2F8`.

| Field | Account-relative offset | Evidence |
|---|---|---|
| Login flag | 0x38, bit 0 | Vtable getter at RVA 0x373DC0 |
| wxid | 0x48 | Getter at RVA 0x36F930; live value matches diagnostic baseline |
| alias | 0x68 | Getter at RVA 0x36F940; live value matches diagnostic baseline |
| nickname | 0x88 | Adjacent string object matches diagnostic baseline |
| phone | 0xA8 | Adjacent string object contains an 11-digit number; independently verify against account settings |

Strings use a 32-byte MSVC layout: 16-byte inline storage/pointer,
64-bit byte length at +0x10 and capacity at +0x18. Capacity 15 is inline;
larger capacities use a heap pointer. Content must be valid UTF-8.

The production endpoint never queries databases, parses account directories,
scans memory, or uses the global SelfInfo cache. The diagnostic baseline was
used only during pointer discovery. Every request validates image identity,
vtable, login flag, string metadata and a second account snapshot. This is
best-effort consistency checking, not synchronization with Weixin internals.

Email, QQ, province, city and signature are not mapped: currently empty
candidate objects do not prove field semantics. Existing response fields are
retained for compatibility, with `unavailable_fields` explicitly naming them.
`ret=-1` reports invalid build, unavailable account, invalid fields or a changing
account. No personal values are recorded in this document or test logs.

The standalone `tools/profile_memory_test.cpp` exercises the exact production
reader with synthetic memory, or accepts a PID and module base to read the live
process. It reports lengths only. It does not inject, patch or modify Weixin.

## Verification on 2026-10-08

The C++ reader was compiled as a standalone Windows x64 executable. Synthetic
checks passed for valid inline/heap strings, Chinese UTF-8, logged-out state,
wrong build, wrong vtable, invalid UTF-8, oversized strings, unreadable heap
storage, empty wxid and null account. Failure outputs were confirmed cleared.
The same executable successfully read the running Weixin process, with byte
lengths 19 (wxid), 14 (alias), 6 (nickname) and 11 (phone). This verifies the
shared production reader, not a rebuilt/reloaded HTTP DLL. Account switching
and logout/relogin have not been exercised against the live process.
The updated GetSelfProfile.cpp also compiled successfully to a Windows x64
object using Clang via Zig; only existing third-party literal-operator warnings
were emitted. The complete MSVC solution has not been built or deployed.
