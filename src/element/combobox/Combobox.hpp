#pragma once

#include <hyprtoolkit/element/Combobox.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Null.hpp>

#include "../../helpers/Memory.hpp"
#include "../../helpers/Signal.hpp"
#include "../../renderer/Polygon.hpp"
#include "../../core/AnimatedVariable.hpp"

namespace Hyprtoolkit {
    class CComboboxClickable;
    class CButtonElement;
    class CColumnLayoutElement;
    class CScrollAreaElement;
    class IWindow;

    struct SDropdownHandleData {
        CDynamicSize size  = {CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}};
        colorFn      color = [] { return CHyprColor{1.F, 0.F, 0.F}; };
    };

    class CDropdownHandleElement : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CDropdownHandleElement> create(const SDropdownHandleData& data);
        virtual ~CDropdownHandleElement() = default;

      private:
        CDropdownHandleElement(const SDropdownHandleData& data);

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);

        SDropdownHandleData                              m_data;
        CPolygon                                         m_poly;

        friend class CCheckboxElement;
    };

    struct SComboboxData {
        std::string                                                                      label       = "Choose one";
        std::vector<std::string>                                                         items       = {"Item A", "Item B"};
        size_t                                                                           currentItem = 0;
        std::function<void(Hyprutils::Memory::CSharedPointer<CComboboxElement>, size_t)> onChanged;
        CDynamicSize                                                                     size{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {}};
        bool                                                                             fill = false;
    };

    class CComboboxClickable : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CComboboxClickable> create(SP<CComboboxElement> element);
        virtual ~CComboboxClickable() = default;

        void updateLabel(const std::string& str);

      private:
        CComboboxClickable(SP<CComboboxElement> element);

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                     acceptsMouseInput();
        virtual ePointerShape                            pointerShape();

        void                                             setSelection(size_t idx);
        void                                             init();

        void                                             openDropdown();
        void                                             closeDropdown();

        SP<CComboboxElement>                             m_parent;

        WP<CComboboxClickable>                           m_self;

        SP<CRectangleElement>                            m_background;
        SP<CRowLayoutElement>                            m_layout;
        SP<CTextElement>                                 m_label;
        SP<CDropdownHandleElement>                       m_handle;
        SP<CNullElement>                                 m_leftPad, m_rightPad;

        struct {
            SP<IWindow>                     popup;
            SP<CRectangleElement>           background;
            SP<CColumnLayoutElement>        layout;
            SP<CScrollAreaElement>          scroll;
            std::vector<SP<CButtonElement>> buttons;
        } m_dropdown;

        bool m_primedForUp = false;

        struct {
            CHyprSignalListener clicked;
        } m_listeners;

        friend class CComboboxElement;
    };

    struct SComboboxImpl {
        SComboboxData          data;

        WP<CComboboxElement>   self;

        SP<CRowLayoutElement>  layout;
        SP<CTextElement>       label;
        SP<CNullElement>       spacer;
        SP<CComboboxClickable> clickable;
    };
}
