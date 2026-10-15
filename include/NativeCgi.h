#pragma once
#include "ProfileMemory.h"
#include <mutex>
#include <stdexcept>
#include <memory>
#include <string_view>

namespace native_cgi {
// MSVC ABI objects are kept separate from this DLL's standard library objects.
struct NativeString { unsigned char storage[16]{}; uint64_t size = 0, capacity = 15; };
struct CgiOptions { uint32_t retries = 0, timeout = 30000; NativeString uri; uint8_t flag = 0; uint8_t pad[3]{}; uint32_t extra = 0; };
static_assert(sizeof(NativeString) == 32 && sizeof(CgiOptions) == 48);

class Native {
    uintptr_t base;
    struct Completion {
        HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        std::mutex mutex;
        uintptr_t base;
        int type = -1, code = -1;
        std::string wire, error;
        explicit Completion(uintptr_t value) : base(value) {
            if (!event) throw std::runtime_error("cannot create native completion event");
        }
        ~Completion() { CloseHandle(event); }
    };
    // MSVC std::function implementation: copy, move, invoke, type, delete, target.
    // The native scene owns this heap object after taking storage.impl.
    struct Callback {
        const uintptr_t* table;
        std::shared_ptr<Completion> state;
        explicit Callback(std::shared_ptr<Completion> value) : table(Table()), state(std::move(value)) {}
        static Callback* Copy(Callback* self, void*) { return new Callback(self->state); }
        static bool Invoke(Callback* self, void** scene, int* type, int* code) noexcept {
            auto state = self->state;
            try {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->type = *type; state->code = *code;
                if (!*type && !*code && scene && *scene) {
                    auto vtable = *reinterpret_cast<uintptr_t**>(*scene);
                    void* response = reinterpret_cast<void*(*)(void*)>(vtable[4])(*scene);
                    if (response) state->wire = Native(state->base).Serialize(response);
                    else state->error = "native response is empty";
                }
            } catch (...) { state->error = "native response serialization failed"; }
            SetEvent(state->event); return true;
        }
        static const void* Type(Callback*) { return nullptr; }
        static void Delete(Callback* self, bool deallocate) {
            if (deallocate) delete self; else self->~Callback();
        }
        static void* Target(Callback*, const void*) { return nullptr; }
        static const uintptr_t* Table() {
            static const uintptr_t table[] = {
                reinterpret_cast<uintptr_t>(&Copy), reinterpret_cast<uintptr_t>(&Copy),
                reinterpret_cast<uintptr_t>(&Invoke), reinterpret_cast<uintptr_t>(&Type),
                reinterpret_cast<uintptr_t>(&Delete), reinterpret_cast<uintptr_t>(&Target)};
            return table;
        }
    };
    struct FunctionStorage { unsigned char bytes[56]{}; Callback* impl = nullptr; };
    static_assert(sizeof(FunctionStorage) == 64);
public:
    explicit Native(uintptr_t value) : base(value) {}
    template<class T> T Function(uintptr_t rva) const { return reinterpret_cast<T>(base + rva); }
    void Free(NativeString& text) const {
        if (text.capacity >= 16) {
            void* memory = nullptr; std::memcpy(&memory, text.storage, sizeof(memory));
            size_t allocation = text.capacity + 1;
            if (allocation >= 4096) { allocation += 39; memory = *(reinterpret_cast<void**>(memory) - 1); }
            Function<void(*)(void*, size_t)>(0x74AB8E0)(memory, allocation);
        }
        text = {};
    }
    std::string Value(const NativeString& text) const {
        if (text.size > 1024 * 1024 || text.capacity < text.size) throw std::runtime_error("invalid native response string");
        const char* memory = reinterpret_cast<const char*>(text.storage);
        if (text.capacity >= 16) std::memcpy(&memory, text.storage, sizeof(memory));
        return std::string(memory, static_cast<size_t>(text.size));
    }
    void Destroy(void* object) const {
        if (object) reinterpret_cast<void(*)(void*, unsigned)>((*reinterpret_cast<uintptr_t**>(object))[0])(object, 1);
    }
    std::string Serialize(void* message) const {
        NativeString out;
        Function<void*(*)(void*, NativeString*)>(0x681AD0)(message, &out);
        try { auto value = Value(out); Free(out); return value; }
        catch (...) { Free(out); throw; }
    }
    bool Cgi(uintptr_t ctor, size_t size, uintptr_t bridge, std::string_view wire,
             std::string& response, std::string& error, bool* submitted = nullptr) const {
        void* request = Function<void*(*)(size_t)>(0x74AB89C)(size);
        Function<void*(*)(void*)>(ctor)(request);
        CgiOptions options;
        auto completion = std::make_shared<Completion>(base);
        FunctionStorage callback;
        try {
            // The native scene adds BaseRequest after copying GetProfileRequest.
            // Match native callers: parse fields without requiring BaseRequest yet.
            NativeString input; input.size = wire.size();
            if (wire.size() <= 15) std::memcpy(input.storage, wire.data(), wire.size());
            else { auto data = wire.data(); input.capacity = wire.size() | 15; std::memcpy(input.storage, &data, sizeof(data)); }
            if (!Function<bool(*)(void*, NativeString*)>(0x681480)(request, &input)) {
                Destroy(request); error = "native protobuf rejected request"; return false;
            }
            uintptr_t manager = 0, service = 0, table = 0;
            if (!profile_memory::ReadValue(GetCurrentProcess(), base + profile_memory::manager_rva, manager) ||
                !profile_memory::ReadValue(GetCurrentProcess(), manager + 0x2C8, service) ||
                !profile_memory::ReadValue(GetCurrentProcess(), service, table) || table != base + 0x8DFA208) {
                Destroy(request); error = "native CGI service is unavailable"; return false;
            }
            NativeString empty;
            // A timeout does not cancel native ownership. Keep callback code loaded
            // until process exit, including completions arriving after this call.
            HMODULE module = nullptr;
            if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                reinterpret_cast<LPCWSTR>(&Callback::Invoke), &module)) {
                Destroy(request); error = "cannot retain native callback module"; return false;
            }
            callback.impl = new Callback(completion);
            // Submit directly through the mutex-protected scene service. Coroutine
            // wrappers require a native coroutine context and cannot run here.
            if (submitted) *submitted = true;
            Function<int(*)(void*, void*, NativeString*, CgiOptions*, FunctionStorage*)>(bridge)
                (reinterpret_cast<void*>(service), request, &empty, &options, &callback);
            Destroy(request); request = nullptr;
            if (WaitForSingleObject(completion->event, 30000) != WAIT_OBJECT_0) {
                error = "native CGI completion timed out; outcome is unknown"; return false;
            }
            std::lock_guard<std::mutex> lock(completion->mutex);
            bool ok = !completion->type && !completion->code && completion->error.empty() && !completion->wire.empty();
            if (ok) response = completion->wire;
            else error = "native CGI failed: type=" + std::to_string(completion->type) + ", code=" + std::to_string(completion->code) + "; " + completion->error;
            return ok;
        } catch (...) { Destroy(request); if (callback.impl) delete callback.impl; throw; }
    }
};
}
