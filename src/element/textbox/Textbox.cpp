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
                     ->rounding(g_palette->m_vars.smallRounding)
                     ->borderColor([] { return g_palette->m_colors.alternateBase; })
                     ->borderThickness(1)
                     ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                     ->commence();

    m_impl->bgInnerCont = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})->commence();
    m_impl->bgInnerCont->setMargin(3);

    m_impl->selectBgCont = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 0.7F}})->commence();
    m_impl->selectBg     = CRectangleBuilder::begin()
                           ->color([] {
                               auto x = g_palette->m_colors.accent.darken(0.4F);
                               x.a    = 0.5F;
                               return x;
                           })
                           ->commence();

    m_impl->selectBgCont->setPositionMode(HT_POSITION_ABSOLUTE);
    m_impl->selectBgCont->setPositionFlag(HT_POSITION_FLAG_VCENTER, true);
    m_impl->selectBg->setPositionMode(HT_POSITION_ABSOLUTE);

    m_impl->selectBgCont->addChild(m_impl->selectBg);

    m_impl->cursorCont = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 0.8F}})->commence();
    m_impl->cursor =
        CRectangleBuilder::begin()->color([] { return g_palette->m_colors.text; })->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})->commence();

    m_impl->cursorCont->setPositionMode(HT_POSITION_ABSOLUTE);
    m_impl->cursorCont->setPositionFlag(HT_POSITION_FLAG_VCENTER, true);
    m_impl->cursor->setPositionMode(HT_POSITION_ABSOLUTE);

    m_impl->placeholder->setPositionMode(HT_POSITION_ABSOLUTE);
    m_impl->placeholder->setPositionFlag(HT_POSITION_FLAG_VCENTER, true);
    m_impl->text->setPositionMode(HT_POSITION_ABSOLUTE);
    m_impl->text->setPositionFlag(HT_POSITION_FLAG_VCENTER, true);

    m_impl->listeners.mouseMove = impl->m_externalEvents.mouseMove.listen([this](Vector2D pos) { m_impl->lastCursorPos = pos; });

    m_impl->listeners.mouseButton = impl->m_externalEvents.mouseButton.listen([this](Input::eMouseButton button, bool down) {
        if (down)
            m_impl->focusCursorAtClickedChar();
    });

    m_impl->listeners.enter = impl->m_externalEvents.keyboardEnter.listen([this] {
        m_impl->bgInnerCont->addChild(m_impl->cursorCont);
        impl->window->setIMTo(impl->position, m_impl->data.text, m_impl->inputState.cursor);
        m_impl->bg->rebuild()->borderColor([] { return g_palette->m_colors.alternateBase.brighten(0.5F); })->commence();
        m_impl->focusCursorAtClickedChar();
    });

    m_impl->listeners.leave = impl->m_externalEvents.keyboardLeave.listen([this] {
        m_impl->bgInnerCont->removeChild(m_impl->cursorCont);
        impl->window->resetIM();
        m_impl->bg->rebuild()->borderColor([] { return g_palette->m_colors.alternateBase; })->commence();
    });

    m_impl->listeners.key = impl->m_externalEvents.key.listen([this](const Input::SKeyboardKeyEvent& ev) {
        if (ev.xkbKeysym == XKB_KEY_BackSpace) {
            if (m_impl->inputState.cursor == 0)
                return;

            if (m_impl->inputState.selectBegin >= 0) {
                m_impl->removeSelectedText();
                m_impl->updateLabel();
                return;
            }

            if (ev.modMask & (Input::HT_MODIFIER_CTRL | Input::HT_MODIFIER_SHIFT)) {
                // remove everything before
                // TODO: make shift remove word?
                // TODO: make this remove until a newline?
                m_impl->data.text         = UTF8::substr(m_impl->data.text, m_impl->inputState.cursor);
                m_impl->inputState.cursor = 0;
                m_impl->updateLabel();
                return;
            }

            m_impl->data.text = UTF8::substr(m_impl->data.text, 0, m_impl->inputState.cursor - 1) + UTF8::substr(m_impl->data.text, m_impl->inputState.cursor);
            m_impl->inputState.cursor--;
            m_impl->updateLabel();
            return;
        }

        if (ev.xkbKeysym == XKB_KEY_Delete) {
            if (m_impl->inputState.cursor == m_impl->data.text.length())
                return;

            m_impl->data.text = UTF8::substr(m_impl->data.text, 0, m_impl->inputState.cursor) + UTF8::substr(m_impl->data.text, m_impl->inputState.cursor + 1);
            m_impl->updateLabel();
            return;
        }

        if (ev.xkbKeysym == XKB_KEY_Left) {
            if (m_impl->inputState.cursor == 0)
                return;

            if (ev.modMask & Input::HT_MODIFIER_CTRL) {
                // go to beginning
                // TODO: make this go back a word instead
                m_impl->inputState.cursor = 0;
                m_impl->updateLabel();
                return;
            }

            if (ev.modMask & Input::HT_MODIFIER_SHIFT) {
                if (m_impl->inputState.selectBegin == -1) {
                    m_impl->inputState.selectBegin = m_impl->inputState.cursor - 1;
                    m_impl->inputState.selectEnd   = m_impl->inputState.cursor;
                } else {
                    if (sc<ssize_t>(m_impl->inputState.cursor) == m_impl->inputState.selectBegin)
                        m_impl->inputState.selectBegin--;
                    else
                        m_impl->inputState.selectEnd--;
                }
                m_impl->updateSelect();
            } else {
                m_impl->inputState.selectBegin = -1;
                m_impl->inputState.selectEnd   = -1;
                m_impl->updateSelect();
            }

            m_impl->inputState.cursor--;
            m_impl->updateCursor();
            return;
        }

        if (ev.xkbKeysym == XKB_KEY_Right) {
            if (m_impl->inputState.cursor == m_impl->data.text.length())
                return;

            if (ev.modMask & Input::HT_MODIFIER_SHIFT) {
                if (m_impl->inputState.selectBegin == -1) {
                    m_impl->inputState.selectBegin = m_impl->inputState.cursor;
                    m_impl->inputState.selectEnd   = m_impl->inputState.cursor + 1;
                } else {
                    if (sc<ssize_t>(m_impl->inputState.cursor) == m_impl->inputState.selectEnd)
                        m_impl->inputState.selectEnd++;
                    else
                        m_impl->inputState.selectBegin++;
                }
                m_impl->updateSelect();
            } else {
                m_impl->inputState.selectBegin = -1;
                m_impl->inputState.selectEnd   = -1;
                m_impl->updateSelect();
            }

            m_impl->inputState.cursor++;
            m_impl->updateCursor();
            return;
        }

        if (ev.xkbKeysym == XKB_KEY_Escape) {
            if (impl->window)
                impl->window->unfocusKeyboard();

            m_impl->inputState.selectBegin = -1;
            m_impl->inputState.selectEnd   = -1;
            m_impl->updateSelect();

            return;
        }

        if ((ev.xkbKeysym == XKB_KEY_A || ev.xkbKeysym == XKB_KEY_a) && ev.modMask & Input::HT_MODIFIER_CTRL) {
            m_impl->inputState.selectBegin = 0;
            m_impl->inputState.selectEnd   = UTF8::length(m_impl->data.text);
            m_impl->inputState.cursor      = m_impl->inputState.selectEnd + 1;
            m_impl->updateSelect();
            m_impl->updateCursor();
            return;
        }

        if (ev.utf8.empty())
            return;

        if (ev.utf8 == "\n" && !m_impl->data.multiline)
            return;

        m_impl->removeSelectedText();

        m_impl->data.text = UTF8::substr(m_impl->data.text, 0, m_impl->inputState.cursor) + ev.utf8 + UTF8::substr(m_impl->data.text, m_impl->inputState.cursor);
        m_impl->inputState.cursor++;
        m_impl->updateLabel();
    });

    m_impl->placeholder->setMargin(1);
    m_impl->text->setMargin(1);

    addChild(m_impl->bg);

    m_impl->bg->addChild(m_impl->bgInnerCont);

    m_impl->cursorCont->addChild(m_impl->cursor);
    m_impl->bg->impl->clipChildren = true;

    m_impl->updateLabel();

    impl->grouped = true;
}

std::string_view CTextboxElement::currentText() {
    return m_impl->data.text;
}

void STextboxImpl::updateLabel() {
    if (data.text.empty()) {
        bgInnerCont->removeChild(text);
        bgInnerCont->addChild(placeholder);
    } else {
        bgInnerCont->removeChild(placeholder);
        bgInnerCont->addChild(text);
    }

    if (!data.password) {

        auto fullLabel = inputState.imText.empty() ? //
            data.text :                              //
            UTF8::substr(data.text, 0, inputState.cursor) + "<u>" + inputState.imText + "</u>" + UTF8::substr(data.text, inputState.cursor);

        text->rebuild()->text(std::move(fullLabel))->commence();
    } else {
        std::string pwdText = "";
        for (size_t i = 0; i < data.text.size(); ++i) {
            pwdText += '*';
        }

        auto fullLabel = inputState.imText.empty() ? //
            pwdText :                                //
            UTF8::substr(pwdText, 0, inputState.cursor) + "<u>" + inputState.imText + "</u>" + UTF8::substr(pwdText, inputState.cursor);

        text->rebuild()->text(std::move(fullLabel))->commence();
    }

    updateCursor();

    if (data.onTextEdited)
        data.onTextEdited(self.lock(), data.text);
}

void CTextboxElement::imCommitNewText(const std::string& s) {
    m_impl->inputState.imText = s;
    m_impl->updateLabel();
}

void CTextboxElement::imApplyText() {
    m_impl->data.text = UTF8::substr(m_impl->data.text, 0, m_impl->inputState.cursor) + m_impl->inputState.imText + UTF8::substr(m_impl->data.text, m_impl->inputState.cursor);
    m_impl->inputState.cursor += UTF8::length(m_impl->inputState.imText);
    m_impl->inputState.imText.clear();
    m_impl->updateLabel();
}

void STextboxImpl::updateCursor() {
    inputState.cursor = std::clamp(inputState.cursor, sc<size_t>(0), sc<size_t>(UTF8::length(data.text)));

    const float WIDTH = text->m_impl->getCursorPos(inputState.cursor);

    cursor->setAbsolutePosition({
        WIDTH,
        0.F,
    });

    if (self->impl->window)
        self->impl->window->setIMTo(self->impl->position.copy().translate({std::clamp(WIDTH, 0.F, sc<float>(self->impl->position.w)), 0.F}), data.text, inputState.cursor);

    g_positioner->repositionNeeded(cursor);
}

void CTextboxElement::focus(bool focus) {
    if (!impl->window)
        return;

    impl->window->setKeyboardFocus(impl->self.lock());
}

void CTextboxElement::paint() {
    ;
}

void STextboxImpl::updateSelect() {
    if (inputState.selectBegin < 0) {
        bgInnerCont->removeChild(selectBgCont);
        return;
    }

    float begin = text->m_impl->getCursorPos(inputState.selectBegin), //
        end     = text->m_impl->getCursorPos(inputState.selectEnd);

    float width = end - begin;

    selectBg->rebuild()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {width, 1.F}})->commence();
    selectBg->setAbsolutePosition(Vector2D{begin, 0.F});

    bgInnerCont->addChild(selectBgCont);
}

void STextboxImpl::removeSelectedText() {
    if (inputState.selectBegin >= 0 && inputState.selectEnd >= 0 && inputState.selectBegin < inputState.selectEnd) {
        data.text              = UTF8::substr(data.text, 0, inputState.selectBegin) + UTF8::substr(data.text, inputState.selectEnd);
        inputState.cursor      = inputState.selectBegin;
        inputState.selectBegin = -1;
        inputState.selectEnd   = -1;
        updateSelect();
    }
}

void STextboxImpl::focusCursorAtClickedChar() {
    inputState.cursor = text->m_impl->vecToCharIdx(lastCursorPos - (text->impl->position.pos() - self->impl->position.pos())).value_or(data.text.size()) + 1;
    updateCursor();
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
        m_impl->updateLabel();

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
