#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprgraphics/color/Color.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

#include <functional>
#include <vector>

namespace Hyprtoolkit {

    struct SSpinboxImpl;
    struct SSpinboxData;
    class CSpinboxElement;

    class CSpinboxBuilder {
      public:
        ~CSpinboxBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CSpinboxBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CSpinboxBuilder>        label(std::string&&);
        Hyprutils::Memory::CSharedPointer<CSpinboxBuilder>        items(std::vector<std::string>&&);
        Hyprutils::Memory::CSharedPointer<CSpinboxBuilder>        currentItem(size_t);
        Hyprutils::Memory::CSharedPointer<CSpinboxBuilder>        onChanged(std::function<void(Hyprutils::Memory::CSharedPointer<CSpinboxElement>, size_t)>&&);
        Hyprutils::Memory::CSharedPointer<CSpinboxBuilder>        fill(bool);
        Hyprutils::Memory::CSharedPointer<CSpinboxBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CSpinboxElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CSpinboxBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<SSpinboxData>  m_data;
        Hyprutils::Memory::CWeakPointer<CSpinboxElement> m_element;

        CSpinboxBuilder() = default;

        friend class CSpinboxElement;
    };

    class CSpinboxElement : public IElement {
      public:
        virtual ~CSpinboxElement() = default;

        Hyprutils::Memory::CSharedPointer<CSpinboxBuilder> rebuild();
virtual Hyprutils::Math::Vector2D                      size();
        size_t                                             current();
        void                                               setCurrent(size_t current);

      private:
        CSpinboxElement(const SSpinboxData& data);
        static Hyprutils::Memory::CSharedPointer<CSpinboxElement> create(const SSpinboxData& data);

        void                                                      replaceData(const SSpinboxData& data);

        void                                                      init();

        virtual void                                              paint();
        virtual void                                              reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D>          preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>          minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>          maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                              acceptsMouseInput();
        virtual ePointerShape                                     pointerShape();

        Hyprutils::Memory::CUniquePointer<SSpinboxImpl>           m_impl;

        friend class CSpinboxSpinner;
        friend class CSpinboxBuilder;
    };
};
