#pragma once

#include <cstdint>
#include <hyprgraphics/color/Color.hpp>

namespace Hyprtoolkit {
    class CHyprColor {
      public:
        CHyprColor();
        CHyprColor(float r, float g, float b, float a = 1.F);
        CHyprColor(const Hyprgraphics::CColor& col, float a);
        CHyprColor(uint64_t);

        // AR32
        uint32_t                     getAsHex() const;
        Hyprgraphics::CColor::SSRGB  asRGB() const;
        Hyprgraphics::CColor::SOkLab asOkLab() const;
        Hyprgraphics::CColor::SHSL   asHSL() const;
        CHyprColor                   stripA() const;

        CHyprColor                   brighten(float coeff) const;
        CHyprColor                   darken(float coeff) const;

        //
        bool operator==(const CHyprColor& c2) const;

        // stubs for the AnimationMgr
        CHyprColor operator-(const CHyprColor& c2) const;
        CHyprColor operator+(const CHyprColor& c2) const;
        CHyprColor operator*(const float& c2) const;

        double     r = 0, g = 0, b = 0, a = 0;

      private:
        Hyprgraphics::CColor::SOkLab okLab; // cache for the OkLab representation
    };
}