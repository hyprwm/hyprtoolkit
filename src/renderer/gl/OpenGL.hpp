#pragma once

#include <hyprutils/math/Mat3x3.hpp>

#include "../Renderer.hpp"

#include "Shader.hpp"

#include <hyprutils/math/Region.hpp>
#include <hyprutils/os/FileDescriptor.hpp>
#include <aquamarine/buffer/Buffer.hpp>

#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <gbm.h>

namespace Hyprtoolkit {
    class IToolkitWindow;
    class IElement;
    class CGLTexture;
    class CRenderbuffer;

    class COpenGLRenderer : public IRenderer {
      public:
        COpenGLRenderer(int drmFD);
        virtual ~COpenGLRenderer();

        virtual void                 beginRendering(SP<IToolkitWindow> window, SP<Aquamarine::IBuffer> buf);
        virtual void                 endRendering();
        virtual void                 renderRectangle(const SRectangleRenderData& data);
        virtual SP<IRendererTexture> uploadTexture(const STextureData& data);
        virtual void                 renderTexture(const STextureRenderData& data);
        virtual void                 renderBorder(const SBorderRenderData& data);
        virtual void                 renderPolygon(const SPolygonRenderData& data);

      private:
        CBox                           logicalToGL(const CBox& box, bool transform = true);
        CRegion                        damageWithClip();
        void                           scissor(const CBox& box);
        void                           scissor(const pixman_box32_t* box);
        void                           renderBreadthfirst(SP<IElement> el);

        void                           initDRMFormats();
        void                           initEGL(bool gbm);
        EGLDeviceEXT                   eglDeviceFromDRMFD(int drmFD);
        EGLImageKHR                    createEGLImage(const Aquamarine::SDMABUFAttrs& attrs);
        void                           makeEGLCurrent();
        void                           unsetEGL();
        SP<CRenderbuffer>              getRBO(SP<Aquamarine::IBuffer> buf);
        void                           onRenderbufferDestroy(CRenderbuffer* p);

        Hyprutils::OS::CFileDescriptor m_gbmFD;
        gbm_device*                    m_gbmDevice    = nullptr;
        EGLContext                     m_eglContext   = nullptr;
        EGLDisplay                     m_eglDisplay   = nullptr;
        EGLDeviceEXT                   m_eglDevice    = nullptr;
        bool                           m_hasModifiers = true;

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

        std::vector<SP<CGLTexture>>    m_glTextures;
        std::vector<SP<CRenderbuffer>> m_rbos;
        SP<CRenderbuffer>              m_currentRBO;

        std::vector<CBox>              m_clipBoxes;
        std::vector<SP<IElement>>      m_alreadyRendered;

        CShader                        m_rectShader;
        CShader                        m_texShader;
        CShader                        m_borderShader;

        Mat3x3                         m_projMatrix = Mat3x3::identity();
        Mat3x3                         m_projection;

        float                          m_scale = 1.F;

        Vector2D                       m_currentViewport;

        friend class CRenderbuffer;
    };

    inline SP<COpenGLRenderer> g_openGL;
}
