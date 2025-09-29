#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprgraphics/color/Color.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

#include <functional>

namespace Hyprtoolkit {

    struct SCheckboxImpl;
    struct SCheckboxData;
    class CCheckboxElement;

    class CCheckboxBuilder {
      public:
        ~CCheckboxBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CCheckboxBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CCheckboxBuilder>        label(std::string&&);
        Hyprutils::Memory::CSharedPointer<CCheckboxBuilder>        onToggled(std::function<void(Hyprutils::Memory::CSharedPointer<CCheckboxElement>, bool)>&&);
        Hyprutils::Memory::CSharedPointer<CCheckboxBuilder>        toggled(bool);
        Hyprutils::Memory::CSharedPointer<CCheckboxBuilder>        fill(bool);
        Hyprutils::Memory::CSharedPointer<CCheckboxBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CCheckboxElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CCheckboxBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<SCheckboxData>  m_data;
        Hyprutils::Memory::CWeakPointer<CCheckboxElement> m_element;

        CCheckboxBuilder() = default;

        friend class CCheckboxElement;
    };

    class CCheckboxElement : public IElement {
      public:
        virtual ~CCheckboxElement() = default;

        Hyprutils::Memory::CSharedPointer<CCheckboxBuilder> rebuild();
virtual Hyprutils::Math::Vector2D                      size();
        bool                                                state();
        void                                                setState(bool state);

      private:
        CCheckboxElement(const SCheckboxData& data);
        static Hyprutils::Memory::CSharedPointer<CCheckboxElement> create(const SCheckboxData& data);

        void                                                       replaceData(const SCheckboxData& data);

        virtual void                                               paint();
        virtual void                                               reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D>           preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>           minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>           maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                               acceptsMouseInput();
        virtual ePointerShape                                      pointerShape();

        Hyprutils::Memory::CUniquePointer<SCheckboxImpl>           m_impl;

        friend class CCheckboxBuilder;
    };
};
