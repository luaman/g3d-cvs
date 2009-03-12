/**
  @file GuiText.h
  @maintainer Morgan McGuire, morgan@cs.williams.edu
  Copyright 2001-2009, Morgan McGuire
 */

#ifndef G3D_GUICAPTION_H
#define G3D_GUICAPTION_H

#include "G3D/platform.h"
#include "G3D/Rect2D.h"
#include "G3D/Table.h"
#include "GLG3D/Texture.h"
#include "GLG3D/GFont.h"

template <> struct HashTrait<G3D::GFontRef> {
    static size_t hashCode(const G3D::GFontRef& key) { return reinterpret_cast<size_t>(key.pointer()); }
};

namespace G3D {


/** 
    Text on a GuiControl. These are normally created
    implicitly by a cast from std::string, but can be created
    explicitly when more information needs to be specified.

    @sa G3D::GuiLabel
 */
class GuiText {
private:

    std::string   m_text;
    GFont::Ref    m_font;
    float         m_size;
    Color4        m_color;
    Color4        m_outlineColor;

    TextureRef    m_texture;
    Rect2D        m_rect;
    
public:

    /**
       Negative alpha values on color and outlineColor mean "use
       default".  Null font and negative size mean "use default".
       Defaults are set on the Gui.
    */
    GuiText(const std::string& text   = "", 
          const GFont::Ref& font         = NULL, 
          float             size         = -1, 
          const Color4&     color        = Color4(-1,-1,-1,-1), 
          const Color4&     outlineColor = Color4(-1,-1,-1,-1));

    /** Uses all default values. */
    GuiText(const char* text);
    
    /** Cast to std::string */
    inline operator const std::string&() const { 
        return m_text; 
    }

    /** Provides the value of default values.*/
    void setDefault(const GFontRef& dfont, float dsize, const Color4& dcolor, const Color4& doutline);

    inline const std::string& text() const {
        return m_text;
    }

    /** If this caption specifies a font, returns that font, otherwise returns the default font */
    inline const GFont::Ref& font(const GFont::Ref& _default) const {
        if (m_font.isNull()) {
            return _default;
        } else {
            return m_font;
        }
    }

    inline const Color4& color(const Color4& _default) const {
        if (m_color.a < 0) {
            return _default;
        } else {
            return m_color;
        }
    }

    inline const Color4& outlineColor(const Color4& _default) const {
        if (m_outlineColor.a < 0) {
            return _default;
        } else {
            return m_outlineColor;
        }
    }

    inline float size(float _default) const {
        if (m_size < 0) {
            return _default;
        } else {
            return m_size;
        }
    }

    /** Loads standard symbols from the greek.fnt and icon.fnt fonts.*/
    class Symbol {
    private:
        static GuiText greek(int asciiCode);
    public:
        static GuiText record();
        static GuiText play();
        static GuiText stop();
        static GuiText pause();
        static GuiText eye();
        static GuiText forward();
        static GuiText reverse();
        static GuiText previous();
        static GuiText next();

        /** >= */
        static GuiText leq();

        /** <= */
        static GuiText geq();

        /** +/- */
        static GuiText pm();

        /** Wiggly equals sign */
        static GuiText approx();

        /** Partial derivative symbol */
        static GuiText partial();

        /** Red heart */
        static GuiText heartSuit();

        static GuiText clubSuit();
        static GuiText spadeSuit();

        /** Red diamond */
        static GuiText diamondSuit();

    };

};

} // G3D
#endif
