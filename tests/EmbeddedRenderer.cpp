#include <gtest/gtest.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>

#include <hyprtoolkit/core/EmbeddedBackend.hpp>
#include <hyprtoolkit/core/Timer.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/window/EmbeddedSurface.hpp>

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
        callback();
    }

    void cancelPending() override {
        ;
    }

    void enterLoop() override {
        ;
    }
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
