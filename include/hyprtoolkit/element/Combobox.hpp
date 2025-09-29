#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprgraphics/color/Color.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

#include <functional>
#include <vector>

namespace Hyprtoolkit {

    struct SComboboxImpl;
    struct SComboboxData;
    class CComboboxElement;

    class CComboboxBuilder {
      public:
        ~CComboboxBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CComboboxBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CComboboxBuilder>        items(std::vector<std::string>&&);
        Hyprutils::Memory::CSharedPointer<CComboboxBuilder>        currentItem(size_t);
        Hyprutils::Memory::CSharedPointer<CComboboxBuilder>        onChanged(std::function<void(Hyprutils::Memory::CSharedPointer<CComboboxElement>, size_t)>&&);
        Hyprutils::Memory::CSharedPointer<CComboboxBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CComboboxElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CComboboxBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<SComboboxData>  m_data;
        Hyprutils::Memory::CWeakPointer<CComboboxElement> m_element;

        CComboboxBuilder() = default;

        friend class CComboboxElement;
    };

    class CComboboxElement : public IElement {
      public:
        virtual ~CComboboxElement() = default;

        Hyprutils::Memory::CSharedPointer<CComboboxBuilder> rebuild();
virtual Hyprutils::Math::Vector2D                      size();
        size_t                                              current();
        void                                                setCurrent(size_t current);

      private:
        CComboboxElement(const SComboboxData& data);
        static Hyprutils::Memory::CSharedPointer<CComboboxElement> create(const SComboboxData& data);

        void                                                       replaceData(const SComboboxData& data);

        virtual void                                               paint();
        virtual void                                               reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D>           preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>           minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>           maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                               acceptsMouseInput();
        virtual ePointerShape                                      pointerShape();

        void                                                       setSelection(size_t idx);
        void                                                       init();
        void                                                       updateLabel(const std::string& str);

        void                                                       openDropdown();
        void                                                       closeDropdown();

        Hyprutils::Memory::CUniquePointer<SComboboxImpl>           m_impl;

        friend class CComboboxBuilder;
        friend class CComboboxClickable;
    };
};
