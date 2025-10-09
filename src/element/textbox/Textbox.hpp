#pragma once

#include <hyprtoolkit/element/Textbox.hpp>
#include <string_view>

#include "../../helpers/Memory.hpp"
#include "../../helpers/Signal.hpp"

namespace Hyprtoolkit {

    class CRectangleElement;
    class CNullElement;
    class CTextElement;

    struct STextboxData {
        std::string                                                                                 text;
        std::string                                                                                 placeholder;
        std::function<void(Hyprutils::Memory::CSharedPointer<CTextboxElement>, const std::string&)> onTextEdited;
        bool                                                                                        multiline = true;
        CDynamicSize                                                                                size{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}};
    };
    struct STextboxImpl {
        STextboxData          data;

        WP<CTextboxElement>   self;

        SP<CRectangleElement> bg;
        SP<CNullElement>      cursorCont;
        SP<CRectangleElement> cursor;
        SP<CNullElement>      selectBgCont;
        SP<CRectangleElement> selectBg;
        SP<CTextElement>      text;
        SP<CTextElement>      placeholder;

        bool                  active = false;

        struct {
            CHyprSignalListener key;
            CHyprSignalListener enter;
            CHyprSignalListener leave;
            CHyprSignalListener mouseMove;
            CHyprSignalListener mouseButton;
        } listeners;

        struct {
            size_t      cursor = 0;
            std::string imText;
            ssize_t     selectBegin = -1, selectEnd = -1;
        } inputState;

        void                      updateSelect();
        void                      removeSelectedText();
        void                      focusCursorAtClickedChar();
        void                      updateLabel();
        void                      updateCursor();

        Hyprutils::Math::Vector2D lastCursorPos;
    };
}
