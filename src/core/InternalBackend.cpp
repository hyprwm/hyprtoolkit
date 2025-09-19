#include "InternalBackend.hpp"

#include "../helpers/Env.hpp"

#include <print>
#include <cstdio>

using namespace Hyprtoolkit;

void CBackendLogger::log(eLogLevel level, std::string str) {
    if (g_backend->m_logFn) {
        g_backend->m_logFn(level, str);
        return;
    }

    std::string coloredStr = str;
    //NOLINTBEGIN
    switch (level) {
        case HT_LOG_TRACE:
            str        = "[HT] TRACE: " + str;
            coloredStr = str;
            break;
        case HT_LOG_DEBUG:
            str        = "[HT] DEBUG: " + str;
            coloredStr = "\033[1;33m" + str + "\033[0m"; // yellow
            break;
        case HT_LOG_WARNING:
            str        = "[HT] WARN: " + str;
            coloredStr = "\033[1;33m" + str + "\033[0m"; // yellow
            break;
        case HT_LOG_ERROR:
            str        = "[HT] ERR: " + str;
            coloredStr = "\033[1;31m" + str + "\033[0m"; // red
            break;
        case HT_LOG_CRITICAL:
            str        = "[HT] CRIT: " + str;
            coloredStr = "\033[1;35m" + str + "\033[0m"; // magenta
            break;
        default: break;
    }
    //NOLINTEND

    static bool LOG_NO_COLOR = Env::envEnabled("HT_NO_COLOR_LOGS");

    if (LOG_NO_COLOR)
        std::println("{}", coloredStr);
    else
        std::println("{}", str);

    std::fflush(stdout);
}