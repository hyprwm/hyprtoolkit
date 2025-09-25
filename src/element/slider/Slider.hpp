#pragma once

#include <hyprtoolkit/element/Slider.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Null.hpp>

#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {

    class CSliderSlider : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CSliderSlider> create(SP<CSliderElement> element);
        virtual ~CSliderSlider() = default;

        void updateLabel(const std::string& str);

      private:
        CSliderSlider(SP<CSliderElement> element);

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                     acceptsMouseInput();
        virtual ePointerShape                            pointerShape();

        void                                             updateValue();

        std::string                                      valueAsText();
        void                                             valueChanged(float perc);
        float                                            maxLabelSize();

        SP<CSliderElement>                               m_parent;

        SP<CRowLayoutElement>                            m_layout;
        SP<CRectangleElement>                            m_background;
        SP<CRectangleElement>                            m_foreground;
        SP<CTextElement>                                 m_valueText;
        SP<CNullElement>                                 m_textContainer;

        bool                                             m_dragging = false;
        Hyprutils::Math::Vector2D                        m_lastPosLocal;

        friend class CSliderElement;
    };

    struct SSliderData {
        std::string                                                                   label = "Slider";
        float                                                                         min = 0, max = 100, current = 50;
        bool                                                                          snapInt = true;
        std::function<void(Hyprutils::Memory::CSharedPointer<CSliderElement>, float)> onChanged;
        CDynamicSize                                                                  size{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {}};
        bool                                                                          fill = false; // FIXME: better layouting needed...
    };

    struct SSliderImpl {
        SSliderData           data;

        WP<CSliderElement>    self;
        SP<CRowLayoutElement> layout;
        SP<CTextElement>      label;
        SP<CNullElement>      spacer;
        SP<CSliderSlider>     slider;

        bool                  labelChanged = true;
    };
}
