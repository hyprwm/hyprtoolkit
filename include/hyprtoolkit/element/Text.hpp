#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"
#include "../palette/Color.hpp"
#include "../types/FontSizeType.hpp"

#include <hyprgraphics/resource/resources/TextResource.hpp>

#include <hyprutils/memory/Atomic.hpp>

#include <optional>

namespace Hyprtoolkit {

    class IRendererTexture;
    struct STextImpl;

    struct STextData {
        std::string                              text;
        CFontSize                                fontSize{CFontSize::HT_FONT_TEXT};
        colorFn                                  color = [] { return CHyprColor{1.F, 1.F, 1.F, 1.F}; };
        float                                    a     = 1.F;
        std::optional<Hyprutils::Math::Vector2D> clampSize;
        CDynamicSize size{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}};
        std::function<void()>                    callback;                                                                 // called after resource is loaded
    };

    class CTextElement : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CTextElement> create(const STextData& data = {});
        virtual ~CTextElement() = default;

        STextData dataCopy();
        void      replaceData(const STextData& data);

      private:
        CTextElement(const STextData& data);

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);

        virtual void                                     recheckColor();

        void                                             renderTex();
        Hyprutils::Math::Vector2D                        unscale();

        STextData                                        m_data;

        Hyprutils::Memory::CUniquePointer<STextImpl>     m_impl;

        friend class CButtonElement;
    };
};
