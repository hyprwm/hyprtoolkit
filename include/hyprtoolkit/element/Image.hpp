#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprgraphics/resource/AsyncResourceGatherer.hpp>
#include <hyprgraphics/resource/resources/ImageResource.hpp>

namespace Hyprtoolkit {

    class IRendererTexture;
    struct SImageImpl;
    struct SImageData;
    class CImageElement;

    class CImageBuilder {
      public:
        ~CImageBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CImageBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CImageBuilder>        path(std::string&&);
        Hyprutils::Memory::CSharedPointer<CImageBuilder>        a(float);
        Hyprutils::Memory::CSharedPointer<CImageBuilder>        rounding(int);
        Hyprutils::Memory::CSharedPointer<CImageBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CImageElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CImageBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<SImageData>  m_data;
        Hyprutils::Memory::CWeakPointer<CImageElement> m_element;

        CImageBuilder() = default;

        friend class CImageElement;
    };

    class CImageElement : public IElement {
      public:
        virtual ~CImageElement() = default;

        Hyprutils::Memory::CSharedPointer<CImageBuilder> rebuild();
        virtual Hyprutils::Math::Vector2D                      size();

      private:
        CImageElement(const SImageData& data);
        static Hyprutils::Memory::CSharedPointer<CImageElement> create(const SImageData& data);

        void                                                    replaceData(const SImageData& data);

        //
        virtual void                                                          paint();
        virtual void                                                          reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D>                      preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>                      minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>                      maximumSize(const Hyprutils::Math::Vector2D& parent);

        void                                                                  renderTex();

        Hyprutils::Memory::CUniquePointer<SImageImpl>                         m_impl;

        Hyprutils::Memory::CSharedPointer<IRendererTexture>                   m_tex;
        Hyprutils::Memory::CAtomicSharedPointer<Hyprgraphics::CImageResource> m_resource;
        Hyprutils::Math::Vector2D                                             m_size;

        bool                                                                  m_waitingForTex = false;

        friend class CImageBuilder;
    };
};
