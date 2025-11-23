#include "Logger.hpp"

using namespace Hyprtoolkit;

static Hyprutils::CLI::eLogLevel levelToHU(eLogLevel l) {
    switch (l) {
        case Hyprtoolkit::HT_LOG_DEBUG: return Hyprutils::CLI::LOG_DEBUG;
        case Hyprtoolkit::HT_LOG_ERROR: return Hyprutils::CLI::LOG_ERR;
        case Hyprtoolkit::HT_LOG_WARNING: return Hyprutils::CLI::LOG_WARN;
        case Hyprtoolkit::HT_LOG_CRITICAL: return Hyprutils::CLI::LOG_CRIT;
        case Hyprtoolkit::HT_LOG_TRACE: return Hyprutils::CLI::LOG_TRACE;
    }
    return Hyprutils::CLI::LOG_DEBUG;
}

CLogger::CLogger() {
    const auto IS_TRACE = Env::isTrace();
    m_logger.setLogLevel(IS_TRACE ? Hyprutils::CLI::LOG_TRACE : Hyprutils::CLI::LOG_DEBUG);
}

void CLogger::log(eLogLevel level, const std::string& str) {
    if (m_logFn) {
        m_logFn(level, str);
        return;
    }

    if (m_loggerConnection) {
        m_loggerConnection->log(levelToHU(level), str);
        return;
    }

    m_logger.log(levelToHU(level), str);
}
