#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"
#include "../palette/Color.hpp"
#include "../types/FontTypes.hpp"

#include <hyprgraphics/resource/resources/TextResource.hpp>

#include <hyprutils/memory/Atomic.hpp>

#include <optional>

namespace Hyprtoolkit {

    class IRendererTexture;
    struct STextImpl;
    struct STextData;
    class CTextElement;

    class CTextBuilder {
      public:
        ~CTextBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CTextBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CTextBuilder>        color(colorFn&&);
        Hyprutils::Memory::CSharedPointer<CTextBuilder>        a(float);
        Hyprutils::Memory::CSharedPointer<CTextBuilder>        fontSize(CFontSize&&);
        Hyprutils::Memory::CSharedPointer<CTextBuilder>        text(std::string&&);
        Hyprutils::Memory::CSharedPointer<CTextBuilder>        fontFamily(std::string&&);
        Hyprutils::Memory::CSharedPointer<CTextBuilder>        clampSize(Hyprutils::Math::Vector2D&&);
        Hyprutils::Memory::CSharedPointer<CTextBuilder>        callback(std::function<void()>&&);
        Hyprutils::Memory::CSharedPointer<CTextBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CTextElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CTextBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<STextData>  m_data;
        Hyprutils::Memory::CWeakPointer<CTextElement> m_element;

        CTextBuilder() = default;

        friend class CTextElement;
    };

    class CTextElement : public IElement {
      public:
        virtual ~CTextElement();

        Hyprutils::Memory::CSharedPointer<CTextBuilder> rebuild();
        virtual Hyprutils::Math::Vector2D               size();

      private:
        CTextElement(const STextData& data);
        static Hyprutils::Memory::CSharedPointer<CTextElement> create(const STextData& data);

        void                                                   replaceData(const STextData& data);

        virtual void                                           paint();
        virtual void                                           reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D>       preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>       minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>       maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                           positioningDependsOnChild();
        virtual void                                           recheckColor();

        void                                                   renderTex();
        Hyprutils::Math::Vector2D                              unscale();

        Hyprutils::Memory::CUniquePointer<STextImpl>           m_impl;

        friend class CButtonElement;
        friend class CTextBuilder;
        friend struct STextboxImpl;
    };
};
