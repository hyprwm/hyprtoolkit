#include "Text.hpp"

#include <cmath>
#include <glib-object.h>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprgraphics/color/Color.hpp>
#include <pango/pango.h>

#include "../../window/ToolkitWindow.hpp"
#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../core/BackendContext.hpp"
#include "../../core/Logger.hpp"
#include "../../core/AnimationManager.hpp"
#include "../../helpers/Memory.hpp"

#include "../Element.hpp"
#include "../../helpers/UTF8.hpp"
#include "../../system/DesktopMethods.hpp"
#include "Macros.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CTextElement> CTextElement::create(const STextData& data) {
    auto p                          = SP<CTextElement>(new CTextElement(data));
    p->impl->self                   = p;
    p->m_impl->self                 = p;
    p->m_impl->colorAnimationConfig = g_animationManager->m_animationTree.getConfig("fast");
    g_animationManager->createAnimation(data.color(), p->m_impl->color, p->m_impl->colorAnimationConfig);
    p->m_impl->color->setCallbackOnBegin(
        [self = WP<CTextElement>{p}](auto) {
            if (self)
                self->impl->damageEntire();
        },
        false);
    p->m_impl->color->setUpdateCallback([self = WP<CTextElement>{p}](auto) {
        if (self)
            self->impl->damageEntire();
    });
    return p;
}

CTextElement::CTextElement(const STextData& data) : IElement(), m_impl(makeUnique<STextImpl>()) {
    m_impl->data = data;
    m_impl->setPangoData();
    m_impl->parseText();

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

void CTextElement::setText(std::string text) {
    if (text == m_impl->data.text)
        return;

    m_impl->data.text = std::move(text);
    m_impl->parseText();
    m_impl->scheduleTexRefresh();

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

void CTextElement::replaceData(const STextData& data) {
    const bool TEXT_DIFFERENT      = data.text != m_impl->data.text;
    const auto COLOR               = data.color();
    const bool COLOR_DIFFERENT     = COLOR != m_impl->color->goal();
    const bool ALIGN_DIFFERENT     = data.align != m_impl->data.align;
    const bool ELLIPSIZE_DIFFERENT = data.noEllipsize != m_impl->data.noEllipsize;
    const bool FONT_DIFFERENT      = data.fontFamily != m_impl->data.fontFamily || data.fontSize.ptSize() != m_impl->data.fontSize.ptSize();

    m_impl->data = data;

    if (ALIGN_DIFFERENT)
        m_impl->setPangoAlign();
    if (ELLIPSIZE_DIFFERENT)
        m_impl->setPangoEllipsize();
    if (FONT_DIFFERENT)
        m_impl->setPangoFont();
    if (TEXT_DIFFERENT)
        m_impl->parseText();

    if (m_impl->colorAnimationEnabled) {
        if (m_impl->color->isBeingAnimated() && (m_impl->color->getConfig() != m_impl->colorAnimationConfig || (m_impl->color->isSpringCurve() && m_impl->color->goal() != COLOR)))
            m_impl->color->setValueAndWarp(m_impl->color->value());
        m_impl->color->setConfig(m_impl->colorAnimationConfig);
        *m_impl->color = COLOR;
    } else {
        m_impl->color->setValueAndWarp(COLOR);
        m_impl->needsTexRefresh = m_impl->needsTexRefresh || COLOR_DIFFERENT;
    }

    if (ALIGN_DIFFERENT || ELLIPSIZE_DIFFERENT || FONT_DIFFERENT || TEXT_DIFFERENT)
        m_impl->scheduleTexRefresh();

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

void CTextElement::animateColor(const SAnimation& animation) {
    m_impl->colorAnimationEnabled = !std::holds_alternative<SNoAnimation>(animation);
    m_impl->colorAnimationConfig  = g_animationManager->configFor(animation);
    m_impl->needsTexRefresh       = true;
    if (!m_impl->colorAnimationEnabled)
        m_impl->color->setValueAndWarp(m_impl->color->goal());
    impl->damageEntire();
}

void CTextElement::paint() {
    SP<IRendererTexture> textureToUse = m_impl->tex;

    if (!m_impl->tex)
        textureToUse = m_impl->oldTex;

    if (!textureToUse) {
        if (!m_impl->waitingForTex && !m_impl->resource)
            m_impl->renderTex();
        return;
    }

    m_impl->updateScale();
    if (m_impl->needsTexRefresh) {
        if (!m_impl->resource)
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

    const auto COLOR = m_impl->color->value();
    g_renderer->renderTexture({
        .box               = renderBox,
        .texture           = textureToUse,
        .a                 = sc<float>(m_impl->data.a * (m_impl->renderColorAtPaint ? COLOR.a : 1.F)),
        .rounding          = 0,
        .tint              = m_impl->renderColorAtPaint ? std::optional<CHyprColor>{COLOR.stripA()} : std::nullopt,
        .tintGrayscaleOnly = m_impl->renderColorAtPaint,
    });
}

void CTextElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    auto initial_position = impl->position;

    IElement::reposition(box);

    if (initial_position != impl->position)
        m_impl->needsTexRefresh = true;

    g_positioner->positionChildren(impl->self.lock());

    pango_layout_set_width(m_impl->pangoData.layout, sc<int>(impl->position.size().x * PANGO_SCALE * m_impl->lastScale));
    pango_layout_set_height(m_impl->pangoData.layout, sc<int>(impl->position.size().y * PANGO_SCALE * m_impl->lastScale));
}

void CTextElement::recheckColor() {
    const auto COLOR = m_impl->data.color();
    if (m_impl->colorAnimationEnabled) {
        if (m_impl->color->isBeingAnimated() && (m_impl->color->getConfig() != m_impl->colorAnimationConfig || (m_impl->color->isSpringCurve() && m_impl->color->goal() != COLOR)))
            m_impl->color->setValueAndWarp(m_impl->color->value());
        m_impl->color->setConfig(m_impl->colorAnimationConfig);
        *m_impl->color = COLOR;
    } else {
        m_impl->color->setValueAndWarp(COLOR);
        m_impl->needsTexRefresh = true;
    }
}

Hyprutils::Math::Vector2D CTextElement::size() {
    return m_impl->unscale(m_impl->size);
}

std::optional<Vector2D> CTextElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return std::nullopt;
}

std::optional<Vector2D> CTextElement::preferredSize(const Hyprutils::Math::Vector2D& parent, bool grow) {
    auto s = m_impl->data.size.calculate(parent, grow);
    if (s.x != -1 && s.y != -1)
        return s;

    auto maxSize = parent - Vector2D{impl->margin * 2, impl->margin * 2};
    if (s.x != -1)
        maxSize.x = s.x;
    if (s.y != -1)
        maxSize.y = s.y;
    maxSize = m_impl->applyClampSize(maxSize);

    m_impl->updateScale();
    auto LAYOUT = m_impl->pangoData.layout;
    if (maxSize.x != 0)
        pango_layout_set_width(LAYOUT, sc<int>(maxSize.x * PANGO_SCALE * m_impl->lastScale));
    else
        pango_layout_set_width(LAYOUT, -1);
    if (maxSize.y != 0)
        pango_layout_set_height(LAYOUT, sc<int>(maxSize.y * PANGO_SCALE * m_impl->lastScale));
    else
        pango_layout_set_height(LAYOUT, -1);

    PangoRectangle ink, logical;
    pango_layout_get_pixel_extents(LAYOUT, &ink, &logical);

    auto result = Vector2D{std::ceil(logical.width / m_impl->lastScale), std::ceil(logical.height / m_impl->lastScale)} + Vector2D(impl->margin * 2, impl->margin * 2);
    if (s.x == -1)
        s.x = result.x;
    if (s.y == -1)
        s.y = result.y;

    pango_layout_set_width(m_impl->pangoData.layout, sc<int>(impl->position.size().x * PANGO_SCALE * m_impl->lastScale));
    pango_layout_set_height(m_impl->pangoData.layout, sc<int>(impl->position.size().y * PANGO_SCALE * m_impl->lastScale));

    return s;
}

std::optional<Vector2D> CTextElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return Vector2D{0, 0};
}

bool CTextElement::acceptsMouseInput() {
    return m_impl->data.interactable.value_or(!m_impl->parsedLinks.empty()) || IElement::acceptsMouseInput();
}

std::function<ePointerShape()> CTextElement::pointerShapeFn() {
    return [this] { return m_impl->hoveredTextLink ? HT_POINTER_POINTER : HT_POINTER_ARROW; };
}

bool CTextElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}

SPangoData::SPangoData() : layout(nullptr), context(nullptr) {}

SPangoData::SPangoData(PangoLayout* layout, PangoContext* context) : layout(layout), context(context) {}

SPangoData::SPangoData(SPangoData&& other) noexcept : layout(other.layout), context(other.context) {
    ref();
}

SPangoData& SPangoData::operator=(SPangoData&& other) noexcept {
    if (this != &other) {
        unref();

        layout  = other.layout;
        context = other.context;

        ref();
    }

    return *this;
}

SPangoData::~SPangoData() {
    unref();
}

void SPangoData::unref() const {
    if (layout)
        g_object_unref(layout);
    if (context)
        g_object_unref(context);
}

void SPangoData::ref() const {
    if (layout)
        g_object_ref(layout);
    if (context)
        g_object_ref(context);
}

void STextImpl::setPangoData() {
    PangoFontMap* font_map = pango_cairo_font_map_get_default();
    PangoContext* context  = pango_font_map_create_context(font_map);
    PangoLayout*  layout   = pango_layout_new(context);

    pangoData = {layout, context};

    setPangoFont();
    setPangoAlign();
    setPangoEllipsize();
}

void STextImpl::setPangoFont() {
    PangoFontDescription* fontDesc = pango_font_description_from_string(data.fontFamily.c_str());
    pango_font_description_set_size(fontDesc, std::round(data.fontSize.ptSize() * lastScale) * PANGO_SCALE);
    pango_layout_set_font_description(pangoData.layout, fontDesc);
    pango_font_description_free(fontDesc);
}

void STextImpl::setPangoAlign() {
    if (data.align == HT_FONT_ALIGN_LEFT)
        pango_layout_set_alignment(pangoData.layout, PANGO_ALIGN_LEFT);
    else if (data.align == HT_FONT_ALIGN_CENTER)
        pango_layout_set_alignment(pangoData.layout, PANGO_ALIGN_CENTER);
    else
        pango_layout_set_alignment(pangoData.layout, PANGO_ALIGN_RIGHT);
}

void STextImpl::setPangoText() {
    PangoAttrList* attrList = nullptr;
    GError*        gError   = nullptr;
    char*          buf      = nullptr;
    if (pango_parse_markup(parsedText.c_str(), -1, 0, &attrList, &buf, nullptr, &gError))
        pango_layout_set_text(pangoData.layout, buf, -1);
    else {
        g_error_free(gError);
        pango_layout_set_text(pangoData.layout, parsedText.c_str(), -1);
    }

    if (!attrList)
        attrList = pango_attr_list_new();

    if (buf)
        free(buf);

    pango_attr_list_insert(attrList, pango_attr_scale_new(1));
    pango_layout_set_attributes(pangoData.layout, attrList);
    pango_attr_list_unref(attrList);
}

void STextImpl::setPangoEllipsize() {
    if (data.noEllipsize) {
        pango_layout_set_wrap(pangoData.layout, PANGO_WRAP_WORD_CHAR);
        pango_layout_set_ellipsize(pangoData.layout, PANGO_ELLIPSIZE_NONE);
    } else {
        pango_layout_set_wrap(pangoData.layout, PANGO_WRAP_NONE);
        pango_layout_set_ellipsize(pangoData.layout, PANGO_ELLIPSIZE_END);
    }
}

void STextImpl::updateScale() {
    if (self && self->impl->window && self->impl->window->scale() != lastScale) {
        lastScale = self->impl->window->scale();
        setPangoFont();
        needsTexRefresh = true;
    }
}

CBox STextImpl::getCharBox(size_t offset) {
    auto           LAYOUT = pangoData.layout;

    PangoRectangle rect;

    pango_layout_index_to_pos(LAYOUT, offset, &rect);

    CBox charBox =
        CBox{
            sc<float>(rect.x) / sc<float>(PANGO_SCALE),
            sc<float>(rect.y) / sc<float>(PANGO_SCALE),
            sc<float>(rect.width) / sc<float>(PANGO_SCALE),
            sc<float>(rect.height) / sc<float>(PANGO_SCALE),
        }
            .scale(1.F / lastScale);

    return charBox;
}

std::optional<size_t> STextImpl::vecToOffset(const Vector2D& vec) {
    auto LAYOUT = pangoData.layout;

    auto pangoX = sc<int>(vec.x * PANGO_SCALE), //
        pangoY  = sc<int>(vec.y * PANGO_SCALE);

    int index = 0, trailing = 0;
    pango_layout_xy_to_index(LAYOUT, pangoX, pangoY, &index, &trailing);

    if (index == -1)
        return std::nullopt;

    return index + trailing;
}

float STextImpl::getCursorPos(size_t offset) {
    if (offset >= parsedText.length() && !parsedText.empty()) {
        auto box = getCharBox(parsedText.length() - 1);
        return box.x + box.size().x;
    } else if (parsedText.empty())
        return 0;

    if (offset == 0)
        return 0;

    auto box = getCharBox(offset);

    return box.x;
}

float STextImpl::getCursorPos(const Hyprutils::Math::Vector2D& click) {
    return getCursorPos(vecToOffset(click).value_or(parsedText.length()));
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

    ASSERT(!resource);

    tex.reset();

    waitingForTex = true;

    self->impl->damageEntire();

    const Vector2D MAX_SIZE = applyClampSize(self->impl->position.size()) * lastScale;
    const auto     COLOR    = colorAnimationEnabled ? CHyprColor{1.F, 1.F, 1.F, 1.F} : data.color();
    resource                = makeAtomicShared<CTextResource>(CTextResource::STextResourceData{
                       .text      = parsedText,
                       .font      = data.fontFamily,
                       .fontSize  = sc<size_t>(std::round(data.fontSize.ptSize() * lastScale)),
                       .color     = CColor{CColor::SSRGB{.r = COLOR.r, .g = COLOR.g, .b = COLOR.b}},
                       .align     = data.align == HT_FONT_ALIGN_LEFT ?
                               Hyprgraphics::CTextResource::TEXT_ALIGN_LEFT :
                               (data.align == HT_FONT_ALIGN_CENTER ? Hyprgraphics::CTextResource::TEXT_ALIGN_CENTER : Hyprgraphics::CTextResource::TEXT_ALIGN_RIGHT),
                       .maxSize   = MAX_SIZE,
                       .ellipsize = !data.noEllipsize,
                       .wrap      = data.noEllipsize,
    });

    if (Env::isTrace()) {
        const std::string TEXT_SHORT = data.text.size() > 20 ? data.text.substr(0, 20) + "..." : data.text;
        g_logger->log(HT_LOG_TRACE, "TextImpl: scheduling rendering of text \"{}\", with the following params:\nfont: {}, fontSize: {}, maxSize: {}, ellipsize: {}, wrap: {}",
                      TEXT_SHORT,                                                 //
                      data.fontFamily,                                            //
                      sc<size_t>(std::round(data.fontSize.ptSize() * lastScale)), //
                      MAX_SIZE,                                                   //
                      !data.noEllipsize,                                          //
                      data.noEllipsize                                            //
        );
    }

    ASP<IAsyncResource> resourceGeneric(resource);

    if (!data.async) {
        g_asyncResourceGatherer->enqueue(resourceGeneric);
        g_asyncResourceGatherer->await(resourceGeneric);
        postTexLoad();
    } else {
        resource->m_events.finished.listenStatic([this, self = self->impl->self, lifetime = WP<SBackendLifetime>{g_backendServices->lifetime}] {
            if (self.expired() || !lifetime)
                return;
            if (!g_backend)
                return;

            g_backend->addIdle([this, self = self, lifetime] {
                if (self.expired() || !lifetime)
                    return;

                postTexLoad();
            });
        });

        g_asyncResourceGatherer->enqueue(resourceGeneric);
    }
}

void STextImpl::postTexLoad() {
    if (!resource)
        return;

    ASP<IAsyncResource> resourceGeneric(resource);
    size               = resource->m_asset.pixelSize;
    tex                = g_renderer->uploadTexture({.resource = resourceGeneric});
    renderColorAtPaint = colorAnimationEnabled;
    oldTex.reset();
    if (self->impl->window)
        self->impl->window->scheduleReposition(self->impl->self);

    if (Env::isTrace()) {
        const std::string TEXT_SHORT = data.text.size() > 20 ? data.text.substr(0, 20) + "..." : data.text;

        if (size.x == 0 || size.y == 0 || !resourceGeneric->m_asset.cairoSurface)
            g_logger->log(HT_LOG_ERROR, "TextImpl: failed to render text \"{}\"!!!", TEXT_SHORT);
        else
            g_logger->log(HT_LOG_TRACE, "TextImpl: got a tex with size {} for text \"{}\"", size, TEXT_SHORT);
    }

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

    setPangoText();
}

void STextImpl::recheckTextBoxes() {
    for (auto& link : parsedLinks) {
        link.region.clear();
        for (size_t i = link.begin; i < link.end + 1; ++i) {
            auto box = getCharBox(i);
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

Vector2D STextImpl::applyClampSize(Vector2D size) {
    if (!data.clampSize.has_value())
        return size;

    if (size.x <= 0 || (data.clampSize->x < size.x && data.clampSize->x > 0))
        size.x = data.clampSize->x;
    if (size.y <= 0 || (data.clampSize->y < size.y && data.clampSize->y > 0))
        size.y = data.clampSize->y;

    return size;
}
