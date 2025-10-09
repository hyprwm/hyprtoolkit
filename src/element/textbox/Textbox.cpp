#include "Textbox.hpp"

#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <pango/pangocairo.h>

#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../core/AnimationManager.hpp"
#include "../../core/InternalBackend.hpp"
#include "../Element.hpp"
#include "../text/Text.hpp"
#include "../../helpers/UTF8.hpp"

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

    m_impl->listeners.enter = impl->m_externalEvents.keyboardEnter.listen([this] {
        m_impl->bg->addChild(m_impl->cursorCont);
        impl->window->setIMTo(impl->position, m_impl->data.text, m_impl->inputState.cursor);
        m_impl->bg->rebuild()->borderColor([] { return g_palette->m_colors.alternateBase.brighten(0.5F); })->commence();
    });

    m_impl->listeners.leave = impl->m_externalEvents.keyboardLeave.listen([this] {
        m_impl->bg->removeChild(m_impl->cursorCont);
        impl->window->resetIM();
        m_impl->bg->rebuild()->borderColor([] { return g_palette->m_colors.alternateBase; })->commence();
    });

    m_impl->listeners.key = impl->m_externalEvents.key.listen([this](const Input::SKeyboardKeyEvent& ev) {
        if (ev.xkbKeysym == XKB_KEY_BackSpace) {
            if (m_impl->inputState.cursor == 0)
                return;

            m_impl->data.text = UTF8::substr(m_impl->data.text, 0, m_impl->inputState.cursor - 1) + UTF8::substr(m_impl->data.text, m_impl->inputState.cursor);
            m_impl->inputState.cursor--;
            updateLabel();
            return;
        }

        if (ev.xkbKeysym == XKB_KEY_Delete) {
            if (m_impl->inputState.cursor == m_impl->data.text.length())
                return;

            m_impl->data.text = UTF8::substr(m_impl->data.text, 0, m_impl->inputState.cursor) + UTF8::substr(m_impl->data.text, m_impl->inputState.cursor + 1);
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

        if (ev.utf8 == "\n" && !m_impl->data.multiline)
            return;

        m_impl->data.text = UTF8::substr(m_impl->data.text, 0, m_impl->inputState.cursor) + ev.utf8 + UTF8::substr(m_impl->data.text, m_impl->inputState.cursor);
        m_impl->inputState.cursor++;
        updateLabel();
    });

    m_impl->placeholder->setMargin(1);
    m_impl->text->setMargin(1);

    addChild(m_impl->bg);
    m_impl->cursorCont->addChild(m_impl->cursor);
    m_impl->bg->impl->clipChildren = true;

    updateLabel();

    impl->grouped = true;
}

void CTextboxElement::updateLabel() {
    if (m_impl->data.text.empty()) {
        m_impl->bg->removeChild(m_impl->text);
        m_impl->bg->addChild(m_impl->placeholder);
    } else {
        m_impl->bg->removeChild(m_impl->placeholder);
        m_impl->bg->addChild(m_impl->text);
    }

    auto fullLabel = m_impl->inputState.imText.empty() ? //
        m_impl->data.text :                              //
        UTF8::substr(m_impl->data.text, 0, m_impl->inputState.cursor) + "<u>" + m_impl->inputState.imText + "</u>" + UTF8::substr(m_impl->data.text, m_impl->inputState.cursor);

    m_impl->text->rebuild()->text(std::move(fullLabel))->commence();

    updateCursor();

    if (m_impl->data.onTextEdited)
        m_impl->data.onTextEdited(m_impl->self.lock(), m_impl->data.text);
}

void CTextboxElement::imCommitNewText(const std::string& s) {
    m_impl->inputState.imText = s;
    updateLabel();
}

void CTextboxElement::imApplyText() {
    m_impl->data.text = UTF8::substr(m_impl->data.text, 0, m_impl->inputState.cursor) + m_impl->inputState.imText + UTF8::substr(m_impl->data.text, m_impl->inputState.cursor);
    m_impl->inputState.cursor += UTF8::length(m_impl->inputState.imText);
    m_impl->inputState.imText.clear();
    updateLabel();
}

void CTextboxElement::updateCursor() {
    m_impl->inputState.cursor = std::clamp(m_impl->inputState.cursor, sc<size_t>(0), sc<size_t>(UTF8::length(m_impl->data.text)));

    const float WIDTH = m_impl->estimateTextSize(UTF8::substr(m_impl->data.text, 0, m_impl->inputState.cursor)).x;

    m_impl->cursorCont->setAbsolutePosition({
        WIDTH,
        0.F,
    });

    if (impl->window)
        impl->window->setIMTo(impl->position.copy().translate({std::clamp(WIDTH, 0.F, sc<float>(impl->position.w)), 0.F}), m_impl->data.text, m_impl->inputState.cursor);

    g_positioner->repositionNeeded(m_impl->self.lock());
}

void CTextboxElement::focus(bool focus) {
    if (!impl->window)
        return;

    impl->window->setKeyboardFocus(impl->self.lock());
}

void CTextboxElement::paint() {
    ;
}

Vector2D STextboxImpl::estimateTextSize(const std::string& s) {
    return text->m_impl->getTextSizePreferred(s) / text->m_impl->lastScale;
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
    const bool TEXTS_DIFFER = data.text != m_impl->data.text;

    m_impl->data = data;

    if (TEXTS_DIFFER)
        updateLabel();

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

bool CTextboxElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}
