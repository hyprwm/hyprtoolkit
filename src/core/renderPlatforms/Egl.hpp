#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {
    class CEGL {
      public:
        CEGL(wl_display*);
        ~CEGL();

        EGLDisplay                               eglDisplay;
        EGLConfig                                eglConfig;
        EGLContext                               eglContext;

        PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC eglCreatePlatformWindowSurfaceEXT;

        void                                     makeCurrent(EGLSurface surf);

        bool                                     m_isNvidia = false;
    };

    inline UP<CEGL> g_pEGL;
};
