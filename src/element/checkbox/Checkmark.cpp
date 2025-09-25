#include "Checkbox.hpp"

#include <hyprtoolkit/palette/Palette.hpp>

#include "../../core/InternalBackend.hpp"
#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../core/AnimationManager.hpp"
#include "../Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CCheckmarkElement> CCheckmarkElement::create(const SCheckmarkData& data) {
    auto p        = SP<CCheckmarkElement>(new CCheckmarkElement(data));
    p->impl->self = p;
    return p;
}

CCheckmarkElement::CCheckmarkElement(const SCheckmarkData& data) : IElement(), m_data(data) {
    g_animationManager->createAnimation(data.color(), m_color, g_animationManager->m_animationTree.getConfig("fast"));
    m_color->setUpdateCallback([this](auto) { impl->damageEntire(); });
    m_color->setCallbackOnBegin([this](auto) { impl->damageEntire(); }, false);
}

void CCheckmarkElement::paint() {
    g_renderer->renderPolygon(IRenderer::SPolygonRenderData{
        .box   = impl->position,
        .color = m_color->value(),
        .poly  = CPolygon::checkmark(),
    });
}

void CCheckmarkElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

Hyprutils::Math::Vector2D CCheckmarkElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CCheckmarkElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_data.size.calculate(parent);
}

std::optional<Vector2D> CCheckmarkElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_data.size.calculate(parent);
}

std::optional<Vector2D> CCheckmarkElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_data.size.calculate(parent);
}
