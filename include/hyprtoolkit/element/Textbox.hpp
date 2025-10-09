#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"
#include "../palette/Color.hpp"

#include <hyprutils/memory/UniquePtr.hpp>

namespace Hyprtoolkit {

    struct STextboxImpl;
    struct STextboxData;
    class CTextboxElement;

    class CTextboxBuilder {
      public:
        ~CTextboxBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CTextboxBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CTextboxBuilder>        placeholder(std::string&&);
        Hyprutils::Memory::CSharedPointer<CTextboxBuilder>        defaultText(std::string&&);
        Hyprutils::Memory::CSharedPointer<CTextboxBuilder>        onTextEdited(std::function<void(Hyprutils::Memory::CSharedPointer<CTextboxElement>, const std::string&)>&&);
        Hyprutils::Memory::CSharedPointer<CTextboxBuilder>        multiline(bool);
        Hyprutils::Memory::CSharedPointer<CTextboxBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CTextboxElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CTextboxBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<STextboxData>  m_data;
        Hyprutils::Memory::CWeakPointer<CTextboxElement> m_element;

        CTextboxBuilder() = default;

        friend class CTextboxElement;
    };

    class CTextboxElement : public IElement {
      public:
        virtual ~CTextboxElement() = default;

        Hyprutils::Memory::CSharedPointer<CTextboxBuilder> rebuild();
        virtual Hyprutils::Math::Vector2D                  size();
        void                                               focus(bool focus = true);

      private:
        static Hyprutils::Memory::CSharedPointer<CTextboxElement> create(const STextboxData& data);
        CTextboxElement(const STextboxData& data);

        void                                             replaceData(const STextboxData& data);
        void                                             init();

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                     acceptsMouseInput();
        virtual ePointerShape                            pointerShape();
        virtual bool                                     acceptsKeyboardInput();
        virtual void                                     imCommitNewText(const std::string&);
        virtual void                                     imApplyText();
        virtual bool                                     positioningDependsOnChild();

        Hyprutils::Memory::CUniquePointer<STextboxImpl>  m_impl;

        friend class CTextboxBuilder;
    };
};
