"""Offline bounded survey for a native collection QR implementation.

Reads installed artifacts only. Does not open a process or send business traffic.
Example: python tools/collection_qr_survey.py --module-dir <installed-version-dir>
"""
import argparse
import hashlib
import json
import pathlib
import struct

PATTERNS = (
    "busif2f", "f2fget", "f2fqrcode", "collectqrcode", "collect_qrcode",
    "receiverqrcode", "payapp.weixin.qq.com", "transferbank", "getpay",
    "getreceipt", "tenpay", "wallet", "收款",
)


def pe_identity(data):
    """Return checked PE metadata and section ranges, without loading a DLL."""
    if data[:2] != b"MZ" or len(data) < 64:
        return None, []
    off = struct.unpack_from("<I", data, 60)[0]
    if off + 24 > len(data) or data[off:off + 4] != b"PE\0\0":
        return None, []
    machine, count, timestamp = struct.unpack_from("<HHI", data, off + 4)
    optional_size = struct.unpack_from("<H", data, off + 20)[0]
    optional = off + 24
    if optional_size < 60 or optional + optional_size + count * 40 > len(data):
        return None, []
    image_size = struct.unpack_from("<I", data, optional + 56)[0]
    sections = []
    for index in range(count):
        start = optional + optional_size + index * 40
        virtual_size, rva, raw_size, raw = struct.unpack_from("<IIII", data, start + 8)
        sections.append((raw, raw_size, rva))
    return {"machine": hex(machine), "timestamp": hex(timestamp),
            "image_size": hex(image_size)}, sections


def survey(path, limit):
    data = path.read_bytes()
    identity, sections = pe_identity(data)
    result = {"file": path.name, "size": len(data), "pe": identity,
              "sha256": hashlib.sha256(data).hexdigest(), "searches": []}
    # Case-sensitive Unicode terms; ASCII byte folding is only used for ASCII tokens.
    lower = data.lower()
    for token in PATTERNS:
        for encoding in ("utf-8", "utf-16le"):
            needle = token.encode(encoding)
            haystack = lower if token.isascii() else data
            if token.isascii():
                needle = needle.lower()
            pos, total, matches = 0, 0, []
            while True:
                at = haystack.find(needle, pos)
                if at < 0:
                    break
                total += 1
                if len(matches) < limit:
                    rva = next((base + at - raw for raw, size, base in sections
                                if raw <= at < raw + size), None)
                    matches.append({"offset": hex(at), "rva": hex(rva) if rva is not None else None})
                pos = at + len(needle)
            result["searches"].append({"pattern": token, "encoding": encoding,
                                      "count": total, "locations": matches,
                                      "locations_truncated": total > len(matches)})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--module-dir", type=pathlib.Path,
                        default=pathlib.Path(r"C:\Program Files\Tencent\Weixin\4.1.15.55"))
    parser.add_argument("--limit", type=int, default=25)
    args = parser.parse_args()
    if not args.module_dir.is_dir():
        parser.error("module directory does not exist")
    if args.limit < 1 or args.limit > 1000:
        parser.error("limit must be between 1 and 1000")
    for path in sorted(args.module_dir.iterdir()):
        if path.is_file() and path.suffix.lower() in (".dll", ".exe", ".bin"):
            print(json.dumps(survey(path, args.limit), ensure_ascii=True))


if __name__ == "__main__":
    main()
