#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

namespace Hyprtoolkit {
    struct SScrollAreaData;
    struct SScrollAreaImpl;
    class CScrollAreaElement;

    class CScrollAreaBuilder {
      public:
        ~CScrollAreaBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CScrollAreaBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CScrollAreaBuilder>        scrollX(bool);
        Hyprutils::Memory::CSharedPointer<CScrollAreaBuilder>        scrollY(bool);
        Hyprutils::Memory::CSharedPointer<CScrollAreaBuilder>        blockUserScroll(bool);
        Hyprutils::Memory::CSharedPointer<CScrollAreaBuilder>        showScrollbar(bool);
        Hyprutils::Memory::CSharedPointer<CScrollAreaBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CScrollAreaElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CScrollAreaBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<SScrollAreaData>  m_data;
        Hyprutils::Memory::CWeakPointer<CScrollAreaElement> m_element;

        CScrollAreaBuilder() = default;

        friend class CScrollAreaElement;
    };

    class CScrollAreaElement : public IElement {
      public:
        virtual ~CScrollAreaElement() = default;

        Hyprutils::Memory::CSharedPointer<CScrollAreaBuilder> rebuild();
        virtual Hyprutils::Math::Vector2D                     size();

        Hyprutils::Math::Vector2D                             getCurrentScroll();
        void                                                  setScroll(const Hyprutils::Math::Vector2D&);

        // user children are routed into the inner scrolled layer
        virtual void addChild(Hyprutils::Memory::CSharedPointer<IElement> child);
        virtual void removeChild(Hyprutils::Memory::CSharedPointer<IElement> child);
        virtual void clearChildren();

      private:
        CScrollAreaElement(const SScrollAreaData& data);
        static Hyprutils::Memory::CSharedPointer<CScrollAreaElement> create(const SScrollAreaData& data);

        void                                                         init();
        void                                                         replaceData(const SScrollAreaData& data);

        // builds or tears down the scrollbar elements to match showScrollbar
        void rebuildScrollbars();
        // sizes, places and shows/hides the scrollbars for the current scroll state
        void                                                         layoutScrollbars();

        virtual void                                                 paint();
        virtual void                                                 reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D>             preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>             minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                                 acceptsMouseInput();
        virtual bool                                                 alwaysGetMouseInput();
        virtual ePointerShape                                        pointerShape();
        virtual bool                                                 positioningDependsOnChild();

        Hyprutils::Memory::CUniquePointer<SScrollAreaImpl>           m_impl;

        friend class CScrollAreaBuilder;
    };
};
