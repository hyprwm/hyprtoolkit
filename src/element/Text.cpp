#include "Text.hpp"

#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprgraphics/color/Color.hpp>

#include "../window/ToolkitWindow.hpp"
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

CTextElement::CTextElement(const STextData& data) : IElement(), m_data(data), m_impl(makeUnique<STextImpl>()) {
    m_impl->lastFontSizeUnscaled = m_data.fontSize.ptSize();
}

STextData CTextElement::dataCopy() {
    return m_data;
}

void CTextElement::replaceData(const STextData& data) {
    m_data                       = data;
    m_impl->lastFontSizeUnscaled = m_data.fontSize.ptSize();
    renderTex();
}

void CTextElement::paint() {
    SP<IRendererTexture> textureToUse = m_impl->tex;

    if (!m_impl->tex)
        textureToUse = m_impl->oldTex;

    if (!textureToUse) {
        if (!m_impl->waitingForTex)
            renderTex();
        return;
    }

    if ((impl->window && impl->window->scale() != m_impl->lastScale) || m_impl->needsTexRefresh) {
        renderTex();
        textureToUse = m_impl->oldTex;
    }

    if (!textureToUse)
        return; // ???

    CBox renderBox = impl->position;
    renderBox.w    = unscale().x;
    renderBox.h    = unscale().y;

    g_renderer->renderTexture({
        .box      = renderBox,
        .texture  = textureToUse,
        .a        = 1.F,
        .rounding = 0,
    });
}

void CTextElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    if (m_impl->lastMaxSize != maxSize) {
        m_impl->needsTexRefresh = true;
        m_impl->lastMaxSize     = maxSize;
    }

    const auto C = impl->children;

    for (const auto& c : C) {
        g_positioner->position(c, impl->position);
    }
}

void CTextElement::renderTex() {
    m_impl->oldTex          = m_impl->tex;
    m_impl->needsTexRefresh = false;

    m_impl->resource.reset();
    m_impl->tex.reset();

    m_impl->waitingForTex = true;

    m_impl->lastScale = impl->window ? impl->window->scale() : 1.F;

    std::optional<Vector2D> maxSize = m_data.clampSize.value_or(m_impl->lastMaxSize);
    if (maxSize == Vector2D{0, 0})
        maxSize = std::nullopt;

    m_impl->resource = makeAtomicShared<CTextResource>(CTextResource::STextResourceData{
        .text = m_data.text,
        // .font
        .fontSize = sc<size_t>(m_impl->lastFontSizeUnscaled * m_impl->lastScale),
        .color    = CColor{CColor::SSRGB{.r = m_data.color.r, .g = m_data.color.g, .b = m_data.color.b}},
        // .align
        .maxSize = maxSize,
    });

    ASP<IAsyncResource> resourceGeneric(m_impl->resource);

    g_asyncResourceGatherer->enqueue(resourceGeneric);

    m_impl->resource->m_events.finished.listenStatic([this, self = impl->self] {
        if (!self)
            return;

        g_backend->addIdle([this, self = self]() {
            if (!self)
                return;

            ASP<IAsyncResource> resourceGeneric(m_impl->resource);
            m_impl->size = m_impl->resource->m_asset.pixelSize;
            m_impl->tex  = g_renderer->uploadTexture({.resource = resourceGeneric});
            if (impl->parent)
                g_positioner->repositionNeeded(impl->self.lock());

            m_impl->waitingForTex = false;

            if (m_data.callback)
                m_data.callback();
        });
    });
}

Hyprutils::Math::Vector2D CTextElement::unscale() {
    if (!impl->window)
        return m_impl->size;
    return m_impl->size / impl->window->scale();
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
