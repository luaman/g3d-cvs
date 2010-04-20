/**
 \file GuiTheme.cpp
 \author Morgan McGuire, http://graphics.cs.williams.edu

 \created 2008-01-01
 \edited  2010-03-04

 Copyright 2000-2010, Morgan McGuire
 All rights reserved
*/

#include "G3D/platform.h"
#include "GLG3D/GuiTheme.h"
#include "G3D/WeakCache.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/TextInput.h"
#include "G3D/fileutils.h"
#include "G3D/Image3.h"
#include "G3D/Log.h"
#include "G3D/FileSystem.h"

namespace G3D {

namespace _internal {    
    Morph::Morph() : active(false) {}

void Morph::morphTo(const Rect2D& startPos, const Rect2D& endPos) {
    active = true;
    start  = startPos;
    end    = endPos;

    // Make the morph approximately constant velocity
    const float pixelsPerSecond = 1500;

    duration = max((double)0.12, (double)(start.center() - end.center()).length() / pixelsPerSecond);

    startTime = System::time();
}

}


static void toGLMatrix(const Matrix4& m, float f[]) {
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            f[r * 4 + c] = m[r][c];
        }
    }
}

WeakReferenceCountedPointer<GuiTheme> GuiTheme::lastThemeLoaded;

GuiTheme::GuiTheme(const std::string& filename,
        const GFont::Ref&   fallbackFont,
        float               fallbackSize, 
        const Color4&       fallbackColor, 
        const Color4&       fallbackOutlineColor) : delayedTextCount(0), inRendering(false){

    alwaysAssertM(FileSystem::exists(filename), "Cannot find " + filename);

    BinaryInput b(filename, G3D_LITTLE_ENDIAN, true);
    m_textStyle.font = fallbackFont;
    m_textStyle.size = fallbackSize;
    m_textStyle.color = fallbackColor;
    m_textStyle.outlineColor = fallbackOutlineColor;
    deserialize(filenamePath(filename), b);
}


GuiThemeRef GuiTheme::fromFile(
    const std::string&  filename,
    GFont::Ref          fallbackFont,
    float               fallbackSize, 
    const Color4&       fallbackColor, 
    const Color4&       fallbackOutlineColor) {

    static WeakCache<std::string, GuiThemeRef> cache;
    
    GuiTheme::Ref instance = cache[filename];
    if (instance.isNull()) {

        if (fallbackFont.isNull()) {
            fallbackFont = GFont::fromFile(System::findDataFile("arial.fnt"));
        }

        instance = new GuiTheme(filename, fallbackFont, fallbackSize, fallbackColor, fallbackOutlineColor);
        cache.set(filename, instance);
    }

    lastThemeLoaded = instance;

    return instance;
}


static Color4 readColor(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "(");
    float R,G,B,A;
    R = b.readNumber();
    b.readSymbol(","); G = b.readNumber();
    b.readSymbol(",");
    B = b.readNumber();
    b.readSymbol(",");
    A = b.readNumber();
    b.readSymbol(")");
    return Color4(R, G, B, A);
}


void GuiTheme::deserialize(const std::string& path, TextInput& b) {
    b.readSymbols("name", "=");
    std::string name = b.readString();

    b.readSymbols("format", "=");
    float version = b.readNumber();
    (void)version;
    debugAssertM(fuzzyEq(version, 0.1), format("Only version 0.1 is supported (version = %f)", version));

    m_textStyle.deserialize(path, "font", b);
    m_disabledTextStyle = m_textStyle;
    m_disabledTextStyle.deserialize(path, "disabledFont", b);
    
    // Controls (all inherit the default text style and may override)
    m_checkBox.textStyle = m_textStyle;
    m_checkBox.disabledTextStyle = m_disabledTextStyle;
    m_checkBox.deserialize("checkBox",path,  b);
    
    m_radioButton.textStyle = m_textStyle;
    m_radioButton.disabledTextStyle = m_disabledTextStyle;
    m_radioButton.deserialize("radioButton",path,  b);

    m_button[NORMAL_BUTTON_STYLE].textStyle = m_textStyle;
    m_button[NORMAL_BUTTON_STYLE].disabledTextStyle = m_disabledTextStyle;
    m_button[NORMAL_BUTTON_STYLE].deserialize("button", path, b);

    m_button[TOOL_BUTTON_STYLE].textStyle = m_textStyle;
    m_button[TOOL_BUTTON_STYLE].disabledTextStyle = m_disabledTextStyle;
    m_button[TOOL_BUTTON_STYLE].deserialize("toolButton", path, b);

    m_closeButton.deserialize("closeButton", b);

    b.readSymbols("windowButtonStyle", "=");
    m_osxWindowButtons = (b.readSymbol() == "osx");


    static std::string windowStyleName[WINDOW_STYLE_COUNT] = {"window", "toolWindow", "dialogWindow", "drawer", "menu", "no"};
    debugAssert(windowStyleName[WINDOW_STYLE_COUNT - 1] == "no");
    // Skip the no-style window
    for (int i = 0; i < WINDOW_STYLE_COUNT - 1; ++i) {
        m_window[i].textStyle = m_textStyle;
        m_window[i].deserialize(windowStyleName[i], path, b);
    }

    m_hSlider.textStyle = m_textStyle;
    m_hSlider.disabledTextStyle = m_disabledTextStyle;
    m_hSlider.deserialize("horizontalSlider", path, b);

    m_pane[SIMPLE_PANE_STYLE].textStyle = m_textStyle;
    m_pane[SIMPLE_PANE_STYLE].disabledTextStyle = m_disabledTextStyle;
    m_pane[SIMPLE_PANE_STYLE].deserialize("simplePane", path, b);

    m_pane[ORNATE_PANE_STYLE].textStyle = m_textStyle;
    m_pane[ORNATE_PANE_STYLE].disabledTextStyle = m_disabledTextStyle;
    m_pane[ORNATE_PANE_STYLE].deserialize("ornatePane", path, b);

    m_textBox.textStyle = m_textStyle;
    m_textBox.disabledTextStyle = m_disabledTextStyle;
    m_textBox.deserialize("textBox", path, b);

    m_dropDownList.textStyle = m_textStyle;
    m_dropDownList.disabledTextStyle = m_disabledTextStyle;
    m_dropDownList.deserialize("dropDownList", path, b);

    // TODO: eventually, set canvas separately when the skin files have been updated
    m_canvas.base = m_textBox.base;
    m_canvas.disabled = m_textBox.disabled;
    m_canvas.disabledTextStyle = m_textBox.disabledTextStyle;
    m_canvas.enabled = m_textBox.enabled;
    m_canvas.pad = m_textBox.textPad;
    m_canvas.textStyle = m_textBox.textStyle;

    m_canvas.pad.bottomRight -= Vector2(4, 2);
    m_canvas.pad.topLeft     -= Vector2(5, 2);

    m_selection.deserialize("selection", b);
}


void GuiTheme::deserialize(const std::string& path, BinaryInput& b) {
    std::string f = b.readString32();
    (void)f;
    debugAssert(f == "G3D Skin File");

    float version = b.readFloat32();
    (void)version;
    debugAssert(fuzzyEq(version, 0.1f));

    std::string coords = b.readString32();
    TextInput t(TextInput::FROM_STRING, coords);
    deserialize(path, t);

    GImage image;
    image.decode(b, GImage::TGA);

    Texture::Preprocess p;
    p.computeMinMaxMean = false;
    
    texture = Texture::fromGImage(b.getFilename(), image, ImageFormat::RGBA8(), Texture::DIM_2D, Texture::Settings::video(), p);

    toGLMatrix(Matrix4(1.0f / texture->width(), 0, 0, 0,
            0, 1.0f / texture->height(), 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1), guiTextureMatrix);
}


void GuiTheme::beginRendering(RenderDevice* rd) {
    this->rd = rd;

    debugAssert(! inRendering);
    inRendering = true;

    rd->push2D();

    rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
    rd->setCullFace(RenderDevice::CULL_NONE);
    rd->setAlphaTest(RenderDevice::ALPHA_GREATER, 0);
    rd->setColor(Color3::white());

    rd->setTexture(TEXTURE_UNIT, texture);
    rd->setTextureMatrix(TEXTURE_UNIT, guiTextureMatrix);
    rd->beginPrimitive(PrimitiveType::QUADS);
}


void GuiTheme::beginText() const {
    rd->endPrimitive();
}


void GuiTheme::endText() const {
    glBindTexture(GL_TEXTURE_2D, texture->openGLID());
    glMatrixMode(GL_TEXTURE);
    glLoadMatrix(guiTextureMatrix);

    glColor3f(1,1,1);
        
    rd->beginPrimitive(PrimitiveType::QUADS);
}


void GuiTheme::pauseRendering() {
    drawDelayedText();
    debugAssert(inRendering);
    rd->endPrimitive();

    rd->setTexture(TEXTURE_UNIT, NULL);
    rd->setTextureMatrix(TEXTURE_UNIT, Matrix4::identity());
    rd->pushState();
}


void GuiTheme::resumeRendering() {
    rd->popState();

    rd->setTexture(TEXTURE_UNIT, texture);
    rd->setTextureMatrix(TEXTURE_UNIT, guiTextureMatrix);
    rd->setColor(Color3::white());
    rd->beginPrimitive(PrimitiveType::QUADS);
}


void GuiTheme::endRendering() {
    // Draw any remaining text
    drawDelayedText();

    debugAssert(inRendering);

    debugAssertM( coordinateFrameStack.size() == 0, 
        "pushClientRect without matching popClientRect");

    rd->endPrimitive();
    rd->pop2D();
    inRendering = false;
    rd = NULL;
}


void GuiTheme::drawCheckable
    (const Checkable& control, const Rect2D& bounds, 
     bool enabled, bool focused, bool selected, const GuiText& text) const {

    debugAssert(inRendering);
    control.render(rd, bounds, enabled, focused, selected);

    if (text.numElements() > 0) {
        const TextStyle& style = enabled ? control.textStyle : control.disabledTextStyle;

        for (int e = 0; e < text.numElements(); ++e) {
            const GuiText::Element& element = text.element(e);
            addDelayedText
            (element.font(style.font), 
             element.text(), 
             Vector2(control.width() + bounds.x0(), 
                     (bounds.y0() + bounds.y1()) / 2) + control.textOffset,
             element.size(style.size), 
             element.color(style.color), 
             element.outlineColor(style.outlineColor), 
             GFont::XALIGN_LEFT);
        }
    }
}


void GuiTheme::addDelayedText
(const GuiText& text, 
 const GuiTheme::TextStyle& defaults,
 const Vector2& position, 
 GFont::XAlign xalign,
 GFont::YAlign yalign) const {
    if (text.numElements() == 0) {
        return;
    }
    
    const GuiText::Element& element = text.element(0);

    float size = element.size(defaults.size);
    const GFont::Ref& font = element.font(defaults.font);
    const Color4& color = element.color(defaults.color);
    const Color4& outlineColor = element.outlineColor(defaults.outlineColor);
 
    addDelayedText(font, element.text(), position, size, color, outlineColor, xalign, yalign);
}


void GuiTheme::renderDropDownList
(const Rect2D&        initialBounds,
 bool                 enabled,
 bool                 focused,
 bool                 down,
 const GuiText&       contentText,
 const GuiText&       text,
 float                captionWidth) const {

    // Dropdown list has a fixed height
    // Offset by left_caption_width
    const Rect2D& bounds = dropDownListToClickBounds(initialBounds, captionWidth);

    m_dropDownList.render(rd, bounds, enabled, focused, down);

    // Area in which text appears
    Rect2D clientArea = Rect2D::xywh(bounds.x0y0() + m_dropDownList.textPad.topLeft, 
                                     bounds.wh() - (m_dropDownList.textPad.bottomRight + m_dropDownList.textPad.topLeft));

    // Display cropped text
    // Draw inside the client area
    const_cast<GuiTheme*>(this)->pushClientRect(clientArea);
    addDelayedText(contentText, m_dropDownList.textStyle, Vector2(0, clientArea.height() / 2), GFont::XALIGN_LEFT, GFont::YALIGN_CENTER);
    const_cast<GuiTheme*>(this)->popClientRect();

    addDelayedText(text, m_dropDownList.textStyle,
                   Vector2(initialBounds.x0(), (initialBounds.y0() + initialBounds.y1()) * 0.5f), 
                   GFont::XALIGN_LEFT);
}


void GuiTheme::renderSelection(const Rect2D& bounds) const {
    m_selection.render(rd, bounds, Vector2(0, 0));
}


void GuiTheme::renderTextBox
    (const Rect2D&      fullBounds, 
     bool               enabled, 
     bool               focused, 
     const GuiText&     caption,
     float              captionWidth,
     const GuiText&     text, 
     const GuiText&     cursor, 
     int                cursorPosition) const {

    const Rect2D& bounds = textBoxToClickBounds(fullBounds, captionWidth);

    m_textBox.render(rd, bounds, enabled, focused);

    alwaysAssertM(text.numElements() < 2, "Text box cannot contain GuiText with more than 1 element");

    
    // Area in which text appears
    Rect2D clientArea = Rect2D::xywh(bounds.x0y0() + m_textBox.textPad.topLeft, 
                                     bounds.wh() - (m_textBox.textPad.bottomRight + m_textBox.textPad.topLeft));


    // Draw inside the client area
    const_cast<GuiTheme*>(this)->pushClientRect(clientArea);
    std::string beforeCursor;
    Vector2 beforeBounds;
    float size          = m_textBox.contentStyle.size;
    GFont::Ref font     = m_textBox.contentStyle.font;
    Color4 color        = m_textBox.contentStyle.color;
    Color4 outlineColor = m_textBox.contentStyle.outlineColor;
    std::string all;
    if (text.numElements() == 1) {
        const GuiText::Element& element = text.element(0);

        // Compute pixel distance from left edge to cursor position
        all = element.text();
        beforeCursor   = all.substr(0, cursorPosition);
        size           = element.size(m_textBox.contentStyle.size);
        font           = element.font(m_textBox.contentStyle.font);
        color          = element.color(m_textBox.contentStyle.color);
        outlineColor   = element.outlineColor(m_textBox.contentStyle.outlineColor);
        beforeBounds   = font->bounds(beforeCursor, size);
    }

    // Slide the text backwards so that the cursor is visible
    float textOffset = -max(0.0f, beforeBounds.x - clientArea.width());

    if (! enabled) {
        // Dim disabled text color
        color.a *= 0.8f;
    }

    // Draw main text
    addDelayedText(font, all, Vector2(textOffset, clientArea.height() / 2), size, color, 
                   outlineColor, GFont::XALIGN_LEFT, GFont::YALIGN_CENTER);

    // Draw cursor
    if (focused) {
        addDelayedText(cursor,
                       m_textBox.contentStyle,
                       Vector2(textOffset + beforeBounds.x, clientArea.height() / 2), 
                       GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
    }
    
    const_cast<GuiTheme*>(this)->popClientRect();
    
    addDelayedText(caption, m_textBox.textStyle,
                   Vector2(fullBounds.x0(), (fullBounds.y0() + fullBounds.y1()) * 0.5f), 
                       GFont::XALIGN_LEFT);
}


    Vector2 GuiTheme::bounds(const GuiText& text) const {
    if (text.numElements() == 0) {
        return Vector2::zero();
    }

    Vector2 b(0, 0);

    // TODO Elements: need to look at each element
    const GuiText::Element& element = text.element(0);
    const std::string& str  = element.text();

    // Loop twice, one for normal style and once for disabled style
    for (int i = 0; i < 2; ++i) {
        const TextStyle& style = (i == 0) ? m_textStyle : m_disabledTextStyle;

        const GFont::Ref&  font = element.font(style.font);
        int                size = element.size(style.size);
        bool               outline = element.outlineColor(style.outlineColor).a > 0;

        Vector2 t = font->bounds(str, size);
        if (outline) {
            t += Vector2(2, 2);
        }
        b = b.max(t);
    }
    return b;
}


void GuiTheme::renderCanvas
    (const Rect2D&      fullBounds, 
     bool               enabled, 
     bool               focused, 
     const GuiText&     caption,
     float              captionHeight) const {

    const Rect2D& bounds = canvasToClickBounds(fullBounds, captionHeight);

    m_canvas.render(rd, bounds, enabled, focused);

    addDelayedText(caption, m_canvas.textStyle, Vector2(fullBounds.x0(), bounds.y0()), 
                   GFont::XALIGN_LEFT, GFont::YALIGN_BOTTOM);
}


void GuiTheme::renderCheckBox(const Rect2D& bounds, bool enabled, bool focused, bool selected, 
                             const GuiText& text) const {
    drawCheckable(m_checkBox, bounds, enabled, focused, selected, text);
}


void GuiTheme::renderPane(const Rect2D& fullBounds, const GuiText& caption, PaneStyle paneStyle) const {
    // TODO: Caption, 
    Rect2D paneRenderBounds = fullBounds;

    if (! caption.empty()) {
        const float pad = paneTopPadding(caption, paneStyle);
        paneRenderBounds = Rect2D::xyxy(fullBounds.x0(), fullBounds.y0() + pad, fullBounds.x1(), fullBounds.y1());
        addDelayedText(caption, m_pane[paneStyle].textStyle, Vector2(fullBounds.x0(), paneRenderBounds.y0()), 
                       GFont::XALIGN_LEFT, GFont::YALIGN_BOTTOM);
    }

    if (paneStyle != NO_PANE_STYLE) {
        m_pane[paneStyle].frame.render(rd, paneRenderBounds, Vector2::zero());
    }
}


void GuiTheme::renderWindow(const Rect2D& bounds, bool focused, bool hasClose, 
                           bool closeIsDown, bool closeIsFocused, const GuiText& text, 
                           WindowStyle windowStyle) const {
    drawWindow(m_window[windowStyle], bounds, focused, hasClose, closeIsDown, closeIsFocused, text);
}


Rect2D GuiTheme::windowToCloseButtonBounds(const Rect2D& bounds, WindowStyle windowStyle) const {
    return closeButtonBounds(m_window[windowStyle], bounds);
}


Rect2D GuiTheme::closeButtonBounds(const Window& window, const Rect2D& bounds) const {
    // If the close button is larger
    // than the title area, draw it half size (e.g., for tool
    // windows)
    float titleHeight = window.borderThickness.topLeft.y;
    float scale = 1.0f;
    if (titleHeight < m_closeButton.base.height()) {
        scale = 0.5f;
    }
    
    // Position button
    Vector2 center; 
    if (m_osxWindowButtons) {
        center.x = bounds.x0() + max(window.borderThickness.topLeft.x, window.borderThickness.topLeft.y * 0.25f) * scale + scale * m_closeButton.base.width() / 2;
    } else {
        center.x = bounds.x1() - max(window.borderThickness.bottomRight.x, window.borderThickness.topLeft.y * 0.25f) * scale - scale * m_closeButton.base.width() / 2;
    }
    center.y = bounds.y0() + window.borderThickness.topLeft.y / 2;
    
    // Draw button
    Vector2 wh = m_closeButton.base.wh() * scale;
    Rect2D vertex = Rect2D::xywh(center - wh / 2, wh);
    
    return vertex;
}


void GuiTheme::drawWindow(const Window& window, const Rect2D& bounds, 
                         bool focused, bool hasClose, bool closeIsDown, bool closeIsFocused,                          
                         const GuiText& text) const {
    // Update any pending text since the window may overlap another window
    drawDelayedText();

    window.render(rd, bounds, focused);

    if (hasClose) {

        const Rect2D& vertex = closeButtonBounds(window, bounds);

        Vector2 offset;
        if (focused) {
            if (closeIsFocused) {
                if (closeIsDown) {
                    offset = m_closeButton.focusedDown;
                } else {
                    offset = m_closeButton.focusedUp;
                }
            } else {
                offset = m_closeButton.defocused;
            }
        } else {
            offset = m_closeButton.windowDefocused;
        }

        drawRect(vertex, m_closeButton.base + offset, rd);
    }
    
    if (window.borderThickness.topLeft.y > 4) {
        const TextStyle& style = focused ? window.textStyle : window.defocusedTextStyle;

        addDelayedText(text, style,
                       Vector2(bounds.center().x, bounds.y0() + window.borderThickness.topLeft.y * 0.5), 
                       GFont::XALIGN_CENTER, 
                       GFont::YALIGN_CENTER);
    }
}


Rect2D GuiTheme::horizontalSliderToSliderBounds(const Rect2D& bounds, float captionWidth) const {
    return Rect2D::xywh(bounds.x0() + captionWidth, bounds.y0(), bounds.width() - captionWidth, bounds.height());
}


Rect2D GuiTheme::horizontalSliderToThumbBounds(const Rect2D& bounds, float pos, float captionWidth) const {
    return m_hSlider.thumbBounds(horizontalSliderToSliderBounds(bounds, captionWidth), pos);
}


Rect2D GuiTheme::horizontalSliderToTrackBounds(const Rect2D& bounds, float captionWidth) const {
    return m_hSlider.trackBounds(horizontalSliderToSliderBounds(bounds, captionWidth));
}


Rect2D GuiTheme::windowToTitleBounds(const Rect2D& bounds, WindowStyle windowStyle) const {
    return Rect2D::xywh(bounds.x0y0(), Vector2(bounds.width(), m_window[windowStyle].borderThickness.topLeft.y));
}


Rect2D GuiTheme::windowToClientBounds(const Rect2D& bounds, WindowStyle windowStyle) const {
    return Rect2D::xywh(bounds.x0y0() + m_window[windowStyle].netClientPad.topLeft, 
                        bounds.wh() - m_window[windowStyle].netClientPad.wh());
}


Rect2D GuiTheme::clientToWindowBounds(const Rect2D& bounds, WindowStyle windowStyle) const {
    return Rect2D::xywh(bounds.x0y0() - m_window[windowStyle].netClientPad.topLeft, 
                        bounds.wh() + m_window[windowStyle].netClientPad.wh());
}


Rect2D GuiTheme::textBoxToClickBounds(const Rect2D& bounds, float captionWidth) const {
    return Rect2D::xyxy(bounds.x0() + captionWidth, bounds.y0(), bounds.x1(), bounds.y1());
}


Rect2D GuiTheme::canvasToClickBounds(const Rect2D& bounds, float captionHeight) const {
    // Canvas does not receive indent; its caption goes on top
    return Rect2D::xyxy(bounds.x0(), bounds.y0() + captionHeight, bounds.x1(), bounds.y1());
}


Rect2D GuiTheme::canvasToClientBounds(const Rect2D& bounds, float captionHeight) const {
    Rect2D r = canvasToClickBounds(bounds, captionHeight);

    return Rect2D::xyxy(r.x0y0() + m_canvas.pad.topLeft, r.x1y1() - m_canvas.pad.bottomRight);
}


Rect2D GuiTheme::dropDownListToClickBounds(const Rect2D& bounds, float captionWidth) const {
    // Note: if you change these bounds to not be the same as the
    // rendering bounds for the control itself then update
    // renderDropDownList to not call dropDownListToClickBounds.
    float h = m_dropDownList.base.left.height();
    return Rect2D::xywh(bounds.x0() + captionWidth,
                        bounds.center().y - h / 2,
                        bounds.width() - captionWidth,
                        h);
}


void GuiTheme::renderRadioButton(const Rect2D& bounds, bool enabled, bool focused, bool selected, const GuiText& text) const {
    drawCheckable(m_radioButton, bounds, enabled, focused, selected, text);
}


Vector2 GuiTheme::minButtonSize(const GuiText& text, ButtonStyle buttonStyle) const {
    Vector2 textBounds = bounds(text);

    Vector2 borderPadding = 
        m_button[buttonStyle].base.centerLeft.rect.wh() + 
        m_button[buttonStyle].base.centerRight.rect.wh();

    return textBounds + borderPadding;
}


void GuiTheme::renderButton(const Rect2D& bounds, bool enabled, bool focused, 
                           bool pushed, const GuiText& text, ButtonStyle buttonStyle) const {

    debugAssert(inRendering);
    if (buttonStyle != NO_BUTTON_STYLE) {
        m_button[buttonStyle].render(rd, bounds, enabled, focused, pushed);
    }

    const TextStyle& style = 
        enabled ? 
        m_button[buttonStyle].textStyle : 
        m_button[buttonStyle].disabledTextStyle;
    
    addDelayedText(text, style,
                   bounds.center() + m_button[buttonStyle].textOffset,
                   GFont::XALIGN_CENTER);
}


void GuiTheme::renderHorizontalSlider
(const Rect2D& bounds, 
 float pos, 
 bool enabled, 
 bool focused, 
 const GuiText& text,
 float captionWidth) const {
    
    debugAssert(inRendering);
    m_hSlider.render
        (rd, 
         horizontalSliderToSliderBounds(bounds, captionWidth),
         pos, enabled, focused);

    const TextStyle& style = enabled ? m_hSlider.textStyle : m_hSlider.disabledTextStyle;

    addDelayedText(text, style,
                   Vector2(bounds.x0(), (bounds.y0() + bounds.y1()) * 0.5f), 
                   GFont::XALIGN_LEFT);
}


void GuiTheme::renderLabel(const Rect2D& bounds, const GuiText& text, GFont::XAlign xalign, GFont::YAlign yalign, bool enabled) const {
    debugAssert(inRendering);

    if (text.numElements() > 0) {
        Vector2 pos;

        switch (xalign) {
        case GFont::XALIGN_LEFT:
            pos.x = bounds.x0();
            break;
        case GFont::XALIGN_CENTER:
            pos.x = bounds.center().x;
            break;
        case GFont::XALIGN_RIGHT:
            pos.x = bounds.x1();
            break;
        }

        switch (yalign) {
        case GFont::YALIGN_TOP:
            pos.y = bounds.y0();
            break;
        case GFont::YALIGN_CENTER:
            pos.y = bounds.center().y;
            break;
        case GFont::YALIGN_BOTTOM:
        case GFont::YALIGN_BASELINE:
            pos.y = bounds.y1();
            break;
        }

        const TextStyle& style = enabled ? m_textStyle : m_disabledTextStyle;

        addDelayedText(text, style, pos, xalign, yalign);
    }
}


Rect2D GuiTheme::readRect2D(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "(");
    float x = b.readNumber();
    b.readSymbol(",");
    float y = b.readNumber();
    b.readSymbol(",");
    float w = b.readNumber();
    b.readSymbol(",");
    float h = b.readNumber();
    b.readSymbol(")");

    return Rect2D::xywh(x, y, w, h);
}


Vector2 GuiTheme::readVector2(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "(");
    float x = b.readNumber();
    b.readSymbol(",");
    float y = b.readNumber();
    b.readSymbol(")");

    return Vector2(x, y);
}


void GuiTheme::drawDelayedText() const {
    if (delayedTextCount == 0) {
        return;
    }

    // Only clean out old fonts periodically to avoid frequent
    // memory allocation costs.
    bool cleanOldFonts = iRandom(0, 10000) == 0;
    
    beginText();
    {
        static Array<GFont::Ref> delayedFont;
        delayedText.getKeys(delayedFont);
        
        for (int f = 0; f < delayedFont.size(); ++f) {
            const GFont::Ref& thisFont = delayedFont[f];
            const Array<Text>& label = delayedText[thisFont];
            
            if (label.size() > 0) {
                // Load this font

                thisFont->begin2DQuads(rd);

                glMatrixMode(GL_TEXTURE);
                glLoadMatrix(thisFont->textureMatrix());

                // Render the text in this font
                for (int t = 0; t < label.size(); ++t) {
                    const Text& text = label[t];
                    thisFont->send2DQuads(rd, text.text, text.position, text.size, text.color, 
                                          text.outlineColor, text.xAlign, text.yAlign);
                }
                thisFont->end2DQuads(rd);

                // Fast clear to avoid memory allocation and deallocation
                const_cast<Array<Text>&>(label).fastClear();
                
            } else if (cleanOldFonts) {
                // Old font that is no longer in use.  Remove the reference in case the font itself
                // needs to be garbage collected
                const_cast<GuiTheme*>(this)->delayedText.remove(thisFont);
            }
        }
    }
    endText();

    // Reset the count
    const_cast<GuiTheme*>(this)->delayedTextCount = 0;
}

    
void GuiTheme::addDelayedText
(GFont::Ref         font,
 const std::string& label, 
 const Vector2&     position,
 float              size, 
 const Color4&      color, 
 const Color4&      outlineColor,
 GFont::XAlign      xalign,
 GFont::YAlign      yalign) const {

    if (font.isNull()) {
        font = m_textStyle.font;
        debugAssertM(font.notNull(), "Must set default font first.");
    }
    
    if (size < 0) {
        size = m_textStyle.size;
    }

    GuiTheme* me = const_cast<GuiTheme*>(this);
    
    ++(me->delayedTextCount);

    if (! delayedText.containsKey(font)) {
        me->delayedText.set(font, Array<Text>());
    }
    
    Text& text = me->delayedText[font].next();
    text.text       = label;
    text.position   = position;
    text.xAlign     = xalign;
    text.yAlign     = yalign;
    text.size       = size;

    if (color.a < 0) {
        text.color = m_textStyle.color;
    } else {
        text.color = color;
    }

    if (outlineColor.a < 0) {
        text.outlineColor = m_textStyle.outlineColor;
    } else {
        text.outlineColor = outlineColor;
    }
}


void GuiTheme::drawRect(const Rect2D& vertex, const Rect2D& texCoord, RenderDevice* rd) {
    (void)rd;
    for (int i = 0; i < 4; ++i) {
        glTexCoord(texCoord.corner(i));
        glVertex(vertex.corner(i));
    }
}


GuiTheme::StretchMode GuiTheme::readStretchMode(TextInput& t) {
    std::string modeStr = t.readSymbol();
    if (modeStr == "STRETCH") {
        return STRETCH;
    } else if (modeStr == "TILE") {
        return TILE;
    } else {
        debugAssertM(false, "Illegal stretch mode: " + modeStr);
    }
    return STRETCH;
}

float GuiTheme::paneTopPadding(const GuiText& caption, PaneStyle paneStyle) const {
    if (caption.empty()) {
        return 0.0f;
    } else if (caption.isIcon()) {
        return caption.height();
    } else {
        // Space for text
        if (m_pane[paneStyle].textStyle.size >= 0) {
            return m_pane[paneStyle].textStyle.size;
        } else {
            return m_textStyle.size;
        }
    }
}

Rect2D GuiTheme::paneToClientBounds(const Rect2D& bounds, const GuiText& caption, PaneStyle paneStyle) const {
    const Vector2 captionSpace(0, paneTopPadding(caption, paneStyle));
    return Rect2D::xywh(bounds.x0y0() + m_pane[paneStyle].clientPad.topLeft + captionSpace,
                        bounds.wh() - m_pane[paneStyle].clientPad.wh() - captionSpace);
}

Rect2D GuiTheme::clientToPaneBounds(const Rect2D& bounds, const GuiText& caption, PaneStyle paneStyle) const {
    const Vector2 captionSpace(0, paneTopPadding(caption, paneStyle));
    return Rect2D::xywh(bounds.x0y0() - m_pane[paneStyle].clientPad.topLeft - captionSpace,
                        bounds.wh() + m_pane[paneStyle].clientPad.wh() + captionSpace);
}


GuiTheme::GuiTheme() {}

void GuiTheme::makeThemeFromSourceFiles
(
 const std::string& sourceDir,
 const std::string& whiteName,
 const std::string& blackName,
 const std::string& coordsFile,
 const std::string& destFile) {

    Image3Ref white = Image3::fromFile(sourceDir + whiteName);
    Image3Ref black = Image3::fromFile(sourceDir + blackName);
    GImage out(white->width(), white->height(), 4);

    for (int y = 0; y < (int)out.height(); ++y) {
        for (int x = 0; x < (int)out.width(); ++x) {
            const Color3& U = white->get(x, y);
            const Color3& V = black->get(x, y);

            // U = F * a + (1-a) * 1
            // V = F * a + (1-a) * 0
            //
            // F * a = V
            // a = 1 - (U - V)
            
            Color3 diff = U - V;
            float a = clamp(1.0f - diff.average(), 0.0f, 1.0f);

            Color3 base = V;
            if (a > 0) {
                base = base / a;
            }

            out.pixel4(x, y) = Color4uint8(Color4(base, a));
        }
    }

    std::string coords = readWholeFile(sourceDir + coordsFile);

    // Test the text formatting
    {
        GuiTheme skin;
        TextInput t(TextInput::FROM_STRING, coords);
        std::string s;
        skin.deserialize(s, t);
    }
    
    BinaryOutput b(destFile, G3D_LITTLE_ENDIAN);
    
    b.writeString32("G3D Skin File");
    b.writeFloat32(0.1f);
    b.writeString32(coords);
    out.encode(GImage::TGA, b);

    b.compress();
    b.commit();
}

void GuiTheme::pushClientRect(const Rect2D& r) {
    debugAssert(inRendering);

    // Must draw old text since we don't keep track of which text 
    // goes with which client rect.
    drawDelayedText();
    rd->endPrimitive();

    const CoordinateFrame& oldMatrix = rd->objectToWorldMatrix();
    coordinateFrameStack.append(oldMatrix);
    const Rect2D& oldRect = rd->clip2D();
    scissorStack.append(oldRect);

    Rect2D newRect = r + oldMatrix.translation.xy();
    newRect = oldRect.intersect(newRect);
    rd->setClip2D(newRect);

    const CoordinateFrame& newMatrix = oldMatrix * CoordinateFrame(Vector3(r.x0y0(), 0));
    rd->setObjectToWorldMatrix(newMatrix);
    rd->beginPrimitive(PrimitiveType::QUADS);
}


void GuiTheme::popClientRect() {
    debugAssertM( coordinateFrameStack.size() > 0, 
        "popClientRect without matching pushClientRect");

    drawDelayedText();
    rd->endPrimitive();

    rd->setObjectToWorldMatrix(coordinateFrameStack.pop());
    rd->setClip2D(scissorStack.pop());
    rd->beginPrimitive(PrimitiveType::QUADS);
}

/////////////////////////////////////////////

void GuiTheme::Pane::deserialize(const std::string& name, const std::string& path, TextInput& t) {
    t.readSymbols(name, "=", "{");
    textStyle.deserialize(path, "font", t);
    disabledTextStyle.deserialize(path, "disabledFont", t);

    frame.deserialize("frame", t);
    clientPad.deserialize("clientPad", t);
    t.readSymbol("}");
}

///////////////////////////////////////////////

void GuiTheme::HSlider::deserialize(const std::string& name, const std::string& path, TextInput& t) {
    t.readSymbols(name, "=", "{");
    textStyle.deserialize(path, "font", t);
    disabledTextStyle.deserialize(path, "disabledFont", t);

    bar.deserialize("bar", t);
    thumb.deserialize("thumb", t);
    t.readSymbol("}");
}


void GuiTheme::HSlider::Bar::deserialize(const std::string& name, TextInput& t) {
    t.readSymbols(name, "=", "{");
    base.deserialize("base", t);
    enabled = readVector2("enabled", t);
    disabled = readVector2("disabled", t);
    t.readSymbol("}");
}


void GuiTheme::HSlider::Thumb::deserialize(const std::string& name, TextInput& t) {
    t.readSymbols(name, "=", "{");
    base = readRect2D("base", t);
    enabled.deserialize("enabled", t);
    disabled = readVector2("disabled", t);
    t.readSymbol("}");
}


void GuiTheme::HSlider::Thumb::Focus::deserialize(const std::string& name, TextInput& t) {
    t.readSymbols(name, "=", "{");
    focused = readVector2("focused", t);
    defocused = readVector2("defocused", t);
    t.readSymbol("}");
}


void GuiTheme::HSlider::render(RenderDevice* rd, const Rect2D& bounds, float thumbPos, 
                              bool _enabled, bool _focused) const {

    Rect2D barBounds = trackBounds(bounds);

    // Draw the bar:
    bar.base.render(rd, barBounds, _enabled ? bar.enabled : bar.disabled);

    // Draw the thumb:
    Vector2 offset = _enabled ? 
        (_focused ? thumb.enabled.focused : thumb.enabled.defocused) : 
        thumb.disabled;

    drawRect(thumbBounds(bounds, thumbPos), thumb.base + offset, rd);
}


Rect2D GuiTheme::HSlider::trackBounds(const Rect2D& sliderBounds) const {
    return
        Rect2D::xywh(sliderBounds.x0(), sliderBounds.center().y - bar.base.height() * 0.5, 
                 sliderBounds.width(), bar.base.height());
}


Rect2D GuiTheme::HSlider::thumbBounds(const Rect2D& sliderBounds, float pos) const {

    float halfWidth = thumb.base.width() * 0.5f;

    Vector2 thumbCenter(sliderBounds.x0() + halfWidth + (sliderBounds.width() - thumb.base.width()) * clamp(pos, 0.0f, 1.0f), 
                        sliderBounds.center().y);

    return Rect2D::xywh(thumbCenter - Vector2(halfWidth, thumb.base.height() * 0.5f), thumb.base.wh());
}
    
//////////////////////////////////////////////////////////////////////////////

void GuiTheme::WindowButton::deserialize(const std::string& name, TextInput& t) {
    t.readSymbols(name, "=", "{");
    base = readRect2D("base", t);
    
    t.readSymbols("focused", "=", "{");
    focusedDown = readVector2("down", t);
    focusedUp = readVector2("up", t);
    t.readSymbol("}");

    defocused = readVector2("defocused", t);
    windowDefocused = readVector2("windowDefocused", t);
    t.readSymbol("}");
}

//////////////////////////////////////////////////////////////////////////////

void GuiTheme::Window::deserialize(const std::string& name, const std::string& path, TextInput& b) {
    b.readSymbols(name, "=", "{");
    textStyle.deserialize(path, "font", b);

    defocusedTextStyle = textStyle;
    defocusedTextStyle.deserialize(path, "defocuseFont", b);

    base.deserialize("base", b);
    borderPad.deserialize("borderPad", b);
    borderThickness.deserialize("borderThickness", b);
    Pad clientPad;
    clientPad.deserialize("clientPad", b);
    netClientPad.topLeft = borderThickness.topLeft + clientPad.topLeft; 
    netClientPad.bottomRight = borderThickness.bottomRight + clientPad.bottomRight; 

    focused = readVector2("focused", b);
    defocused = readVector2("defocused", b);
    b.readSymbol("}");
}


void GuiTheme::Pad::deserialize(const std::string& name, TextInput& t) {
    t.readSymbols(name, "=", "{");
    topLeft = readVector2("topLeft", t);
    bottomRight = readVector2("bottomRight", t);
    t.readSymbol("}");
}


void GuiTheme::Window::render(RenderDevice* rd, const Rect2D& bounds, bool _focused) const {
    Vector2 offset = _focused ? focused : defocused;

    // Grow bounds to accomodate the true extent of the window
    base.render(rd,
                Rect2D::xywh(bounds.x0y0() - borderPad.topLeft, bounds.wh() + borderPad.wh()),
                offset);
}

//////////////////////////////////////////////////////////////////////////////

void GuiTheme::Checkable::deserialize(const std::string& name, const std::string& path, TextInput& b) {
    b.readSymbols(name, "=", "{");
    textStyle.deserialize(path, "font", b);
    disabledTextStyle.deserialize(path, "disabledFont", b);

    enabled.deserialize("enabled", b);
    disabled.deserialize("disabled", b);
    textOffset = readVector2("textOffset", b);
    b.readSymbol("}");
}


void GuiTheme::Checkable::Focus::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");
    focused.deserialize("focused", b);
    defocused.deserialize("defocused", b);
    b.readSymbol("}");
}


void GuiTheme::Checkable::Pair::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");
    checked   = readRect2D("checked", b);
    unchecked = readRect2D("unchecked", b);
    b.readSymbol("}");
}


void GuiTheme::Checkable::render(RenderDevice* rd, const Rect2D& bounds, bool _enabled, bool _focused, bool _checked) const {
    const Rect2D* r = NULL;

    if (_enabled) {
        if (_focused) {
            if (_checked) {
                r = &enabled.focused.checked;
            } else {
                r = &enabled.focused.unchecked;
            }
        } else {
            if (_checked) {
                r = &enabled.defocused.checked;
            } else {
                r = &enabled.defocused.unchecked;
            }
        }
    } else {
        if (_checked) {
            r = &disabled.checked;
        } else {
            r = &disabled.unchecked;
        }
    }

    Vector2 extent(r->width(), r->height());
    drawRect(Rect2D::xywh(bounds.x0y0() + Vector2(0, (bounds.height() - extent.y) / 2), extent), *r, rd);
}

//////////////////////////////////////////////////////////////////////////////

void GuiTheme::StretchRectHV::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");

    top.deserialize("top", b);

    centerLeft.deserialize("centerLeft", b);
    centerCenter.deserialize("centerCenter", b);
    centerRight.deserialize("centerRight", b);

    bottom.deserialize("bottom", b);

    b.readSymbol("}");
}


void GuiTheme::StretchRectHV::render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset) const {
    float topHeight    = top.left.height();
    float bottomHeight = bottom.left.height();
    float centerHeight = max(0.0f, bounds.height() - topHeight - bottomHeight);
    
    top.render(rd, Rect2D::xywh(bounds.x0y0(), 
                                Vector2(bounds.width(), topHeight)),
               texOffset);

    centerLeft.render(rd, Rect2D::xywh(bounds.x0y0() + Vector2(0, topHeight),
                                       Vector2(top.left.width(), centerHeight)),
                      texOffset);

    centerCenter.render(rd, Rect2D::xywh(bounds.x0y0() + Vector2(top.left.width(), topHeight),
                                       Vector2(max(0.0f, bounds.width() - (top.left.width() + top.right.width())), centerHeight)),
                        texOffset);

    centerRight.render(rd, Rect2D::xywh(bounds.x1y0() + Vector2(-top.right.width(), topHeight),
                                       Vector2(top.right.width(), centerHeight)),
                      texOffset);

    bottom.render(rd, Rect2D::xywh(bounds.x0y0() + Vector2(0, topHeight + centerHeight), 
                                   Vector2(bounds.width(), bottomHeight)),
                  texOffset);
}

//////////////////////////////////////////////////////////////////////////////

void GuiTheme::Fill::deserialize(const std::string& name, TextInput& b) {
    rect   = readRect2D(name, b);
    b.readSymbols(",", "(");
    
    horizontalMode = readStretchMode(b);
    b.readSymbol(",");
    verticalMode = readStretchMode(b);
    b.readSymbol(")");
}


void GuiTheme::Fill::render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset) const {
    if (horizontalMode == STRETCH) {
        if (verticalMode == STRETCH) {
            // Stretch, Stretch
            drawRect(bounds, rect + texOffset, rd);
        } else {
            // Stretch, Tile

            // Draw horizontal strips
            float height = rect.height();
            float x0 = bounds.x0();
            float y1 = bounds.y1();
            float y = bounds.y0();
            Rect2D strip = Rect2D::xywh(0, 0, bounds.width(), rect.height());
            while (y <= y1 - height) {
                drawRect(strip + Vector2(x0, y), rect + texOffset, rd);
                y += height;
            }

            if (y < y1) {
                // Draw the remaining fraction of a strip
                Rect2D src = Rect2D::xywh(rect.x0y0() + texOffset, Vector2(rect.width(), y1 - y));
                Rect2D dst = Rect2D::xywh(Vector2(x0, y), Vector2(bounds.width(), src.height()));
                drawRect(dst, src, rd);
            }
        }
    } else {
        if (verticalMode == STRETCH) {
            // Tile, Stretch
            // Draw vertical strips
            float width = rect.width();
            float y0 = bounds.y0();
            float x1 = bounds.x1();
            float x = bounds.x0();
            Rect2D strip = Rect2D::xywh(0, 0, rect.width(), bounds.height());
            while (x <= x1 - width) {
                drawRect(strip + Vector2(x, y0), rect + texOffset, rd);
                x += width;
            }

            if (x < x1) {
                // Draw the remaining fraction of a strip
                Rect2D src = Rect2D::xywh(rect.x0y0() + texOffset, Vector2(x1 - x, rect.height()));
                Rect2D dst = Rect2D::xywh(Vector2(x, y0), Vector2(src.width(), bounds.height()));
                drawRect(dst, src, rd);
            }

        } else {
            // Tile, Tile

            // Work in horizontal strips first

            float width = rect.width();
            float height = rect.height();
            float x0 = bounds.x0();
            float x1 = bounds.x1();
            float y1 = bounds.y1();
            float y = bounds.y0();

            Rect2D tile = Rect2D::xywh(Vector2(0, 0), rect.wh());

            while (y <= y1 - height) {
                float x = x0;
                while (x <= x1 - width) {
                    drawRect(tile + Vector2(x, y), rect + texOffset, rd);
                    x += width;
                }
                
                // Draw the remaining fraction of a tile
                if (x < x1) {
                    Rect2D src = Rect2D::xywh(rect.x0y0() + texOffset, Vector2(x1 - x, height));
                    Rect2D dst = Rect2D::xywh(Vector2(x, y), src.wh());
                    drawRect(dst, src, rd);
                }
                
                y += height;
            }
            
            if (y < y1) {
                float x = x0;
                
                float height = y1 - y;
                tile = Rect2D::xywh(0, 0, width, height);
                while (x <= x1 - width) {
                    drawRect(tile + Vector2(x, y), tile + (rect.x0y0() + texOffset), rd);
                    x += width;
                }
                
                // Draw the remaining fraction of a tile
                if (x < x1) {
                    Rect2D src = Rect2D::xywh(rect.x0y0() + texOffset, Vector2(x1 - x, height));
                    Rect2D dst = Rect2D::xywh(Vector2(x, y), src.wh());
                    drawRect(dst, src, rd);
                }
            }
        }
    }        
}

//////////////////////////////////////////////////////////////////////////////

void GuiTheme::StretchRectH::deserialize(const std::string& name, TextInput& b) {
    left   = readRect2D(name + "Left", b);
    center.deserialize(name + "Center", b);
    right = readRect2D(name + "Right", b);
}


void GuiTheme::StretchRectH::render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset) const {
    drawRect(Rect2D::xywh(bounds.x0y0(), left.wh()), left + texOffset, rd);
    center.render(rd, Rect2D::xywh(bounds.x0y0() + Vector2(left.width(), 0), 
                                   Vector2(bounds.width() - left.width() - right.width(), bounds.height())), texOffset);
    drawRect(Rect2D::xywh(bounds.x1y0() - Vector2(right.width(), 0), right.wh()), right + texOffset, rd);
}

//////////////////////////////////////////////////////////////////////////////

void GuiTheme::Button::deserialize(const std::string& name, const std::string& path, TextInput& b) {
    b.readSymbols(name, "=", "{");
    textStyle.deserialize(path, "font", b);
    disabledTextStyle.deserialize(path, "disabledFont", b);
    base.deserialize("base", b);
    textOffset = readVector2("textOffset", b);
    enabled.deserialize("enabled", b);
    disabled.deserialize("disabled", b);
    b.readSymbol("}");
}


void GuiTheme::Button::Focus::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");
    focused.deserialize("focused", b);
    defocused.deserialize("defocused", b);
    b.readSymbol("}");
}


void GuiTheme::Button::Pair::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");
    down = readVector2("down", b);
    up = readVector2("up", b);
    b.readSymbol("}");
}


void GuiTheme::Button::render(RenderDevice* rd, const Rect2D& bounds, bool _enabled, bool _focused, bool _checked) const {
    const Vector2* r = NULL;

    if (_enabled) {
        if (_focused) {
            if (_checked) {
                r = &enabled.focused.down;
            } else {
                r = &enabled.focused.up;
            }
        } else {
            if (_checked) {
                r = &enabled.defocused.down;
            } else {
                r = &enabled.defocused.up;
            }
        }
    } else {
        if (_checked) {
            r = &disabled.down;
        } else {
            r = &disabled.up;
        }
    }

    base.render(rd, bounds, *r);
}

////////////////////////////////////////////////////////////////////////////////////////

void GuiTheme::TextBox::deserialize(const std::string& name, const std::string& path, TextInput& b) {
    b.readSymbols(name, "=", "{");

    textStyle.deserialize(path, "font", b);
    disabledTextStyle.deserialize(path, "disabledFont", b);

    contentStyle = textStyle;
    contentStyle.deserialize(path, "contentFont", b);

    base.deserialize("base", b);
    textPad.deserialize("textPad", b);
    enabled.deserialize("enabled", b);
    disabled = readVector2("disabled", b);
    b.readSymbol("}");
}


void GuiTheme::TextBox::Focus::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");
    focused = readVector2("focused", b);
    defocused = readVector2("defocused", b);
    b.readSymbol("}");
}


void GuiTheme::TextBox::render(RenderDevice* rd, const Rect2D& bounds, bool _enabled, bool focused) const {
    const Vector2* r = NULL;

    if (_enabled) {
        if (focused) {
            r = &enabled.focused;
        } else {
            r = &enabled.defocused;
        }
    } else {
        r = &disabled;
    }

    base.render(rd, bounds, *r);
}
///////////////////////////////////////////////////////


void GuiTheme::Canvas::render(RenderDevice* rd, const Rect2D& bounds, bool _enabled, bool focused) const {
    const Vector2* r = NULL;

    if (_enabled) {
        if (focused) {
            r = &enabled.focused;
        } else {
            r = &enabled.defocused;
        }
    } else {
        r = &disabled;
    }

    base.render(rd, bounds, *r);
}

///////////////////////////////////////////////////////

void GuiTheme::DropDownList::render(RenderDevice* rd, const Rect2D& bounds, bool _enabled, bool _focused, bool _down) const {
    const Vector2* r = NULL;

    if (_enabled) {
        if (_focused) {
            if (_down) {
                r = &enabled.focused.down;
            } else {
                r = &enabled.focused.up;
            }
        } else {
            r = &enabled.defocused;
        }
    } else {
        r = &disabled;
    }
    
    base.render(rd, bounds, *r);
}


void GuiTheme::DropDownList::deserialize(const std::string& name, const std::string& path, TextInput& b) {
    b.readSymbols(name, "=", "{");

    textStyle.deserialize(path, "font", b);
    disabledTextStyle.deserialize(path, "disabledFont", b);

    base.deserialize("base", b);
    textPad.deserialize("textPad", b);

    enabled.deserialize("enabled", b);
    disabled = readVector2("disabled", b);
    b.readSymbol("}");
}

void GuiTheme::DropDownList::Focus::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");
    focused.deserialize("focused", b);
    defocused = readVector2("defocused", b);
    b.readSymbol("}");
}

void GuiTheme::DropDownList::Pair::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");
    down = readVector2("down", b);
    up = readVector2("up", b);
    b.readSymbol("}");
}


///////////////////////////////////////////////////////

void GuiTheme::TextStyle::deserialize(const std::string& path, const std::string& name, TextInput& t) {

    Token token = t.peek();

    if ((token.type() == Token::SYMBOL) &&
        (token.string() == name)) {
        // Font
        t.readSymbols(name, "=", "{");

        do {
            token = t.peek();

            alwaysAssertM(token.type() == Token::SYMBOL, 
                format("Unexpected token at line %d", token.line()));

            std::string s = token.string();

            if (s == "face") {
                t.readSymbols("face", "=");

                // Try to load the font
                std::string fontFilename = t.readString();

                if (FileSystem::exists(fontFilename)) {
                    font = GFont::fromFile(fontFilename);
                } else {
                    std::string x = System::findDataFile(fontFilename);
                    if (x != "") {
                        font = GFont::fromFile(x);
                    } else {
                        std::string x = System::findDataFile(path + filenameBaseExt(fontFilename));
                        if (x != "") {
                            font = GFont::fromFile(x);
                        } else {
                            logPrintf("GuiTheme Warning: could not find font %s\n", fontFilename.c_str());
                        }
                    }
                }

            } else if (s == "size") {

                t.readSymbols("size", "=");
                size = t.readNumber();
            } else if (s == "color") {
                color = readColor("color", t);
            } else if (s == "outlineColor") {
                outlineColor = readColor("outlineColor", t);
            } else if (s == "}") {
                t.readSymbol("}");
                return;
            } else {
                alwaysAssertM(false, format("Bad symbol: %s at line %d", s.c_str(), token.line()));
            }
        } while (true);
    }
}

} // namespace G3D
