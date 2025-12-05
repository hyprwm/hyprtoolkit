-- Simple Form Example for Hyprtoolkit Lua Bindings
-- This demonstrates creating a basic registration form using Lua

-- Create the backend (entry point)
local backend = IBackend.create()
local palette = backend:getPalette()

-- Helper function for palette text color
local function textColor()
    return palette.text
end

-- Helper function for palette background color
local function bgColor()
    return palette.background
end

-- Helper function for alternate base color
local function altBaseColor()
    return palette.alternateBase
end

-- Create main window
local window = CWindowBuilder.begin()
    :appTitle("Lua Form Example")
    :appClass("hyprtoolkit-lua-form")
    :preferredSize(Vector2D.new(450, 400))
    :minSize(Vector2D.new(350, 300))
    :commence()

-- Background rectangle (fills window)
local bg = CRectangleBuilder.begin()
    :color(bgColor)
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_PERCENT,
        CDynamicSize.HT_SIZE_PERCENT,
        Vector2D.new(1, 1)
    ))
    :commence()

window.m_rootElement:addChild(bg)

-- Main content layout (column, centered)
local mainLayout = CColumnLayoutBuilder.begin()
    :gap(12)
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_PERCENT,
        CDynamicSize.HT_SIZE_AUTO,
        Vector2D.new(0.85, 1)
    ))
    :commence()

mainLayout:setMargin(20)
mainLayout:setPositionMode(PositionMode.ABSOLUTE)
mainLayout:setPositionFlag(PositionFlag.HCENTER, true)

bg:addChild(mainLayout)

-- Title
local title = CTextBuilder.begin()
    :text("User Registration")
    :fontSize(CFontSize.new(CFontSize.HT_FONT_H1, 1.0))
    :color(textColor)
    :align(FontAlignment.CENTER)
    :commence()

title:setPositionFlag(PositionFlag.HCENTER, true)
mainLayout:addChild(title)

-- Subtitle
local subtitle = CTextBuilder.begin()
    :text("Please fill in your details below")
    :fontSize(CFontSize.new(CFontSize.HT_FONT_TEXT, 1.0))
    :color(function()
        return palette.text:darken(0.3)
    end)
    :align(FontAlignment.CENTER)
    :commence()

subtitle:setPositionFlag(PositionFlag.HCENTER, true)
mainLayout:addChild(subtitle)

-- Separator line
local separator = CRectangleBuilder.begin()
    :color(function()
        return palette.text:darken(0.7)
    end)
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_PERCENT,
        CDynamicSize.HT_SIZE_ABSOLUTE,
        Vector2D.new(0.6, 2)
    ))
    :commence()

separator:setMargin(8)
separator:setPositionFlag(PositionFlag.HCENTER, true)
mainLayout:addChild(separator)

-- Name field label
local nameLabel = CTextBuilder.begin()
    :text("Full Name:")
    :fontSize(CFontSize.new(CFontSize.HT_FONT_TEXT, 1.0))
    :color(textColor)
    :commence()

mainLayout:addChild(nameLabel)

-- Name input field
local nameInput = CTextboxBuilder.begin()
    :placeholder("Enter your full name")
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_PERCENT,
        CDynamicSize.HT_SIZE_ABSOLUTE,
        Vector2D.new(1, 32)
    ))
    :onTextEdited(function(textbox, text)
        print("[Form] Name changed to: " .. text)
    end)
    :commence()

mainLayout:addChild(nameInput)

-- Email field label
local emailLabel = CTextBuilder.begin()
    :text("Email Address:")
    :fontSize(CFontSize.new(CFontSize.HT_FONT_TEXT, 1.0))
    :color(textColor)
    :commence()

mainLayout:addChild(emailLabel)

-- Email input field
local emailInput = CTextboxBuilder.begin()
    :placeholder("you@example.com")
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_PERCENT,
        CDynamicSize.HT_SIZE_ABSOLUTE,
        Vector2D.new(1, 32)
    ))
    :commence()

mainLayout:addChild(emailInput)

-- Priority slider row
local sliderRow = CRowLayoutBuilder.begin()
    :gap(12)
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_PERCENT,
        CDynamicSize.HT_SIZE_AUTO,
        Vector2D.new(1, 1)
    ))
    :commence()

local sliderLabel = CTextBuilder.begin()
    :text("Priority:")
    :fontSize(CFontSize.new(CFontSize.HT_FONT_TEXT, 1.0))
    :color(textColor)
    :commence()

local priorityValue = CTextBuilder.begin()
    :text("5")
    :fontSize(CFontSize.new(CFontSize.HT_FONT_TEXT, 1.0))
    :color(function()
        return palette.accent
    end)
    :commence()

local prioritySlider = CSliderBuilder.begin()
    :min(1)
    :max(10)
    :val(5)
    :snapInt(true)
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_PERCENT,
        CDynamicSize.HT_SIZE_ABSOLUTE,
        Vector2D.new(0.5, 14)
    ))
    :onChanged(function(slider, value)
        local intVal = math.floor(value + 0.5)
        priorityValue:rebuild():text(tostring(intVal)):commence()
        print("[Form] Priority set to: " .. intVal)
    end)
    :commence()

sliderRow:addChild(sliderLabel)
sliderRow:addChild(prioritySlider)
sliderRow:addChild(priorityValue)
mainLayout:addChild(sliderRow)

-- Category dropdown
local categoryRow = CRowLayoutBuilder.begin()
    :gap(12)
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_PERCENT,
        CDynamicSize.HT_SIZE_AUTO,
        Vector2D.new(1, 1)
    ))
    :commence()

local categoryLabel = CTextBuilder.begin()
    :text("Category:")
    :fontSize(CFontSize.new(CFontSize.HT_FONT_TEXT, 1.0))
    :color(textColor)
    :commence()

local categoryCombo = CComboboxBuilder.begin()
    :items({"General Inquiry", "Technical Support", "Sales", "Feedback", "Other"})
    :currentItem(0)
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_PERCENT,
        CDynamicSize.HT_SIZE_ABSOLUTE,
        Vector2D.new(0.6, 28)
    ))
    :onChanged(function(combo, index)
        print("[Form] Category selected: " .. tostring(index))
    end)
    :commence()

categoryRow:addChild(categoryLabel)
categoryRow:addChild(categoryCombo)
mainLayout:addChild(categoryRow)

-- Checkbox row for newsletter
local checkboxRow = CRowLayoutBuilder.begin()
    :gap(10)
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_AUTO,
        CDynamicSize.HT_SIZE_AUTO,
        Vector2D.new(1, 1)
    ))
    :commence()

-- Track checkbox state via local variable (state() method not yet implemented)
local newsletterSubscribed = false

local newsletterCheck = CCheckboxBuilder.begin()
    :toggled(false)
    :onToggled(function(checkbox, state)
        newsletterSubscribed = state
        print("[Form] Newsletter subscription: " .. tostring(state))
    end)
    :commence()

local checkboxLabel = CTextBuilder.begin()
    :text("Subscribe to newsletter")
    :fontSize(CFontSize.new(CFontSize.HT_FONT_TEXT, 1.0))
    :color(textColor)
    :commence()

checkboxRow:addChild(newsletterCheck)
checkboxRow:addChild(checkboxLabel)
mainLayout:addChild(checkboxRow)

-- Terms checkbox
local termsRow = CRowLayoutBuilder.begin()
    :gap(10)
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_AUTO,
        CDynamicSize.HT_SIZE_AUTO,
        Vector2D.new(1, 1)
    ))
    :commence()

-- Track terms checkbox state via local variable
local termsAccepted = false

local termsCheck = CCheckboxBuilder.begin()
    :toggled(false)
    :onToggled(function(checkbox, state)
        termsAccepted = state
    end)
    :commence()

local termsLabel = CTextBuilder.begin()
    :text("I agree to the terms and conditions")
    :fontSize(CFontSize.new(CFontSize.HT_FONT_TEXT, 1.0))
    :color(textColor)
    :commence()

termsRow:addChild(termsCheck)
termsRow:addChild(termsLabel)
mainLayout:addChild(termsRow)

-- Spacer
local spacer = CNullBuilder.begin()
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_ABSOLUTE,
        CDynamicSize.HT_SIZE_ABSOLUTE,
        Vector2D.new(0, 16)
    ))
    :commence()

mainLayout:addChild(spacer)

-- Button row
local buttonRow = CRowLayoutBuilder.begin()
    :gap(12)
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_PERCENT,
        CDynamicSize.HT_SIZE_AUTO,
        Vector2D.new(1, 1)
    ))
    :commence()

-- Right-align buttons with a spacer
local buttonSpacer = CNullBuilder.begin():commence()
buttonSpacer:setGrow(true)

-- Cancel button
local cancelButton = CButtonBuilder.begin()
    :label("Cancel")
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_AUTO,
        CDynamicSize.HT_SIZE_AUTO,
        Vector2D.new(1, 1)
    ))
    :onMainClick(function(btn)
        print("[Form] Cancelled")
        window:close()
        backend:destroy()
    end)
    :commence()

-- Submit button
local submitButton = CButtonBuilder.begin()
    :label("Submit")
    :size(CDynamicSize.new(
        CDynamicSize.HT_SIZE_AUTO,
        CDynamicSize.HT_SIZE_AUTO,
        Vector2D.new(1, 1)
    ))
    :onMainClick(function(btn)
        local name = nameInput:currentText()
        local email = emailInput:currentText()
        local category = categoryCombo:current()

        print("=== Form Submitted ===")
        print("Name: " .. name)
        print("Email: " .. email)
        print("Category: " .. tostring(category))
        print("Newsletter: " .. tostring(newsletterSubscribed))
        print("Terms accepted: " .. tostring(termsAccepted))
        print("======================")

        if not termsAccepted then
            print("[Error] You must accept the terms and conditions!")
            return
        end

        if name == "" then
            print("[Error] Name is required!")
            return
        end

        -- Success - update button text
        btn:rebuild():label("Submitted!"):commence()
    end)
    :commence()

buttonRow:addChild(buttonSpacer)
buttonRow:addChild(cancelButton)
buttonRow:addChild(submitButton)
mainLayout:addChild(buttonRow)

-- Handle window close request
window:onCloseRequest(function()
    print("[Form] Window close requested")
    window:close()
    backend:destroy()
end)

-- Open window and run the event loop
print("[Form] Starting application...")
window:open()
backend:enterLoop()

print("[Form] Application exited")
