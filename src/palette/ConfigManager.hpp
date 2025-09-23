#pragma once
#include <hyprlang.hpp>

#include <string>

#include "../helpers/Memory.hpp"

namespace Hyprtoolkit {

  class CPalette;

    class CConfigManager {
      public:
        // gets all the data from the config
        CConfigManager();
        void                  parse();

        UP<Hyprlang::CConfig> m_config;

        SP<CPalette>          getPalette();

      private:
        friend class CIPCSocket;
    };
}
