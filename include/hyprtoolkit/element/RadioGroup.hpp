#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprutils/memory/UniquePtr.hpp>

#include <functional>
#include <string>
#include <vector>

namespace Hyprtoolkit {

    struct SRadioGroupImpl;
    struct SRadioGroupData;
    class CRadioGroupElement;

    class CRadioGroupBuilder {
      public:
        ~CRadioGroupBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CRadioGroupBuilder> begin();

        Hyprutils::Memory::CSharedPointer<CRadioGroupBuilder>        items(std::vector<std::string>&&);
        Hyprutils::Memory::CSharedPointer<CRadioGroupBuilder>        selected(int);
        Hyprutils::Memory::CSharedPointer<CRadioGroupBuilder>        gap(float);
        Hyprutils::Memory::CSharedPointer<CRadioGroupBuilder>        onSelected(std::function<void(Hyprutils::Memory::CSharedPointer<CRadioGroupElement>, int)>&&);
        Hyprutils::Memory::CSharedPointer<CRadioGroupBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CRadioGroupElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CRadioGroupBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<SRadioGroupData>  m_data;
        Hyprutils::Memory::CWeakPointer<CRadioGroupElement> m_element;

        CRadioGroupBuilder() = default;

        friend class CRadioGroupElement;
    };

    class CRadioGroupElement : public IElement {
      public:
        virtual ~CRadioGroupElement() = default;

        Hyprutils::Memory::CSharedPointer<CRadioGroupBuilder> rebuild();
        virtual Hyprutils::Math::Vector2D                     size();
        int                                                   selectedIndex();
        void                                                  setSelected(int);

      private:
        CRadioGroupElement(const SRadioGroupData& data);
        static Hyprutils::Memory::CSharedPointer<CRadioGroupElement> create(const SRadioGroupData& data);

        void                                                         replaceData(const SRadioGroupData& data);

        virtual void                                                 paint();
        virtual void                                                 reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D>             preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>             minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>             maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                                 positioningDependsOnChild();

        Hyprutils::Memory::CUniquePointer<SRadioGroupImpl>           m_impl;

        friend class CRadioGroupBuilder;
    };
};
