#include <hyprtoolkit/element/Text.hpp>

#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"
#include "../core/InternalBackend.hpp"
#include "../helpers/Memory.hpp"

#include "Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CTextElement> CTextElement::create(const STextData& data) {
    auto p = SP<CTextElement>(new CTextElement(data));
    p->m_self = p;
    return p;
}

CTextElement::CTextElement(const STextData& data) : IElement(), m_data(data) {
    m_resource = makeAtomicShared<CTextResource>(CTextResource::STextResourceData{
        .text = data.text,
        // .font
        // .fontSize
        .color = data.color,
        // .align
        .maxSize = data.clampSize,
    });

    ASP<IAsyncResource> resourceGeneric(m_resource);

    g_asyncResourceGatherer->enqueue(resourceGeneric);

    // FIXME: UAF possible, store wref
    m_resource->m_events.finished.listenStatic([this] {
        g_backend->addIdle(
            [this]() {
                ASP<IAsyncResource> resourceGeneric(m_resource);
                m_size = m_resource->m_asset.pixelSize;
                m_tex  = g_renderer->uploadTexture({.resource = resourceGeneric});
                if (m_elementData->parent)
                    g_positioner->position(m_elementData->parent.lock(), m_elementData->parent->m_position);
            });
    });
}

void CTextElement::paint() {
    if (!m_tex)
        return;

    g_renderer->renderTexture({
        .box      = m_position,
        .texture  = m_tex,
        .a        = 1.F,
        .rounding = 0,
    });
}

void CTextElement::reposition(const Hyprutils::Math::CBox& box) {
    m_position   = box;
    m_position.w = m_size.x;
    m_position.h = m_size.y;

    const auto C = m_elementData->children;

    for (const auto& c : C) {
        g_positioner->position(c, m_position);
    }
}

Hyprutils::Math::Vector2D CTextElement::size() {
    return m_size;
}

std::optional<Vector2D> CTextElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_size;
}

std::optional<Vector2D> CTextElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_size;
}

std::optional<Vector2D> CTextElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_size;
}
