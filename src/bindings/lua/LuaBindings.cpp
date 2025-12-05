#ifdef HYPRTOOLKIT_LUA_ENABLED

#include "LuaBindings.hpp"
#include "LuaState.hpp"

#include "helpers/SmartPtrAdapter.hpp"
#include "helpers/CallbackAdapter.hpp"
#include "helpers/ColorFnAdapter.hpp"

using namespace Hyprutils::Memory;

namespace Hyprtoolkit::Lua {

void registerAllBindings(sol::state& lua) {
    // 1. Basic types first (Vector2D, CBox, CHyprColor, etc.)
    registerTypes(lua);

    // 2. Core types (Backend, Timer, Output, Icons)
    registerCore(lua);

    // 3. Base element (IElement)
    registerElement(lua);

    // 4. All element builders and instances
    registerElementBuilders(lua);

    // 5. Window (depends on IElement)
    registerWindow(lua);
}

CSharedPointer<CLuaState> createLuaState() {
    auto state = makeShared<CLuaState>();
    state->openLibs();
    registerAllBindings(state->lua());
    return state;
}

} // namespace Hyprtoolkit::Lua

#endif // HYPRTOOLKIT_LUA_ENABLED
