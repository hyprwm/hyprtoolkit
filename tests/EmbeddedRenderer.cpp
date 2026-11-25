#include <gtest/gtest.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>

#include <hyprtoolkit/core/EmbeddedBackend.hpp>
#include <hyprtoolkit/core/Timer.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/window/EmbeddedSurface.hpp>
#include <hyprutils/utils/ScopeGuard.hpp>

#include "window/EmbeddedSurface.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string_view>
#include <thread>

using namespace std::chrono_literals;

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

class CTestEventLoop final : public IEventLoop {
  public:
    void addFd(int fd, std::function<void()>&& callback) override {
        ;
    }

    void removeFd(int fd) override {
        ;
    }

    CAtomicSharedPointer<CTimer> addTimer(const TimerDuration& timeout, std::function<void(CAtomicSharedPointer<CTimer> self, void* data)> callback, void* data,
                                          bool force) override {
        return makeAtomicShared<CTimer>(timeout, std::move(callback), data, force);
    }

    void addIdle(const std::function<void()>& callback) override {
        m_idles.emplace_back(callback);
    }

    void cancelPending() override {
        ;
    }

    void enterLoop() override {
        ;
    }

    // runs the idles that were queued before the call; idles queued during the flush wait for the next one
    void flushIdles() {
        for (const auto& idle : std::exchange(m_idles, {}))
            idle();
    }

  private:
    std::vector<std::function<void()>> m_idles;
};

class CEmbeddedRendererTest : public testing::Test {
  protected:
    void SetUp() override {
        m_display = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
        if (m_display == EGL_NO_DISPLAY || !eglInitialize(m_display, nullptr, nullptr))
            GTEST_SKIP() << "No surfaceless EGL display";

        ASSERT_TRUE(eglBindAPI(EGL_OPENGL_ES_API));

        const EGLint configAttributes[] = {
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_NONE,
        };
        EGLint configCount = 0;
        ASSERT_TRUE(eglChooseConfig(m_display, configAttributes, &m_config, 1, &configCount));
        ASSERT_EQ(configCount, 1);

        const EGLint contextAttributes[] = {
            EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE,
        };
        m_context = eglCreateContext(m_display, m_config, EGL_NO_CONTEXT, contextAttributes);
        ASSERT_NE(m_context, EGL_NO_CONTEXT);

        const EGLint surfaceAttributes[] = {
            EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE,
        };
        m_surface = eglCreatePbufferSurface(m_display, m_config, surfaceAttributes);
        ASSERT_NE(m_surface, EGL_NO_SURFACE);
        ASSERT_TRUE(eglMakeCurrent(m_display, m_surface, m_surface, m_context));
    }

    void TearDown() override {
        if (m_display == EGL_NO_DISPLAY)
            return;

        eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (m_surface != EGL_NO_SURFACE)
            eglDestroySurface(m_display, m_surface);
        if (m_context != EGL_NO_CONTEXT)
            eglDestroyContext(m_display, m_context);
        eglTerminate(m_display);
    }

    EGLDisplay m_display = EGL_NO_DISPLAY;
    EGLConfig  m_config  = nullptr;
    EGLContext m_context = EGL_NO_CONTEXT;
    EGLSurface m_surface = EGL_NO_SURFACE;
};

TEST_F(CEmbeddedRendererTest, rendersAndRestoresHostState) {
    IEmbeddedBackend::SCreationData creationData;
    creationData.eventLoop = makeShared<CTestEventLoop>();
    auto backend           = IEmbeddedBackend::create(creationData);
    ASSERT_TRUE(backend);

    auto surface = backend->createSurface();
    ASSERT_TRUE(surface);

    surface->rootElement()->addChild(CRectangleBuilder::begin()
                                         ->color([] { return CHyprColor{1.F, 0.F, 0.F, 1.F}; })
                                         ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                                         ->commence());
    surface->resize({64, 64}, {64, 64}, 1.F);

    GLuint framebuffer = 0, texture = 0, hostVAO = 0, hostVBO = 0, hostUnpackBuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);

    glGenVertexArrays(1, &hostVAO);
    glGenBuffers(1, &hostVBO);
    glGenBuffers(1, &hostUnpackBuffer);
    glBindVertexArray(hostVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hostVBO);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, hostUnpackBuffer);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 5);
    glViewport(3, 4, 17, 19);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RASTERIZER_DISCARD);
    glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);

    surface->render(0);

    GLint value = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &value);
    EXPECT_EQ(value, framebuffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &value);
    EXPECT_EQ(value, hostVAO);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &value);
    EXPECT_EQ(value, hostVBO);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &value);
    EXPECT_EQ(value, hostUnpackBuffer);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &value);
    EXPECT_EQ(value, 1);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &value);
    EXPECT_EQ(value, 5);
    EXPECT_TRUE(glIsEnabled(GL_DEPTH_TEST));
    EXPECT_TRUE(glIsEnabled(GL_RASTERIZER_DISCARD));

    GLboolean colorMask[4] = {};
    glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
    EXPECT_FALSE(colorMask[0]);
    EXPECT_TRUE(colorMask[1]);
    EXPECT_FALSE(colorMask[2]);
    EXPECT_TRUE(colorMask[3]);

    GLint viewport[4] = {};
    glGetIntegerv(GL_VIEWPORT, viewport);
    EXPECT_EQ(viewport[0], 3);
    EXPECT_EQ(viewport[1], 4);
    EXPECT_EQ(viewport[2], 17);
    EXPECT_EQ(viewport[3], 19);

    GLubyte pixel[4] = {};
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    EXPECT_GT(pixel[0], 240);
    EXPECT_LT(pixel[1], 10);
    EXPECT_LT(pixel[2], 10);
    EXPECT_GT(pixel[3], 240);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_RASTERIZER_DISCARD);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glDeleteBuffers(1, &hostUnpackBuffer);
    glDeleteBuffers(1, &hostVBO);
    glDeleteVertexArrays(1, &hostVAO);
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &texture);

    surface.reset();
    backend->destroy();
    backend.reset();
}

TEST_F(CEmbeddedRendererTest, fractionalBordersSurviveReflowAndEdgeDamage) {
    using namespace Hyprutils::Math;

    IEmbeddedBackend::SCreationData creationData;
    creationData.eventLoop = makeShared<CTestEventLoop>();
    auto backend           = IEmbeddedBackend::create(creationData);
    ASSERT_TRUE(backend);
    auto surface = backend->createSurface();
    ASSERT_TRUE(surface);
    auto window = dynamicPointerCast<CEmbeddedSurface>(surface);
    ASSERT_TRUE(window);

    GLuint framebuffer = 0, texture = 0;
    glGenFramebuffers(1, &framebuffer);
    glGenTextures(1, &texture);
    Hyprutils::Utils::CScopeGuard cleanup{[&] {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &texture);
        window.reset();
        surface.reset();
        backend->destroy();
    }};

    surface->rootElement()->addChild(CRectangleBuilder::begin()
                                         ->color([] { return CHyprColor{1.F, 0.F, 0.F, 1.F}; })
                                         ->borderColor([] { return CHyprColor{0.F, 1.F, 0.F, 1.F}; })
                                         ->borderThickness(1)
                                         ->rounding(8)
                                         ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                                         ->commence());

    for (const float scale : {1.F, 1.25F, 4.F / 3.F, 1.5F, 2.F}) {
        for (const auto size : {Vector2D{400, 260}, Vector2D{401, 259}, Vector2D{398, 261}}) {
            SCOPED_TRACE(::testing::Message() << scale << ", " << size.x << "x" << size.y);
            const auto pixels  = (size * scale).floor();
            const auto logical = pixels / scale;
            const int  width = pixels.x, height = pixels.y;
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
            ASSERT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
            surface->resize(logical, pixels, scale);

            for (int frame = 0; frame < 3; ++frame) {
                SCOPED_TRACE(frame);
                if (frame == 1)
                    surface->rootElement()->forceReposition();
                else if (frame == 2) {
                    // Erase the framebuffer edges, then request only logical edge damage.
                    glEnable(GL_SCISSOR_TEST);
                    glClearColor(0.F, 0.F, 0.F, 0.F);
                    for (const auto box : {CBox{0, 0, pixels.x, 1}, CBox{0, pixels.y - 1, pixels.x, 1}, CBox{0, 0, 1, pixels.y}, CBox{pixels.x - 1, 0, 1, pixels.y}}) {
                        glScissor(box.x, box.y, box.w, box.h);
                        glClear(GL_COLOR_BUFFER_BIT);
                    }
                    glDisable(GL_SCISSOR_TEST);

                    const auto w = std::ceil(logical.x), h = std::ceil(logical.y);
                    CRegion    edges{CBox{0, 0, w, h}};
                    edges.subtract(CBox{2, 2, w - 4, h - 4});
                    window->damage(std::move(edges));
                }

                surface->render(frame == 0 ? 0 : 1);

                for (const auto point : {Vector2D{width / 2, 0}, Vector2D{width / 2, height - 1}, Vector2D{0, height / 2}, Vector2D{width - 1, height / 2}}) {
                    SCOPED_TRACE(::testing::Message() << point.x << ", " << point.y);
                    std::array<GLubyte, 4> pixel = {};
                    glReadPixels(point.x, point.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
                    EXPECT_LT(pixel[0], 10);
                    EXPECT_GT(pixel[1], 240);
                    EXPECT_LT(pixel[2], 10);
                    EXPECT_GT(pixel[3], 240);
                }

                std::array<GLubyte, 4> center = {};
                glReadPixels(width / 2, height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, center.data());
                EXPECT_GT(center[0], 240);
                EXPECT_LT(center[1], 10);
                EXPECT_GT(center[3], 240);
            }
        }
    }
}

namespace {
    using Pixel = std::array<GLubyte, 4>;

    constexpr std::string_view PROBE_FROM_SHADER = R"#(#version 300 es
precision highp float;
in vec2 v_texcoord;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform float progress;
uniform float alpha;
layout(location = 0) out vec4 fragColor;
void main() { fragColor = texture(tex1, v_texcoord) * alpha; }
)#";

    constexpr std::string_view PROBE_TO_SHADER = R"#(#version 300 es
precision highp float;
in vec2 v_texcoord;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform float progress;
uniform float alpha;
layout(location = 0) out vec4 fragColor;
void main() { fragColor = texture(tex2, v_texcoord) * alpha; }
)#";

    constexpr std::string_view OUTGOING_SHADER = R"#(#version 300 es
precision highp float;
in vec2 v_texcoord;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform float progress;
uniform float alpha;
layout(location = 0) out vec4 fragColor;
void main() { fragColor = mix(texture(tex1, v_texcoord), vec4(1.0, 0.0, 0.0, 1.0), progress) * alpha; }
)#";

    constexpr std::string_view INCOMING_SHADER = R"#(#version 300 es
precision highp float;
in vec2 v_texcoord;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform float progress;
uniform float alpha;
layout(location = 0) out vec4 fragColor;
void main() { fragColor = mix(texture(tex1, v_texcoord), vec4(0.0, 0.0, 1.0, 1.0), progress) * alpha; }
)#";

    constexpr int              SURFACE_W = 96, SURFACE_H = 64;

    void                       put16(std::string& s, const uint16_t v) {
        s.push_back(sc<char>(v & 0xFF));
        s.push_back(sc<char>(v >> 8));
    }

    void put32(std::string& s, const uint32_t v) {
        for (int i = 0; i < 4; ++i)
            s.push_back(sc<char>((v >> (8 * i)) & 0xFF));
    }

    // minimal uncompressed 24bpp bottom-up BMP, which the hyprgraphics loader accepts
    void writeBMP(const std::filesystem::path& path, const uint32_t w, const uint32_t h, const std::function<void(uint32_t, uint32_t, uint8_t[3])>& pixel) {
        constexpr uint32_t DATA_OFFSET = 54;
        const uint32_t     imageSize   = w * h * 3;

        std::string        bmp;
        bmp.reserve(DATA_OFFSET + imageSize);
        bmp += "BM";
        put32(bmp, DATA_OFFSET + imageSize);
        put16(bmp, 0);
        put16(bmp, 0);
        put32(bmp, DATA_OFFSET);
        put32(bmp, 40);
        put32(bmp, w);
        put32(bmp, h);
        put16(bmp, 1);
        put16(bmp, 24);
        put32(bmp, 0);
        put32(bmp, imageSize);
        put32(bmp, 2835);
        put32(bmp, 2835);
        put32(bmp, 0);
        put32(bmp, 0);
        for (uint32_t y = h; y-- > 0;) {
            for (uint32_t x = 0; x < w; ++x) {
                uint8_t rgb[3];
                pixel(x, y, rgb);
                bmp.push_back(sc<char>(rgb[2]));
                bmp.push_back(sc<char>(rgb[1]));
                bmp.push_back(sc<char>(rgb[0]));
            }
        }
        std::filesystem::create_directories(path.parent_path());
        std::ofstream{path, std::ios::binary}.write(bmp.data(), sc<std::streamsize>(bmp.size()));
    }

    // per test: ctest -j runs the tests as parallel processes
    std::filesystem::path imageDir() {
        const auto DIR = std::filesystem::temp_directory_path() / "hyprtoolkit-transition-test" / testing::UnitTest::GetInstance()->current_test_info()->name();
        std::filesystem::remove_all(DIR);
        std::filesystem::create_directories(DIR);
        return DIR;
    }

    std::vector<Pixel> readGrid(const int inset, const int gw, const int gh) {
        std::vector<Pixel> grid;
        grid.reserve(sc<size_t>(gw * gh));
        for (int gy = 0; gy < gh; ++gy) {
            for (int gx = 0; gx < gw; ++gx) {
                const int x = inset + gx * (SURFACE_W - 2 * inset - 1) / (gw - 1);
                const int y = inset + gy * (SURFACE_H - 2 * inset - 1) / (gh - 1);
                Pixel     p = {};
                glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p.data());
                grid.push_back(p);
            }
        }
        return grid;
    }

    void expectGridsClose(const std::vector<Pixel>& a, const std::vector<Pixel>& b, const int tol) {
        ASSERT_EQ(a.size(), b.size());
        for (size_t i = 0; i < a.size(); ++i) {
            SCOPED_TRACE(::testing::Message() << "pixel " << i);
            for (int c = 0; c < 4; ++c) {
                EXPECT_NEAR(a[i][c], b[i][c], tol) << "channel " << c;
                if (::testing::Test::HasFailure())
                    return;
            }
        }
    }

    SP<CImageElement> addImageElement(const SP<IBackend>& backend, std::string path, const eImageFitMode fitMode) {
        auto element = CImageBuilder::begin()
                           ->path(std::move(path))
                           ->fitMode(fitMode)
                           ->sync(true)
                           ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                           ->commence();
        element->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
        return element;
    }

    void swapImage(const SP<CImageElement>& element, std::string path, const eImageFitMode fitMode, SImageTransition&& transition) {
        element->rebuild()
            ->path(std::move(path))
            ->fitMode(fitMode)
            ->sync(true)
            ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
            ->transition(std::move(transition))
            ->commence();
    }

    GLuint bindTestFramebuffer(GLuint& framebuffer, GLuint& texture) {
        if (!framebuffer) {
            glGenFramebuffers(1, &framebuffer);
            glGenTextures(1, &texture);
        }
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SURFACE_W, SURFACE_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
        return framebuffer;
    }
}

TEST_F(CEmbeddedRendererTest, transitionFramesMatchNormalRendering) {
    const auto DIR = imageDir();

    // two images with different resolutions and aspect ratios, striped so
    // letterboxing, cropping and tiling are all detectable
    const auto A_PATH = (DIR / "a.bmp").string();
    const auto B_PATH = (DIR / "b.bmp").string();
    writeBMP(A_PATH, 64, 32, [](const uint32_t, const uint32_t y, uint8_t rgb[3]) {
        const bool stripe = (y / 8) % 2;
        rgb[0]            = 0;
        rgb[1]            = stripe ? 0 : 255;
        rgb[2]            = stripe ? 255 : 0;
    });
    writeBMP(B_PATH, 20, 50, [](const uint32_t x, const uint32_t, uint8_t rgb[3]) {
        const bool stripe = (x / 5) % 2;
        rgb[0]            = stripe ? 255 : 255;
        rgb[1]            = stripe ? 255 : 0;
        rgb[2]            = stripe ? 0 : 255;
    });

    IEmbeddedBackend::SCreationData creationData;
    creationData.eventLoop = makeShared<CTestEventLoop>();
    auto backend           = IEmbeddedBackend::create(creationData);
    ASSERT_TRUE(backend);
    auto surface = backend->createSurface();
    ASSERT_TRUE(surface);
    auto                          loop = dynamicPointerCast<CTestEventLoop>(creationData.eventLoop);

    GLuint                        framebuffer = 0, texture = 0;
    Hyprutils::Utils::CScopeGuard cleanup{[&] {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &texture);
        surface.reset();
        backend->destroy();
        backend.reset();
    }};

    const auto                    window  = dynamicPointerCast<CEmbeddedSurface>(surface);
    const auto                    present = [&](const bool first = false) {
        loop->flushIdles();
        bindTestFramebuffer(framebuffer, texture);
        surface->resize({SURFACE_W, SURFACE_H}, {SURFACE_W, SURFACE_H}, 1.F);
        window->damageEntire(); // full repaint: the buffer-age bookkeeping of the driven renders is not under test
        surface->render(first ? 0 : 1);
    };

    const auto image = addImageElement(backend, A_PATH, IMAGE_FIT_MODE_CONTAIN);
    surface->rootElement()->addChild(image);
    surface->resize({SURFACE_W, SURFACE_H}, {SURFACE_W, SURFACE_H}, 1.F);

    struct FitPair {
        eImageFitMode from, to;
        const char*   name;
    };
    for (const auto& PAIR : std::vector<FitPair>{
             {IMAGE_FIT_MODE_CONTAIN, IMAGE_FIT_MODE_COVER, "contain->cover"},
             {IMAGE_FIT_MODE_COVER, IMAGE_FIT_MODE_TILE, "cover->tile"},
             {IMAGE_FIT_MODE_TILE, IMAGE_FIT_MODE_CONTAIN, "tile->contain"},
             {IMAGE_FIT_MODE_STRETCH, IMAGE_FIT_MODE_COVER, "stretch->cover"},
         }) {
        SCOPED_TRACE(PAIR.name);

        // start from a clean state: a same-source swap does not cancel the previous
        // pair's still-running transition
        swapImage(image, B_PATH, IMAGE_FIT_MODE_COVER, SImageTransition{});
        swapImage(image, A_PATH, PAIR.from, SImageTransition{});
        present(true);
        const auto GRID_A = readGrid(3, 13, 9);

        swapImage(image, B_PATH, PAIR.to, SImageTransition{});
        present();
        const auto GRID_B = readGrid(3, 13, 9);

        // same image rendered normally twice in a row is identical
        swapImage(image, A_PATH, PAIR.from, SImageTransition{});
        present();
        {
            SCOPED_TRACE("plain swap-back");
            expectGridsClose(GRID_A, readGrid(3, 13, 9), 1);
        }

        // probe shaders only output one frame: the transition must render it
        // exactly like the normal path did, at every fit-mode pair
        swapImage(image, B_PATH, PAIR.to, SImageTransition{.duration = 60s, .shader = std::string{PROBE_FROM_SHADER}});
        present();
        {
            SCOPED_TRACE("probe tex1 (outgoing frame)");
            expectGridsClose(GRID_A, readGrid(3, 13, 9), 2);
        }

        swapImage(image, A_PATH, PAIR.from, SImageTransition{.duration = 60s, .shader = std::string{PROBE_TO_SHADER}});
        present();
        {
            SCOPED_TRACE("probe tex2 (incoming frame)");
            expectGridsClose(GRID_A, readGrid(3, 13, 9), 2);
        }
    }
}

TEST_F(CEmbeddedRendererTest, interruptedTransitionFreezesInPlace) {
    const auto DIR = imageDir();

    const auto A_PATH = (DIR / "a.bmp").string();
    const auto B_PATH = (DIR / "b.bmp").string();
    const auto C_PATH = (DIR / "c.bmp").string();
    const auto D_PATH = (DIR / "d.bmp").string();
    writeBMP(A_PATH, 64, 32, [](const uint32_t, const uint32_t y, uint8_t rgb[3]) {
        const bool stripe = (y / 8) % 2;
        rgb[0]            = 0;
        rgb[1]            = stripe ? 0 : 255;
        rgb[2]            = stripe ? 255 : 0;
    });
    writeBMP(B_PATH, 20, 50, [](const uint32_t x, const uint32_t, uint8_t rgb[3]) {
        const bool stripe = (x / 5) % 2;
        rgb[0]            = 255;
        rgb[1]            = stripe ? 255 : 0;
        rgb[2]            = stripe ? 0 : 255;
    });
    writeBMP(C_PATH, 40, 40, [](const uint32_t, const uint32_t, uint8_t rgb[3]) {
        rgb[0] = 255;
        rgb[1] = 140;
        rgb[2] = 0;
    });
    writeBMP(D_PATH, 30, 60, [](const uint32_t x, const uint32_t y, uint8_t rgb[3]) {
        const bool checker = ((x / 5) + (y / 5)) % 2;
        rgb[0]             = checker ? 255 : 60;
        rgb[1]             = checker ? 0 : 60;
        rgb[2]             = checker ? 255 : 60;
    });

    IEmbeddedBackend::SCreationData creationData;
    creationData.eventLoop = makeShared<CTestEventLoop>();
    auto backend           = IEmbeddedBackend::create(creationData);
    ASSERT_TRUE(backend);
    auto surface = backend->createSurface();
    ASSERT_TRUE(surface);
    auto                          loop = dynamicPointerCast<CTestEventLoop>(creationData.eventLoop);

    GLuint                        framebuffer = 0, texture = 0;
    Hyprutils::Utils::CScopeGuard cleanup{[&] {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &texture);
        surface.reset();
        backend->destroy();
        backend.reset();
    }};

    const auto                    window  = dynamicPointerCast<CEmbeddedSurface>(surface);
    const auto                    present = [&](const bool first = false) {
        loop->flushIdles();
        bindTestFramebuffer(framebuffer, texture);
        surface->resize({SURFACE_W, SURFACE_H}, {SURFACE_W, SURFACE_H}, 1.F);
        window->damageEntire(); // full repaint: the buffer-age bookkeeping of the driven renders is not under test
        surface->render(first ? 0 : 1);
    };
    const auto centerPixel = [&]() {
        Pixel p = {};
        glReadPixels(SURFACE_W / 2, SURFACE_H / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p.data());
        return p;
    };
    const auto pixelsDiffer = [](const Pixel& a, const Pixel& b) { return std::abs(sc<int>(a[0]) - b[0]) + std::abs(sc<int>(a[1]) - b[1]) + std::abs(sc<int>(a[2]) - b[2]) > 10; };

    const auto image = addImageElement(backend, A_PATH, IMAGE_FIT_MODE_COVER);
    surface->rootElement()->addChild(image);
    surface->resize({SURFACE_W, SURFACE_H}, {SURFACE_W, SURFACE_H}, 1.F);

    present(true);
    swapImage(image, C_PATH, IMAGE_FIT_MODE_COVER, SImageTransition{});
    present();
    const auto GRID_C = readGrid(3, 13, 9);

    swapImage(image, A_PATH, IMAGE_FIT_MODE_COVER, SImageTransition{});
    present();
    const Pixel CENTER = centerPixel();
    ASSERT_GT(CENTER[1], CENTER[0] + 50); // A's green dominates the center (the exact pixel sits on a stripe edge)
    ASSERT_GT(CENTER[1], CENTER[2] + 50);

    // A -> B with a progress-dependent shader
    swapImage(image, B_PATH, IMAGE_FIT_MODE_COVER, SImageTransition{.duration = 2s, .shader = std::string{OUTGOING_SHADER}});
    present();
    const auto BEFORE_MOVE = centerPixel();

    std::this_thread::sleep_for(250ms);
    present();
    const auto FROZEN_AT = centerPixel();
    ASSERT_TRUE(pixelsDiffer(BEFORE_MOVE, FROZEN_AT)) << "progress did not advance before the interrupt";

    // changing the image mid-transition freezes: the first frame after the change is
    // identical to the last frame before it, and the frozen blend stays put
    swapImage(image, C_PATH, IMAGE_FIT_MODE_COVER, SImageTransition{.duration = 60s, .shader = std::string{INCOMING_SHADER}});
    present();
    expectGridsClose({FROZEN_AT}, {centerPixel()}, 3);

    std::this_thread::sleep_for(150ms);
    present();
    expectGridsClose({FROZEN_AT}, {centerPixel()}, 4);

    // a second change re-freezes on the same frame; only the target moves
    swapImage(image, D_PATH, IMAGE_FIT_MODE_COVER, SImageTransition{.duration = 60s, .shader = std::string{INCOMING_SHADER}});
    present();
    expectGridsClose({FROZEN_AT}, {centerPixel()}, 5);

    // the next transition starts on the frozen frame and ends on the new image rendered normally
    swapImage(image, C_PATH, IMAGE_FIT_MODE_COVER, SImageTransition{.duration = 150ms, .shader = std::string{INCOMING_SHADER}});
    present();
    expectGridsClose({FROZEN_AT}, {centerPixel()}, 5);

    std::this_thread::sleep_for(400ms);
    present();
    present();
    expectGridsClose(GRID_C, readGrid(3, 13, 9), 2);
}
