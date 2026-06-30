#pragma once

#include <optional>
#include <string>

namespace Hyprtoolkit::Env {
    bool envEnabled(const std::string& env);
    bool isTrace();
    std::optional<std::string_view> getEnv(const std::string& env);
}
