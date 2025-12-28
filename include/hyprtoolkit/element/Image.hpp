#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"
#include "../types/ImageTypes.hpp"

#include <hyprgraphics/resource/AsyncResourceGatherer.hpp>
#include <hyprgraphics/resource/resources/ImageResource.hpp>

namespace Hyprtoolkit {

    class IRendererTexture;
    struct SImageImpl;
    struct SImageData;
    class CImageElement;
    class ISystemIconDescription;

    class CImageBuilder {
      public:
        ~CImageBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CImageBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CImageBuilder>        path(std::string&&);
        Hyprutils::Memory::CSharedPointer<CImageBuilder>        icon(const Hyprutils::Memory::CSharedPointer<ISystemIconDescription>&);
        Hyprutils::Memory::CSharedPointer<CImageBuilder>        data(std::vector<uint8_t>&& data);
        Hyprutils::Memory::CSharedPointer<CImageBuilder>        a(float);
        Hyprutils::Memory::CSharedPointer<CImageBuilder>        fitMode(eImageFitMode);
        Hyprutils::Memory::CSharedPointer<CImageBuilder>        sync(bool);
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
        virtual Hyprutils::Math::Vector2D                size();

      private:
        CImageElement(const SImageData& data);
        static Hyprutils::Memory::CSharedPointer<CImageElement> create(const SImageData& data);

        void                                                    replaceData(const SImageData& data);

        //
        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                     positioningDependsOnChild();

        void                                             renderTex();

        Hyprutils::Memory::CUniquePointer<SImageImpl>    m_impl;

        friend class CImageBuilder;
    };
};
