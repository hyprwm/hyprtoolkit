#include "Text.hpp"

#include <cmath>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprgraphics/color/Color.hpp>
#include <pango-1.0/pango/pangocairo.h>

#include "../../window/ToolkitWindow.hpp"
#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../core/Logger.hpp"
#include "../../helpers/Memory.hpp"

#include "../Element.hpp"
#include "../../helpers/UTF8.hpp"
#include "../../system/DesktopMethods.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CTextElement> CTextElement::create(const STextData& data) {
    auto p          = SP<CTextElement>(new CTextElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    return p;
}

CTextElement::CTextElement(const STextData& data) : IElement(), m_impl(makeUnique<STextImpl>()) {
    m_impl->data = data;
    m_impl->parseText();
    m_impl->lastFontSizeUnscaled = m_impl->data.fontSize.ptSize();
    m_impl->preferred            = m_impl->getTextSizePreferred();

    impl->m_externalEvents.mouseMove.listenStatic([this](const Vector2D& pos) {
        m_impl->lastCursorPos = pos;
        m_impl->onMouseMove();
    });

    impl->m_externalEvents.mouseButton.listenStatic([this](const Input::eMouseButton button, bool down) {
        if (!down)
            return;

        m_impl->onMouseDown();
    });
}

CTextElement::~CTextElement() = default;

void CTextElement::replaceData(const STextData& data) {
    const bool TEXT_DIFFERENT = data.text != m_impl->data.text;

    m_impl->data = data;

    if (m_impl->lastFontSizeUnscaled != m_impl->data.fontSize.ptSize() || TEXT_DIFFERENT) {
        m_impl->parseText();
        m_impl->lastFontSizeUnscaled = m_impl->data.fontSize.ptSize();
        m_impl->preferred            = m_impl->getTextSizePreferred();
        m_impl->scheduleTexRefresh();
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
            m_impl->renderTex();
        return;
    }

    if ((impl->window && impl->window->scale() != m_impl->lastScale) || m_impl->needsTexRefresh) {
        m_impl->lastScale = impl->window ? impl->window->scale() : 1.F;
        m_impl->preferred = m_impl->getTextSizePreferred();
        m_impl->renderTex();
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
    if (impl->positionFlags & HT_POSITION_FLAG_HCENTER)
        renderBox.translate({(renderBox.size() - texSizeLogical).x / 2, 0.F});
    if (impl->positionFlags & HT_POSITION_FLAG_VCENTER)
        renderBox.translate({0.F, (renderBox.size() - texSizeLogical).y / 2});
    renderBox.w = texSizeLogical.x;
    renderBox.h = texSizeLogical.y;

    g_renderer->renderTexture({
        .box      = renderBox,
        .texture  = textureToUse,
        .a        = m_impl->data.a,
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

bool CTextElement::acceptsMouseInput() {
    return true;
}

std::function<ePointerShape()> CTextElement::pointerShapeFn() {
    return [this] { return m_impl->hoveredTextLink ? HT_POINTER_POINTER : HT_POINTER_ARROW; };
}

bool CTextElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}

std::tuple<UP<Hyprgraphics::CCairoSurface>, cairo_t*, PangoLayout*, Vector2D> STextImpl::prepPangoLayout() {
    auto                  CAIROSURFACE = makeUnique<CCairoSurface>(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1 /* dummy value */));
    auto                  CAIRO        = cairo_create(CAIROSURFACE->cairo());

    PangoLayout*          layout = pango_cairo_create_layout(CAIRO);

    PangoFontDescription* fontDesc = pango_font_description_from_string(data.fontFamily.c_str());
    pango_font_description_set_size(fontDesc, std::round(lastFontSizeUnscaled * lastScale) * PANGO_SCALE);
    pango_layout_set_font_description(layout, fontDesc);
    pango_font_description_free(fontDesc);

    if (data.align == HT_FONT_ALIGN_LEFT)
        pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
    else if (data.align == HT_FONT_ALIGN_CENTER)
        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    else
        pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);

    PangoAttrList* attrList = nullptr;
    GError*        gError   = nullptr;
    char*          buf      = nullptr;
    if (pango_parse_markup(parsedText.c_str(), -1, 0, &attrList, &buf, nullptr, &gError))
        pango_layout_set_text(layout, buf, -1);
    else {
        g_error_free(gError);
        pango_layout_set_text(layout, parsedText.c_str(), -1);
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

    PangoRectangle ink, logical;
    pango_layout_get_pixel_extents(layout, &ink, &logical);

    std::optional<Vector2D> maxSize = data.clampSize.value_or(lastMaxSize).round();
    if (maxSize == Vector2D{0, 0})
        maxSize = std::nullopt;

    if (maxSize.has_value())
        (*maxSize) *= lastScale;

    if (maxSize.has_value()) {
        const auto CLAMP_SIZE = maxSize.value() * lastScale;
        if (!data.noEllipsize)
            pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
        if (CLAMP_SIZE.x >= 0)
            pango_layout_set_width(layout, std::min(logical.width * PANGO_SCALE, sc<int>(CLAMP_SIZE.x * PANGO_SCALE)));
        if (CLAMP_SIZE.y >= 0)
            pango_layout_set_height(layout, std::min(logical.height * PANGO_SCALE, sc<int>(CLAMP_SIZE.y * PANGO_SCALE)));
        if (CLAMP_SIZE.x >= 0)
            pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

        pango_layout_get_pixel_extents(layout, &ink, &logical);
    }

    pango_layout_get_pixel_extents(layout, &ink, &logical);

    return std::make_tuple<>(std::move(CAIROSURFACE), CAIRO, layout, Vector2D{logical.width, logical.height});
}

Hyprutils::Math::Vector2D STextImpl::getTextSizePreferred() {
    auto [CAIROSURFACE, CAIRO, LAYOUT, LAYOUTSIZE] = prepPangoLayout();

    g_object_unref(LAYOUT);
    cairo_destroy(CAIRO);

    return LAYOUTSIZE / lastScale;
}

CBox STextImpl::getCharBox(size_t charIdxUTF8) {
    auto [CAIROSURFACE, CAIRO, LAYOUT, LAYOUTSIZE] = prepPangoLayout();

    PangoRectangle rect;

    pango_layout_index_to_pos(LAYOUT, UTF8::utf8ToOffset(parsedText, charIdxUTF8), &rect);

    CBox charBox =
        CBox{
            sc<float>(rect.x) / sc<float>(PANGO_SCALE),
            sc<float>(rect.y) / sc<float>(PANGO_SCALE),
            sc<float>(rect.width) / sc<float>(PANGO_SCALE),
            sc<float>(rect.height) / sc<float>(PANGO_SCALE),
        }
            .scale(1.F / lastScale);

    g_object_unref(LAYOUT);
    cairo_destroy(CAIRO);

    return charBox;
}

std::optional<size_t> STextImpl::vecToCharIdx(const Vector2D& vec) {
    auto [CAIROSURFACE, CAIRO, LAYOUT, LAYOUTSIZE] = prepPangoLayout();

    auto pangoX = sc<int>(vec.x * PANGO_SCALE), //
        pangoY  = sc<int>(vec.y * PANGO_SCALE);

    int index = 0, trailing = 0;
    pango_layout_xy_to_index(LAYOUT, pangoX, pangoY, &index, &trailing);

    g_object_unref(LAYOUT);
    cairo_destroy(CAIRO);

    if (index == -1)
        return std::nullopt;

    return UTF8::offsetToUTF8Len(parsedText, index + trailing);
}

float STextImpl::getCursorPos(size_t charIdx) {
    if (charIdx >= UTF8::length(parsedText))
        return preferred.x;

    if (charIdx == 0)
        return 0;

    auto box = getCharBox(charIdx - 1);

    return (box.x + box.w);
}

float STextImpl::getCursorPos(const Hyprutils::Math::Vector2D& click) {
    return getCursorPos(vecToCharIdx(click).value_or(UTF8::length(parsedText)));
}

Vector2D STextImpl::unscale(const Vector2D& x) {
    if (!self->impl->window)
        return x + Vector2D{self->impl->margin * 2, self->impl->margin * 2};
    return (x + Vector2D{self->impl->margin * 2, self->impl->margin * 2}) / self->impl->window->scale();
}

void STextImpl::scheduleTexRefresh() {
    if (data.async) {
        needsTexRefresh = true;
        return;
    }
}

void STextImpl::renderTex() {
    oldTex          = tex;
    needsTexRefresh = false;

    resource.reset();
    tex.reset();

    waitingForTex = true;

    lastScale = self->impl->window ? self->impl->window->scale() : 1.F;

    self->impl->damageEntire();

    std::optional<Vector2D> maxSize = data.clampSize.value_or(lastMaxSize).round();
    if (maxSize == Vector2D{0, 0})
        maxSize = std::nullopt;

    if (maxSize.has_value())
        (*maxSize) *= lastScale;

    auto col = data.color();

    resource = makeAtomicShared<CTextResource>(CTextResource::STextResourceData{
        .text      = parsedText,
        .font      = data.fontFamily,
        .fontSize  = sc<size_t>(std::round(lastFontSizeUnscaled * lastScale)),
        .color     = CColor{CColor::SSRGB{.r = col.r, .g = col.g, .b = col.b}},
        .align     = data.align == HT_FONT_ALIGN_LEFT ?
                Hyprgraphics::CTextResource::TEXT_ALIGN_LEFT :
                (data.align == HT_FONT_ALIGN_CENTER ? Hyprgraphics::CTextResource::TEXT_ALIGN_CENTER : Hyprgraphics::CTextResource::TEXT_ALIGN_RIGHT),
        .maxSize   = maxSize,
        .ellipsize = maxSize.has_value() && maxSize->y >= 0,
        .wrap      = maxSize.has_value() && maxSize->x >= 0,
    });

    ASP<IAsyncResource> resourceGeneric(resource);

    g_asyncResourceGatherer->enqueue(resourceGeneric);

    if (!data.async) {
        g_asyncResourceGatherer->await(resourceGeneric);
        postTexLoad();
    } else {
        resource->m_events.finished.listenStatic([this, self = self->impl->self] {
            if (!self)
                return;

            g_backend->addIdle([this, self = self]() {
                if (!self)
                    return;

                postTexLoad();
            });
        });
    }
}

void STextImpl::postTexLoad() {
    if (!resource)
        return;

    ASP<IAsyncResource> resourceGeneric(resource);
    size = resource->m_asset.pixelSize;
    tex  = g_renderer->uploadTexture({.resource = resourceGeneric});
    oldTex.reset();
    if (self->impl->window)
        self->impl->window->scheduleReposition(self->impl->self);

    waitingForTex = false;
    newTex        = true;
    resource.reset();

    recheckTextBoxes();

    if (data.callback)
        data.callback();
}

static std::string formatColor(uint32_t col) {
    return std::format("#{0:08x}", ((col & 0x00ffffffu) << 8) | (col >> 24));
}

void STextImpl::parseText() {
    parsedLinks.clear();
    hoveredTextLink = nullptr;

    size_t      lastTagClose = 0;
    const auto& ORIGINAL     = data.text;
    std::string newString;

    while (true) {
        size_t tagOpen = ORIGINAL.find("<a href=\"", lastTagClose);

        if (tagOpen == std::string::npos) {
            // no more tags
            newString += ORIGINAL.substr(lastTagClose);
            break;
        }

        newString += ORIGINAL.substr(lastTagClose, tagOpen - lastTagClose);

        // find the close
        size_t linkOpen  = tagOpen + 9;
        size_t linkClose = ORIGINAL.find('"', linkOpen);

        if (linkClose == std::string::npos)
            break; // broken tag

        const std::string_view LINK = std::string_view{ORIGINAL}.substr(linkOpen, linkClose - linkOpen);

        // expect spaces or >
        size_t needle = linkClose + 1;
        while (needle < ORIGINAL.size() && (ORIGINAL[needle] == ' ')) {
            needle++;
        }

        if (needle >= ORIGINAL.size() || ORIGINAL[needle] != '>')
            break; // broken tag

        size_t contentOpen  = needle + 1;
        size_t contentClose = ORIGINAL.find("</a>", contentOpen);

        if (contentClose == std::string::npos)
            break; // broken tag

        std::string replaceWith = std::format("<u><span foreground=\"{}\">{}</span></u>", formatColor(g_palette->m_colors.linkText.getAsHex()),
                                              std::string_view{ORIGINAL}.substr(contentOpen, contentClose - contentOpen));

        parsedLinks.emplace_back(STextLink{
            .begin = newString.size(),
            .end   = newString.size() + replaceWith.size(),
            .link  = std::string{LINK},
        });

        newString += replaceWith;

        lastTagClose = contentClose + 4;
    }

    parsedText = std::move(newString);
}

void STextImpl::recheckTextBoxes() {
    for (auto& link : parsedLinks) {
        link.region.clear();
        for (size_t i = link.begin; i < link.end + 1; ++i) {
            auto box = getCharBox(UTF8::offsetToUTF8Len(parsedText, i));
            link.region.add(box);
        }
    }
}

void STextImpl::onMouseDown() {
    if (!hoveredTextLink)
        return;

    g_logger->log(HT_LOG_DEBUG, "STextImpl::onMouseDown: running link {}", hoveredTextLink->link);

    auto ret = DesktopMethods::openLink(hoveredTextLink->link);

    if (!ret)
        g_logger->log(HT_LOG_ERROR, "STextImpl::onMouseDown: failed to open link, ret: {}", ret.error());
}

void STextImpl::onMouseMove() {
    hoveredTextLink = nullptr;

    for (auto& link : parsedLinks) {
        if (!link.region.containsPoint(lastCursorPos))
            continue;

        hoveredTextLink = &link;
        break;
    }
}
