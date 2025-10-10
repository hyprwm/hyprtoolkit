#include "Text.hpp"

#include <cmath>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprgraphics/color/Color.hpp>
#include <pango-1.0/pango/pangocairo.h>

#include "../../window/ToolkitWindow.hpp"
#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../helpers/Memory.hpp"

#include "../Element.hpp"
#include "../../helpers/UTF8.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CTextElement> CTextElement::create(const STextData& data) {
    auto p          = SP<CTextElement>(new CTextElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    return p;
}

CTextElement::CTextElement(const STextData& data) : IElement(), m_impl(makeUnique<STextImpl>()) {
    m_impl->data                 = data;
    m_impl->lastFontSizeUnscaled = m_impl->data.fontSize.ptSize();
    m_impl->preferred            = m_impl->getTextSizePreferred();
}

CTextElement::~CTextElement() = default;

void CTextElement::replaceData(const STextData& data) {
    const bool TEXT_DIFFERENT = data.text != m_impl->data.text;

    m_impl->data = data;

    if (m_impl->lastFontSizeUnscaled != m_impl->data.fontSize.ptSize() || TEXT_DIFFERENT) {
        m_impl->lastFontSizeUnscaled = m_impl->data.fontSize.ptSize();
        m_impl->preferred            = m_impl->getTextSizePreferred();
        m_impl->needsTexRefresh      = true;
    }

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

SP<CTextBuilder> CTextElement::rebuild() {
    auto p       = SP<CTextBuilder>(new CTextBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<STextData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
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
        m_impl->lastScale = impl->window ? impl->window->scale() : 1.F;
        m_impl->preferred = m_impl->getTextSizePreferred();
        renderTex();
        textureToUse = m_impl->oldTex;
    }

    if (!textureToUse)
        return; // ???

    if (m_impl->newTex) {
        m_impl->newTex = false;
        impl->damageEntire();
    }

    CBox     renderBox      = impl->position;
    Vector2D texSizeLogical = m_impl->size / impl->window->scale();
    if (impl->positionFlags & HT_POSITION_FLAG_CENTER)
        renderBox.translate((renderBox.size() - texSizeLogical) / 2);
    else if (impl->positionFlags & HT_POSITION_FLAG_HCENTER)
        renderBox.translate({(renderBox.size() - texSizeLogical).x / 2, 0.F});
    else if (impl->positionFlags & HT_POSITION_FLAG_VCENTER)
        renderBox.translate({0.F, (renderBox.size() - texSizeLogical).y / 2});
    renderBox.w = texSizeLogical.x;
    renderBox.h = texSizeLogical.y;

    g_renderer->renderTexture({
        .box      = renderBox,
        .texture  = textureToUse,
        .a        = 1.F,
        .rounding = 0,
    });
}

void CTextElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    const auto DESIRED = m_impl->preferred;
    if (DESIRED.x > 0 && DESIRED.y > 0 && !m_impl->data.noEllipsize) {
        const auto PREV     = m_impl->lastMaxSize;
        m_impl->lastMaxSize = {-1, -1};
        const auto SIZE     = box.size();
        if (maxSize.x > 0)
            m_impl->lastMaxSize.x = maxSize.x;
        if (maxSize.y > 0)
            m_impl->lastMaxSize.y = maxSize.y;
        if (SIZE.x + 1 < DESIRED.x)
            m_impl->lastMaxSize.x = SIZE.x;
        if (SIZE.y + 1 < DESIRED.y)
            m_impl->lastMaxSize.y = SIZE.y;

        if (PREV != m_impl->lastMaxSize)
            m_impl->needsTexRefresh = true;
    }

    g_positioner->positionChildren(impl->self.lock());
}

void CTextElement::renderTex() {
    m_impl->oldTex          = m_impl->tex;
    m_impl->needsTexRefresh = false;

    m_impl->resource.reset();
    m_impl->tex.reset();

    m_impl->waitingForTex = true;

    m_impl->lastScale = impl->window ? impl->window->scale() : 1.F;

    impl->damageEntire();

    std::optional<Vector2D> maxSize = m_impl->data.clampSize.value_or(m_impl->lastMaxSize);
    if (maxSize == Vector2D{0, 0})
        maxSize = std::nullopt;

    auto col = m_impl->data.color();

    m_impl->resource = makeAtomicShared<CTextResource>(CTextResource::STextResourceData{
        .text     = m_impl->data.text,
        .font     = m_impl->data.fontFamily,
        .fontSize = sc<size_t>(std::round(m_impl->lastFontSizeUnscaled * m_impl->lastScale)),
        .color    = CColor{CColor::SSRGB{.r = col.r, .g = col.g, .b = col.b}},
        // .align
        .maxSize   = maxSize,
        .ellipsize = maxSize.has_value() && maxSize->y != -1,
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
            m_impl->oldTex.reset();
            if (impl->window)
                impl->window->scheduleReposition(impl->self);

            m_impl->waitingForTex = false;
            m_impl->newTex        = true;

            if (m_impl->data.callback)
                m_impl->data.callback();
        });
    });
}

void CTextElement::recheckColor() {
    m_impl->needsTexRefresh = true;
}

Hyprutils::Math::Vector2D CTextElement::size() {
    return m_impl->unscale(m_impl->size);
}

std::optional<Vector2D> CTextElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return std::nullopt;
}

std::optional<Vector2D> CTextElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_impl->preferred;
}

std::optional<Vector2D> CTextElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return Vector2D{0, 0};
}

bool CTextElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}

std::tuple<UP<Hyprgraphics::CCairoSurface>, cairo_t*, PangoLayout*, Vector2D> STextImpl::prepPangoLayout() {
    auto                  CAIROSURFACE = makeUnique<CCairoSurface>(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1 /* dummy value */));
    auto                  CAIRO        = cairo_create(CAIROSURFACE->cairo());

    PangoLayout*          layout = pango_cairo_create_layout(CAIRO);

    PangoFontDescription* fontDesc = pango_font_description_from_string("Sans Serif");
    pango_font_description_set_size(fontDesc, std::round(lastFontSizeUnscaled * lastScale) * PANGO_SCALE);
    pango_layout_set_font_description(layout, fontDesc);
    pango_font_description_free(fontDesc);

    pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);

    PangoAttrList* attrList = nullptr;
    GError*        gError   = nullptr;
    char*          buf      = nullptr;
    if (pango_parse_markup(data.text.c_str(), -1, 0, &attrList, &buf, nullptr, &gError))
        pango_layout_set_text(layout, buf, -1);
    else {
        g_error_free(gError);
        pango_layout_set_text(layout, data.text.c_str(), -1);
    }

    if (!attrList)
        attrList = pango_attr_list_new();

    if (buf)
        free(buf);

    pango_attr_list_insert(attrList, pango_attr_scale_new(1));
    pango_layout_set_attributes(layout, attrList);
    pango_attr_list_unref(attrList);

    int layoutWidth, layoutHeight;
    pango_layout_get_size(layout, &layoutWidth, &layoutHeight);

    if (data.clampSize) {
        layoutWidth  = data.clampSize->x > 0 ? std::min(layoutWidth, sc<int>(data.clampSize->x * PANGO_SCALE)) : layoutWidth;
        layoutHeight = data.clampSize->y > 0 ? std::min(layoutHeight, sc<int>(data.clampSize->y * PANGO_SCALE)) : layoutHeight;
        if (!data.noEllipsize)
            pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
        if (data.clampSize->x >= 0)
            pango_layout_set_width(layout, layoutWidth);
        if (data.clampSize->y >= 0)
            pango_layout_set_height(layout, layoutHeight);

        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

        pango_layout_get_size(layout, &layoutWidth, &layoutHeight);
    }

    return std::make_tuple<>(std::move(CAIROSURFACE), CAIRO, layout, Vector2D{sc<float>(layoutWidth) / PANGO_SCALE, sc<float>(layoutHeight) / PANGO_SCALE});
}

Hyprutils::Math::Vector2D STextImpl::getTextSizePreferred() {
    auto [CAIROSURFACE, CAIRO, LAYOUT, LAYOUTSIZE] = prepPangoLayout();

    cairo_destroy(CAIRO);

    return LAYOUTSIZE / lastScale;
}

CBox STextImpl::getCharBox(size_t charIdxUTF8) {
    auto [CAIROSURFACE, CAIRO, LAYOUT, LAYOUTSIZE] = prepPangoLayout();

    PangoRectangle rect;

    pango_layout_index_to_pos(LAYOUT, UTF8::utf8ToOffset(data.text, charIdxUTF8), &rect);

    CBox charBox =
        CBox{
            sc<float>(rect.x) / sc<float>(PANGO_SCALE),
            sc<float>(rect.y) / sc<float>(PANGO_SCALE),
            sc<float>(rect.width) / sc<float>(PANGO_SCALE),
            sc<float>(rect.height) / sc<float>(PANGO_SCALE),
        }
            .scale(1.F / lastScale);

    cairo_destroy(CAIRO);

    return charBox;
}

std::optional<size_t> STextImpl::vecToCharIdx(const Vector2D& vec) {
    auto [CAIROSURFACE, CAIRO, LAYOUT, LAYOUTSIZE] = prepPangoLayout();

    auto pangoX = sc<int>(vec.x * PANGO_SCALE), //
        pangoY  = sc<int>(vec.y * PANGO_SCALE);

    int index = 0, trailing = 0;
    pango_layout_xy_to_index(LAYOUT, pangoX, pangoY, &index, &trailing);

    cairo_destroy(CAIRO);

    if (index == -1)
        return std::nullopt;

    return UTF8::offsetToUTF8Len(data.text, index + trailing);
}

float STextImpl::getCursorPos(size_t charIdx) {
    if (charIdx >= UTF8::length(data.text))
        return preferred.x;

    if (charIdx == 0)
        return 0;

    auto box = getCharBox(charIdx - 1);

    return (box.x + box.w);
}

float STextImpl::getCursorPos(const Hyprutils::Math::Vector2D& click) {
    return getCursorPos(vecToCharIdx(click).value_or(UTF8::length(data.text)));
}

Vector2D STextImpl::unscale(const Vector2D& x) {
    if (!self->impl->window)
        return x + Vector2D{self->impl->margin * 2, self->impl->margin * 2};
    return (x + Vector2D{self->impl->margin * 2, self->impl->margin * 2}) / self->impl->window->scale();
}
