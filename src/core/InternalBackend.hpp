#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprutils/os/FileDescriptor.hpp>
#include <hyprgraphics/resource/AsyncResourceGatherer.hpp>

#include "../helpers/Env.hpp"

namespace Hyprtoolkit {

    class CPalette;
    class CConfigManager;

    class CBackendLogger {
      public:
        void log(eLogLevel level, std::string str);

        template <typename... Args>
        //NOLINTNEXTLINE
        void log(eLogLevel level, std::format_string<Args...> fmt, Args&&... args) {
            static bool LOG_DISABLED = Env::envEnabled("HT_NO_LOGS");

            if (LOG_DISABLED)
                return;

            std::string logMsg = "";

            // no need for try {} catch {} because std::format_string<Args...> ensures that vformat never throw std::format_error
            // because
            // 1. any faulty format specifier that sucks will cause a compilation error.
            // 2. and `std::bad_alloc` is catastrophic, (Almost any operation in stdlib could throw this.)
            // 3. this is actually what std::format in stdlib does
            logMsg += std::vformat(fmt.get(), std::make_format_args(args...));

            log(level, logMsg);
        }
    };

    inline Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CBackend>                g_backend;
    inline Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CBackendLogger>          g_logger;
    inline Hyprutils::Memory::CSharedPointer<Hyprgraphics::CAsyncResourceGatherer> g_asyncResourceGatherer;
    inline Hyprutils::Memory::CSharedPointer<CPalette>                             g_palette;
    inline Hyprutils::Memory::CSharedPointer<CConfigManager>                       g_config;
}
