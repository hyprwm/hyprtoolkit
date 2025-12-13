#include "DesktopMethods.hpp"

#include <hyprutils/os/Process.hpp>

using namespace Hyprtoolkit;
using namespace Hyprtoolkit::DesktopMethods;

using namespace Hyprutils::OS;

std::expected<void, std::string> DesktopMethods::openLink(const std::string_view& sv) {
    CProcess proc("xdg-open", {std::string{sv}});

    if (!proc.runAsync())
        return std::unexpected("Failed to spawn process");

    return {};
}
