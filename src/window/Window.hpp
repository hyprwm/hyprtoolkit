#pragma once

#include <hyprtoolkit/window/Window.hpp>
#include <ext-session-lock-v1.hpp>

#include "../helpers/Memory.hpp"

namespace Hyprtoolkit {
    struct SWindowCreationData {
        eWindowType                              type = HT_WINDOW_TOPLEVEL;
        std::optional<Hyprutils::Math::Vector2D> preferredSize;
        std::optional<Hyprutils::Math::Vector2D> minSize;
        std::optional<Hyprutils::Math::Vector2D> maxSize;
        std::string                              title  = "Hyprtoolkit App";
        std::string                              class_ = "hyprtoolkit-app";
        uint32_t                                 prefferedOutputId;

        // popups
        Hyprutils::Math::Vector2D pos;
        SP<IWindow>               parent;

        // layers
        uint32_t                  layer         = 0;
        uint32_t                  anchor        = 0;
        int32_t                   exclusiveZone = 0;
        uint32_t                  exclusiveEdge = 0;
        Hyprutils::Math::Vector2D marginTopLeft, marginBottomRight;
        uint32_t                  kbInteractive = 0;
    };
};
