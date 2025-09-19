#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

namespace Hyprtoolkit {
    struct SRowLayoutData {
        CDynamicSize size = CDynamicSize(CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1});
    };

    class CRowLayoutElement : public IElement {
      public:
        CRowLayoutElement(const SRowLayoutData& data = {});
        virtual ~CRowLayoutElement() = default;

      private:
        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box);
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);

        Hyprutils::Math::Vector2D                        childSize(Hyprutils::Memory::CSharedPointer<IElement> child);

        Hyprutils::Math::CBox                            m_position;

        SRowLayoutData                                   m_data;
    };
};
