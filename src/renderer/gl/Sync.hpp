#pragma once

#include "OpenGL.hpp"
#include "../../helpers/Memory.hpp"
#include <hyprutils/os/FileDescriptor.hpp>

namespace Hyprtoolkit {
    class CEGLSync {
      public:
        static UP<CEGLSync> create();

        ~CEGLSync();

        Hyprutils::OS::CFileDescriptor&  fd();
        Hyprutils::OS::CFileDescriptor&& takeFd();
        bool                             isValid();

      private:
        CEGLSync() = default;

        Hyprutils::OS::CFileDescriptor m_fd;
        EGLSyncKHR                     m_sync  = EGL_NO_SYNC_KHR;
        bool                           m_valid = false;

        friend class CHyprOpenGLImpl;
    };
}
