#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprgraphics/color/Color.hpp>
#include <hyprgraphics/resource/resources/TextResource.hpp>

#include <hyprutils/memory/Atomic.hpp>

#include <optional>

namespace Hyprtoolkit {

    class IRendererTexture;

    struct STextData {
        std::string                              text;
        Hyprgraphics::CColor                     color = Hyprgraphics::CColor::SSRGB{.r = 1.F, .g = 1.F, .b = 1.F};
        float                                    a     = 1.F;
        std::optional<Hyprutils::Math::Vector2D> clampSize;
        CDynamicSize                             size{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {}}; // 0,0 means no size, automatic, fits parent
    };

    class CTextElement : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CTextElement> create(const STextData& data = {});
        virtual ~CTextElement() = default;

      private:
        CTextElement(const STextData& data);

        virtual void                                                         paint();
        virtual void                                                         reposition(const Hyprutils::Math::CBox& box);
        virtual Hyprutils::Math::Vector2D                                    size();
        virtual std::optional<Hyprutils::Math::Vector2D>                     preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>                     minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>                     maximumSize(const Hyprutils::Math::Vector2D& parent);

        STextData                                                            m_data;

        Hyprutils::Memory::CSharedPointer<IRendererTexture>                  m_tex;
        Hyprutils::Memory::CAtomicSharedPointer<Hyprgraphics::CTextResource> m_resource;
        Hyprutils::Math::Vector2D                                            m_size;
    };
};
