#pragma once

#ifdef HYPRTOOLKIT_LUA_ENABLED

#include <sol/sol.hpp>
#include <hyprtoolkit/palette/Color.hpp>
#include <functional>

namespace Hyprtoolkit::Lua {

using colorFn = std::function<CHyprColor()>;

// Convert a Lua object (either a CHyprColor or a function) to colorFn
inline colorFn luaToColorFn(sol::object obj) {
    if (obj.is<CHyprColor>()) {
        // Static color - capture by value
        CHyprColor color = obj.as<CHyprColor>();
        return [color]() { return color; };
    } else if (obj.is<sol::function>()) {
        // Dynamic color function
        sol::protected_function fn = obj.as<sol::function>();
        return [fn]() -> CHyprColor {
            sol::protected_function_result result = fn();
            if (result.valid()) {
                return result.get<CHyprColor>();
            }
            // Return black on error
            fprintf(stderr, "[Lua] colorFn error: %s\n", sol::error(result).what());
            return CHyprColor(0.0, 0.0, 0.0, 1.0);
        };
    }
    // Default to black
    return []() { return CHyprColor(0.0, 0.0, 0.0, 1.0); };
}

// Convert a Lua object to an optional colorFn (returns nullopt if nil)
inline std::optional<colorFn> luaToOptionalColorFn(sol::object obj) {
    if (obj.is<sol::nil_t>() || !obj.valid()) {
        return std::nullopt;
    }
    return luaToColorFn(obj);
}

} // namespace Hyprtoolkit::Lua

#endif // HYPRTOOLKIT_LUA_ENABLED
