#include "Combobox.hpp"

#include <hyprtoolkit/palette/Palette.hpp>

#include "../../core/InternalBackend.hpp"
#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../core/AnimationManager.hpp"
#include "../Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CDropdownHandleElement> CDropdownHandleElement::create(const SDropdownHandleData& data) {
    auto p        = SP<CDropdownHandleElement>(new CDropdownHandleElement(data));
    p->impl->self = p;
    return p;
}

CDropdownHandleElement::CDropdownHandleElement(const SDropdownHandleData& data) : IElement(), m_data(data), m_poly(CPolygon::dropdown()) {
    ;
}

void CDropdownHandleElement::paint() {
    g_renderer->renderPolygon(IRenderer::SPolygonRenderData{
        .box   = impl->position,
        .color = m_data.color(),
        .poly  = m_poly,
    });
}

void CDropdownHandleElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

Hyprutils::Math::Vector2D CDropdownHandleElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CDropdownHandleElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_data.size.calculate(parent);
}

std::optional<Vector2D> CDropdownHandleElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_data.size.calculate(parent);
}

std::optional<Vector2D> CDropdownHandleElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_data.size.calculate(parent);
}
