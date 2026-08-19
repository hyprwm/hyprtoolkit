#pragma once

#include "Backend.hpp"
#include "BackendServices.hpp"

namespace Hyprtoolkit {
    class IEmbeddedSurface;

    class IEmbeddedBackend : public IBackend {
      public:
        struct SCreationData {
            SBackendCreationData                                    common;

            Hyprutils::Memory::CSharedPointer<IEventLoop>           eventLoop;
            Hyprutils::Memory::CSharedPointer<IClipboard>           clipboard;
            Hyprutils::Memory::CSharedPointer<ITextInput>           textInput;
            Hyprutils::Memory::CSharedPointer<ICursor>              cursor;
            Hyprutils::Memory::CSharedPointer<IOutputProvider>      outputs;
            Hyprutils::Memory::CSharedPointer<ISessionLockProvider> sessionLock;
        };

        // Creation and destruction require the GLES3 EGL context that will be
        // used for rendering to be current on the calling thread.
        static Hyprutils::Memory::CSharedPointer<IEmbeddedBackend> create(const SCreationData& data);

        // Embedded surfaces do not support native popups yet. Popup-backed
        // controls, including combobox dropdowns and tooltips, remain closed.
        virtual Hyprutils::Memory::CSharedPointer<IEmbeddedSurface> createSurface() = 0;

      protected:
        IEmbeddedBackend() = default;
    };
}
