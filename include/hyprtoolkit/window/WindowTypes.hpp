#pragma once

#include <optional>
#include <hyprutils/math/Vector2D.hpp>

namespace Hyprtoolkit {
    struct SWindowCreationData {
        std::optional<Hyprutils::Math::Vector2D> preferredSize;
        std::optional<Hyprutils::Math::Vector2D> minSize;
        std::optional<Hyprutils::Math::Vector2D> maxSize;
        std::string                              title  = "Hyprtoolkit App";
        std::string                              class_ = "hyprtoolkit-app";
    };
}
