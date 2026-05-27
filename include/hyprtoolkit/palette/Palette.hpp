#pragma once

#include <hyprutils/memory/SharedPtr.hpp>

#include "Color.hpp"

#include <string>

namespace Hyprtoolkit {

    class CPalette {
      public:
        ~CPalette() = default;

        /* Get the best palette possible. Retrieves configured system palette if available, default
            otherwise */
        static Hyprutils::Memory::CSharedPointer<CPalette> palette();

        /* Empty palette with just black */
        static Hyprutils::Memory::CSharedPointer<CPalette> emptyPalette();

        struct {
            CHyprColor background;
            CHyprColor text;
            CHyprColor base;
            CHyprColor alternateBase;
            CHyprColor brightText;
            CHyprColor linkText;
            CHyprColor accent;
            CHyprColor accentSecondary;
        } m_colors;

        struct {
            int         h1Size              = 19;
            int         h2Size              = 15;
            int         h3Size              = 13;
            int         fontSize            = 11;
            int         smallFontSize       = 10;
            std::string iconTheme           = ""; // first one found
            int         bigRounding         = 10;
            int         smallRounding       = 5;
            std::string fontFamily          = "Sans Serif";
            std::string fontFamilyMonospace = "monospace";
        } m_vars;

      private:
        CPalette() = default;

        bool m_isConfig = false;

        friend class CBackend;
    };
}
