#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

namespace Hyprtoolkit {
    struct SRowLayoutData;
    struct SRowLayoutImpl;
    class CRowLayoutElement;

    class CRowLayoutBuilder {
      public:
        ~CRowLayoutBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CRowLayoutBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CRowLayoutBuilder>        gap(size_t gap);
        Hyprutils::Memory::CSharedPointer<CRowLayoutBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CRowLayoutElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CRowLayoutBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<SRowLayoutData>  m_data;
        Hyprutils::Memory::CWeakPointer<CRowLayoutElement> m_element;

        CRowLayoutBuilder() = default;

        friend class CRowLayoutElement;
    };

    class CRowLayoutElement : public IElement {
      public:
        virtual ~CRowLayoutElement() = default;

        Hyprutils::Memory::CSharedPointer<CRowLayoutBuilder> rebuild();

      private:
        CRowLayoutElement(const SRowLayoutData& data);
        static Hyprutils::Memory::CSharedPointer<CRowLayoutElement> create(const SRowLayoutData& data);

        void                                                        replaceData(const SRowLayoutData& data);

        virtual void                                                paint();
        virtual void                                                reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                           size();
        virtual std::optional<Hyprutils::Math::Vector2D>            preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>            minimumSize(const Hyprutils::Math::Vector2D& parent);

        Hyprutils::Math::Vector2D                                   childSize(Hyprutils::Memory::CSharedPointer<IElement> child);

        Hyprutils::Memory::CUniquePointer<SRowLayoutImpl>           m_impl;

        friend class CCheckboxElement;
        friend class CSpinboxElement;
        friend class CSpinboxSpinner;
        friend class CRowLayoutBuilder;
    };
};
