/**
 @file GuiCaption.cpp
 @author Morgan McGuire, morgan@cs.williams.edu
 */

#include "G3D/platform.h"
#include "GLG3D/GuiCaption.h"

namespace G3D {

GuiCaption::GuiCaption
(const std::string& text, 
 const GFontRef& font, 
 float size, 
 const Color4& color,
 const Color4& outlineColor) : m_text(text), m_font(font), m_size(size), m_color(color), m_outlineColor(outlineColor) {
    
}

GuiCaption::GuiCaption
(const char* text) : m_text(text), m_font(NULL), m_size(-1), m_color(-1,-1,-1,-1), m_outlineColor(-1,-1,-1,-1) {
    
}

/** Provides the value of default values; called by Gui to overwrite the illegal values.*/
void GuiCaption::setDefault(const GFontRef& dfont, float dsize, const Color4& dcolor, const Color4& doutline) {
    if (m_font.isNull()) {
        m_font = dfont;
    }

    if (m_size < 0) {
        m_size = dsize;
    }

    if (m_color.a < 0) {
        m_color = dcolor;
    }
    
    if (m_outlineColor.a < 0) {
        m_outlineColor = doutline;
    }
}

}
