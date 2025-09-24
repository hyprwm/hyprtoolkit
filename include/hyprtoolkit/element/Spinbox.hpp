#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprgraphics/color/Color.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

#include <functional>
#include <vector>

namespace Hyprtoolkit {

    struct SSpinboxImpl;
    class CSpinboxElement;

    struct SSpinboxData {
        std::string                                                                     label       = "Choose one";
        std::vector<std::string>                                                        items       = {"Item A", "Item B"};
        size_t                                                                          currentItem = 0;
        std::function<void(Hyprutils::Memory::CSharedPointer<CSpinboxElement>, size_t)> onChanged;
        CDynamicSize                                                                    size{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {}};
        bool                                                                            fill = false;
    };

    class CSpinboxElement : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CSpinboxElement> create(const SSpinboxData& data = {});
        virtual ~CSpinboxElement() = default;

        SSpinboxData dataCopy();
        void         replaceData(const SSpinboxData& data);

        size_t       current();
        void         setCurrent(size_t current);

      private:
        CSpinboxElement(const SSpinboxData& data);

        void                                             init();

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                     acceptsMouseInput();
        virtual ePointerShape                            pointerShape();

        Hyprutils::Memory::CUniquePointer<SSpinboxImpl>  m_impl;

        friend class CSpinboxSpinner;
    };
};
