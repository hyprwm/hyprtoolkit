#pragma once

#include <chrono>
#include <variant>

#include <hyprutils/math/Vector2D.hpp>

namespace Hyprtoolkit {

    struct SNoAnimation {
        bool operator==(const SNoAnimation&) const = default;
    };

    struct SBezierAnimation {
        std::chrono::milliseconds duration{200};
        Hyprutils::Math::Vector2D control1{0.23, 1.0};
        Hyprutils::Math::Vector2D control2{0.32, 1.0};

        bool                      operator==(const SBezierAnimation&) const = default;
    };

    struct SSpringAnimation {
        float stiffness       = 250.F;
        float damping         = 25.F;
        float mass            = 1.F;
        float valueEpsilon    = 0.001F;
        float velocityEpsilon = 0.001F;

        bool  operator==(const SSpringAnimation&) const = default;
    };

    using SAnimation = std::variant<SNoAnimation, SBezierAnimation, SSpringAnimation>;
}
