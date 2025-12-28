#include "Tricks.hpp"
#include <core/InternalBackend.hpp>
#include <palette/ConfigManager.hpp>
#include <system/Icons.hpp>

using namespace Hyprtoolkit::Tests::Tricks;
using namespace Hyprtoolkit::Tests;
using namespace Hyprutils::Memory;

void Tricks::createBackendSupport() {
    g_logger = makeShared<CLogger>();
    g_config = makeShared<CConfigManager>();
    g_config->parse();
    g_palette     = CPalette::palette();
    g_iconFactory = SP<CSystemIconFactory>(new CSystemIconFactory());
}
