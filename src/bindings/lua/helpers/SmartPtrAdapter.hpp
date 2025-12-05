#pragma once

#ifdef HYPRTOOLKIT_LUA_ENABLED

#include <sol/sol.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include <hyprutils/memory/Atomic.hpp>

namespace sol {

// Tell sol3 that CSharedPointer is a smart pointer
template <typename T>
struct unique_usertype_traits<Hyprutils::Memory::CSharedPointer<T>> {
    using type        = T;
    using actual_type = Hyprutils::Memory::CSharedPointer<T>;

    static const bool value = true;

    static bool is_null(const actual_type& ptr) {
        return !ptr;
    }

    static type* get(const actual_type& ptr) {
        return ptr.get();
    }
};

// Tell sol3 that CWeakPointer is a smart pointer (will auto-lock)
template <typename T>
struct unique_usertype_traits<Hyprutils::Memory::CWeakPointer<T>> {
    using type        = T;
    using actual_type = Hyprutils::Memory::CWeakPointer<T>;

    static const bool value = true;

    static bool is_null(const actual_type& ptr) {
        return ptr.expired();
    }

    static type* get(const actual_type& ptr) {
        auto locked = ptr.lock();
        return locked ? locked.get() : nullptr;
    }
};

// Tell sol3 that CAtomicSharedPointer is a smart pointer
template <typename T>
struct unique_usertype_traits<Hyprutils::Memory::CAtomicSharedPointer<T>> {
    using type        = T;
    using actual_type = Hyprutils::Memory::CAtomicSharedPointer<T>;

    static const bool value = true;

    static bool is_null(const actual_type& ptr) {
        return !ptr;
    }

    static type* get(const actual_type& ptr) {
        return ptr.get();
    }
};

} // namespace sol

#endif // HYPRTOOLKIT_LUA_ENABLED
