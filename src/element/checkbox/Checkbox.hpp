#pragma once

#include <hyprtoolkit/element/Checkbox.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Null.hpp>

#include "../../helpers/Memory.hpp"
#include "../../core/AnimatedVariable.hpp"

namespace Hyprtoolkit {
    struct SCheckmarkData {
        CDynamicSize size  = {CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}};
        colorFn      color = [] { return CHyprColor{1.F, 0.F, 0.F}; };
    };

    class CCheckmarkElement : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CCheckmarkElement> create(const SCheckmarkData& data);
        virtual ~CCheckmarkElement() = default;

      private:
        CCheckmarkElement(const SCheckmarkData& data);

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);

        SCheckmarkData                                   m_data;

        PHLANIMVAR<CHyprColor>                           m_color;

        friend class CCheckboxElement;
    };

    struct SCheckboxData {
        bool                                                                           toggled = false;
        std::function<void(Hyprutils::Memory::CSharedPointer<CCheckboxElement>, bool)> onToggled;
        CDynamicSize                                                                   size{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {}};
    };

    struct SCheckboxImpl {
        SCheckboxData               data;

        WP<CCheckboxElement>        self;
        SP<CRectangleElement>       background;
        SP<CCheckmarkElement>       foreground;

        bool                        labelChanged = true;

        bool                        primedForUp = false;

        std::function<CHyprColor()> getFgColor();
    };
}
