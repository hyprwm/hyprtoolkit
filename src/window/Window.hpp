#pragma once

#include <hyprtoolkit/window/Window.hpp>

#include "../helpers/Memory.hpp"

namespace Hyprtoolkit {
    struct SWindowCreationData {
        eWindowType                              type = HT_WINDOW_TOPLEVEL;
        std::optional<Hyprutils::Math::Vector2D> preferredSize;
        std::optional<Hyprutils::Math::Vector2D> minSize;
        std::optional<Hyprutils::Math::Vector2D> maxSize;
        std::string                              title  = "Hyprtoolkit App";
        std::string                              class_ = "hyprtoolkit-app";

        // popups
        Hyprutils::Math::Vector2D pos;
        SP<IWindow>               parent;
    };
};
