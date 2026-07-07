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

        // Starts a transition to the image at `path`.
        //
        // duration <= 0 means an immediate (non-animated) swap: the transition
        // completes on the first rendered frame.
        //
        // The caller supplies the fragment stage only; the vertex stage is the fixed
        // built-in tex300.vert (declares `in vec2 pos; in vec2 texcoord; uniform mat3 proj;`
        // and emits v_texcoord). The custom fragment must declare
        // `uniform sampler2D tex1, tex2; uniform float progress;` and should consume
        // v_texcoord (a fragment that ignores v_texcoord can have the attribute
        // dead-stripped). Optional uniforms: alpha, topLeft, fullSize, radius,
        // randomPixel, u_duration. A shader that fails to compile/link, or that is
        // missing any of pos/texcoord/proj/tex1/tex2/progress, is rejected and the
        // default transition shader is used instead.
        void transitionTo(const std::string& path,
                          eImageFitMode fitMode,
                          float duration,
                          const std::string& shaderSource = "");

        bool isTransitioning() const;

        float getTransitionProgressOut() const;
        float getTransitionProgressIn() const;

      private:
        CImageElement(const SImageData& data);
        static Hyprutils::Memory::CSharedPointer<CImageElement> create(const SImageData& data);

        // NOTE: calling replaceData() while a transitionTo()-driven transition is
        // active is not a supported interleaving: replaceData bumps the request
        // generation and updates the data, but does not touch transition state.
        void                                                    replaceData(const SImageData& data);

        //
        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                     positioningDependsOnChild();

        void                                             renderTex();

        void                                             renderTransition();

        Hyprutils::Memory::CUniquePointer<SImageImpl>    m_impl;

        friend class CImageBuilder;
        friend struct SImageImpl;
    };
};
