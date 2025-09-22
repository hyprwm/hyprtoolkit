#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprgraphics/resource/AsyncResourceGatherer.hpp>
#include <hyprgraphics/resource/resources/ImageResource.hpp>

namespace Hyprtoolkit {

    class IRendererTexture;

    struct SImageData {
        std::string  path;
        float        a        = 1.F;
        int          rounding = 0;
        CDynamicSize size{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {}}; // 0,0 means no size, automatic, fits parent
    };

    class CImageElement : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CImageElement> create(const SImageData& data = {});
        virtual ~CImageElement() = default;

      private:
        CImageElement(const SImageData& data = {});

        virtual void                                                          paint();
        virtual void                                                          reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                                     size();
        virtual std::optional<Hyprutils::Math::Vector2D>                      preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>                      minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>                      maximumSize(const Hyprutils::Math::Vector2D& parent);

        void                                                                  renderTex();

        SImageData                                                            m_data;

        Hyprutils::Memory::CSharedPointer<IRendererTexture>                   m_tex;
        Hyprutils::Memory::CAtomicSharedPointer<Hyprgraphics::CImageResource> m_resource;
        Hyprutils::Math::Vector2D                                             m_size;

        bool                                                                  m_waitingForTex = false;
    };
};
