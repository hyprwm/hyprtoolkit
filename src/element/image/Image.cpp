#include "Image.hpp"

#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../system/Icons.hpp"
#include "../../resource/assetCache/AssetCache.hpp"
#include "../../renderer/RendererTexture.hpp"

#include "../Element.hpp"

using namespace Hyprtoolkit;

SP<CImageElement> CImageElement::create(const SImageData& data) {
    auto p          = SP<CImageElement>(new CImageElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    return p;
}

CImageElement::CImageElement(const SImageData& data) : IElement(), m_impl(makeUnique<SImageImpl>()) {
    m_impl->data = data;
}

void CImageElement::paint() {
    if (m_impl->failed)
        return;

    auto assetToUse = m_impl->cacheEntry;

    if (!assetToUse || !assetToUse->tex())
        assetToUse = m_impl->oldCacheEntry;

    if (!assetToUse || !assetToUse->tex()) {
        if (!m_impl->waitingForTex)
            renderTex();
        return;
    }

    if (impl->window)
        m_impl->lastScale = impl->window->scale();

    if (m_impl->data.icon && m_impl->preferredSvgSize() != m_impl->size && !m_impl->waitingForTex) {
        renderTex();
        assetToUse = m_impl->oldCacheEntry;
    }

    if (!assetToUse || !assetToUse->tex())
        return; // ???

    g_renderer->renderTexture({
        .box      = impl->position,
        .texture  = assetToUse->tex(),
        .a        = 1.F,
        .rounding = 0,
    });
}

void CImageElement::renderTex() {
    if (m_impl->waitingForTex)
        return;

    m_impl->resource.reset();
    m_impl->oldCacheEntry = m_impl->cacheEntry;
    m_impl->cacheEntry.reset();

    const auto CACHE_STR = m_impl->getCacheString();

    const auto ASSET = Asset::assetCache()->get(CACHE_STR);
    if (ASSET) {
        g_logger->log(HT_LOG_DEBUG, "CImageElement: path {} was already cached, reusing entry", m_impl->data.path);
        m_impl->failed     = false;
        m_impl->cacheEntry = ASSET;
        if (ASSET->status() == Asset::CACHE_ENTRY_DONE)
            m_impl->postImageScheduleRecalc();
        else {
            m_impl->listeners.cacheEntryDone = ASSET->m_events.done.listen([this] {
                m_impl->postImageScheduleRecalc();
                m_impl->listeners.cacheEntryDone.reset();
            });
        }
        return;
    }

    if (!m_impl->data.data.empty()) {
        const auto SIZE  = m_impl->preferredSvgSize();
        m_impl->resource = makeAtomicShared<CImageResource>(m_impl->data.data, SIZE);
        m_impl->lastData = m_impl->data.data.data();
    } else if (!m_impl->data.icon) {
        m_impl->resource = makeAtomicShared<CImageResource>(m_impl->data.path);
        m_impl->lastPath = m_impl->data.path;
    } else {
        m_impl->lastPath = reinterpretPointerCast<CSystemIconDescription>(m_impl->data.icon)->m_bestPath;
        const auto SIZE  = m_impl->preferredSvgSize();
        m_impl->resource = makeAtomicShared<CImageResource>(m_impl->lastPath, SIZE);

        if (SIZE.x == 0 || SIZE.y == 0)
            return;
    }

    m_impl->cacheEntry = makeShared<Asset::CAssetCacheEntry>(CACHE_STR);
    Asset::assetCache()->cache(m_impl->cacheEntry);

    m_impl->waitingForTex = true;

    ASP<IAsyncResource> resourceGeneric(m_impl->resource);

    g_asyncResourceGatherer->enqueue(resourceGeneric);

    if (!m_impl->data.sync) {
        m_impl->resource->m_events.finished.listenStatic([this, self = impl->self] {
            if (!self)
                return;

            g_backend->addIdle([this, self = self]() {
                if (!self)
                    return;

                m_impl->postImageLoad();
            });
        });
    } else {
        g_asyncResourceGatherer->await(resourceGeneric);
        m_impl->postImageLoad();
    }
}

void SImageImpl::postImageLoad() {
    if (!resource || !cacheEntry)
        return;

    if (resource->m_asset.cairoSurface) {
        ASP<IAsyncResource> resourceGeneric(resource);
        size = resource->m_asset.pixelSize;
        cacheEntry->texDone(g_renderer->uploadTexture({.resource = resourceGeneric, .fitMode = data.fitMode}));
    } else {
        failed = true;
        g_logger->log(HT_LOG_ERROR, "Image: failed loading, hyprgraphics couldn't load asset {}", lastPath);
    }

    oldCacheEntry.reset();
    resource.reset();

    postImageScheduleRecalc();
}

void SImageImpl::postImageScheduleRecalc() {
    waitingForTex = false;
    if (!failed) {
        if (cacheEntry && cacheEntry->tex())
            size = cacheEntry->tex()->size();
        self->impl->damageEntire();

        if (self->impl->window)
            self->impl->window->scheduleReposition(self);
    }
}

std::string SImageImpl::getCacheString() {
    if (!data.data.empty()) // uncacheable
        return std::format("data-{:x}", rc<uintptr_t>(data.data.data()));

    if (!data.icon)
        return data.path.ends_with(".svg") ? std::format("{}-{}x{}", data.path, preferredSvgSize().x, preferredSvgSize().y) : data.path;
    else
        return std::format("icon-{}-{}x{}", reinterpretPointerCast<CSystemIconDescription>(data.icon)->m_bestPath, preferredSvgSize().x, preferredSvgSize().y);
}

SP<CImageBuilder> CImageElement::rebuild() {
    auto p       = SP<CImageBuilder>(new CImageBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<SImageData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

void CImageElement::replaceData(const SImageData& data) {
    m_impl->data = data;

    renderTex();

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

void CImageElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

Hyprutils::Math::Vector2D CImageElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CImageElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;

    const float SCALE = impl->window ? impl->window->scale() : 1.F;

    if (s.x == -1 && s.y == -1)
        return m_impl->size / SCALE;

    if (m_impl->size.y == 0)
        return impl->getPreferredSizeGeneric(m_impl->data.size, parent);

    const double ASPECT_RATIO = m_impl->size.x / m_impl->size.y;

    if (s.y == -1)
        return Vector2D{s.x, s.x * (1 / ASPECT_RATIO)};

    return Vector2D{ASPECT_RATIO * s.y, s.y};
}

std::optional<Vector2D> CImageElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;
    return Vector2D{0, 0};
}

std::optional<Vector2D> CImageElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;
    return std::nullopt;
}

bool CImageElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}

Vector2D SImageImpl::preferredSvgSize() {
    auto max = std::max(self->impl->position.size().x, self->impl->position.size().y);

    return Vector2D{max * lastScale, max * lastScale}.round();
}
