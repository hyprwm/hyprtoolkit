#pragma once

#include <hyprutils/math/Mat3x3.hpp>

#include "../Renderer.hpp"

#include "Shader.hpp"

#include <hyprutils/math/Region.hpp>
#include <hyprutils/os/FileDescriptor.hpp>
#include <aquamarine/buffer/Buffer.hpp>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <gbm.h>
#include <mutex>

namespace Hyprtoolkit {
    class IToolkitWindow;
    class IElement;
    class CGLTexture;
    class CRenderbuffer;
    class CFramebuffer;
    class CEGLSync;

    class COpenGLRenderer : public IRenderer {
      public:
        COpenGLRenderer(int drmFD);
        COpenGLRenderer();
        virtual ~COpenGLRenderer();

        virtual void                 beginRendering(SP<IToolkitWindow> window, SP<Aquamarine::IBuffer> buf);
        virtual void                 beginRenderingExternal(SP<IToolkitWindow> window, uint32_t bufferAge);
        virtual void                 render(bool ignoreSync);
        virtual void                 endRendering();
        virtual void                 renderRectangle(const SRectangleRenderData& data);
        virtual SP<IRendererTexture> uploadTexture(const STextureData& data);
        virtual void                 renderTexture(const STextureRenderData& data);
        virtual void                 renderTransition(const STransitionRenderData& data);
        virtual void                 renderBorder(const SBorderRenderData& data);
        virtual void                 renderPolygon(const SPolygonRenderData& data);
        virtual SP<IRendererTexture> captureTransitionState(const STransitionRenderData& data);
        // Pre-render a single texture to a fit-resolved offscreen FBO (cached).
        SP<IRendererTexture>         renderFitFrame(const SP<IRendererTexture>& src, const CBox& box, eImageFitMode fitMode);
        void                         clearTransitionCache();
        virtual size_t               ensureTransitionShader(const std::string& source);
        virtual void                 renderLine(const SLineRenderData& data);
        virtual SP<CSyncTimeline>    exportSync(SP<Aquamarine::IBuffer> buf);
        virtual void                 signalRenderPoint(SP<CSyncTimeline> timeline);

        virtual bool                 explicitSyncSupported();

        virtual int                  getMaxTextureSize();

      private:
        CBox                           logicalToGL(const CBox& box, bool transform = true);
        CBox                           presentationBox(const CBox& box) const;
        float                          presentationOpacity() const;
        CRegion                        damageWithClip();
        void                           scissor(const CBox& box);
        void                           scissor(const pixman_box32_t* box);
        void                           renderBreadthfirst(SP<IElement> el);
        void                           waitOnSync();

        void                           initEGL(bool gbm);
        void                           initGLResources();
        EGLDeviceEXT                   eglDeviceFromDRMFD(int drmFD);
        EGLImageKHR                    createEGLImage(const Aquamarine::SDMABUFAttrs& attrs);
        void                           makeEGLCurrent();
        void                           unsetEGL();
        SP<CRenderbuffer>              getRBO(SP<Aquamarine::IBuffer> buf);
        void                           onRenderbufferDestroy(CRenderbuffer* p);

        Hyprutils::OS::CFileDescriptor m_gbmFD;
        gbm_device*                    m_gbmDevice        = nullptr;
        EGLContext                     m_eglContext       = nullptr;
        EGLDisplay                     m_eglDisplay       = nullptr;
        EGLDeviceEXT                   m_eglDevice        = nullptr;
        bool                           m_hasModifiers     = true;
        int                            m_drmFD            = -1;
        bool                           m_syncobjSupported = false;
        bool                           m_borrowedContext  = false;
        GLint                          m_maxTextureSize   = 0;

        struct {
            PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC glEGLImageTargetRenderbufferStorageOES = nullptr;
            PFNGLEGLIMAGETARGETTEXTURE2DOESPROC           glEGLImageTargetTexture2DOES           = nullptr;
            PFNEGLCREATEIMAGEKHRPROC                      eglCreateImageKHR                      = nullptr;
            PFNEGLDESTROYIMAGEKHRPROC                     eglDestroyImageKHR                     = nullptr;
            PFNEGLQUERYDMABUFFORMATSEXTPROC               eglQueryDmaBufFormatsEXT               = nullptr;
            PFNEGLQUERYDMABUFMODIFIERSEXTPROC             eglQueryDmaBufModifiersEXT             = nullptr;
            PFNEGLGETPLATFORMDISPLAYEXTPROC               eglGetPlatformDisplayEXT               = nullptr;
            PFNEGLDEBUGMESSAGECONTROLKHRPROC              eglDebugMessageControlKHR              = nullptr;
            PFNEGLQUERYDEVICESEXTPROC                     eglQueryDevicesEXT                     = nullptr;
            PFNEGLQUERYDEVICESTRINGEXTPROC                eglQueryDeviceStringEXT                = nullptr;
            PFNEGLQUERYDISPLAYATTRIBEXTPROC               eglQueryDisplayAttribEXT               = nullptr;
            PFNEGLCREATESYNCKHRPROC                       eglCreateSyncKHR                       = nullptr;
            PFNEGLDESTROYSYNCKHRPROC                      eglDestroySyncKHR                      = nullptr;
            PFNEGLDUPNATIVEFENCEFDANDROIDPROC             eglDupNativeFenceFDANDROID             = nullptr;
            PFNEGLWAITSYNCKHRPROC                         eglWaitSyncKHR                         = nullptr;
        } m_proc;

        struct {
            bool EXT_read_format_bgra               = false;
            bool EXT_image_dma_buf_import           = false;
            bool EXT_image_dma_buf_import_modifiers = false;
            bool KHR_display_reference              = false;
            bool IMG_context_priority               = false;
            bool EXT_create_context_robustness      = false;
            bool EGL_ANDROID_native_fence_sync_ext  = false;
        } m_exts;

        SP<IToolkitWindow>             m_window;
        CRegion                        m_damage;
        float                          m_scale = 1.F;
        SP<CFramebuffer>               m_polyRenderFb;

        std::vector<SP<CRenderbuffer>> m_rbos;
        SP<CRenderbuffer>              m_currentRBO;
        GLuint                         m_targetFB = 0;
        GLuint                         m_vao      = 0;
        GLuint                         m_vbo      = 0;

        SP<IElement>                   m_currentElement;

        struct SGLState {
            GLint     program             = 0;
            GLint     drawFramebuffer     = 0;
            GLint     readFramebuffer     = 0;
            GLint     vertexArray         = 0;
            GLint     arrayBuffer         = 0;
            GLint     pixelUnpackBuffer   = 0;
            GLint     activeTexture       = GL_TEXTURE0;
            GLint     texture2D           = 0;
            GLint     texture2DUnit0      = 0;
            GLint     viewport[4]         = {};
            GLint     scissorBox[4]       = {};
            GLfloat   clearColor[4]       = {};
            GLint     blendSrcRGB         = GL_ONE;
            GLint     blendDstRGB         = GL_ZERO;
            GLint     blendSrcAlpha       = GL_ONE;
            GLint     blendDstAlpha       = GL_ZERO;
            GLint     blendEquationRGB    = GL_FUNC_ADD;
            GLint     blendEquationAlpha  = GL_FUNC_ADD;
            GLint     samplerUnit0        = 0;
            GLint     unpackAlignment     = 4;
            GLint     unpackRowLength     = 0;
            GLint     unpackSkipPixels    = 0;
            GLint     unpackSkipRows      = 0;
            GLint     unpackImageHeight   = 0;
            GLint     unpackSkipImages    = 0;
            GLboolean blend               = GL_FALSE;
            GLboolean scissor             = GL_FALSE;
            GLboolean depth               = GL_FALSE;
            GLboolean stencil             = GL_FALSE;
            GLboolean cull                = GL_FALSE;
            GLboolean rasterizerDiscard   = GL_FALSE;
            GLboolean sampleCoverage      = GL_FALSE;
            GLboolean sampleAlphaCoverage = GL_FALSE;
            GLboolean colorMask[4]        = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
        } m_savedState;

        void                               saveGLState();
        void                               restoreGLState();
        bool                               contextCurrent();
        void                               enqueueGL(std::function<void()>&& callback);
        void                               runQueuedGL();
        void                               prepareRenderState();
        void                               uploadVertices(const float* data, size_t size);
        void                               registerTexture(const SP<CGLTexture>& texture);

        std::mutex                         m_queuedGLMutex;
        std::vector<std::function<void()>> m_queuedGL;
        std::vector<WP<CGLTexture>>        m_textures;

        std::vector<CBox>                  m_clipBoxes;
        std::vector<SP<IElement>>          m_alreadyRendered;

        CShader                            m_rectShader;
        CShader                            m_texShader;
        CShader                            m_transitionShader;
        CShader                            m_borderShader;

        // Custom transition shaders, cached by source hash.
        struct SCachedShader {
            CShader                            shader;
        };
        std::unordered_map<size_t, SCachedShader> m_customShaders;

        // Fit-resolved pre-render cache for transitions. Keyed by the source texture
        // identity, its size, the target box (pixels) and fit mode, so a frame is only
        // re-rendered when one of those changes.
        struct SFitFrameKey {
            GLuint   texID   = 0;
            Vector2D size;
            int      boxW    = 0;
            int      boxH    = 0;
            int      fitMode = 0;
            bool     operator==(const SFitFrameKey& o) const {
                return texID == o.texID && size == o.size && boxW == o.boxW && boxH == o.boxH && fitMode == o.fitMode;
            }
        };
        struct SFitFrameKeyHash {
            size_t operator()(const SFitFrameKey& k) const {
                return std::hash<GLuint>()(k.texID) ^ std::hash<float>()(k.size.x) ^ std::hash<float>()(k.size.y) ^
                    std::hash<int>()(k.boxW) ^ std::hash<int>()(k.boxH) ^ std::hash<int>()(k.fitMode);
            }
        };
        std::unordered_map<SFitFrameKey, SP<CGLTexture>, SFitFrameKeyHash> m_fitFrameCache;

        Mat3x3                             m_projMatrix = Mat3x3::identity();
        Mat3x3                             m_projection;

        Vector2D                           m_currentViewport;
        CBox                               m_lastScissorBox;

        friend class CRenderbuffer;
        friend class CGLTexture;
        friend class CFramebuffer;
        friend class CEGLSync;
    };

    inline SP<COpenGLRenderer> g_openGL;
}
