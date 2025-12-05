#ifdef HYPRTOOLKIT_LUA_ENABLED

#include "LuaState.hpp"

namespace Hyprtoolkit::Lua {

CLuaState::CLuaState() = default;

CLuaState::~CLuaState() = default;

sol::state& CLuaState::lua() {
    return m_lua;
}

const sol::state& CLuaState::lua() const {
    return m_lua;
}

void CLuaState::openLibs() {
    m_lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine, sol::lib::string, sol::lib::os, sol::lib::math, sol::lib::table, sol::lib::io);
}

sol::protected_function_result CLuaState::doFile(const std::string& path) {
    return m_lua.safe_script_file(path, sol::script_pass_on_error);
}

sol::protected_function_result CLuaState::doString(const std::string& code) {
    return m_lua.safe_script(code, sol::script_pass_on_error);
}

bool CLuaState::has(const std::string& name) const {
    return m_lua[name].valid();
}

} // namespace Hyprtoolkit::Lua

#endif // HYPRTOOLKIT_LUA_ENABLED
