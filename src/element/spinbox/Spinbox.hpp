#pragma once

#include <hyprtoolkit/element/Spinbox.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Null.hpp>

#include "../../helpers/Memory.hpp"
#include "../../renderer/Polygon.hpp"
#include "../../core/AnimatedVariable.hpp"

namespace Hyprtoolkit {
    class CSpinboxSpinner;

    struct SSpinboxAngleData {
        CDynamicSize        size        = {CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}};
        colorFn             color       = [] { return CHyprColor{1.F, 0.F, 0.F}; };
        colorFn             colorActive = [] { return CHyprColor{1.F, 1.F, 0.F}; };
        bool                right       = false;
        WP<CSpinboxSpinner> spinner;
    };

    class CSpinboxAngleElement : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CSpinboxAngleElement> create(const SSpinboxAngleData& data);
        virtual ~CSpinboxAngleElement() = default;

      private:
        CSpinboxAngleElement(const SSpinboxAngleData& data);

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                     acceptsMouseInput();
        virtual ePointerShape                            pointerShape();

        SSpinboxAngleData                                m_data;
        CPolygon                                         m_poly;
        bool                                             m_primedForUp = false;

        PHLANIMVAR<CHyprColor>                           m_color;

        friend class CCheckboxElement;
    };

    struct SSpinboxData {
        std::string                                                                     label       = "Choose one";
        std::vector<std::string>                                                        items       = {"Item A", "Item B"};
        size_t                                                                          currentItem = 0;
        std::function<void(Hyprutils::Memory::CSharedPointer<CSpinboxElement>, size_t)> onChanged;
        CDynamicSize                                                                    size{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {}};
        bool                                                                            fill = false;
    };

    class CSpinboxSpinner : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CSpinboxSpinner> create(SP<CSpinboxElement> element);
        virtual ~CSpinboxSpinner() = default;

        void updateLabel(const std::string& str);

      private:
        CSpinboxSpinner(SP<CSpinboxElement> element);

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                     acceptsMouseInput();
        virtual ePointerShape                            pointerShape();
        virtual bool                                     alwaysGetMouseInput();

        void                                             moveSelection(bool forward);
        void                                             init();

        SP<CSpinboxElement>                              m_parent;

        SP<CRectangleElement>                            m_background;
        SP<CRowLayoutElement>                            m_layout;
        SP<CTextElement>                                 m_label;
        SP<CSpinboxAngleElement>                         m_left;
        SP<CSpinboxAngleElement>                         m_right;
        SP<CNullElement>                                 m_leftPad, m_rightPad;

        WP<CSpinboxSpinner>                              m_self;

        friend class CSpinboxElement;
        friend class CSpinboxAngleElement;
    };

    struct SSpinboxImpl {
        SSpinboxData          data;

        WP<CSpinboxElement>   self;

        SP<CRowLayoutElement> layout;
        SP<CTextElement>      label;
        SP<CNullElement>      spacer;
        SP<CSpinboxSpinner>   spinner;
    };
}
