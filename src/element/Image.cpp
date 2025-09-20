#include <hyprtoolkit/element/Image.hpp>

#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"
#include "../core/InternalBackend.hpp"

#include "Element.hpp"

using namespace Hyprtoolkit;

SP<CImageElement> CImageElement::create(const SImageData& data) {
    auto p        = SP<CImageElement>(new CImageElement(data));
    p->impl->self = p;
    return p;
}

CImageElement::CImageElement(const SImageData& data) : IElement(), m_data(data) {
    ;
}

void CImageElement::paint() {
    if (!m_tex && m_waitingForTex)
        return;

    if (!m_tex) {
        renderTex();
        return;
    }

    g_renderer->renderTexture({
        .box      = impl->position,
        .texture  = m_tex,
        .a        = 1.F,
        .rounding = 0,
    });
}

void CImageElement::renderTex() {
    m_resource.reset();
    m_tex.reset();

    m_waitingForTex = true;

    m_resource = makeAtomicShared<CImageResource>(m_data.path);

    ASP<IAsyncResource> resourceGeneric(m_resource);

    g_asyncResourceGatherer->enqueue(resourceGeneric);

    m_resource->m_events.finished.listenStatic([this, self = impl->self] {
        if (!self)
            return;

        g_backend->addIdle([this, self = self]() {
            if (!self)
                return;

            ASP<IAsyncResource> resourceGeneric(m_resource);
            m_size = m_resource->m_asset.pixelSize;
            m_tex  = g_renderer->uploadTexture({.resource = resourceGeneric});

            m_waitingForTex = false;
        });
    });
}

void CImageElement::reposition(const Hyprutils::Math::CBox& box) {
    impl->position = box;

    const auto C = impl->children;

    for (const auto& c : C) {
        g_positioner->position(c, impl->position);
    }
}

Hyprutils::Math::Vector2D CImageElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CImageElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_data.size.calculate(parent);
}

std::optional<Vector2D> CImageElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;
    return Vector2D{0, 0};
}

std::optional<Vector2D> CImageElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;
    return std::nullopt;
}
