#include "Spinbox.hpp"

#include <hyprtoolkit/palette/Palette.hpp>

#include "../../core/InternalBackend.hpp"
#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../core/AnimationManager.hpp"
#include "../Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CSpinboxAngleElement> CSpinboxAngleElement::create(const SSpinboxAngleData& data) {
    auto p        = SP<CSpinboxAngleElement>(new CSpinboxAngleElement(data));
    p->impl->self = p;
    return p;
}

CSpinboxAngleElement::CSpinboxAngleElement(const SSpinboxAngleData& data) : IElement(), m_data(data), m_poly(data.right ? CPolygon::rangle() : CPolygon::langle()) {
    g_animationManager->createAnimation(data.color(), m_color, g_animationManager->m_animationTree.getConfig("fast"));
    m_color->setUpdateCallback([this](auto) { impl->damageEntire(); });
    m_color->setCallbackOnBegin([this](auto) { impl->damageEntire(); }, false);

    impl->m_externalEvents.mouseEnter.listenStatic([this](const Vector2D& pos) {
        m_primedForUp = false;
        *m_color      = m_data.colorActive();
    });

    impl->m_externalEvents.mouseLeave.listenStatic([this]() {
        m_primedForUp = false;
        *m_color      = m_data.color();
    });

    impl->m_externalEvents.mouseButton.listenStatic([this](const Input::eMouseButton button, bool down) {
        if (button != Input::MOUSE_BUTTON_LEFT)
            return;

        if (down) {
            m_primedForUp = true;
            return;
        }

        if (!m_primedForUp)
            return;

        m_data.spinner->moveSelection(m_data.right);
    });
}

void CSpinboxAngleElement::paint() {
    g_renderer->renderPolygon(IRenderer::SPolygonRenderData{
        .box   = impl->position,
        .color = m_color->value(),
        .poly  = m_poly,
    });
}

void CSpinboxAngleElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

Hyprutils::Math::Vector2D CSpinboxAngleElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CSpinboxAngleElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_data.size.calculate(parent);
}

std::optional<Vector2D> CSpinboxAngleElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_data.size.calculate(parent);
}

std::optional<Vector2D> CSpinboxAngleElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_data.size.calculate(parent);
}

bool CSpinboxAngleElement::acceptsMouseInput() {
    return true;
}

ePointerShape CSpinboxAngleElement::pointerShape() {
    return HT_POINTER_POINTER;
}
