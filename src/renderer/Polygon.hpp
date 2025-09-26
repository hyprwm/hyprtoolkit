#pragma once

#include <vector>
#include <hyprutils/math/Vector2D.hpp>

namespace Hyprtoolkit {
    // Describes a polygon, from 0.0 - 1.0 coordinates.
    // Uses STRIP methodology like in OGL.
    class CPolygon {
      public:
        CPolygon(std::vector<Hyprutils::Math::Vector2D> points);
        ~CPolygon() = default;

        static CPolygon checkmark();
        static CPolygon rangle();
        static CPolygon langle();

      private:
        std::vector<Hyprutils::Math::Vector2D> m_points;

        friend class COpenGLRenderer;
        friend class IRenderer;
    };
}
