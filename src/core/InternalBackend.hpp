#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprutils/os/FileDescriptor.hpp>
#include <hyprgraphics/resource/AsyncResourceGatherer.hpp>

#include "Backend.hpp"
#include "../helpers/Env.hpp"
#include "Logger.hpp"

namespace Hyprtoolkit {

    class CPalette;
    class CConfigManager;
    class CSystemIconFactory;

    inline Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CBackend>                g_backend;
    inline Hyprutils::Memory::CSharedPointer<Hyprgraphics::CAsyncResourceGatherer> g_asyncResourceGatherer;
    inline Hyprutils::Memory::CSharedPointer<CPalette>                             g_palette;
    inline Hyprutils::Memory::CSharedPointer<CConfigManager>                       g_config;
    inline Hyprutils::Memory::CSharedPointer<CSystemIconFactory>                   g_iconFactory;
}
