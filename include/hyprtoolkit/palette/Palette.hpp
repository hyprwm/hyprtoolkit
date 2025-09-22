#pragma once

#include <hyprutils/memory/SharedPtr.hpp>

#include "Color.hpp"

namespace Hyprtoolkit {

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
            CHyprColor background;
            CHyprColor text;
            CHyprColor base;
            CHyprColor alternateBase;
            CHyprColor brightText;
            CHyprColor accent;
            CHyprColor accentSecondary;
        } m_colors;

        struct {
            int fontSize      = 12;
            int smallFontSize = 11;
        } m_vars;

      private:
        CPalette() = default;
    };
}
