#pragma once

#ifdef HYPRTOOLKIT_LUA_ENABLED

#include <sol/sol.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <string>

namespace Hyprtoolkit::Lua {

class CLuaState {
  public:
    CLuaState();
    ~CLuaState();

    // Non-copyable
    CLuaState(const CLuaState&)            = delete;
    CLuaState& operator=(const CLuaState&) = delete;

    // Movable
    CLuaState(CLuaState&&)            = default;
    CLuaState& operator=(CLuaState&&) = default;

    // Access the underlying sol::state
    sol::state&       lua();
    const sol::state& lua() const;

    // Open standard Lua libraries
    void openLibs();

    // Execute a Lua script file
    sol::protected_function_result doFile(const std::string& path);

    // Execute a Lua string
    sol::protected_function_result doString(const std::string& code);

    // Get a global variable
    template <typename T>
    T get(const std::string& name) {
        return m_lua[name].get<T>();
    }

    // Set a global variable
    template <typename T>
    void set(const std::string& name, T&& value) {
        m_lua[name] = std::forward<T>(value);
    }

    // Check if a global exists
    bool has(const std::string& name) const;

  private:
    sol::state m_lua;
};

} // namespace Hyprtoolkit::Lua

#endif // HYPRTOOLKIT_LUA_ENABLED
