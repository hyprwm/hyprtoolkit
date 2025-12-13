#pragma once

#include <string>
#include <expected>

namespace Hyprtoolkit::DesktopMethods {
    std::expected<void, std::string> openLink(const std::string_view& sv);
};