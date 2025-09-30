#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"
#include "../palette/Color.hpp"

#include <hyprutils/memory/UniquePtr.hpp>

namespace Hyprtoolkit {

    struct SNullData;
    struct SNullImpl;
    class CNullElement;

    class CNullBuilder {
      public:
        ~CNullBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CNullBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CNullBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CNullElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CNullBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<SNullData>  m_data;
        Hyprutils::Memory::CWeakPointer<CNullElement> m_element;

        CNullBuilder() = default;

        friend class CNullElement;
    };

    class CNullElement : public IElement {
      public:
        virtual ~CNullElement() = default;

        Hyprutils::Memory::CSharedPointer<CNullBuilder> rebuild();
        virtual Hyprutils::Math::Vector2D               size();

      private:
        CNullElement(const SNullData& data);
        static Hyprutils::Memory::CSharedPointer<CNullElement> create(const SNullData& data);

        void                                                   replaceData(const SNullData& data);

        virtual void                                           paint();
        virtual void                                           reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D>       preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>       minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>       maximumSize(const Hyprutils::Math::Vector2D& parent);

        Hyprutils::Memory::CUniquePointer<SNullImpl>           m_impl;

        friend class CNullBuilder;
    };
};
