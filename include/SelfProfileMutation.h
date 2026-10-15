#pragma once
#include <mutex>
// Shared by complete-profile writers inside this DLL; does not lock other modules.
inline std::mutex& SelfProfileMutationMutex() {
    static std::mutex mutex;
    return mutex;
}
