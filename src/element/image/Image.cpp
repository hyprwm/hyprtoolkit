#include "Image.hpp"

#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../core/BackendContext.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../system/Icons.hpp"
#include "../../resource/assetCache/AssetCache.hpp"
#include "../../renderer/RendererTexture.hpp"
#include "../../core/AnimationManager.hpp"

#include "../Element.hpp"

#include <random>

using namespace Hyprtoolkit;
using namespace std::chrono_literals;

static bool imageSourcesDiffer(const SImageData& a, const SImageData& b) {
    return a.path != b.path || a.icon != b.icon || a.data != b.data;
}

static Hyprutils::Math::Vector2D randomSeed() {
    static std::mt19937                          rng{std::random_device{}()};
    static std::uniform_real_distribution<float> dist{0.F, 1.F};
    return {dist(rng), dist(rng)};
}

// Renders `tex` into a fit-resolved frame of `pixelSize` via the normal renderTexture path
static SP<IRendererTexture> renderFrame(const SP<IRendererTexture>& tex, const Hyprutils::Math::Vector2D& pixelSize) {
    return g_renderer->renderToTexture(pixelSize, [&tex, &pixelSize] {
        g_renderer->renderTexture(IRenderer::STextureRenderData{
            .box     = {0, 0, pixelSize.x, pixelSize.y},
            .texture = tex,
            .a       = 1.F,
        });
    });
}

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
    if (m_impl->transition.pending || m_impl->transitionActive()) {
        m_impl->paintTransition();
        if (m_impl->transitionActive() || (m_impl->transition.pending && m_impl->transition.from))
            return;
    }

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
    });
}

void CImageElement::renderTex() {
    const uint64_t GENERATION = ++m_impl->requestGeneration;
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
            m_impl->postImageScheduleRecalc();
        else {
            m_impl->waitingForTex            = true;
            m_impl->listeners.cacheEntryDone = ASSET->m_events.done.listen([this, self = impl->self, generation = GENERATION, entry = WP<Asset::CAssetCacheEntry>{ASSET}] {
                if (!self || generation != m_impl->requestGeneration)
                    return;

                const auto ENTRY = entry.lock();
                m_impl->failed   = !ENTRY || ENTRY->status() == Asset::CACHE_ENTRY_FAILED;
                m_impl->postImageScheduleRecalc();
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

    const bool CURRENT = request->generation == requestGeneration;
    if (CURRENT) {
        cacheEntry = request->cacheEntry;
        failed     = !loaded;
        if (loaded)
            size = request->resource->m_asset.pixelSize;
        oldCacheEntry.reset();
    }

    std::erase(requests, request);

    if (CURRENT)
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
        return data.path.ends_with(".svg") ? std::format("{}-{}x{}-{}", data.path, preferredSvgSize().x, preferredSvgSize().y, sc<int>(data.fitMode)) :
                                             std::format("{}-{}", data.path, sc<int>(data.fitMode));
    else
        return std::format("icon-{}-{}x{}-{}", reinterpretPointerCast<CSystemIconDescription>(data.icon)->m_bestPath, preferredSvgSize().x, preferredSvgSize().y,
                           sc<int>(data.fitMode));
}

bool SImageImpl::scalable() const {
    return data.icon ? data.icon->scalable() : data.path.ends_with(".svg");
}

bool SImageImpl::transitionWanted(const SImageData& newData) const {
    return imageSourcesDiffer(data, newData) && newData.transition.duration > 0ms && (transitionActive() || transition.pending || displayedTexture());
}

bool SImageImpl::transitionActive() const {
    return transition.progress && transition.progress->isBeingAnimated();
}

SP<IRendererTexture> SImageImpl::displayedTexture() const {
    if (cacheEntry && cacheEntry->tex())
        return cacheEntry->tex();
    if (oldCacheEntry && oldCacheEntry->tex())
        return oldCacheEntry->tex();
    return nullptr;
}

void SImageImpl::requestTransition() {
    if (transitionActive()) {
        // freeze in place: the in-flight blend becomes the next transition's start frame
        transition.capturePending = true;
        transition.progress->setValueAndWarp(transition.progress->value());
        transition.pending = true;
        return;
    }

    if (!transition.fromSource) {
        transition.fromSource = displayedTexture();
        if (!transition.fromSource)
            return; // nothing displayed yet: upstream swap
    }

    transition.pending = true;
}

void SImageImpl::cancelTransition() {
    if (transition.progress) {
        if (transitionActive())
            transition.progress->setValueAndWarp(transition.progress->value());
        transition.progress->resetAllCallbacks();
    }
    transition.pending        = false;
    transition.capturePending = false;
    transition.from.reset();
    transition.to.reset();
    transition.fromSource.reset();
}

static void startTransitionAnimation(SImageImpl* impl) {
    g_animationManager->createAnimation(0.F, impl->transition.progress,
                                        g_animationManager->configFor(SBezierAnimation{.duration = impl->data.transition.duration, .control1 = {0, 0}, .control2 = {1, 1}}));

    // setCallbackOnEnd fires immediately on a not-animating variable, so the goal comes first
    *impl->transition.progress = 1.F;

    impl->transition.progress->setUpdateCallback([impl](WP<Hyprutils::Animation::CBaseAnimatedVariable>) { impl->self->impl->damageEntire(); });
    impl->transition.progress->setCallbackOnEnd(
        [impl](WP<Hyprutils::Animation::CBaseAnimatedVariable>) {
            if (impl->transition.capturePending)
                return; // frozen for an interrupt: the frames are captured in paint()
            impl->transition.from.reset();
            impl->transition.to.reset();
            impl->transition.fromSource.reset();
            impl->self->impl->damageEntire();
        },
        false);
}

static Hyprutils::Math::Vector2D transitionPixelSize(const SImageImpl* impl) {
    return (impl->self->impl->position.size() * impl->lastScale).round();
}

void SImageImpl::paintTransition() {
    if (self->impl->window)
        lastScale = self->impl->window->scale();

    if (failed) {
        cancelTransition();
        return;
    }

    if (transition.capturePending) {
        // freeze the running blend into a texture with the old shader, seed, duration and progress
        transition.from       = g_renderer->renderToTexture(transition.frameSize, [this] {
            g_renderer->renderTransition(IRenderer::STransitionRenderData{
                .box      = {0, 0, transition.frameSize.x, transition.frameSize.y},
                .from     = transition.from,
                .to       = transition.to,
                .shader   = transition.shader,
                .progress = transition.progress->value(),
                .a        = 1.F,
                .duration = transition.durationSecs,
                .seed     = transition.seed,
            });
        });
        transition.fromSource = transition.from;
        transition.to.reset();
        transition.capturePending = false;
    }

    const auto PIXELS = transitionPixelSize(this);
    const auto NEWTEX = cacheEntry && cacheEntry->tex() ? cacheEntry->tex() : SP<IRendererTexture>{};

    if (transition.pending && NEWTEX) {
        // the progress clock starts when the new texture is ready
        transition.frameSize = PIXELS;
        if (transition.fromSource) // null after a failed capture
            transition.from = renderFrame(transition.fromSource, PIXELS);
        transition.to = renderFrame(NEWTEX, PIXELS);
        if (!transition.from || !transition.to) {
            cancelTransition();
            return;
        }

        if (transition.shaderSource != data.transition.shader) {
            transition.shaderSource = data.transition.shader;
            transition.shader       = transition.shaderSource.empty() ? nullptr : g_renderer->createTransitionShader(transition.shaderSource);
        }
        transition.seed         = randomSeed();
        transition.durationSecs = sc<float>(data.transition.duration.count()) / 1000.F;

        startTransitionAnimation(this);
        transition.pending = false;
    } else if (transitionActive() && transition.frameSize != PIXELS) {
        // element resized mid-transition: re-render the frames at the new size
        transition.frameSize = PIXELS;
        transition.from      = renderFrame(transition.fromSource, PIXELS);
        if (NEWTEX)
            transition.to = renderFrame(NEWTEX, PIXELS);
        if (!transition.from || !transition.to) {
            cancelTransition();
            return;
        }
    }

    if (transitionActive()) {
        g_renderer->renderTransition(IRenderer::STransitionRenderData{
            .box      = self->impl->position,
            .from     = transition.from,
            .to       = transition.to,
            .shader   = transition.shader,
            .progress = transition.progress->value(),
            .a        = data.a,
            .duration = transition.durationSecs,
            .seed     = transition.seed,
        });
        return;
    }

    if (transition.pending && transition.from) {
        // the new image is still loading: keep showing the frozen frame
        if (transition.frameSize != PIXELS) {
            transition.frameSize = PIXELS;
            transition.from      = renderFrame(transition.fromSource, PIXELS);
            if (!transition.from)
                return;
        }
        g_renderer->renderTexture(IRenderer::STextureRenderData{
            .box     = self->impl->position,
            .texture = transition.from,
            .a       = data.a,
        });
    }
}

SP<CImageBuilder> CImageElement::rebuild() {
    auto p       = SP<CImageBuilder>(new CImageBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<SImageData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

void CImageElement::replaceData(const SImageData& data) {
    if (m_impl->transitionWanted(data))
        m_impl->requestTransition();
    else if (imageSourcesDiffer(m_impl->data, data))
        m_impl->cancelTransition();

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

std::optional<Vector2D> CImageElement::preferredSize(const Hyprutils::Math::Vector2D& parent, bool grow) {
    auto s = m_impl->data.size.calculate(parent, grow);
    if (s.x != -1 && s.y != -1)
        return s;

    const float SCALE = impl->window ? impl->window->scale() : 1.F;

    if (s.x == -1 && s.y == -1)
        return m_impl->size / SCALE;

    if (m_impl->size.y == 0)
        return impl->getPreferredSizeGeneric(m_impl->data.size, parent, grow);

    const double ASPECT_RATIO = m_impl->size.x / m_impl->size.y;

    if (s.y == -1)
        return Vector2D{s.x, s.x * (1 / ASPECT_RATIO)};

    return Vector2D{ASPECT_RATIO * s.y, s.y};
}

std::optional<Vector2D> CImageElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent, false);
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
