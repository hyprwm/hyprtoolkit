#ifdef HYPRTOOLKIT_LUA_ENABLED

#include <sol/sol.hpp>
#include <hyprtoolkit/element/Element.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/element/Checkbox.hpp>
#include <hyprtoolkit/element/Slider.hpp>
#include <hyprtoolkit/element/Combobox.hpp>
#include <hyprtoolkit/element/Spinbox.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/Line.hpp>
#include <hyprtoolkit/core/Input.hpp>

#include "../helpers/SmartPtrAdapter.hpp"
#include "../helpers/CallbackAdapter.hpp"

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;

namespace Hyprtoolkit::Lua {

void registerElement(sol::state& lua) {
    // Position mode enum
    lua.new_enum<IElement::ePositionMode>("PositionMode",
        {
            {"ABSOLUTE", IElement::HT_POSITION_ABSOLUTE},
            {"AUTO", IElement::HT_POSITION_AUTO}
        }
    );

    // Position flag enum
    lua.new_enum<IElement::ePositionFlag>("PositionFlag",
        {
            {"HCENTER", IElement::HT_POSITION_FLAG_HCENTER},
            {"VCENTER", IElement::HT_POSITION_FLAG_VCENTER},
            {"CENTER", IElement::HT_POSITION_FLAG_CENTER},
            {"LEFT", IElement::HT_POSITION_FLAG_LEFT},
            {"RIGHT", IElement::HT_POSITION_FLAG_RIGHT},
            {"TOP", IElement::HT_POSITION_FLAG_TOP},
            {"BOTTOM", IElement::HT_POSITION_FLAG_BOTTOM},
            {"ALL", IElement::HT_POSITION_FLAG_ALL}
        }
    );

    lua.new_usertype<IElement>("IElement",
        sol::no_constructor,

        // Size and position
        "size", &IElement::size,
        "posFromParent", &IElement::posFromParent,
        "reposition", &IElement::reposition,
        "forceReposition", &IElement::forceReposition,

        // Position mode and flags
        "setPositionMode", &IElement::setPositionMode,
        "setPositionFlag", &IElement::setPositionFlag,
        "setAbsolutePosition", &IElement::setAbsolutePosition,

        // Child management - use lambda to handle derived element types
        "addChild", [](IElement* self, sol::object child) {
            // Try to extract as CSharedPointer<IElement>
            if (child.is<CSharedPointer<IElement>>()) {
                self->addChild(child.as<CSharedPointer<IElement>>());
                return;
            }
            // Try common element types that inherit from IElement
            // The sol::bases doesn't automatically convert smart pointers
            #define TRY_ELEMENT_TYPE(T) \
                if (child.is<CSharedPointer<T>>()) { \
                    auto ptr = child.as<CSharedPointer<T>>(); \
                    self->addChild(CSharedPointer<IElement>(ptr)); \
                    return; \
                }

            // All element types need to be listed here for proper conversion
            TRY_ELEMENT_TYPE(CTextElement)
            TRY_ELEMENT_TYPE(CButtonElement)
            TRY_ELEMENT_TYPE(CTextboxElement)
            TRY_ELEMENT_TYPE(CCheckboxElement)
            TRY_ELEMENT_TYPE(CSliderElement)
            TRY_ELEMENT_TYPE(CComboboxElement)
            TRY_ELEMENT_TYPE(CSpinboxElement)
            TRY_ELEMENT_TYPE(CRectangleElement)
            TRY_ELEMENT_TYPE(CColumnLayoutElement)
            TRY_ELEMENT_TYPE(CRowLayoutElement)
            TRY_ELEMENT_TYPE(CScrollAreaElement)
            TRY_ELEMENT_TYPE(CImageElement)
            TRY_ELEMENT_TYPE(CNullElement)
            TRY_ELEMENT_TYPE(CLineElement)

            #undef TRY_ELEMENT_TYPE

            throw std::runtime_error("addChild: argument is not a valid element type");
        },
        "removeChild", &IElement::removeChild,
        "clearChildren", &IElement::clearChildren,

        // Styling
        "setMargin", &IElement::setMargin,
        "setGrouped", &IElement::setGrouped,
        "setTooltip", [](IElement* self, const std::string& tooltip) {
            self->setTooltip(std::string(tooltip));
        },

        // Growth
        "setGrow", sol::overload(
            static_cast<void (IElement::*)(bool)>(&IElement::setGrow),
            static_cast<void (IElement::*)(bool, bool)>(&IElement::setGrow)
        ),

        // Mouse input
        "setReceivesMouse", &IElement::setReceivesMouse,

        "setMouseEnter", [](IElement* self, sol::function fn) {
            self->setMouseEnter([fn](const Vector2D& pos) {
                sol::protected_function_result result = fn(pos);
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] mouseEnter callback error: %s\n", err.what());
                }
            });
        },

        "setMouseLeave", [](IElement* self, sol::function fn) {
            self->setMouseLeave([fn]() {
                sol::protected_function_result result = fn();
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] mouseLeave callback error: %s\n", err.what());
                }
            });
        },

        "setMouseMove", [](IElement* self, sol::function fn) {
            self->setMouseMove([fn](const Vector2D& pos) {
                sol::protected_function_result result = fn(pos);
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] mouseMove callback error: %s\n", err.what());
                }
            });
        },

        "setMouseButton", [](IElement* self, sol::function fn) {
            self->setMouseButton([fn](Input::eMouseButton button, bool pressed) {
                sol::protected_function_result result = fn(button, pressed);
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] mouseButton callback error: %s\n", err.what());
                }
            });
        },

        "setMouseAxis", [](IElement* self, sol::function fn) {
            self->setMouseAxis([fn](Input::eAxisAxis axis, float delta) {
                sol::protected_function_result result = fn(axis, delta);
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] mouseAxis callback error: %s\n", err.what());
                }
            });
        },

        "setRepositioned", [](IElement* self, sol::function fn) {
            self->setRepositioned([fn]() {
                sol::protected_function_result result = fn();
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] repositioned callback error: %s\n", err.what());
                }
            });
        }
    );
}

} // namespace Hyprtoolkit::Lua

#endif // HYPRTOOLKIT_LUA_ENABLED
