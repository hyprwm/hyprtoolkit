#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

namespace Hyprtoolkit {
    struct SColumnLayoutData {
        CDynamicSize size = CDynamicSize(CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1});
        size_t       gap  = 0;
    };

    class CColumnLayoutElement : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CColumnLayoutElement> create(const SColumnLayoutData& data = {});
        virtual ~CColumnLayoutElement() = default;

      private:
        CColumnLayoutElement(const SColumnLayoutData& data);

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box);
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);

        Hyprutils::Math::Vector2D                        childSize(Hyprutils::Memory::CSharedPointer<IElement> child);

        SColumnLayoutData                                m_data;
    };
};
