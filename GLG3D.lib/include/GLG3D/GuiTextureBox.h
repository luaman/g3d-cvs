/**
 @file GLG3D/GuiTextureBox.h

 @created 2009-09-11
 @edited  2009-09-11

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2009, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GuiTextureBox_h
#define G3D_GuiTextureBox_h

#include "G3D/platform.h"
#include "G3D/Vector2.h"
#include "GLG3D/GuiContainer.h"
#include "GLG3D/Texture.h"
#include "GLG3D/GuiText.h"

namespace G3D {

class GuiPane;
class GuiButton;

class GuiTextureBox : public GuiContainer {
public:
    /** Which channels to display. */
    enum Channels {
        /** RGB as a color*/
        RGB,

        /** Red only */
        R, 

        /** Green only */
        G, 

        /** Blue only */
        B, 

        /** Red as grayscale */
        RasL,

        /** Green as grayscale */
        GasL,

        /** Blue as grayscale */
        BasL,
        
        /** Alpha as grayscale */
        AasL, 

        /** Luminance: (R + G + B) / 3 */
        RGBasL};
    
    class Settings {
    public:
        Channels         channels;

        /** Texture's gamma. Texels will be converted to pixels by p = t^(g/2.2)*/
        float            documentGamma;

        /** Lowest expected value */
        float            min;

        /** Highest expected value */
        float            max;

        /** Defaults to linear data on [0, 1]: packed normal maps,
            reflectance maps, etc. */
        Settings(Channels c = RGB, float g = 1.0f, float mn = 0.0f, float mx = 1.0f);

        /** For photographs and other images with document gamma 2.2 */
        static const Settings& image();

        /** For signed unit vectors, like a GBuffer's normals, on the
            range [-1, 1] for RGB channels */
        static const Settings& unitVector();

        /** For bump map packed in an alpha channel. */
        static const Settings& bumpInAlpha();

        /** For a hyperbolic depth map in the red channel (e.g., a shadow map). */
        static const Settings& zBuffer();

        /** True if these settings require the use of a GLSL shader */
        bool needsShader() const;
    };

private:

    /** Padding around the image */
    enum {BORDER = 1};

    Texture::Ref             m_texture;

    Settings                 m_settings;

    /** Bounds for mouse clicks and scissor region, updated by every render. */
    Rect2D                   m_clipBounds;
    
    bool                     m_showInfo;

    /** Cached formatting of m_lastSize */
    mutable GuiText          m_lastSizeCaption;
    mutable Vector2int16     m_lastSize;                      

    GuiText                  m_drawerCollapseCaption;
    GuiText                  m_drawerExpandCaption;
    GuiButton*               m_drawerButton;
    GuiPane*                 m_drawerPane;
    bool                     m_drawerOpen;

    Shader::Ref              m_shader;

    float                    m_zoom;
    Vector2                  m_offset;

    /** True when dragging the image */
    bool                     m_dragging;
    Vector2                  m_dragStart;
    Vector2                  m_offsetAtDragStart;

    /** Readback texel */
    Color4                   m_texel;
    /** True if the mouse moved since m_texel was updated */
    bool                     m_needReadback;

    static WeakReferenceCountedPointer<Shader> g_cachedShader;

    /** Returns the bounds of the canvas (display) region */
    Rect2D canvasRect() const;

public:

    GuiTextureBox
    (GuiContainer*       parent,
     const GuiText&      caption,
     const Texture::Ref& t = NULL,
     const Settings&     s = Settings());

    virtual ~GuiTextureBox();

    /** Zoom factor for the texture display.  Greater than 1 = zoomed in. */
    inline float viewZoom() const {
        return m_zoom;
    }
    
    void setViewZoom(float z);

    /** Offset of the texture from the centered position. Positive = right and down. */
    inline const Vector2& viewOffset() const {
        return m_offset;
    }

    void zoomIn();
    void zoomOut();

    /** Brings up the modal save dialog */
    void save();

    void setViewOffset(const Vector2& x);

    /** Change the scale to 1:1 pixel */
    void zoomTo1();

    /** Center the image and scale it to fill the viewport */
    void zoomToFit();

    void setTexture(const Texture::Ref& t);
    void setSettings(const Settings& s);

    inline const Texture::Ref& texture() const {
        return m_texture;
    }

    inline const Settings& settings() const {
        return m_settings;
    }

    /** Sizes the control so that exactly \a dims of viewing space is available. 
        Useful for ensuring that textures are viewed at 1:1.*/
    void setSizeFromInterior(const Vector2& dims);

    virtual void render(RenderDevice* rd, const GuiTheme::Ref& theme) const;
    virtual void setRect(const Rect2D& rect);
    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) const;

    virtual bool onEvent(const GEvent& event);

    /** Invoked by the drawer button. Do not call directly. */
    void toggleDrawer();
};

} // namespace

#endif
