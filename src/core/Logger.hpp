#pragma once

#include <hyprtoolkit/core/LogTypes.hpp>
#include <hyprtoolkit/core/Backend.hpp>

#include <hyprutils/cli/Logger.hpp>
#include <optional>

#include "../helpers/Env.hpp"

namespace Hyprtoolkit {
    class CLogger {
      public:
        CLogger();
        ~CLogger() = default;

        void log(eLogLevel level, const std::string& str);

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

        Hyprutils::CLI::CLogger                                              m_logger;
        IBackend::LogFn                                                      m_logFn;
        Hyprutils::Memory::CSharedPointer<Hyprutils::CLI::CLoggerConnection> m_loggerConnection;
        Hyprutils::Memory::CSharedPointer<Hyprutils::CLI::CLoggerConnection> m_aqLoggerConnection;
    };

    inline Hyprutils::Memory::CSharedPointer<CLogger> g_logger = Hyprutils::Memory::makeShared<CLogger>();
};