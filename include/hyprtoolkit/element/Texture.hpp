#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprgraphics/resource/AsyncResourceGatherer.hpp>

namespace Hyprtoolkit {

    struct STextureData {
        Hyprutils::Memory::CAtomicSharedPointer<Hyprgraphics::IAsyncResource> resource;
        float                                                                 a        = 1.F;
        int                                                                   rounding = 0;
        CDynamicSize size{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {}}; // 0,0 means no size, automatic, fits parent
    };

    class CTextureElement : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CTextureElement> create(const STextureData& data = {});
        virtual ~CTextureElement() = default;

      private:
        CTextureElement(const STextureData& data = {});

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box);
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);

        STextureData                                     m_data;
    };
};
