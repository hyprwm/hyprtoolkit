
#pragma once

#include <hyprtoolkit/element/Text.hpp>
#include <pango/pangocairo.h>

#include <mutex>
#include <atomic>

#include "../../helpers/Memory.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../core/AnimatedVariable.hpp"

namespace Hyprtoolkit {
    struct STextData {
        std::string                              text;
        std::string                              fontFamily = g_palette ? g_palette->m_vars.fontFamily : "Sans Serif";
        CFontSize                                fontSize{CFontSize::HT_FONT_TEXT};
        eFontAlignment                           align       = HT_FONT_ALIGN_LEFT;
        colorFn                                  color       = [] { return g_palette->m_colors.text; };
        float                                    a           = 1.F;
        bool                                     noEllipsize = false;
        std::optional<Hyprutils::Math::Vector2D> clampSize;
        CDynamicSize                             size{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}};
        std::function<void()>                    callback; // called after resource is loaded
        bool                                     async = true;
        std::optional<bool>                      interactable;
    };

    struct STextLink {
        uint64_t                 begin = 0, end = 0;
        std::string              link;
        Hyprutils::Math::CRegion region;
    };

    struct SPangoData {
        PangoLayout*  layout;
        PangoContext* context;

        SPangoData();
        SPangoData(PangoLayout* layout, PangoContext* context);
        SPangoData(const SPangoData&) = delete;
        SPangoData(SPangoData&&) noexcept;
        SPangoData& operator=(const SPangoData&) = delete;
        SPangoData& operator=(SPangoData&&) noexcept;

        void        ref() const;
        void        unref() const;

        ~SPangoData();
    };

    struct STextImpl {
        STextData                                                                                      data;

        std::string                                                                                    parsedText;
        std::vector<STextLink>                                                                         parsedLinks;
        STextLink*                                                                                     hoveredTextLink = nullptr;

        WP<CTextElement>                                                                               self;

        float                                                                                          lastScale       = 1.F;
        bool                                                                                           needsTexRefresh = false, newTex = false;

        SPangoData                                                                                     pangoData;

        SP<IRendererTexture>                                                                           tex;
        SP<IRendererTexture>                                                                           oldTex; // while loading a new one
        PHLANIMVAR<CHyprColor>                                                                         color;
        SP<Hyprutils::Animation::SAnimationPropertyConfig>                                             colorAnimationConfig;
        ASP<Hyprgraphics::CTextResource>                                                               resource;
        Hyprutils::Math::Vector2D                                                                      size;

        Hyprutils::Math::Vector2D                                                                      lastCursorPos;

        bool                                                                                           waitingForTex         = false;
        bool                                                                                           colorAnimationEnabled = false;
        bool                                                                                           renderColorAtPaint    = false;

        Hyprutils::Math::CBox                                                                          getCharBox(size_t offset);
        std::optional<size_t>                                                                          vecToOffset(const Hyprutils::Math::Vector2D& vec);
        float                                                                                          getCursorPos(size_t offset);
        float                                                                                          getCursorPos(const Hyprutils::Math::Vector2D& click);
        Hyprutils::Math::Vector2D                                                                      unscale(const Hyprutils::Math::Vector2D& x);
        void                                                                                           setPangoData();
        void                                                                                           setPangoFont();
        void                                                                                           setPangoAlign();
        void                                                                                           setPangoText();
        void                                                                                           setPangoEllipsize();
        void                                                                                           updateScale();
        void                                                                                           scheduleTexRefresh();
        void                                                                                           renderTex();
        void                                                                                           postTexLoad();
        void                                                                                           parseText();
        void                                                                                           recheckTextBoxes();
        void                                                                                           onMouseDown();
        void                                                                                           onMouseMove();
        Hyprutils::Math::Vector2D                                                                      applyClampSize(Hyprutils::Math::Vector2D);

        friend class CTextboxElement;
        friend struct STextboxImpl;
    };
}
