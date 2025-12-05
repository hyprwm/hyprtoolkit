#pragma once

#ifdef HYPRTOOLKIT_LUA_ENABLED

#include <sol/sol.hpp>
#include <functional>
#include <string>

namespace Hyprtoolkit::Lua {

// Convert a Lua function to a std::function with error handling
template <typename Ret, typename... Args>
std::function<Ret(Args...)> luaToCallback(sol::protected_function fn) {
    return [fn](Args... args) -> Ret {
        sol::protected_function_result result = fn(args...);
        if (!result.valid()) {
            sol::error err = result;
            // TODO: integrate with hyprtoolkit logging
            fprintf(stderr, "[Lua] Callback error: %s\n", err.what());
            if constexpr (!std::is_void_v<Ret>) {
                return Ret{};
            }
        } else {
            if constexpr (!std::is_void_v<Ret>) {
                return result.get<Ret>();
            }
        }
    };
}

// Specialization for void return type
template <typename... Args>
std::function<void(Args...)> luaToVoidCallback(sol::protected_function fn) {
    return [fn](Args... args) {
        sol::protected_function_result result = fn(args...);
        if (!result.valid()) {
            sol::error err = result;
            fprintf(stderr, "[Lua] Callback error: %s\n", err.what());
        }
    };
}

// Safe callback wrapper that captures the Lua function
template <typename... Args>
auto makeSafeCallback(sol::function fn) {
    return [fn](Args... args) {
        sol::protected_function pfn = fn;
        sol::protected_function_result result = pfn(args...);
        if (!result.valid()) {
            sol::error err = result;
            fprintf(stderr, "[Lua] Callback error: %s\n", err.what());
        }
    };
}

} // namespace Hyprtoolkit::Lua

#endif // HYPRTOOLKIT_LUA_ENABLED
