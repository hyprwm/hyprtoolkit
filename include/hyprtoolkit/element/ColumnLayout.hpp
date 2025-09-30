#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

namespace Hyprtoolkit {

    struct SColumnLayoutData;
    struct SColumnLayoutImpl;
    class CColumnLayoutElement;

    class CColumnLayoutBuilder {
      public:
        ~CColumnLayoutBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CColumnLayoutBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CColumnLayoutBuilder>        gap(size_t gap);
        Hyprutils::Memory::CSharedPointer<CColumnLayoutBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CColumnLayoutElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CColumnLayoutBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<SColumnLayoutData>  m_data;
        Hyprutils::Memory::CWeakPointer<CColumnLayoutElement> m_element;

        CColumnLayoutBuilder() = default;

        friend class CColumnLayoutElement;
    };

    class CColumnLayoutElement : public IElement {
      public:
        virtual ~CColumnLayoutElement() = default;

        Hyprutils::Memory::CSharedPointer<CColumnLayoutBuilder> rebuild();
        virtual Hyprutils::Math::Vector2D                       size();

      private:
        CColumnLayoutElement(const SColumnLayoutData& data);
        static Hyprutils::Memory::CSharedPointer<CColumnLayoutElement> create(const SColumnLayoutData& data);

        void                                                           replaceData(const SColumnLayoutData& data);

        virtual void                                                   paint();
        virtual void                                                   reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D>               preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>               minimumSize(const Hyprutils::Math::Vector2D& parent);

        Hyprutils::Math::Vector2D                                      childSize(Hyprutils::Memory::CSharedPointer<IElement> child);

        Hyprutils::Memory::CUniquePointer<SColumnLayoutImpl>           m_impl;

        friend class CColumnLayoutBuilder;
    };
};
