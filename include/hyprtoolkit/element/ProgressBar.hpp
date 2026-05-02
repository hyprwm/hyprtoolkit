#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprutils/memory/UniquePtr.hpp>

namespace Hyprtoolkit {

    struct SProgressBarImpl;
    struct SProgressBarData;
    class CProgressBarElement;

    class CProgressBarBuilder {
      public:
        ~CProgressBarBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CProgressBarBuilder> begin();

        // 0.0 - 1.0 (clamped). ignored when indeterminate is true.
        Hyprutils::Memory::CSharedPointer<CProgressBarBuilder>        value(float);

        // when true, the bar shows a looping pulse instead of value-driven fill
        Hyprutils::Memory::CSharedPointer<CProgressBarBuilder>        indeterminate(bool);

        Hyprutils::Memory::CSharedPointer<CProgressBarBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CProgressBarElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CProgressBarBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<SProgressBarData>  m_data;
        Hyprutils::Memory::CWeakPointer<CProgressBarElement> m_element;

        CProgressBarBuilder() = default;

        friend class CProgressBarElement;
    };

    class CProgressBarElement : public IElement {
      public:
        virtual ~CProgressBarElement() = default;

        Hyprutils::Memory::CSharedPointer<CProgressBarBuilder> rebuild();
        virtual Hyprutils::Math::Vector2D                      size();

        // imperative updates that skip the builder allocation
        float                                                  value();
        void                                                   setValue(float);

      private:
        CProgressBarElement(const SProgressBarData& data);
        static Hyprutils::Memory::CSharedPointer<CProgressBarElement> create(const SProgressBarData& data);

        void                                                          replaceData(const SProgressBarData& data);

        virtual void                                                  paint();
        virtual void                                                  reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D>              preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>              minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>              maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                                  positioningDependsOnChild();

        Hyprutils::Memory::CUniquePointer<SProgressBarImpl>           m_impl;

        friend class CProgressBarBuilder;
    };
};
