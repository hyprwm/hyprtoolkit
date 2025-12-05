#ifdef HYPRTOOLKIT_LUA_ENABLED

#include <sol/sol.hpp>
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
#include <hyprtoolkit/types/ImageTypes.hpp>

#include "../helpers/SmartPtrAdapter.hpp"
#include "../helpers/CallbackAdapter.hpp"
#include "../helpers/ColorFnAdapter.hpp"

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;

namespace Hyprtoolkit::Lua {

// Text Element
void registerTextElement(sol::state& lua) {
    lua.new_usertype<CTextBuilder>("CTextBuilder",
        sol::no_constructor,
        "begin", &CTextBuilder::begin,
        "text", [](CSharedPointer<CTextBuilder> self, const std::string& t) {
            return self->text(std::string(t));
        },
        "color", [](CSharedPointer<CTextBuilder> self, sol::object colorObj) {
            return self->color(luaToColorFn(colorObj));
        },
        "a", &CTextBuilder::a,
        "fontSize", [](CSharedPointer<CTextBuilder> self, CFontSize fontSize) {
            return self->fontSize(std::move(fontSize));
        },
        "align", &CTextBuilder::align,
        "fontFamily", [](CSharedPointer<CTextBuilder> self, const std::string& f) {
            return self->fontFamily(std::string(f));
        },
        "clampSize", [](CSharedPointer<CTextBuilder> self, double x, double y) {
            return self->clampSize(Vector2D{x, y});
        },
        "callback", [](CSharedPointer<CTextBuilder> self, sol::function fn) {
            return self->callback([fn]() {
                sol::protected_function_result result = fn();
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Text callback error: %s\n", err.what());
                }
            });
        },
        "noEllipsize", &CTextBuilder::noEllipsize,
        "size", [](CSharedPointer<CTextBuilder> self, CDynamicSize size) {
            return self->size(std::move(size));
        },
        "async", &CTextBuilder::async,
        "commence", &CTextBuilder::commence
    );

    lua.new_usertype<CTextElement>("CTextElement",
        sol::no_constructor,
        sol::base_classes, sol::bases<IElement>(),
        "rebuild", &CTextElement::rebuild,
        "size", &CTextElement::size
    );
}

// Button Element
void registerButtonElement(sol::state& lua) {
    lua.new_usertype<CButtonBuilder>("CButtonBuilder",
        sol::no_constructor,
        "begin", &CButtonBuilder::begin,
        "label", [](CSharedPointer<CButtonBuilder> self, const std::string& l) {
            return self->label(std::string(l));
        },
        "noBorder", &CButtonBuilder::noBorder,
        "noBg", &CButtonBuilder::noBg,
        "alignText", &CButtonBuilder::alignText,
        "fontFamily", [](CSharedPointer<CButtonBuilder> self, const std::string& f) {
            return self->fontFamily(std::string(f));
        },
        "fontSize", [](CSharedPointer<CButtonBuilder> self, CFontSize fontSize) {
            return self->fontSize(std::move(fontSize));
        },
        "onMainClick", [](CSharedPointer<CButtonBuilder> self, sol::function fn) {
            return self->onMainClick([fn](CSharedPointer<CButtonElement> el) {
                sol::protected_function_result result = fn(el);
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Button onMainClick error: %s\n", err.what());
                }
            });
        },
        "onRightClick", [](CSharedPointer<CButtonBuilder> self, sol::function fn) {
            return self->onRightClick([fn](CSharedPointer<CButtonElement> el) {
                sol::protected_function_result result = fn(el);
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Button onRightClick error: %s\n", err.what());
                }
            });
        },
        "size", [](CSharedPointer<CButtonBuilder> self, CDynamicSize size) {
            return self->size(std::move(size));
        },
        "commence", &CButtonBuilder::commence
    );

    lua.new_usertype<CButtonElement>("CButtonElement",
        sol::no_constructor,
        sol::base_classes, sol::bases<IElement>(),
        "rebuild", &CButtonElement::rebuild,
        "size", &CButtonElement::size
    );
}

// Textbox Element
void registerTextboxElement(sol::state& lua) {
    lua.new_usertype<CTextboxBuilder>("CTextboxBuilder",
        sol::no_constructor,
        "begin", &CTextboxBuilder::begin,
        "placeholder", [](CSharedPointer<CTextboxBuilder> self, const std::string& p) {
            return self->placeholder(std::string(p));
        },
        "defaultText", [](CSharedPointer<CTextboxBuilder> self, const std::string& t) {
            return self->defaultText(std::string(t));
        },
        "onTextEdited", [](CSharedPointer<CTextboxBuilder> self, sol::function fn) {
            return self->onTextEdited([fn](CSharedPointer<CTextboxElement> el, const std::string& text) {
                sol::protected_function_result result = fn(el, text);
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Textbox onTextEdited error: %s\n", err.what());
                }
            });
        },
        "multiline", &CTextboxBuilder::multiline,
        "size", [](CSharedPointer<CTextboxBuilder> self, CDynamicSize size) {
            return self->size(std::move(size));
        },
        "commence", &CTextboxBuilder::commence
    );

    lua.new_usertype<CTextboxElement>("CTextboxElement",
        sol::no_constructor,
        sol::base_classes, sol::bases<IElement>(),
        "rebuild", &CTextboxElement::rebuild,
        "size", &CTextboxElement::size,
        "focus", &CTextboxElement::focus,
        "currentText", [](CTextboxElement* self) {
            return std::string(self->currentText());
        }
    );
}

// Checkbox Element
void registerCheckboxElement(sol::state& lua) {
    lua.new_usertype<CCheckboxBuilder>("CCheckboxBuilder",
        sol::no_constructor,
        "begin", &CCheckboxBuilder::begin,
        "toggled", &CCheckboxBuilder::toggled,
        "onToggled", [](CSharedPointer<CCheckboxBuilder> self, sol::function fn) {
            return self->onToggled([fn](CSharedPointer<CCheckboxElement> el, bool state) {
                sol::protected_function_result result = fn(el, state);
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Checkbox onToggled error: %s\n", err.what());
                }
            });
        },
        "size", [](CSharedPointer<CCheckboxBuilder> self, CDynamicSize size) {
            return self->size(std::move(size));
        },
        "commence", &CCheckboxBuilder::commence
    );

    lua.new_usertype<CCheckboxElement>("CCheckboxElement",
        sol::no_constructor,
        sol::base_classes, sol::bases<IElement>(),
        "rebuild", &CCheckboxElement::rebuild,
        "size", &CCheckboxElement::size
    );
}

// Slider Element
void registerSliderElement(sol::state& lua) {
    lua.new_usertype<CSliderBuilder>("CSliderBuilder",
        sol::no_constructor,
        "begin", &CSliderBuilder::begin,
        "min", &CSliderBuilder::min,
        "max", &CSliderBuilder::max,
        "val", &CSliderBuilder::val,
        "snapInt", &CSliderBuilder::snapInt,
        "onChanged", [](CSharedPointer<CSliderBuilder> self, sol::function fn) {
            return self->onChanged([fn](CSharedPointer<CSliderElement> el, float value) {
                sol::protected_function_result result = fn(el, value);
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Slider onChanged error: %s\n", err.what());
                }
            });
        },
        "size", [](CSharedPointer<CSliderBuilder> self, CDynamicSize size) {
            return self->size(std::move(size));
        },
        "commence", &CSliderBuilder::commence
    );

    lua.new_usertype<CSliderElement>("CSliderElement",
        sol::no_constructor,
        sol::base_classes, sol::bases<IElement>(),
        "rebuild", &CSliderElement::rebuild,
        "size", &CSliderElement::size,
        "sliding", &CSliderElement::sliding
    );
}

// Combobox Element
void registerComboboxElement(sol::state& lua) {
    lua.new_usertype<CComboboxBuilder>("CComboboxBuilder",
        sol::no_constructor,
        "begin", &CComboboxBuilder::begin,
        "items", [](CSharedPointer<CComboboxBuilder> self, sol::table itemsTable) {
            std::vector<std::string> items;
            for (size_t i = 1; i <= itemsTable.size(); ++i) {
                items.push_back(itemsTable[i].get<std::string>());
            }
            return self->items(std::move(items));
        },
        "currentItem", &CComboboxBuilder::currentItem,
        "onChanged", [](CSharedPointer<CComboboxBuilder> self, sol::function fn) {
            return self->onChanged([fn](CSharedPointer<CComboboxElement> el, size_t idx) {
                sol::protected_function_result result = fn(el, idx);
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Combobox onChanged error: %s\n", err.what());
                }
            });
        },
        "size", [](CSharedPointer<CComboboxBuilder> self, CDynamicSize size) {
            return self->size(std::move(size));
        },
        "commence", &CComboboxBuilder::commence
    );

    lua.new_usertype<CComboboxElement>("CComboboxElement",
        sol::no_constructor,
        sol::base_classes, sol::bases<IElement>(),
        "rebuild", &CComboboxElement::rebuild,
        "size", &CComboboxElement::size,
        "current", &CComboboxElement::current,
        "setCurrent", &CComboboxElement::setCurrent
    );
}

// Spinbox Element
void registerSpinboxElement(sol::state& lua) {
    lua.new_usertype<CSpinboxBuilder>("CSpinboxBuilder",
        sol::no_constructor,
        "begin", &CSpinboxBuilder::begin,
        "label", [](CSharedPointer<CSpinboxBuilder> self, const std::string& l) {
            return self->label(std::string(l));
        },
        "items", [](CSharedPointer<CSpinboxBuilder> self, sol::table itemsTable) {
            std::vector<std::string> items;
            for (size_t i = 1; i <= itemsTable.size(); ++i) {
                items.push_back(itemsTable[i].get<std::string>());
            }
            return self->items(std::move(items));
        },
        "currentItem", &CSpinboxBuilder::currentItem,
        "onChanged", [](CSharedPointer<CSpinboxBuilder> self, sol::function fn) {
            return self->onChanged([fn](CSharedPointer<CSpinboxElement> el, size_t idx) {
                sol::protected_function_result result = fn(el, idx);
                if (!result.valid()) {
                    sol::error err = result;
                    fprintf(stderr, "[Lua] Spinbox onChanged error: %s\n", err.what());
                }
            });
        },
        "fill", &CSpinboxBuilder::fill,
        "size", [](CSharedPointer<CSpinboxBuilder> self, CDynamicSize size) {
            return self->size(std::move(size));
        },
        "commence", &CSpinboxBuilder::commence
    );

    lua.new_usertype<CSpinboxElement>("CSpinboxElement",
        sol::no_constructor,
        sol::base_classes, sol::bases<IElement>(),
        "rebuild", &CSpinboxElement::rebuild,
        "size", &CSpinboxElement::size,
        "current", &CSpinboxElement::current,
        "setCurrent", &CSpinboxElement::setCurrent
    );
}

// Rectangle Element
void registerRectangleElement(sol::state& lua) {
    lua.new_usertype<CRectangleBuilder>("CRectangleBuilder",
        sol::no_constructor,
        "begin", &CRectangleBuilder::begin,
        "color", [](CSharedPointer<CRectangleBuilder> self, sol::object colorObj) {
            return self->color(luaToColorFn(colorObj));
        },
        "borderColor", [](CSharedPointer<CRectangleBuilder> self, sol::object colorObj) {
            return self->borderColor(luaToColorFn(colorObj));
        },
        "rounding", &CRectangleBuilder::rounding,
        "borderThickness", &CRectangleBuilder::borderThickness,
        "size", [](CSharedPointer<CRectangleBuilder> self, CDynamicSize size) {
            return self->size(std::move(size));
        },
        "commence", &CRectangleBuilder::commence
    );

    lua.new_usertype<CRectangleElement>("CRectangleElement",
        sol::no_constructor,
        sol::base_classes, sol::bases<IElement>(),
        "rebuild", &CRectangleElement::rebuild,
        "size", &CRectangleElement::size
    );
}

// Column Layout Element
void registerColumnLayoutElement(sol::state& lua) {
    lua.new_usertype<CColumnLayoutBuilder>("CColumnLayoutBuilder",
        sol::no_constructor,
        "begin", &CColumnLayoutBuilder::begin,
        "gap", &CColumnLayoutBuilder::gap,
        "size", [](CSharedPointer<CColumnLayoutBuilder> self, CDynamicSize size) {
            return self->size(std::move(size));
        },
        "commence", &CColumnLayoutBuilder::commence
    );

    lua.new_usertype<CColumnLayoutElement>("CColumnLayoutElement",
        sol::no_constructor,
        sol::base_classes, sol::bases<IElement>(),
        "rebuild", &CColumnLayoutElement::rebuild,
        "size", &CColumnLayoutElement::size
    );
}

// Row Layout Element
void registerRowLayoutElement(sol::state& lua) {
    lua.new_usertype<CRowLayoutBuilder>("CRowLayoutBuilder",
        sol::no_constructor,
        "begin", &CRowLayoutBuilder::begin,
        "gap", &CRowLayoutBuilder::gap,
        "size", [](CSharedPointer<CRowLayoutBuilder> self, CDynamicSize size) {
            return self->size(std::move(size));
        },
        "commence", &CRowLayoutBuilder::commence
    );

    lua.new_usertype<CRowLayoutElement>("CRowLayoutElement",
        sol::no_constructor,
        sol::base_classes, sol::bases<IElement>(),
        "size", &CRowLayoutElement::size
    );
}

// Scroll Area Element
void registerScrollAreaElement(sol::state& lua) {
    lua.new_usertype<CScrollAreaBuilder>("CScrollAreaBuilder",
        sol::no_constructor,
        "begin", &CScrollAreaBuilder::begin,
        "scrollX", &CScrollAreaBuilder::scrollX,
        "scrollY", &CScrollAreaBuilder::scrollY,
        "blockUserScroll", &CScrollAreaBuilder::blockUserScroll,
        "size", [](CSharedPointer<CScrollAreaBuilder> self, CDynamicSize size) {
            return self->size(std::move(size));
        },
        "commence", &CScrollAreaBuilder::commence
    );

    lua.new_usertype<CScrollAreaElement>("CScrollAreaElement",
        sol::no_constructor,
        sol::base_classes, sol::bases<IElement>(),
        "size", &CScrollAreaElement::size,
        "getCurrentScroll", &CScrollAreaElement::getCurrentScroll,
        "setScroll", &CScrollAreaElement::setScroll
    );
}

// Image Element
void registerImageElement(sol::state& lua) {
    // Image fit mode enum
    lua.new_enum<eImageFitMode>("ImageFitMode",
        {
            {"STRETCH", IMAGE_FIT_MODE_STRETCH},
            {"COVER", IMAGE_FIT_MODE_COVER},
            {"CONTAIN", IMAGE_FIT_MODE_CONTAIN},
            {"TILE", IMAGE_FIT_MODE_TILE}
        }
    );

    lua.new_usertype<CImageBuilder>("CImageBuilder",
        sol::no_constructor,
        "begin", &CImageBuilder::begin,
        "path", [](CSharedPointer<CImageBuilder> self, const std::string& p) {
            return self->path(std::string(p));
        },
        "icon", &CImageBuilder::icon,
        "a", &CImageBuilder::a,
        "fitMode", &CImageBuilder::fitMode,
        "sync", &CImageBuilder::sync,
        "rounding", &CImageBuilder::rounding,
        "size", [](CSharedPointer<CImageBuilder> self, CDynamicSize size) {
            return self->size(std::move(size));
        },
        "commence", &CImageBuilder::commence
    );

    lua.new_usertype<CImageElement>("CImageElement",
        sol::no_constructor,
        sol::base_classes, sol::bases<IElement>(),
        "rebuild", &CImageElement::rebuild,
        "size", &CImageElement::size
    );
}

// Null Element (Spacer)
void registerNullElement(sol::state& lua) {
    lua.new_usertype<CNullBuilder>("CNullBuilder",
        sol::no_constructor,
        "begin", &CNullBuilder::begin,
        "size", [](CSharedPointer<CNullBuilder> self, CDynamicSize size) {
            return self->size(std::move(size));
        },
        "commence", &CNullBuilder::commence
    );

    lua.new_usertype<CNullElement>("CNullElement",
        sol::no_constructor,
        sol::base_classes, sol::bases<IElement>(),
        "rebuild", &CNullElement::rebuild,
        "size", &CNullElement::size
    );
}

// Line Element
void registerLineElement(sol::state& lua) {
    lua.new_usertype<CLineBuilder>("CLineBuilder",
        sol::no_constructor,
        "begin", &CLineBuilder::begin,
        "color", [](CSharedPointer<CLineBuilder> self, sol::object colorObj) {
            return self->color(luaToColorFn(colorObj));
        },
        "thick", &CLineBuilder::thick,
        "points", [](CSharedPointer<CLineBuilder> self, sol::table pointsTable) {
            std::vector<Vector2D> points;
            for (size_t i = 1; i <= pointsTable.size(); ++i) {
                sol::table pt = pointsTable[i];
                points.push_back(Vector2D{pt[1].get<double>(), pt[2].get<double>()});
            }
            return self->points(std::move(points));
        },
        "size", [](CSharedPointer<CLineBuilder> self, CDynamicSize size) {
            return self->size(std::move(size));
        },
        "commence", &CLineBuilder::commence
    );

    lua.new_usertype<CLineElement>("CLineElement",
        sol::no_constructor,
        sol::base_classes, sol::bases<IElement>(),
        "rebuild", &CLineElement::rebuild,
        "size", &CLineElement::size
    );
}

// Main registration function for all element builders
void registerElementBuilders(sol::state& lua) {
    registerTextElement(lua);
    registerButtonElement(lua);
    registerTextboxElement(lua);
    registerCheckboxElement(lua);
    registerSliderElement(lua);
    registerComboboxElement(lua);
    registerSpinboxElement(lua);
    registerRectangleElement(lua);
    registerColumnLayoutElement(lua);
    registerRowLayoutElement(lua);
    registerScrollAreaElement(lua);
    registerImageElement(lua);
    registerNullElement(lua);
    registerLineElement(lua);
}

} // namespace Hyprtoolkit::Lua

#endif // HYPRTOOLKIT_LUA_ENABLED
