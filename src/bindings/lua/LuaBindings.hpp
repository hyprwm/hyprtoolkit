#pragma once

#ifdef HYPRTOOLKIT_LUA_ENABLED

#include <sol/sol.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <string>

#include "LuaState.hpp"

namespace Hyprtoolkit::Lua {

// Forward declarations for registration functions
void registerTypes(sol::state& lua);
void registerCore(sol::state& lua);
void registerElement(sol::state& lua);
void registerElementBuilders(sol::state& lua);
void registerWindow(sol::state& lua);

// Register all hyprtoolkit bindings to an existing sol::state
void registerAllBindings(sol::state& lua);

// Create a new Lua state with all hyprtoolkit bindings registered
Hyprutils::Memory::CSharedPointer<CLuaState> createLuaState();

} // namespace Hyprtoolkit::Lua

#endif // HYPRTOOLKIT_LUA_ENABLED
