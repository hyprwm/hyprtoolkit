#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"
#include "../types/FontTypes.hpp"

#include <hyprgraphics/color/Color.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

#include <functional>

namespace Hyprtoolkit {

    struct SButtonImpl;
    class CButtonElement;
    struct SButtonData;

    class CButtonBuilder {
      public:
        ~CButtonBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CButtonBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CButtonBuilder>        label(std::string&&);
        Hyprutils::Memory::CSharedPointer<CButtonBuilder>        noBorder(bool);
        Hyprutils::Memory::CSharedPointer<CButtonBuilder>        noBg(bool);
        Hyprutils::Memory::CSharedPointer<CButtonBuilder>        alignText(eFontAlignment);
        Hyprutils::Memory::CSharedPointer<CButtonBuilder>        fontFamily(std::string&&);
        Hyprutils::Memory::CSharedPointer<CButtonBuilder>        fontSize(CFontSize&&);
        Hyprutils::Memory::CSharedPointer<CButtonBuilder>        onMainClick(std::function<void(Hyprutils::Memory::CSharedPointer<CButtonElement>)>&&);
        Hyprutils::Memory::CSharedPointer<CButtonBuilder>        onRightClick(std::function<void(Hyprutils::Memory::CSharedPointer<CButtonElement>)>&&);
        Hyprutils::Memory::CSharedPointer<CButtonBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CButtonElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CButtonBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<SButtonData>  m_data;
        Hyprutils::Memory::CWeakPointer<CButtonElement> m_element;

        CButtonBuilder() = default;

        friend class CButtonElement;
    };

    class CButtonElement : public IElement {
      public:
        virtual ~CButtonElement() = default;

        Hyprutils::Memory::CSharedPointer<CButtonBuilder> rebuild();
        virtual Hyprutils::Math::Vector2D                 size();

      private:
        CButtonElement(const SButtonData& data);
        static Hyprutils::Memory::CSharedPointer<CButtonElement> create(const SButtonData& data);

        void                                                     replaceData(const SButtonData& data);

        virtual void                                             paint();
        virtual void                                             reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D>         preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>         minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>         maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                             acceptsMouseInput();
        virtual ePointerShape                                    pointerShape();

        Hyprutils::Memory::CUniquePointer<SButtonImpl>           m_impl;

        friend class CButtonBuilder;
    };
};
