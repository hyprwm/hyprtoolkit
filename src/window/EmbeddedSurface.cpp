#include "EmbeddedSurface.hpp"

#include <atomic>
#include <hyprtoolkit/element/Null.hpp>

#include "../core/AnimationManager.hpp"
#include "../core/BackendContext.hpp"
#include "../core/EmbeddedBackend.hpp"
#include "../element/Element.hpp"
#include "../renderer/Renderer.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

CEmbeddedSurface::CEmbeddedSurface() {
    m_rootElement = CNullBuilder::begin()->commence();
}

CEmbeddedSurface::~CEmbeddedSurface() {
    close();
}

void CEmbeddedSurface::setSelf(const SP<CEmbeddedSurface>& self, const SP<CEmbeddedBackend>& backend) {
    static std::atomic<uint64_t> nextID = 1;

    m_embeddedSelf = self;
    m_self         = dynamicPointerCast<IToolkitWindow>(self);
    m_backend      = backend;
    m_id           = nextID.fetch_add(1, std::memory_order_relaxed);
}

void CEmbeddedSurface::invalidate() {
    close();
    m_valid = false;
}

void CEmbeddedSurface::open() {
    if (!m_valid || m_open)
        return;

    m_open = true;
    m_rootElement->impl->breadthfirst([this](SP<IElement> element) { element->impl->setWindow(m_self.lock()); });
    m_needsFrame = false;
}

void CEmbeddedSurface::close() {
    if (!m_open)
        return;

    if (g_backendServices)
        resetIM();
    pointerLeave();
    unfocusKeyboard();
    m_touchFocus.clear();
    m_primaryTouch.reset();
    m_rootElement->impl->breadthfirst([](SP<IElement> element) { element->impl->setWindow(nullptr); });
    m_open = false;
}

void CEmbeddedSurface::resize(const Vector2D& logicalSize, const Vector2D& pixelSize, float scale) {
    if (!m_valid || logicalSize.x <= 0 || logicalSize.y <= 0 || pixelSize.x <= 0 || pixelSize.y <= 0 || scale <= 0)
        return;

    m_logicalSize = logicalSize;
    m_pixelSize   = pixelSize;
    m_scale       = scale;
    m_damageRing.setSize(pixelSize);
    m_rootElement->reposition({{}, logicalSize});
    damageEntire();
}

void CEmbeddedSurface::render(uint32_t bufferAge) {
    if (!m_valid || !m_open || !g_renderer || m_pixelSize.x <= 0 || m_pixelSize.y <= 0)
        return;

    const auto backend = m_backend.lock();
    if (!backend || !backend->beginFrame())
        return;

    auto renderer = g_renderer;
    bool began    = false;
    try {
        m_needsFrame = false;
        onPreRender();

        renderer->beginRenderingExternal(m_self.lock(), bufferAge);
        began = true;
        renderer->render(true);
        renderer->endRendering();
        began = false;

        if (g_animationManager->shouldTickForNext())
            scheduleFrame();
    } catch (...) {
        if (began)
            renderer->endRendering();
        renderer.reset();
        backend->endFrame();
        throw;
    }
    renderer.reset();
    backend->endFrame();
}

void CEmbeddedSurface::render() {
    render(1);
}

void CEmbeddedSurface::scheduleFrame() {
    if (!m_valid || m_needsFrame)
        return;

    m_needsFrame = true;
    IEmbeddedSurface::m_events.frameRequested.emit();
}

void CEmbeddedSurface::damage(CRegion&& region) {
    if (!m_valid)
        return;

    auto logical = region.copy();
    IToolkitWindow::damage(std::move(region));
    IEmbeddedSurface::m_events.damaged.emit(logical);
}

void CEmbeddedSurface::damageEntire() {
    if (!m_valid)
        return;

    m_damageRing.damageEntire();
    scheduleFrame();
    IEmbeddedSurface::m_events.damaged.emit(CRegion{CBox{{}, m_logicalSize}});
}

Vector2D CEmbeddedSurface::pixelSize() {
    return m_pixelSize;
}

float CEmbeddedSurface::scale() {
    return m_scale;
}

SP<IElement> CEmbeddedSurface::rootElement() {
    return m_rootElement;
}

EmbeddedSurfaceID CEmbeddedSurface::id() {
    return m_id;
}

void CEmbeddedSurface::pointerEnter(const Vector2D& local) {
    if (!m_valid)
        return;
    mouseEnter(local);
}

void CEmbeddedSurface::pointerMotion(const Vector2D& local) {
    if (!m_valid)
        return;
    mouseMove(local);
}

void CEmbeddedSurface::pointerButton(Input::eMouseButton button, bool pressed) {
    if (!m_valid)
        return;
    mouseButton(button, pressed);
}

void CEmbeddedSurface::pointerAxis(Input::eAxisAxis axis, float delta) {
    if (!m_valid)
        return;
    mouseAxis(axis, delta);
}

void CEmbeddedSurface::pointerLeave() {
    mouseLeave();
}

void CEmbeddedSurface::keyboardEnter() {
    ;
}

void CEmbeddedSurface::keyboardKey(const Input::SKeyboardKeyEvent& event) {
    if (!m_valid)
        return;
    IToolkitWindow::keyboardKey(event);
}

void CEmbeddedSurface::keyboardLeave() {
    unfocusKeyboard();
}

void CEmbeddedSurface::imCommit(const std::string& text) {
    if (!m_valid || !m_keyboardFocus)
        return;
    m_keyboardFocus->imCommitNewText(text);
}

void CEmbeddedSurface::imApply() {
    if (!m_valid || !m_keyboardFocus)
        return;
    m_keyboardFocus->imApplyText();
}

bool CEmbeddedSurface::clippedAt(SP<IElement> element, const Vector2D& local) {
    auto parent = element->impl->parent;
    while (parent) {
        if (parent->impl->clipChildren && !parent->impl->position.containsPoint(local))
            return true;
        parent = parent->impl->parent;
    }
    return false;
}

SP<IElement> CEmbeddedSurface::touchTarget(const Vector2D& local) {
    SP<IElement> target;
    m_rootElement->impl->breadthfirst([this, &target, local](SP<IElement> element) {
        if (!element->acceptsTouchInput() || !element->impl->position.containsPoint(local) || clippedAt(element, local))
            return;
        target = element;
    });
    return target;
}

void CEmbeddedSurface::touchDown(int32_t id, const Vector2D& local, uint32_t timeMs) {
    if (!m_valid || m_touchFocus.contains(id))
        return;

    if (const auto target = touchTarget(local)) {
        const auto elementLocal = local - target->impl->position.pos();
        m_touchFocus.emplace(id, STouchFocus{.element = target, .lastLocal = elementLocal});
        target->impl->m_externalEvents.touchDown.emit(Input::STouchEvent{.id = id, .local = elementLocal, .timeMs = timeMs});
        return;
    }

    if (m_primaryTouch)
        return;

    m_primaryTouch = id;
    pointerEnter(local);
    m_mouseIsDown = true;
}

void CEmbeddedSurface::touchMotion(int32_t id, const Vector2D& local, uint32_t timeMs) {
    if (!m_valid)
        return;

    if (const auto focus = m_touchFocus.find(id); focus != m_touchFocus.end()) {
        if (const auto target = focus->second.element.lock()) {
            focus->second.lastLocal = local - target->impl->position.pos();
            target->impl->m_externalEvents.touchMotion.emit(Input::STouchEvent{.id = id, .local = focus->second.lastLocal, .timeMs = timeMs});
        }
        return;
    }

    if (m_primaryTouch == id)
        pointerMotion(local);
}

void CEmbeddedSurface::touchUp(int32_t id, uint32_t timeMs) {
    if (!m_valid)
        return;

    if (const auto focus = m_touchFocus.find(id); focus != m_touchFocus.end()) {
        if (const auto target = focus->second.element.lock())
            target->impl->m_externalEvents.touchUp.emit(Input::STouchEvent{.id = id, .local = focus->second.lastLocal, .timeMs = timeMs});
        m_touchFocus.erase(focus);
        return;
    }

    if (m_primaryTouch != id)
        return;

    pointerButton(Input::MOUSE_BUTTON_LEFT, true);
    pointerButton(Input::MOUSE_BUTTON_LEFT, false);
    pointerLeave();
    m_primaryTouch.reset();
}

void CEmbeddedSurface::touchCancel(int32_t id, uint32_t timeMs) {
    if (!m_valid)
        return;

    if (const auto focus = m_touchFocus.find(id); focus != m_touchFocus.end()) {
        if (const auto target = focus->second.element.lock())
            target->impl->m_externalEvents.touchCancel.emit(Input::STouchEvent{.id = id, .local = focus->second.lastLocal, .timeMs = timeMs});
        m_touchFocus.erase(focus);
        return;
    }

    if (m_primaryTouch != id)
        return;

    pointerLeave();
    m_mouseIsDown = false;
    m_primaryTouch.reset();
}

void CEmbeddedSurface::setCursor(ePointerShape shape) {
    if (!m_valid || !g_backendServices)
        return;

    g_backendServices->cursor->setShape(m_id, shape);
    IEmbeddedSurface::m_events.cursorChanged.emit(shape);
}

void CEmbeddedSurface::setIMTo(const CBox& box, const std::string& text, size_t cursor) {
    if (!m_valid || !g_backendServices)
        return;

    m_currentInput       = text;
    m_currentInputCursor = cursor;
    g_backendServices->textInput->activate(m_id, box, text, cursor);
}

void CEmbeddedSurface::resetIM() {
    m_currentInput.clear();
    m_currentInputCursor = 0;
    if (g_backendServices)
        g_backendServices->textInput->deactivate(m_id);
}

SP<IWindow> CEmbeddedSurface::openPopup(const SWindowCreationData& data) {
    return nullptr;
}
