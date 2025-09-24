#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprgraphics/color/Color.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

#include <functional>

namespace Hyprtoolkit {

    struct SCheckboxImpl;
    class CCheckboxElement;

    struct SCheckboxData {
        std::string                                                                    label   = "Checkbox";
        bool                                                                           toggled = false;
        std::function<void(Hyprutils::Memory::CSharedPointer<CCheckboxElement>, bool)> onToggled;
        CDynamicSize size{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {}};
    };

    class CCheckboxElement : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CCheckboxElement> create(const SCheckboxData& data = {});
        virtual ~CCheckboxElement() = default;

        SCheckboxData dataCopy();
        void          replaceData(const SCheckboxData& data);

        bool          state();
        void          setState(bool state);

      private:
        CCheckboxElement(const SCheckboxData& data);

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                     acceptsMouseInput();
        virtual ePointerShape                            pointerShape();

        Hyprutils::Memory::CUniquePointer<SCheckboxImpl> m_impl;
    };
};
