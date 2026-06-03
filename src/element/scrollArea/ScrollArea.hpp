#include <hyprtoolkit/element/ScrollArea.hpp>

#include "../../helpers/Memory.hpp"
#include "../../helpers/Signal.hpp"

namespace Hyprtoolkit {
    class CNullElement;
    class CRectangleElement;

    struct SScrollAreaData {
        CDynamicSize              size            = CDynamicSize(CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1});
        bool                      scrollX         = false;
        bool                      scrollY         = false;
        bool                      blockUserScroll = false;
        bool                      showScrollbar   = false;
        Hyprutils::Math::Vector2D currentScroll;
    };

    // one draggable scrollbar (track + thumb) living in the element tree, so it can take input
    struct SScrollbar {
        SP<CNullElement>      strip; // edge-anchored hit area that holds the track and thumb
        SP<CRectangleElement> track;
        SP<CRectangleElement> thumb;

        bool                  shown    = false;
        bool                  hovered  = false;
        bool                  dragging = false;
        float                 grab     = 0.F; // pointer offset inside the thumb when the drag started, in px

        // geometry cached from the last layout, in the strip's local frame along the main axis;
        // the input handlers reuse it so a click does not have to recompute the layout
        float                     trackLen = 0.F;
        float                     thumbOff = 0.F;
        float                     thumbLen = 0.F;
        float                     maxOff   = 0.F;
        Hyprutils::Math::Vector2D lastLocal;

        struct {
            CHyprSignalListener button;
            CHyprSignalListener move;
            CHyprSignalListener enter;
            CHyprSignalListener leave;
        } listeners;
    };

    struct SScrollAreaImpl {
        SScrollAreaData        data;

        WP<CScrollAreaElement> self;

        SP<CNullElement>       inner; // content layer that actually scrolls; user children go here

        SScrollbar             vbar;
        SScrollbar             hbar;

        void                   clampMaxScroll();

        // max scroll offset per axis, 0 on an axis whose content fits. cached so an off-frame
        // clamp (a wheel event between layouts) does not re-measure the content mid-reflow and
        // snap the scroll. recalcMaxScroll() refreshes it once per reposition once the content
        // is laid out, maxScroll() just returns the cache.
        Hyprutils::Math::Vector2D maxScroll();
        void                      recalcMaxScroll();

        Hyprutils::Math::Vector2D cachedMaxScroll;

        struct {
            CHyprSignalListener axis;
        } listeners;
    };
}
