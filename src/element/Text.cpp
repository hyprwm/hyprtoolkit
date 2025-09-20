#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/window/Window.hpp>

#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"
#include "../core/InternalBackend.hpp"
#include "../helpers/Memory.hpp"

#include "Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CTextElement> CTextElement::create(const STextData& data) {
    auto p        = SP<CTextElement>(new CTextElement(data));
    p->impl->self = p;
    return p;
}

CTextElement::CTextElement(const STextData& data) : IElement(), m_data(data) {
    ;
}

void CTextElement::paint() {
    if (!m_tex && m_waitingForTex)
        return;

    if (!m_tex) {
        renderTex();
        return;
    }

    if (impl->window && impl->window->scale() != m_lastScale) {
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

void CTextElement::reposition(const Hyprutils::Math::CBox& box) {
    impl->position   = box;
    impl->position.w = unscale().x;
    impl->position.h = unscale().y;

    const auto C = impl->children;

    for (const auto& c : C) {
        g_positioner->position(c, impl->position);
    }
}

void CTextElement::renderTex() {
    m_resource.reset();
    m_tex.reset();

    m_waitingForTex = true;

    m_lastScale = impl->window ? impl->window->scale() : 1.F;

    m_resource = makeAtomicShared<CTextResource>(CTextResource::STextResourceData{
        .text = m_data.text,
        // .font
        .fontSize = sc<size_t>(m_rawFontSize * m_lastScale),
        .color    = m_data.color,
        // .align
        .maxSize = m_data.clampSize,
    });

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
            if (impl->parent)
                g_positioner->position(impl->parent.lock(), impl->parent->impl->position);

            m_waitingForTex = false;
        });
    });
}

Hyprutils::Math::Vector2D CTextElement::unscale() {
    if (!impl->window)
        return m_size;
    return m_size / impl->window->scale();
}

Hyprutils::Math::Vector2D CTextElement::size() {
    return unscale();
}

std::optional<Vector2D> CTextElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return unscale();
}

std::optional<Vector2D> CTextElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return unscale();
}

std::optional<Vector2D> CTextElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return unscale();
}
