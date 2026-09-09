#include "Image.hpp"

#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../core/BackendContext.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../system/Icons.hpp"
#include "../../resource/assetCache/AssetCache.hpp"
#include "../../renderer/RendererTexture.hpp"
#include <random>

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
    if (m_impl->transition.active) { // precedence over the failed/fallback guard
        renderTransition();
        return;
    }

    auto assetToUse = m_impl->cacheEntry;

    if (!assetToUse || !assetToUse->tex())
        assetToUse = m_impl->oldCacheEntry;

    if (m_impl->failed && (!assetToUse || !assetToUse->tex()))
        return; // blank only if failed AND nothing to show

    if (!assetToUse || !assetToUse->tex()) {
        if (!m_impl->waitingForTex)
            renderTex();
        return;
    }

    if (impl->window)
        m_impl->lastScale = impl->window->scale();

    if (m_impl->scalable() && m_impl->preferredSvgSize() != m_impl->size && !m_impl->waitingForTex) {
        renderTex();
        assetToUse = m_impl->oldCacheEntry;
    }

    if (!assetToUse || !assetToUse->tex())
        return; // ???

    g_renderer->renderTexture({
        .box      = impl->position,
        .texture  = assetToUse->tex(),
        .a        = m_impl->data.a,
        .rounding = m_impl->data.rounding,
        .fitMode  = m_impl->data.fitMode,
    });
}

void CImageElement::renderTex() {
    const uint64_t GENERATION = ++m_impl->requestedGen;
    // Disarm any prior cached-listener so a stale callback for an old request cannot
    // fire after this new request.
    m_impl->listeners.cacheEntryDone.reset();
    m_impl->failed = false;
    if (m_impl->cacheEntry && m_impl->cacheEntry->tex())
        m_impl->oldCacheEntry = m_impl->cacheEntry;
    m_impl->cacheEntry.reset();

    const auto CACHE_STR = m_impl->getCacheString();

    const auto ASSET = Asset::assetCache()->get(CACHE_STR);
    if (ASSET) {
        g_logger->log(HT_LOG_DEBUG, "CImageElement: path {} was already cached, reusing entry", m_impl->data.path);
        m_impl->failed     = false;
        m_impl->cacheEntry = ASSET;
        if (ASSET->status() == Asset::CACHE_ENTRY_DONE)
            m_impl->postImageScheduleRecalc(m_impl->requestedGen);
        else {
            m_impl->waitingForTex            = true;
            m_impl->listeners.cacheEntryDone = ASSET->m_events.done.listen([this, self = impl->self, generation = GENERATION, entry = WP<Asset::CAssetCacheEntry>{ASSET}] {
                if (!self || generation != m_impl->requestedGen)
                    return; // stale callback: a newer request superseded this one

                const auto ENTRY = entry.lock();
                m_impl->failed   = !ENTRY || ENTRY->status() == Asset::CACHE_ENTRY_FAILED;
                m_impl->postImageScheduleRecalc(generation);
                m_impl->listeners.cacheEntryDone.reset();
            });
        }
        return;
    }

    const auto REQUEST  = makeShared<SImageLoadRequest>();
    REQUEST->generation = GENERATION;
    REQUEST->fitMode    = m_impl->data.fitMode;
    REQUEST->cacheEntry = makeShared<Asset::CAssetCacheEntry>(CACHE_STR);
    m_impl->cacheEntry  = REQUEST->cacheEntry;

    if (!m_impl->data.data.empty()) {
        const auto SIZE   = m_impl->preferredSvgSize();
        REQUEST->resource = makeAtomicShared<CImageResource>(m_impl->data.data, SIZE);
        m_impl->lastData  = m_impl->data.data.data();
    } else if (!m_impl->data.icon) {
        REQUEST->resource = makeAtomicShared<CImageResource>(m_impl->data.path);
        REQUEST->path     = m_impl->data.path;
        m_impl->lastPath  = m_impl->data.path;
    } else {
        REQUEST->path     = reinterpretPointerCast<CSystemIconDescription>(m_impl->data.icon)->m_bestPath;
        m_impl->lastPath  = REQUEST->path;
        const auto SIZE   = m_impl->preferredSvgSize();
        REQUEST->resource = makeAtomicShared<CImageResource>(REQUEST->path, SIZE);

        if (SIZE.x == 0 || SIZE.y == 0)
            return;
    }

    Asset::assetCache()->cache(REQUEST->cacheEntry);
    m_impl->requests.emplace_back(REQUEST);

    m_impl->waitingForTex = true;
    m_impl->inflightGen   = m_impl->requestedGen;
    m_impl->failed        = false; // a fresh attempt is not blocked by a stale failure flag

    ASP<IAsyncResource> resourceGeneric(REQUEST->resource);

    if (!m_impl->data.sync) {
        // attach listener before enqueueing to avoid missing the finished event
        REQUEST->resource->m_events.finished.listenStatic(
            [this, self = impl->self, lifetime = WP<SBackendLifetime>{g_backendServices->lifetime}, request = WP<SImageLoadRequest>{REQUEST}] {
                if (!self || !lifetime || !request)
                    return;

                g_backend->addIdle([this, self = self, lifetime, request] {
                    if (!self || !lifetime)
                        return;

                    const auto REQUEST = request.lock();
                    if (REQUEST)
                        m_impl->postImageLoad(REQUEST);
                });
            });

        g_asyncResourceGatherer->enqueue(resourceGeneric);
    } else {
        g_asyncResourceGatherer->enqueue(resourceGeneric);
        g_asyncResourceGatherer->await(resourceGeneric);
        m_impl->postImageLoad(REQUEST);
    }
}

void SImageImpl::postImageLoad(const SP<SImageLoadRequest>& request) {
    if (!request || !request->resource || !request->cacheEntry)
        return;

    bool loaded = false;
    if (request->resource->m_asset.cairoSurface) {
        ASP<IAsyncResource> resourceGeneric(request->resource);
        const auto          imageSize = request->resource->m_asset.pixelSize;

        const auto          MAX_SIZE = g_renderer->getMaxTextureSize();
        if (imageSize.x > MAX_SIZE || imageSize.y > MAX_SIZE) {
            request->cacheEntry->fail();
            g_logger->log(HT_LOG_ERROR, "Image: failed loading, image dimensions {}x{} exceed max texture size {}", (int)imageSize.x, (int)imageSize.y, MAX_SIZE);
        } else {
            request->cacheEntry->texDone(g_renderer->uploadTexture({.resource = resourceGeneric, .fitMode = request->fitMode}));
            loaded = true;
        }
    } else {
        request->cacheEntry->fail();
        g_logger->log(HT_LOG_ERROR, "Image: failed loading, hyprgraphics couldn't load asset {}", request->path);
    }

    const bool CURRENT = request->generation == requestedGen;
    if (CURRENT) {
        cacheEntry = request->cacheEntry;
        failed     = !loaded;
        if (loaded) {
            size           = request->resource->m_asset.pixelSize;
            oldCacheEntry.reset(); // only on success: on failure keep it as the visible fallback
        }
    }

    std::erase(requests, request);

    if (CURRENT)
        postImageScheduleRecalc(request->generation);
}

void SImageImpl::postImageScheduleRecalc(uint64_t gen) {
    // Ignore a stale callback when a load for the newest request is already in flight
    // (e.g. a cached-listener for an old request firing after a newer load started).
    if (gen != requestedGen && inflightGen == requestedGen)
        return;

    waitingForTex = false;
    satisfiedGen  = gen;

    // Newest request unsatisfied AND no load for it in flight -> load it now (current data).
    if (satisfiedGen != requestedGen && inflightGen != requestedGen) {
        self->renderTex();
        return; // defer damage until the newest request settles
    }

    // Settled on the newest request. On failure this size update is a deliberate
    // no-op: paint() owns fallback display (oldCacheEntry is kept alive on failure);
    // we still damage below so the fallback is repainted.
    if (!failed && cacheEntry && cacheEntry->tex())
        size = cacheEntry->tex()->size();
    self->impl->damageEntire();

    if (self->impl->window)
        self->impl->window->scheduleReposition(self);
}

std::string SImageImpl::getCacheString() {
    if (!data.data.empty()) // uncacheable
        return std::format("data-{:x}", rc<uintptr_t>(data.data.data()));

    if (!data.icon)
        return data.path.ends_with(".svg") ? std::format("{}-{}x{}-{}", data.path, preferredSvgSize().x, preferredSvgSize().y, sc<int>(data.fitMode)) :
                                             std::format("{}-{}", data.path, sc<int>(data.fitMode));
    else
        return std::format("icon-{}-{}x{}-{}", reinterpretPointerCast<CSystemIconDescription>(data.icon)->m_bestPath, preferredSvgSize().x, preferredSvgSize().y,
                           sc<int>(data.fitMode));
}

bool SImageImpl::scalable() const {
    return data.icon ? data.icon->scalable() : data.path.ends_with(".svg");
}

SP<CImageBuilder> CImageElement::rebuild() {
    auto p       = SP<CImageBuilder>(new CImageBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<SImageData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

void CImageElement::replaceData(const SImageData& data) {
    m_impl->requestedGen++;
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

void CImageElement::transitionTo(const std::string& path,
                                  eImageFitMode fitMode,
                                  float duration,
                                  const std::string& shaderSource) {
    // New transition: drop the previous transition's cached fit-resolved FBOs.
    g_renderer->clearTransitionCache();

    if (m_impl->transition.active) {
        // Capture the in-flight blend to an FBO and use it as the new start texture.
        if (m_impl->transition.startTexture && m_impl->transition.endTexture) {
            IRenderer::STransitionRenderData captureData = {
                .box = impl->position,
                .startTexture = m_impl->transition.startTexture,
                .endTexture = m_impl->transition.endTexture,
                .progress = m_impl->transition.progress,
                .a = 1.F,
                .fitMode = m_impl->data.fitMode,
                .shaderKey = m_impl->transition.shaderKey,
                .randomPixel = m_impl->transition.randomPixel,
                .duration = m_impl->transition.duration,
            };

            auto capturedTex = g_renderer->captureTransitionState(captureData);
            if (capturedTex)
                m_impl->transition.startTexture = capturedTex;
        }
    } else if (m_impl->cacheEntry && m_impl->cacheEntry->tex()) {
        // Pre-render the current wallpaper to a full-screen, fit-resolved frame at the fit
        // mode it is currently displayed with (data.fitMode is still the outgoing mode here;
        // it is reassigned below). The fit mode is consumed at pre-render time and is not
        // carried as transition state, so the start frame stays fixed for the whole transition.
        m_impl->transition.startTexture = g_renderer->renderFitFrame(m_impl->cacheEntry->tex(), impl->position, m_impl->data.fitMode);
    }

    m_impl->transition.active     = true;
    m_impl->transition.progress   = 0.0f;
    m_impl->transition.shaderKey  = g_renderer->ensureTransitionShader(shaderSource);
    m_impl->transition.startTime  = std::chrono::steady_clock::now();

    m_impl->transition.randomPixel = Hyprutils::Math::Vector2D(
        static_cast<float>(rand() % 1000) / 1000.0f,
        static_cast<float>(rand() % 1000) / 1000.0f
    );

    // GLES cannot initialize uniform values, so u_duration is pushed to the shader
    // each frame; a shader may further remap progress via a local const.
    // duration <= 0 means an immediate (non-animated) swap.
    m_impl->transition.duration = std::max(0.0f, duration);

    // Populated once the new asset finishes loading.
    m_impl->transition.endTexture.reset();

    m_impl->requestedGen++;
    m_impl->data.path     = path;
    m_impl->data.fitMode = fitMode;
    renderTex();

    if (impl->window)
        impl->window->scheduleReposition(impl->self.lock());
}

bool CImageElement::isTransitioning() const {
    return m_impl->transition.active;
}

float CImageElement::getTransitionProgressOut() const {
    return m_impl->transition.progress;
}

float CImageElement::getTransitionProgressIn() const {
    return m_impl->transition.progress;
}

void CImageElement::renderTransition() {
    auto& trans = m_impl->transition;

    auto  now     = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - trans.startTime).count();
    trans.progress = (trans.duration > 0.0f) ? std::clamp(elapsed / trans.duration, 0.0f, 1.0f) : 1.0f;

    const bool targetReady  = m_impl->cacheEntry && m_impl->cacheEntry->tex();
    const bool targetFailed = m_impl->failed;

    if (trans.progress >= 1.0f) {
        if (targetReady) {
            // Normal completion.
            trans.active = false;
            trans.startTexture.reset();
            trans.endTexture.reset();
            g_renderer->clearTransitionCache();
            g_renderer->renderTexture({
                .box = impl->position,
                .texture = m_impl->cacheEntry->tex(),
                .a = 1.F,
                .rounding = 0,
                .fitMode = m_impl->data.fitMode,
            });
            return;
        }
        if (targetFailed) {
            // Failure completion: stop transitioning. oldCacheEntry is kept alive on
            // failure, so the next paint() shows the previous image. Render the start
            // texture for this frame if present.
            trans.active = false;
            g_renderer->clearTransitionCache();
            if (trans.startTexture)
                g_renderer->renderTexture({
                    .box = impl->position,
                    .texture = trans.startTexture,
                    .a = 1.F,
                    .rounding = 0,
                });
            trans.startTexture.reset();
            trans.endTexture.reset();
            return;
        }
        // Still loading after the duration elapsed: keep showing the outgoing frame and
        // DO NOT schedule animation frames (progress is saturated).
        // postImageScheduleRecalc() damages on load completion.
        if (trans.startTexture)
            g_renderer->renderTexture({
                .box = impl->position,
                .texture = trans.startTexture,
                .a = 1.F,
                .rounding = 0,
            });
        return; // no scheduleReposition -> no busy-repaint while waiting
    }

    // The end texture is resolved once the new asset finishes loading.
    SP<IRendererTexture> endTex;
    if (m_impl->cacheEntry && m_impl->cacheEntry->tex()) {
        endTex = m_impl->cacheEntry->tex();
        trans.endTexture = endTex;
    } else
        endTex = trans.endTexture;

    if (trans.startTexture && endTex) {
        g_renderer->renderTransition({
            .box = impl->position,
            .startTexture = trans.startTexture,
            .endTexture = endTex,
            .progress = trans.progress,
            .a = 1.F,
            .rounding = 0,
            .fitMode = m_impl->data.fitMode,
            .shaderKey = trans.shaderKey,
            .randomPixel = trans.randomPixel,
            .duration = trans.duration,
        });
    } else if (trans.startTexture) {
        // End texture not loaded yet, keep showing the start.
        g_renderer->renderTexture({
            .box = impl->position,
            .texture = trans.startTexture,
            .a = 1.F,
            .rounding = 0,
        });
    }

    if (impl->window)
        impl->window->scheduleReposition(impl->self.lock());
}
