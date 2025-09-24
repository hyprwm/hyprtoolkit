#pragma once

#include <hyprtoolkit/element/Spinbox.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Null.hpp>

#include "../helpers/Memory.hpp"

namespace Hyprtoolkit {
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

        SP<CSpinboxElement>                              m_parent;

        SP<CRectangleElement>                            m_background;
        SP<CRowLayoutElement>                            m_layout;
        SP<CTextElement>                                 m_label;
        SP<CTextElement>                                 m_left;
        SP<CTextElement>                                 m_right;

        bool                                             m_primedForUp = false;
        Hyprutils::Math::Vector2D                        m_lastPosLocal;

        friend class CSpinboxElement;
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
