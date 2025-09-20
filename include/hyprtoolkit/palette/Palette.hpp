#pragma once

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprgraphics/color/Color.hpp>

namespace Hyprtoolkit {
    class CToolkitColor {
      public:
        CToolkitColor() = default;
        CToolkitColor(const Hyprgraphics::CColor& color, float a = 1.F);
        ~CToolkitColor() = default;

        Hyprgraphics::CColor m_color = Hyprgraphics::CColor::SSRGB{.r = 0.F, .g = 0.F, .b = 0.F};
        float                m_a     = 1.F;
    };

    class CPalette {
      public:
        ~CPalette() = default;

        /* Get the best palette possible. Retrieves configured system palette if available, default
            otherwise */
        static Hyprutils::Memory::CSharedPointer<CPalette> palette();

        /* Default fallback palette */
        static Hyprutils::Memory::CSharedPointer<CPalette> defaultPalette();
        /* Empty palette with just black */
        static Hyprutils::Memory::CSharedPointer<CPalette> emptyPalette();
        /* Config palette if exists */
        static Hyprutils::Memory::CSharedPointer<CPalette> configPalette();

        struct {
            CToolkitColor background;
            CToolkitColor text;
            CToolkitColor base;
            CToolkitColor alternateBase;
            CToolkitColor brightText;
            CToolkitColor accent;
            CToolkitColor accentSecondary;
        } m_colors;

      private:
        CPalette() = default;
    };
}
