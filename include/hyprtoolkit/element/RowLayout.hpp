#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

namespace Hyprtoolkit {
    struct SRowLayoutData {
        CDynamicSize size = CDynamicSize(CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1});
        size_t       gap  = 0;
    };

    class CRowLayoutElement : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CRowLayoutElement> create(const SRowLayoutData& data = {});
        virtual ~CRowLayoutElement() = default;

      private:
        CRowLayoutElement(const SRowLayoutData& data);

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);

        Hyprutils::Math::Vector2D                        childSize(Hyprutils::Memory::CSharedPointer<IElement> child);

        SRowLayoutData                                   m_data;

        Hyprutils::Math::Vector2D                        m_lastSize;

        friend class CCheckboxElement;
        friend class CSpinboxElement;
        friend class CSpinboxSpinner;
    };
};
