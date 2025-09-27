#include "Textbox.hpp"

#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <pango/pangoft2.h>

#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../core/AnimationManager.hpp"
#include "../../core/InternalBackend.hpp"
#include "../Element.hpp"
#include "../text/Text.hpp"

using namespace Hyprtoolkit;

SP<CTextboxElement> CTextboxElement::create(const STextboxData& data) {
    auto p          = SP<CTextboxElement>(new CTextboxElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    p->init();
    return p;
}

CTextboxElement::CTextboxElement(const STextboxData& data) : IElement(), m_impl(makeUnique<STextboxImpl>()) {
    m_impl->data = data;
}

void CTextboxElement::init() {
    m_impl->text = CTextBuilder::begin()
                       ->text(std::string{m_impl->data.text})
                       ->color([] { return g_palette->m_colors.text; })
                       ->callback([this] { impl->window->scheduleReposition(impl->self); })
                       ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                       ->commence();

    m_impl->placeholder = CTextBuilder::begin()
                              ->text(std::string{m_impl->data.placeholder})
                              ->color([] { return g_palette->m_colors.text.darken(0.4F); })
                              ->callback([this] { impl->window->scheduleReposition(impl->self); })
                              ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                              ->commence();

    m_impl->bg = CRectangleBuilder::begin()
                     ->color([] { return g_palette->m_colors.base; })
                     ->rounding(4)
                     ->borderColor([] { return g_palette->m_colors.alternateBase; })
                     ->borderThickness(1)
                     ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                     ->commence();

    m_impl->cursorCont = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})->commence();
    m_impl->cursor =
        CRectangleBuilder::begin()->color([] { return g_palette->m_colors.text; })->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 0.75F}})->commence();

    m_impl->cursorCont->setPositionMode(HT_POSITION_ABSOLUTE);
    m_impl->cursor->setPositionMode(HT_POSITION_CENTER);

    m_impl->placeholder->setPositionMode(HT_POSITION_VCENTER);
    m_impl->text->setPositionMode(HT_POSITION_VCENTER);

    m_impl->listeners.enter = impl->m_externalEvents.keyboardEnter.listen([this] { m_impl->bg->addChild(m_impl->cursorCont); });

    m_impl->listeners.leave = impl->m_externalEvents.keyboardLeave.listen([this] { m_impl->bg->removeChild(m_impl->cursorCont); });

    m_impl->listeners.key = impl->m_externalEvents.key.listen([this](const Input::SKeyboardKeyEvent& ev) {
        if (ev.xkbKeysym == XKB_KEY_BackSpace) {
            if (m_impl->inputState.cursor == 0)
                return;

            m_impl->data.text = m_impl->data.text.substr(0, m_impl->inputState.cursor - 1) + m_impl->data.text.substr(m_impl->inputState.cursor);
            m_impl->inputState.cursor--;
            updateLabel();
            return;
        }

        if (ev.xkbKeysym == XKB_KEY_Delete) {
            if (m_impl->inputState.cursor == m_impl->data.text.length())
                return;

            m_impl->data.text = m_impl->data.text.substr(0, m_impl->inputState.cursor) + m_impl->data.text.substr(m_impl->inputState.cursor + 1);
            updateLabel();
            return;
        }

        if (ev.xkbKeysym == XKB_KEY_Left) {
            if (m_impl->inputState.cursor == 0)
                return;
            m_impl->inputState.cursor--;
            updateCursor();
            return;
        }

        if (ev.xkbKeysym == XKB_KEY_Right) {
            if (m_impl->inputState.cursor == m_impl->data.text.length())
                return;
            m_impl->inputState.cursor++;
            updateCursor();
            return;
        }

        if (ev.xkbKeysym == XKB_KEY_Escape) {
            if (impl->window)
                impl->window->unfocusKeyboard();

            return;
        }

        if (ev.utf8.empty())
            return;

        m_impl->data.text = m_impl->data.text.substr(0, m_impl->inputState.cursor) + ev.utf8 + m_impl->data.text.substr(m_impl->inputState.cursor);
        m_impl->inputState.cursor++;
        updateLabel();
    });

    // m_impl->placeholder->setMargin(1);
    // m_impl->text->setMargin(1);

    addChild(m_impl->bg);
    m_impl->bg->addChild(m_impl->cursorCont);
    m_impl->cursorCont->addChild(m_impl->cursor);
    m_impl->bg->impl->clipChildren = true;

    updateLabel();
}

void CTextboxElement::updateLabel() {
    if (m_impl->data.text.empty()) {
        m_impl->bg->removeChild(m_impl->text);
        m_impl->bg->addChild(m_impl->placeholder);
    } else {
        m_impl->bg->removeChild(m_impl->placeholder);
        m_impl->bg->addChild(m_impl->text);
    }

    m_impl->text->rebuild()->text(std::string{m_impl->data.text})->commence();

    updateCursor();

    if (m_impl->data.onTextEdited)
        m_impl->data.onTextEdited(m_impl->self.lock(), m_impl->data.text);
}

void CTextboxElement::updateCursor() {
    m_impl->cursorCont->setAbsolutePosition({
        m_impl->estimateTextSize(m_impl->data.text.substr(0, m_impl->inputState.cursor)).x,
        0.F,
    });

    g_positioner->repositionNeeded(m_impl->self.lock());
}

void CTextboxElement::paint() {
    ;
}

Vector2D STextboxImpl::estimateTextSize(const std::string& s) {
    const auto            SCALE = text->m_impl->lastScale;

    PangoFontMap*         fm     = pango_ft2_font_map_new();
    PangoContext*         ctx    = pango_font_map_create_context(fm);
    PangoLayout*          layout = pango_layout_new(ctx);

    PangoFontDescription* fontDesc = pango_font_description_from_string("Sans Serif");
    pango_font_description_set_size(fontDesc, text->m_impl->data.fontSize.ptSize() * SCALE * PANGO_SCALE);
    pango_layout_set_font_description(layout, fontDesc);
    pango_font_description_free(fontDesc);

    PangoAlignment pangoAlign = PANGO_ALIGN_LEFT;

    pango_layout_set_alignment(layout, pangoAlign);

    PangoAttrList* attrList = nullptr;
    GError*        gError   = nullptr;
    char*          buf      = nullptr;
    if (pango_parse_markup(s.c_str(), -1, 0, &attrList, &buf, nullptr, &gError))
        pango_layout_set_text(layout, buf, -1);
    else {
        g_error_free(gError);
        pango_layout_set_text(layout, s.c_str(), -1);
    }

    if (!attrList)
        attrList = pango_attr_list_new();

    if (buf)
        free(buf);

    pango_attr_list_insert(attrList, pango_attr_scale_new(1));
    pango_layout_set_attributes(layout, attrList);
    pango_attr_list_unref(attrList);

    int layoutWidth, layoutHeight;
    pango_layout_get_size(layout, &layoutWidth, &layoutHeight);

    if (text->m_impl->data.clampSize) {
        layoutWidth  = text->m_impl->data.clampSize->x > 0 ? std::min(layoutWidth, sc<int>(text->m_impl->data.clampSize->x * PANGO_SCALE)) : layoutWidth;
        layoutHeight = text->m_impl->data.clampSize->y > 0 ? std::min(layoutHeight, sc<int>(text->m_impl->data.clampSize->y * PANGO_SCALE)) : layoutHeight;
        if (text->m_impl->data.clampSize->x >= 0)
            pango_layout_set_width(layout, layoutWidth);
        if (text->m_impl->data.clampSize->y >= 0)
            pango_layout_set_height(layout, layoutHeight);

        pango_layout_get_size(layout, &layoutWidth, &layoutHeight);
    }

    return Vector2D{layoutWidth / PANGO_SCALE, layoutHeight / PANGO_SCALE};
}

void CTextboxElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

SP<CTextboxBuilder> CTextboxElement::rebuild() {
    auto p       = SP<CTextboxBuilder>(new CTextboxBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<STextboxData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

void CTextboxElement::replaceData(const STextboxData& data) {
    m_impl->data = data;

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

Hyprutils::Math::Vector2D CTextboxElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CTextboxElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_impl->data.size.calculate(parent);
}

std::optional<Vector2D> CTextboxElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;
    return Vector2D{0, 0};
}

std::optional<Vector2D> CTextboxElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;
    return std::nullopt;
}

bool CTextboxElement::acceptsMouseInput() {
    return true;
}

ePointerShape CTextboxElement::pointerShape() {
    return HT_POINTER_TEXT;
}

bool CTextboxElement::acceptsKeyboardInput() {
    return true;
}
