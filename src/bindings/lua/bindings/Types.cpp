#ifdef HYPRTOOLKIT_LUA_ENABLED

#include <sol/sol.hpp>
#include <hyprutils/math/Vector2D.hpp>
#include <hyprutils/math/Box.hpp>
#include <hyprtoolkit/palette/Color.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/types/SizeType.hpp>
#include <hyprtoolkit/types/FontTypes.hpp>
#include <hyprtoolkit/core/Input.hpp>

#include "../helpers/SmartPtrAdapter.hpp"

using namespace Hyprutils::Math;
using namespace Hyprutils::Memory;

namespace Hyprtoolkit::Lua {

void registerVector2D(sol::state& lua) {
    lua.new_usertype<Vector2D>("Vector2D",
        sol::constructors<Vector2D(), Vector2D(double, double)>(),
        "x", &Vector2D::x,
        "y", &Vector2D::y,
        "floor", &Vector2D::floor,
        "round", &Vector2D::round,
        "clamp", &Vector2D::clamp,
        "distance", &Vector2D::distance,
        sol::meta_function::addition, static_cast<Vector2D (Vector2D::*)(const Vector2D&) const>(&Vector2D::operator+),
        sol::meta_function::subtraction, static_cast<Vector2D (Vector2D::*)(const Vector2D&) const>(&Vector2D::operator-),
        sol::meta_function::multiplication, sol::overload(
            [](const Vector2D& self, double scalar) { return self * scalar; },
            static_cast<Vector2D (Vector2D::*)(const Vector2D&) const>(&Vector2D::operator*)
        ),
        sol::meta_function::division, [](const Vector2D& self, double scalar) { return self / scalar; },
        sol::meta_function::equal_to, &Vector2D::operator==,
        sol::meta_function::to_string, [](const Vector2D& v) {
            return "Vector2D(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
        }
    );

    // Convenience constructor
    lua["Vector2D"]["new"] = [](double x, double y) { return Vector2D{x, y}; };
}

void registerBox(sol::state& lua) {
    lua.new_usertype<CBox>("CBox",
        sol::constructors<CBox(), CBox(double, double, double, double), CBox(const Vector2D&, const Vector2D&)>(),
        "x", &CBox::x,
        "y", &CBox::y,
        "w", &CBox::w,
        "h", &CBox::h,
        "pos", &CBox::pos,
        "size", &CBox::size,
        "middle", &CBox::middle,
        "containsPoint", &CBox::containsPoint,
        "empty", &CBox::empty,
        "intersection", &CBox::intersection,
        "expand", [](CBox& self, double d) -> CBox& { return self.expand(d); },
        "round", &CBox::round,
        "translate", &CBox::translate,
        "scale", sol::overload(
            [](CBox& self, double d) -> CBox& { return self.scale(d); },
            static_cast<CBox& (CBox::*)(const Vector2D&)>(&CBox::scale)
        )
    );

    lua["CBox"]["new"] = [](double x, double y, double w, double h) { return CBox{x, y, w, h}; };
}

void registerColor(sol::state& lua) {
    lua.new_usertype<CHyprColor>("CHyprColor",
        sol::constructors<
            CHyprColor(),
            CHyprColor(float, float, float, float),
            CHyprColor(uint64_t)
        >(),
        "r", &CHyprColor::r,
        "g", &CHyprColor::g,
        "b", &CHyprColor::b,
        "a", &CHyprColor::a,
        "getAsHex", &CHyprColor::getAsHex,
        "brighten", &CHyprColor::brighten,
        "darken", &CHyprColor::darken,
        "mix", &CHyprColor::mix,
        "stripA", &CHyprColor::stripA,
        sol::meta_function::equal_to, &CHyprColor::operator==,
        sol::meta_function::to_string, [](const CHyprColor& c) {
            return "CHyprColor(" + std::to_string(c.r) + ", " + std::to_string(c.g) + ", " +
                   std::to_string(c.b) + ", " + std::to_string(c.a) + ")";
        }
    );

    // Convenience constructors
    lua["CHyprColor"]["new"] = sol::overload(
        [](float r, float g, float b) { return CHyprColor(r, g, b, 1.0f); },
        [](float r, float g, float b, float a) { return CHyprColor(r, g, b, a); }
    );
    lua["CHyprColor"]["fromHex"] = [](uint64_t hex) { return CHyprColor(hex); };
}

void registerDynamicSize(sol::state& lua) {
    // Sizing type enum
    lua.new_enum<CDynamicSize::eSizingType>("SizingType",
        {
            {"ABSOLUTE", CDynamicSize::HT_SIZE_ABSOLUTE},
            {"PERCENT", CDynamicSize::HT_SIZE_PERCENT},
            {"AUTO", CDynamicSize::HT_SIZE_AUTO}
        }
    );

    lua.new_usertype<CDynamicSize>("CDynamicSize",
        sol::constructors<CDynamicSize(CDynamicSize::eSizingType, CDynamicSize::eSizingType, const Vector2D&)>(),
        "calculate", &CDynamicSize::calculate,
        // Enum values accessible via CDynamicSize.HT_SIZE_*
        "HT_SIZE_ABSOLUTE", sol::var(CDynamicSize::HT_SIZE_ABSOLUTE),
        "HT_SIZE_PERCENT", sol::var(CDynamicSize::HT_SIZE_PERCENT),
        "HT_SIZE_AUTO", sol::var(CDynamicSize::HT_SIZE_AUTO)
    );

    // Convenience table for creating sizes
    lua["DynamicSize"] = lua.create_table_with(
        "absolute", [](double w, double h) {
            return CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {w, h});
        },
        "percent", [](double w, double h) {
            return CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {w, h});
        },
        "auto_", []() {
            return CDynamicSize(CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {0, 0});
        },
        "mixed", [](CDynamicSize::eSizingType typeX, CDynamicSize::eSizingType typeY, double w, double h) {
            return CDynamicSize(typeX, typeY, {w, h});
        }
    );
}

void registerFontTypes(sol::state& lua) {
    // Font sizing base enum
    lua.new_enum<CFontSize::eSizingBase>("FontSizeBase",
        {
            {"H1", CFontSize::HT_FONT_H1},
            {"H2", CFontSize::HT_FONT_H2},
            {"H3", CFontSize::HT_FONT_H3},
            {"TEXT", CFontSize::HT_FONT_TEXT},
            {"SMALL", CFontSize::HT_FONT_SMALL},
            {"ABSOLUTE", CFontSize::HT_FONT_ABSOLUTE}
        }
    );

    lua.new_usertype<CFontSize>("CFontSize",
        sol::constructors<CFontSize(CFontSize::eSizingBase, float)>(),
        "ptSize", &CFontSize::ptSize,
        // Enum values accessible via CFontSize.HT_FONT_*
        "HT_FONT_H1", sol::var(CFontSize::HT_FONT_H1),
        "HT_FONT_H2", sol::var(CFontSize::HT_FONT_H2),
        "HT_FONT_H3", sol::var(CFontSize::HT_FONT_H3),
        "HT_FONT_TEXT", sol::var(CFontSize::HT_FONT_TEXT),
        "HT_FONT_SMALL", sol::var(CFontSize::HT_FONT_SMALL),
        "HT_FONT_ABSOLUTE", sol::var(CFontSize::HT_FONT_ABSOLUTE)
    );

    // Font alignment enum
    lua.new_enum<eFontAlignment>("FontAlignment",
        {
            {"LEFT", HT_FONT_ALIGN_LEFT},
            {"CENTER", HT_FONT_ALIGN_CENTER},
            {"RIGHT", HT_FONT_ALIGN_RIGHT}
        }
    );

    // Convenience table for creating font sizes
    lua["FontSize"] = lua.create_table_with(
        "h1", [](float mult) { return CFontSize(CFontSize::HT_FONT_H1, mult); },
        "h2", [](float mult) { return CFontSize(CFontSize::HT_FONT_H2, mult); },
        "h3", [](float mult) { return CFontSize(CFontSize::HT_FONT_H3, mult); },
        "text", [](float mult) { return CFontSize(CFontSize::HT_FONT_TEXT, mult); },
        "small", [](float mult) { return CFontSize(CFontSize::HT_FONT_SMALL, mult); },
        "absolute", [](float size) { return CFontSize(CFontSize::HT_FONT_ABSOLUTE, size); }
    );

    // Default versions (multiplier = 1)
    lua["FontSize"]["H1"] = CFontSize(CFontSize::HT_FONT_H1, 1.0f);
    lua["FontSize"]["H2"] = CFontSize(CFontSize::HT_FONT_H2, 1.0f);
    lua["FontSize"]["H3"] = CFontSize(CFontSize::HT_FONT_H3, 1.0f);
    lua["FontSize"]["TEXT"] = CFontSize(CFontSize::HT_FONT_TEXT, 1.0f);
    lua["FontSize"]["SMALL"] = CFontSize(CFontSize::HT_FONT_SMALL, 1.0f);
}

void registerInputTypes(sol::state& lua) {
    // Mouse button enum
    lua.new_enum<Input::eMouseButton>("MouseButton",
        {
            {"UNKNOWN", Input::MOUSE_BUTTON_UNKNOWN},
            {"LEFT", Input::MOUSE_BUTTON_LEFT},
            {"RIGHT", Input::MOUSE_BUTTON_RIGHT},
            {"MIDDLE", Input::MOUSE_BUTTON_MIDDLE}
        }
    );

    // Axis enum
    lua.new_enum<Input::eAxisAxis>("AxisAxis",
        {
            {"HORIZONTAL", Input::AXIS_AXIS_HORIZONTAL},
            {"VERTICAL", Input::AXIS_AXIS_VERTICAL}
        }
    );

    // Keyboard modifiers enum
    lua.new_enum<Input::eKeyboardModifier>("KeyboardModifier",
        {
            {"SHIFT", Input::HT_MODIFIER_SHIFT},
            {"CAPS", Input::HT_MODIFIER_CAPS},
            {"CTRL", Input::HT_MODIFIER_CTRL},
            {"ALT", Input::HT_MODIFIER_ALT},
            {"MOD2", Input::HT_MODIFIER_MOD2},
            {"MOD3", Input::HT_MODIFIER_MOD3},
            {"META", Input::HT_MODIFIER_META},
            {"MOD5", Input::HT_MODIFIER_MOD5}
        }
    );

    // Keyboard key event struct
    lua.new_usertype<Input::SKeyboardKeyEvent>("KeyboardKeyEvent",
        sol::no_constructor,
        "xkbKeysym", &Input::SKeyboardKeyEvent::xkbKeysym,
        "down", &Input::SKeyboardKeyEvent::down,
        "repeat", &Input::SKeyboardKeyEvent::repeat,
        "utf8", &Input::SKeyboardKeyEvent::utf8,
        "modMask", &Input::SKeyboardKeyEvent::modMask,
        // Helper to check for modifiers
        "hasModifier", [](const Input::SKeyboardKeyEvent& e, Input::eKeyboardModifier mod) {
            return (e.modMask & static_cast<uint32_t>(mod)) != 0;
        }
    );
}

void registerPalette(sol::state& lua) {
    // CPalette with direct access to color/var fields via lambdas
    // (anonymous structs don't work well with sol2's usertype system)
    lua.new_usertype<CPalette>("CPalette",
        sol::no_constructor,
        "palette", &CPalette::palette,
        "emptyPalette", &CPalette::emptyPalette,
        // Color accessors
        "background", sol::property([](CPalette& p) { return p.m_colors.background; }),
        "text", sol::property([](CPalette& p) { return p.m_colors.text; }),
        "base", sol::property([](CPalette& p) { return p.m_colors.base; }),
        "alternateBase", sol::property([](CPalette& p) { return p.m_colors.alternateBase; }),
        "brightText", sol::property([](CPalette& p) { return p.m_colors.brightText; }),
        "accent", sol::property([](CPalette& p) { return p.m_colors.accent; }),
        "accentSecondary", sol::property([](CPalette& p) { return p.m_colors.accentSecondary; }),
        // Vars accessors
        "h1Size", sol::property([](CPalette& p) { return p.m_vars.h1Size; }),
        "h2Size", sol::property([](CPalette& p) { return p.m_vars.h2Size; }),
        "h3Size", sol::property([](CPalette& p) { return p.m_vars.h3Size; }),
        "fontSize", sol::property([](CPalette& p) { return p.m_vars.fontSize; }),
        "smallFontSize", sol::property([](CPalette& p) { return p.m_vars.smallFontSize; }),
        "iconTheme", sol::property([](CPalette& p) { return p.m_vars.iconTheme; }),
        "bigRounding", sol::property([](CPalette& p) { return p.m_vars.bigRounding; }),
        "smallRounding", sol::property([](CPalette& p) { return p.m_vars.smallRounding; }),
        "fontFamily", sol::property([](CPalette& p) { return p.m_vars.fontFamily; }),
        "fontFamilyMonospace", sol::property([](CPalette& p) { return p.m_vars.fontFamilyMonospace; })
    );
}

// Main registration function for all types
void registerTypes(sol::state& lua) {
    registerVector2D(lua);
    registerBox(lua);
    registerColor(lua);
    registerDynamicSize(lua);
    registerFontTypes(lua);
    registerInputTypes(lua);
    registerPalette(lua);
}

} // namespace Hyprtoolkit::Lua

#endif // HYPRTOOLKIT_LUA_ENABLED
