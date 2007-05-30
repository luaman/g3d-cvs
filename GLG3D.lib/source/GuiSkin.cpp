/**
 @file GuiSkin.cpp
 @author Morgan McGuire, morgan@cs.williams.edu
 */

#include "G3D/platform.h"
#include "GLG3D/GuiSkin.h"
#include "G3D/WeakCache.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/TextInput.h"
#include "G3D/fileutils.h"
#include "G3D/Image3.h"

namespace G3D {

static void toGLMatrix(const Matrix4& m, float f[]) {
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            f[r * 4 + c] = m[r][c];
        }
    }
}


GuiSkin::GuiSkin(const std::string& filename) : delayedTextCount(0), inRendering(false){
    BinaryInput b(filename, G3D_LITTLE_ENDIAN, true);
    deserialize(filenamePath(filename), b);
}


GuiSkinRef GuiSkin::fromFile(const std::string& filename) {
    static WeakCache<std::string, GuiSkinRef> cache;
    
    GuiSkinRef instance = cache[filename];
    if (instance.isNull()) {
        instance = new GuiSkin(filename);
        cache.set(filename, instance);
    }

    return instance;
}


static Color4 readColor(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "(");
    float R,G,B,A;
    R = b.readNumber();
    b.readSymbol(",");
    G = b.readNumber();
    b.readSymbol(",");
    B = b.readNumber();
    b.readSymbol(",");
    A = b.readNumber();
    b.readSymbol(")");
    return Color4(R, G, B, A);
}


void GuiSkin::deserialize(const std::string& path, TextInput& b) {
    b.readSymbols("name", "=");
    std::string name = b.readString();

    b.readSymbols("format", "=");
    float version = b.readNumber();
    (void)version;
    debugAssertM(fuzzyEq(version, 0.1), format("Only version 0.1 is supported (version = %f)", version));

    // Font
    b.readSymbols("font", "=", "{");
    b.readSymbols("face", "=");
    std::string fontFilename = b.readString();
    if (fileExists(path + "/" + fontFilename)) {
        font = GFont::fromFile(path + "/" + fontFilename);
    }
    
    b.readSymbols("size", "=");
    fontSize = b.readNumber();

    fontColor = readColor("color", b);
    fontOutlineColor = readColor("outlineColor", b);

    b.readSymbol("}");
    
    // Controls
    m_checkBox.deserialize("checkBox", b);
    m_radioButton.deserialize("radioButton", b);
    m_button.deserialize("button", b);
    m_window.deserialize("window", b);
    m_toolWindow.deserialize("toolWindow", b);
    m_hSlider.deserialize("horizontalSlider", b);
}


void GuiSkin::deserialize(const std::string& path, BinaryInput& b) {
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

    texture = Texture::fromGImage(b.getFilename(), image, TextureFormat::RGBA8, Texture::DIM_2D, Texture::Settings::video());

    toGLMatrix(Matrix4(1.0f / texture->width(), 0, 0, 0,
            0, 1.0f / texture->height(), 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1), guiTextureMatrix);
}


void GuiSkin::beginRendering(RenderDevice* rd, const Vector2& offset) {
    debugAssert(! inRendering);
    inRendering = true;

    rd->push2D();

    rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
    rd->setCullFace(RenderDevice::CULL_NONE);
    rd->setAlphaTest(RenderDevice::ALPHA_GREATER, 0);
    rd->setColor(Color3::white());

    rd->setObjectToWorldMatrix(Vector3(offset, 0));

    rd->setTexture(TEXTURE_UNIT, texture);
    rd->setTextureMatrix(TEXTURE_UNIT, guiTextureMatrix);
    rd->beginPrimitive(RenderDevice::QUADS);
}


void GuiSkin::beginText(RenderDevice* rd) const {
    rd->endPrimitive();
}


void GuiSkin::endText(RenderDevice* rd) const {
    glBindTexture(GL_TEXTURE_2D, texture->openGLID());
    glMatrixMode(GL_TEXTURE);
    glLoadMatrix(guiTextureMatrix);

    glColor3f(1,1,1);
        
    rd->beginPrimitive(RenderDevice::QUADS);
}


void GuiSkin::endRendering(RenderDevice* rd) {
    // Draw any remaining text
    drawDelayedText(rd);

    debugAssert(inRendering);
    rd->endPrimitive();
    rd->pop2D();
    inRendering = false;
}


void GuiSkin::drawCheckable(const Checkable& control, RenderDevice* rd, const Rect2D& bounds, bool enabled, bool focused, bool selected, const GuiText& text) const {
    debugAssert(inRendering);
    control.render(rd, bounds, enabled, focused, selected);

    if (text.text != "") {
        addDelayedText(text.font, text.text, Vector2(control.width() + bounds.x0(), 
                                      (bounds.y0() + bounds.y1()) / 2) + control.textOffset,
                       text.size, text.color, text.outlineColor, GFont::XALIGN_LEFT);
    }
}


void GuiSkin::renderCheckBox(class RenderDevice* rd, const Rect2D& bounds, bool enabled, bool focused, bool selected, const GuiText& text) const {
    drawCheckable(m_checkBox, rd, bounds, enabled, focused, selected, text);
}


void GuiSkin::renderWindow(RenderDevice* rd, const Rect2D& bounds, bool focused, const GuiText& text) const {
    drawWindow(m_window, rd, bounds, focused, text);
}


void GuiSkin::renderToolWindow(RenderDevice* rd, const Rect2D& bounds, bool focused, const GuiText& text) const {
    drawWindow(m_toolWindow, rd, bounds, focused, text);
}


void GuiSkin::drawWindow(const Window& window, RenderDevice* rd, const Rect2D& bounds, 
                         bool focused, const GuiText& text) const {
    // Update any pending text since the window may overlap another window
    drawDelayedText(rd);

    window.render(rd, bounds, focused);
    
    if (text.text != "") {
        addDelayedText(text.font, text.text, Vector2(bounds.center().x, bounds.y0() + window.clientPad.topLeft.y * 0.5), 
                       min(text.size, window.clientPad.topLeft.y - 2), text.color, text.outlineColor, GFont::XALIGN_CENTER);
    }
}


Rect2D GuiSkin::horizontalSliderToSliderBounds(const Rect2D& bounds) const {
    return Rect2D::xywh(bounds.x1() - SLIDER_WIDTH, bounds.y0(), SLIDER_WIDTH, bounds.height());
}


Rect2D GuiSkin::horizontalSliderToThumbBounds(const Rect2D& bounds, float pos) const {
    return m_hSlider.thumbBounds(horizontalSliderToSliderBounds(bounds), pos);
}


Rect2D GuiSkin::horizontalSliderToTrackBounds(const Rect2D& bounds) const {
    return m_hSlider.trackBounds(horizontalSliderToSliderBounds(bounds));

}


Rect2D GuiSkin::windowToTitleBounds(const Rect2D& bounds) const {
    return Rect2D::xywh(bounds.x0y0(), Vector2(bounds.width(), m_window.clientPad.topLeft.y));
}


Rect2D GuiSkin::windowToClientBounds(const Rect2D& bounds) const {
    return Rect2D::xywh(bounds.x0y0() + m_window.clientPad.topLeft, bounds.wh() - m_window.clientPad.wh());
}


Rect2D GuiSkin::clientToWindowBounds(const Rect2D& bounds) const {
    return Rect2D::xywh(bounds.x0y0() - m_window.clientPad.topLeft, bounds.wh() + m_window.clientPad.wh());
}

Rect2D GuiSkin::toolWindowToClientBounds(const Rect2D& bounds) const {
    return Rect2D::xywh(bounds.x0y0() + m_toolWindow.clientPad.topLeft, bounds.wh() - m_toolWindow.clientPad.wh());
}


Rect2D GuiSkin::clientToToolWindowBounds(const Rect2D& bounds) const {
    return Rect2D::xywh(bounds.x0y0() - m_toolWindow.clientPad.topLeft, bounds.wh() + m_toolWindow.clientPad.wh());
}


Rect2D GuiSkin::toolWindowToTitleBounds(const Rect2D& bounds) const {
    return Rect2D::xywh(bounds.x0y0(), Vector2(bounds.width(), m_toolWindow.clientPad.topLeft.y));
}


void GuiSkin::renderRadioButton(RenderDevice* rd, const Rect2D& bounds, bool enabled, bool focused, bool selected, const GuiText& text) const {
    drawCheckable(m_radioButton, rd, bounds, enabled, focused, selected, text);
}


void GuiSkin::renderButton(RenderDevice* rd, const Rect2D& bounds, bool enabled, bool focused, 
                           bool pushed, const GuiText& text) const {
    debugAssert(inRendering);
    m_button.render(rd, bounds, enabled, focused, pushed);

    if (text.text != "") {
        addDelayedText(text.font, text.text, bounds.center() + m_button.textOffset, text.size, text.color, fontOutlineColor, GFont::XALIGN_CENTER);
    }
}


void GuiSkin::renderHorizontalSlider(RenderDevice* rd, const Rect2D& bounds, float pos, bool enabled, 
                           bool focused, const GuiText& text) const {
    debugAssert(inRendering);
    m_hSlider.render
        (rd, 
         horizontalSliderToSliderBounds(bounds),
         pos, enabled, focused);

    if (text.text != "") {
        addDelayedText(text.font, text.text, 
                       Vector2(bounds.x0(), (bounds.y0() + bounds.y1()) * 0.5f), text.size, 
                       text.color, text.outlineColor, GFont::XALIGN_LEFT);
    }
}


void GuiSkin::renderLabel(class RenderDevice* rd, const Rect2D& bounds, const GuiText& text, GFont::XAlign xalign, GFont::YAlign yalign) const {
    debugAssert(inRendering);

    if (text.text != "") {
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
        addDelayedText(text.font, text.text, pos, text.size, text.color, text.outlineColor, xalign, yalign);
    }
}

Rect2D GuiSkin::readRect2D(const std::string& name, TextInput& b) {
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


Vector2 GuiSkin::readVector2(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "(");
    float x = b.readNumber();
    b.readSymbol(",");
    float y = b.readNumber();
    b.readSymbol(")");

    return Vector2(x, y);
}


void GuiSkin::drawDelayedText(RenderDevice* rd) const {
    if (delayedTextCount == 0) {
        return;
    }

    // Only clean out old fonts periodically to avoid frequent
    // memory allocation costs.
    bool cleanOldFonts = iRandom(0, 100) == 0;
    
    beginText(rd);
    {
        static Array<GFontRef> delayedFont;
        delayedText.getKeys(delayedFont);
        
        for (int f = 0; f < delayedFont.size(); ++f) {
            const GFontRef& thisFont = delayedFont[f];
            const Array<Text>& label = delayedText[thisFont];
            
            if (label.size() > 0) {
                // Load this font
                glBindTexture(GL_TEXTURE_2D, thisFont->texture()->openGLID());
                glMatrixMode(GL_TEXTURE);
                glLoadMatrix(thisFont->textureMatrix());

                // Render the text in this font
                for (int t = 0; t < label.size(); ++t) {
                    const Text& text = label[t];
                    thisFont->send2DQuads(rd, text.text, text.position, text.size, text.color, 
                                          text.outlineColor, text.xAlign, text.yAlign);
                }
                
                // Fast clear to avoid memory allocation and deallocation
                const_cast<Array<Text>&>(label).fastClear();
                
            } else if (cleanOldFonts) {
                // Old font that is no longer in use.  Remove the reference in case the font itself
                // needs to be garbage collected
                const_cast<GuiSkin*>(this)->delayedText.remove(thisFont);
            }
        }
    }
    endText(rd);

    // Reset the count
    const_cast<GuiSkin*>(this)->delayedTextCount = 0;
}

    
/** Set the values to be used for default GuiText parameters. */
void GuiSkin::setFont(const GFontRef& font, float size, const Color4& color, const Color4& outlineColor) {
    this->font = font;
    fontSize = size;
    fontColor = color;
    fontOutlineColor = outlineColor;
}


void GuiSkin::addDelayedText
(
 GFontRef font,
 const std::string& label, const Vector2& position, float size, 
 const Color4& color, const Color4& outlineColor,
 GFont::XAlign xalign, GFont::YAlign yalign) const {

    if (font.isNull()) {
        font = this->font;
        debugAssertM(font.notNull(), "Must set default font first.");
    }
    
    if (size < 0) {
        size = fontSize;
    }

    GuiSkin* me = const_cast<GuiSkin*>(this);
    
    ++(me->delayedTextCount);

    if (! delayedText.containsKey(font)) {
        me->delayedText.set(font, Array<Text>());
    }
    
    Text& text = me->delayedText[font].next();
    text.text = label;
    text.position = position;
    text.xAlign = xalign;
    text.yAlign = yalign;
    text.size = size;

    if (color.a < 0) {
        text.color = fontColor;
    } else {
        text.color = color;
    }

    if (outlineColor.a < 0) {
        text.outlineColor = fontOutlineColor;
    } else {
        text.outlineColor = outlineColor;
    }
}


void GuiSkin::drawRect(const Rect2D& vertex, const Rect2D& texCoord, RenderDevice* rd) {
    for (int i = 0; i < 4; ++i) {
        glTexCoord2fv(texCoord.corner(i));
        glVertex2fv(vertex.corner(i));
    }
}


GuiSkin::StretchMode GuiSkin::readStretchMode(TextInput& t) {
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


void GuiSkin::makeSkinFromSourceFiles
(
 const std::string& sourceDir,
 const std::string& whiteName,
 const std::string& blackName,
 const std::string& coordsFile,
 const std::string& destFile) {

    Image3Ref white = Image3::fromFile(sourceDir + whiteName);
    Image3Ref black = Image3::fromFile(sourceDir + blackName);
    GImage out(white->width(), white->height(), 4);

    for (int y = 0; y < (int)out.height; ++y) {
        for (int x = 0; x < (int)out.width; ++x) {
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
    //    out.save("/Volumes/McGuire/Projects/g3dui/matting/test.tga", GImage::TGA);
    std::string coords = readWholeFile(sourceDir + coordsFile);

    BinaryOutput b(destFile, G3D_LITTLE_ENDIAN);
    
    b.writeString32("G3D Skin File");
    b.writeFloat32(0.1f);
    b.writeString32(coords);
    out.encode(GImage::TGA, b);

    b.compress();
    b.commit();
}

//////////////////////////////////////////////////////////////////////////////

void GuiSkin::HSlider::deserialize(const std::string& name, TextInput& t) {
    t.readSymbols(name, "=", "{");
    bar.deserialize("bar", t);
    thumb.deserialize("thumb", t);
    t.readSymbol("}");
}


void GuiSkin::HSlider::Bar::deserialize(const std::string& name, TextInput& t) {
    t.readSymbols(name, "=", "{");
    base.deserialize("base", t);
    enabled = readVector2("enabled", t);
    disabled = readVector2("disabled", t);
    t.readSymbol("}");
}


void GuiSkin::HSlider::Thumb::deserialize(const std::string& name, TextInput& t) {
    t.readSymbols(name, "=", "{");
    base = readRect2D("base", t);
    enabled.deserialize("enabled", t);
    disabled = readVector2("disabled", t);
    t.readSymbol("}");
}


void GuiSkin::HSlider::Thumb::Focus::deserialize(const std::string& name, TextInput& t) {
    t.readSymbols(name, "=", "{");
    focused = readVector2("focused", t);
    defocused = readVector2("defocused", t);
    t.readSymbol("}");
}


void GuiSkin::HSlider::render(RenderDevice* rd, const Rect2D& bounds, float thumbPos, 
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


Rect2D GuiSkin::HSlider::trackBounds(const Rect2D& sliderBounds) const {
    return
        Rect2D::xywh(sliderBounds.x0(), sliderBounds.center().y - bar.base.height() * 0.5, 
                 sliderBounds.width(), bar.base.height());
}


Rect2D GuiSkin::HSlider::thumbBounds(const Rect2D& sliderBounds, float pos) const {

    float halfWidth = thumb.base.width() * 0.5f;

    Vector2 thumbCenter(sliderBounds.x0() + halfWidth + (sliderBounds.width() - thumb.base.width()) * clamp(pos, 0.0f, 1.0f), 
                        sliderBounds.center().y);

    return Rect2D::xywh(thumbCenter - Vector2(halfWidth, thumb.base.height() * 0.5f), thumb.base.wh());
}
    

//////////////////////////////////////////////////////////////////////////////

void GuiSkin::Window::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");
    base.deserialize("base", b);
    borderPad.deserialize("borderPad", b);
    clientPad.deserialize("clientPad", b);
    focused = readVector2("focused", b);
    defocused = readVector2("defocused", b);
    b.readSymbol("}");
}


void GuiSkin::Window::Pad::deserialize(const std::string& name, TextInput& t) {
    t.readSymbols(name, "=", "{");
    topLeft = readVector2("topLeft", t);
    bottomRight = readVector2("bottomRight", t);
    t.readSymbol("}");
}


void GuiSkin::Window::render(RenderDevice* rd, const Rect2D& bounds, bool _focused) const {
    Vector2 offset = _focused ? focused : defocused;

    // Grow bounds to accomodate the true extent of the window
    base.render(rd,
                Rect2D::xywh(bounds.x0y0() - borderPad.topLeft, bounds.wh() + borderPad.wh()),
                offset);
}

//////////////////////////////////////////////////////////////////////////////

void GuiSkin::Checkable::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");
    enabled.deserialize("enabled", b);
    disabled.deserialize("disabled", b);
    textOffset = readVector2("textOffset", b);
    b.readSymbol("}");
}

void GuiSkin::Checkable::Focus::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");
    focused.deserialize("focused", b);
    defocused.deserialize("defocused", b);
    b.readSymbol("}");
}

void GuiSkin::Checkable::Pair::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");
    checked   = readRect2D("checked", b);
    unchecked = readRect2D("unchecked", b);
    b.readSymbol("}");
}

void GuiSkin::Checkable::render(RenderDevice* rd, const Rect2D& bounds, bool _enabled, bool _focused, bool _checked) const {
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

void GuiSkin::StretchRectHV::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");

    top.deserialize("top", b);

    centerLeft.deserialize("centerLeft", b);
    centerCenter.deserialize("centerCenter", b);
    centerRight.deserialize("centerRight", b);

    bottom.deserialize("bottom", b);

    b.readSymbol("}");
}


void GuiSkin::StretchRectHV::render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset) const {
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
                                       Vector2(top.left.width(), centerHeight)),
                      texOffset);

    bottom.render(rd, Rect2D::xywh(bounds.x0y0() + Vector2(0, topHeight + centerHeight), 
                                   Vector2(bounds.width(), bottomHeight)),
                  texOffset);
}

//////////////////////////////////////////////////////////////////////////////

void GuiSkin::Fill::deserialize(const std::string& name, TextInput& b) {
    rect   = readRect2D(name, b);
    b.readSymbols(",", "(");
    
    horizontalMode = readStretchMode(b);
    b.readSymbol(",");
    verticalMode = readStretchMode(b);
    b.readSymbol(")");

}


void GuiSkin::Fill::render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset) const {
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

void GuiSkin::StretchRectH::deserialize(const std::string& name, TextInput& b) {
    left   = readRect2D(name + "Left", b);
    center.deserialize(name + "Center", b);
    right = readRect2D(name + "Right", b);
}

void GuiSkin::StretchRectH::render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset) const {
    drawRect(Rect2D::xywh(bounds.x0y0(), left.wh()), left + texOffset, rd);
    center.render(rd, Rect2D::xywh(bounds.x0y0() + Vector2(left.width(), 0), 
                                   Vector2(bounds.width() - left.width() - right.width(), bounds.height())), texOffset);
    drawRect(Rect2D::xywh(bounds.x1y0() - Vector2(right.width(), 0), right.wh()), right + texOffset, rd);
}

//////////////////////////////////////////////////////////////////////////////

void GuiSkin::Button::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");
    base.deserialize("base", b);
    textOffset = readVector2("textOffset", b);
    enabled.deserialize("enabled", b);
    disabled.deserialize("disabled", b);
    b.readSymbol("}");
}

void GuiSkin::Button::Focus::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");
    focused.deserialize("focused", b);
    defocused.deserialize("defocused", b);
    b.readSymbol("}");
}

void GuiSkin::Button::Pair::deserialize(const std::string& name, TextInput& b) {
    b.readSymbols(name, "=", "{");
    down = readVector2("down", b);
    up = readVector2("up", b);
    b.readSymbol("}");
}

void GuiSkin::Button::render(RenderDevice* rd, const Rect2D& bounds, bool _enabled, bool _focused, bool _checked) const {
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

GuiText::GuiText
(const std::string& text, 
 const GFontRef& font, 
 float size, 
 const Color4& color,
 const Color4& outlineColor) : text(text), font(font), size(size), color(color), outlineColor(outlineColor) {
    
}

GuiText::GuiText
(const char* text) : text(text), font(NULL), size(-1), color(-1,-1,-1,-1), outlineColor(-1,-1,-1,-1) {
    
}

/** Provides the value of default values; called by Gui to overwrite the illegal values.*/
void GuiText::setDefault(const GFontRef& dfont, float dsize, const Color4& dcolor, const Color4& doutline) {
    if (font.isNull()) {
        font = dfont;
    }

    if (size < 0) {
        size = dsize;
    }

    if (color.a < 0) {
        color = dcolor;
    }
    
    if (outlineColor.a < 0) {
        outlineColor = doutline;
    }
}


} // namespace G3D
