#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"
#include "../palette/Color.hpp"

#include <hyprutils/memory/UniquePtr.hpp>

namespace Hyprtoolkit {
    struct SNullData {
        CDynamicSize size{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {}}; // 0,0 means no size, automatic, fits parent
    };

    class CNullElement : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CNullElement> create(const SNullData& data = {});
        virtual ~CNullElement() = default;

      private:
        CNullElement(const SNullData& data);

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);

        SNullData                                        m_data;
    };
};
