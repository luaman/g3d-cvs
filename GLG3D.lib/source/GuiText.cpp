/**
 @file GuiText.cpp
 @author Morgan McGuire, morgan@cs.williams.edu
 */

#include "G3D/platform.h"
#include "GLG3D/GuiText.h"

namespace G3D {

GuiText::GuiText
(const std::string& text, 
 const GFontRef& font, 
 float size, 
 const Color4& color,
 const Color4& outlineColor) : m_text(text), m_font(font), m_size(size), m_color(color), m_outlineColor(outlineColor) {
    
}

GuiText::GuiText
(const char* text) : m_text(text), m_font(NULL), m_size(-1), m_color(-1,-1,-1,-1), m_outlineColor(-1,-1,-1,-1) {
    
}

/** Provides the value of default values; called by Gui to overwrite the illegal values.*/
void GuiText::setDefault(const GFontRef& dfont, float dsize, const Color4& dcolor, const Color4& doutline) {
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


GuiText GuiText::Symbol::record() {
    return GuiText("=", GFont::fromFile(System::findDataFile("icon.fnt")), 16, Color3::red() * 0.5f);
}


GuiText GuiText::Symbol::play() {
    return GuiText("4", GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::eye() {
    return GuiText("N", GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::pause() {
    return GuiText(";", GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::stop() {
    return GuiText("<", GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::forward() {
    return GuiText(std::string() + (char)0x38, GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::reverse() {
    return GuiText(std::string() + (char)0x37, GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::previous() {
    return GuiText("1", GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::next() {
    return GuiText("2", GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::greek(int code) {
    return GuiText(std::string() + (char)code, GFont::fromFile(System::findDataFile("symbol.fnt")));
}


GuiText GuiText::Symbol::leq() {
    return greek(0xA3);
}


GuiText GuiText::Symbol::geq() {
    return greek(0xB3);
}


GuiText GuiText::Symbol::pm() {
    return greek(0xB1);
}


GuiText GuiText::Symbol::heartSuit() {
    return GuiText(std::string() + char(0xA9), GFont::fromFile(System::findDataFile("symbol.fnt")), -1, Color3::red() * 0.6f);
}


GuiText GuiText::Symbol::diamondSuit() {
    return GuiText(std::string() + char(0xA8), GFont::fromFile(System::findDataFile("symbol.fnt")), -1, Color3::red() * 0.6f);
}


GuiText GuiText::Symbol::clubSuit() {
    return greek(0xA7);
}


GuiText GuiText::Symbol::spadeSuit() {
    return greek(0xAA);
}


GuiText GuiText::Symbol::approx() {
    return greek(0xBB);
}


GuiText GuiText::Symbol::partial() {
    return greek(0xB6);
}

}
