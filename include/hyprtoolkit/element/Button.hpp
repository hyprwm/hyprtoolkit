#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprgraphics/color/Color.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

#include <functional>

namespace Hyprtoolkit {

    struct SButtonImpl;
    class CButtonElement;

    struct SButtonData {
        std::string                                                            label = "Click me";
        std::function<void(Hyprutils::Memory::CSharedPointer<CButtonElement>)> onMainClick;
        std::function<void(Hyprutils::Memory::CSharedPointer<CButtonElement>)> onRightClick;
        CDynamicSize size{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {}}; // 0,0 means no size, automatic, fits parent
    };

    class CButtonElement : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CButtonElement> create(const SButtonData& data = {});
        virtual ~CButtonElement() = default;

        SButtonData dataCopy();
        void        replaceData(const SButtonData& data);

      private:
        CButtonElement(const SButtonData& data);

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                     acceptsMouseInput();
        virtual ePointerShape                            pointerShape();

        Hyprutils::Memory::CUniquePointer<SButtonImpl>   m_impl;
    };
};
