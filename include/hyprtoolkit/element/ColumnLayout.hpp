#pragma once

#include "Element.hpp"

namespace Hyprtoolkit {
    class CColumnLayoutElement : public IElement {
      public:
        CColumnLayoutElement()          = default;
        virtual ~CColumnLayoutElement() = default;

      private:
        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box);
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize();
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize();

        Hyprutils::Math::Vector2D                        childSize(Hyprutils::Memory::CSharedPointer<IElement> child);

        Hyprutils::Math::CBox                            m_position;
    };
};
