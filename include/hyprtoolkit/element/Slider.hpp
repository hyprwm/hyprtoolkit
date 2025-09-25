#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprgraphics/color/Color.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

#include <functional>

namespace Hyprtoolkit {

    struct SSliderImpl;
    struct SSliderData;
    class CSliderElement;

    class CSliderBuilder {
      public:
        ~CSliderBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CSliderBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CSliderBuilder>        label(std::string&&);
        Hyprutils::Memory::CSharedPointer<CSliderBuilder>        onChanged(std::function<void(Hyprutils::Memory::CSharedPointer<CSliderElement>, float)>&&);
        Hyprutils::Memory::CSharedPointer<CSliderBuilder>        min(float);
        Hyprutils::Memory::CSharedPointer<CSliderBuilder>        max(float);
        Hyprutils::Memory::CSharedPointer<CSliderBuilder>        val(float);
        Hyprutils::Memory::CSharedPointer<CSliderBuilder>        fill(bool);
        Hyprutils::Memory::CSharedPointer<CSliderBuilder>        snapInt(bool);
        Hyprutils::Memory::CSharedPointer<CSliderBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CSliderElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CSliderBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<SSliderData>  m_data;
        Hyprutils::Memory::CWeakPointer<CSliderElement> m_element;

        CSliderBuilder() = default;

        friend class CSliderElement;
    };

    class CSliderElement : public IElement {
      public:
        virtual ~CSliderElement() = default;

        Hyprutils::Memory::CSharedPointer<CSliderBuilder> rebuild();

        bool                                              state();
        void                                              setState(bool state);

      private:
        CSliderElement(const SSliderData& data);
        static Hyprutils::Memory::CSharedPointer<CSliderElement> create(const SSliderData& data);

        void                                                     replaceData(const SSliderData& data);
        void                                                     valueChanged(float perc);

        void                                                     init();

        virtual void                                             paint();
        virtual void                                             reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                        size();
        virtual std::optional<Hyprutils::Math::Vector2D>         preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>         minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>         maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                             acceptsMouseInput();
        virtual ePointerShape                                    pointerShape();

        Hyprutils::Memory::CUniquePointer<SSliderImpl>           m_impl;

        friend class CSliderBuilder;
        friend class CSliderSlider;
    };
};
