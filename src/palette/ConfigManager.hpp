#pragma once
#include <hyprlang.hpp>
#include <string>

#include "../helpers/Memory.hpp"

#include <sys/inotify.h>

#include <hyprutils/os/FileDescriptor.hpp>

namespace Hyprtoolkit {

    class CPalette;

    class CConfigManager {
      public:
        // gets all the data from the config
        CConfigManager();
        void                           parse();

        void                           onInotifyEvent();

        SP<CPalette>                   getPalette();

        UP<Hyprlang::CConfig>          m_config;
        Hyprutils::OS::CFileDescriptor m_inotifyFd;
        std::vector<int>               m_watches;
        std::string                    m_configPath;

        void                           replantWatch();
    };
}
