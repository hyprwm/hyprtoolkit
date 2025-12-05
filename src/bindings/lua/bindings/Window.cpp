#ifdef HYPRTOOLKIT_LUA_ENABLED

#include <sol/sol.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Element.hpp>
#include <hyprtoolkit/core/Output.hpp>

#include "../helpers/SmartPtrAdapter.hpp"
#include "../helpers/CallbackAdapter.hpp"

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;

namespace Hyprtoolkit::Lua {

void registerWindow(sol::state& lua) {
    // Window type enum
    lua.new_enum<eWindowType>("WindowType",
        {
            {"TOPLEVEL", HT_WINDOW_TOPLEVEL},
            {"POPUP", HT_WINDOW_POPUP},
            {"LAYER", HT_WINDOW_LAYER},
            {"LOCK_SURFACE", HT_WINDOW_LOCK_SURFACE}
        }
    );

    // Window builder
    lua.new_usertype<CWindowBuilder>("CWindowBuilder",
        sol::no_constructor,
        "begin", &CWindowBuilder::begin,
        "type", &CWindowBuilder::type,
        "appTitle", [](CSharedPointer<CWindowBuilder> self, const std::string& t) {
            return self->appTitle(std::string(t));
        },
        "appClass", [](CSharedPointer<CWindowBuilder> self, const std::string& c) {
            return self->appClass(std::string(c));
        },
        "preferredSize", &CWindowBuilder::preferredSize,
        "minSize", &CWindowBuilder::minSize,
        "maxSize", &CWindowBuilder::maxSize,

        // Layer and lock surface options
        "prefferedOutput", &CWindowBuilder::prefferedOutput,
        "marginTopLeft", &CWindowBuilder::marginTopLeft,
        "marginBottomRight", &CWindowBuilder::marginBottomRight,
        "layer", &CWindowBuilder::layer,
        "anchor", &CWindowBuilder::anchor,
        "exclusiveEdge", &CWindowBuilder::exclusiveEdge,
        "exclusiveZone", &CWindowBuilder::exclusiveZone,
        "kbInteractive", &CWindowBuilder::kbInteractive,

        // Popup options
        "parent", &CWindowBuilder::parent,
        "pos", &CWindowBuilder::pos,

        "commence", &CWindowBuilder::commence
    );

    // IWindow interface
    lua.new_usertype<IWindow>("IWindow",
        sol::no_constructor,
        "pixelSize", &IWindow::pixelSize,
        "scale", &IWindow::scale,
        "close", &IWindow::close,
        "open", &IWindow::open,
        "cursorPos", &IWindow::cursorPos,
        "m_rootElement", &IWindow::m_rootElement,

        // Event listeners - we expose them as methods that accept callbacks
        "onResized", [](IWindow* self, sol::function fn) {
            self->m_events.resized.listenStatic([fn](Vector2D size) {
                sol::protected_function_result result = fn(size);
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Window resized callback error: %s\n", err.what());
                }
            });
        },
        "onCloseRequest", [](IWindow* self, sol::function fn) {
            self->m_events.closeRequest.listenStatic([fn]() {
                sol::protected_function_result result = fn();
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Window closeRequest callback error: %s\n", err.what());
                }
            });
        },
        "onPopupClosed", [](IWindow* self, sol::function fn) {
            self->m_events.popupClosed.listenStatic([fn]() {
                sol::protected_function_result result = fn();
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Window popupClosed callback error: %s\n", err.what());
                }
            });
        },
        "onLayerClosed", [](IWindow* self, sol::function fn) {
            self->m_events.layerClosed.listenStatic([fn]() {
                sol::protected_function_result result = fn();
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Window layerClosed callback error: %s\n", err.what());
                }
            });
        },
        "onKeyboardKey", [](IWindow* self, sol::function fn) {
            self->m_events.keyboardKey.listenStatic([fn](Input::SKeyboardKeyEvent event) {
                sol::protected_function_result result = fn(event);
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Window keyboardKey callback error: %s\n", err.what());
                }
            });
        }
    );
}

} // namespace Hyprtoolkit::Lua

#endif // HYPRTOOLKIT_LUA_ENABLED
