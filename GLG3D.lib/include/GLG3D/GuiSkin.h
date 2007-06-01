/**
  @file GuiSkin.h
  @maintainer Morgan McGuire, morgan@cs.williams.edu
  Copyright 2001-2007, Morgan McGuire
 */

#ifndef G3D_GUISKIN_H
#define G3D_GUISKIN_H

#include "G3D/platform.h"
#include "G3D/Rect2D.h"
#include "GLG3D/Texture.h"
#include "GLG3D/GFont.h"
#include "G3D/Table.h"

inline unsigned int hashCode(const G3D::GFontRef& font) {
    return hashCode(font.pointer());
}


namespace G3D {

/** 
    Text label on a control. These are normally created implicitly by a cast from std::string,
    but can be created explicitly when more information needs to be specified.
 */
class GuiText {
public:
    std::string text;
    GFontRef    font;
    float       size;
    Color4      color;
    Color4      outlineColor;
    
    /**
       Negative alpha values on color and outlineColor mean "use default".  Null font and negative size mean "use default".
       Defaults are set on the Gui.
    */
    GuiText(const std::string& text ="", 
          const GFontRef& font = NULL, 
          float size = -1, 
          const Color4& color = Color4(-1,-1,-1,-1), 
          const Color4& outlineColor = Color4(-1,-1,-1,-1));

    GuiText(const char* text);
    
    /** Provides the value of default values; called by G3D::Gui to overwrite the illegal values.*/
    void setDefault(const GFontRef& dfont, float dsize, const Color4& dcolor, const Color4& doutline);
};
    

typedef ReferenceCountedPointer<class GuiSkin> GuiSkinRef;

/**
   Renders UI elements.
   Loads and renders G3D GUI .skn files, which can be used with G3D::Gui.  .skn files
   can be found in the data module data/gui directory.

   Can also be used to explicitly render a UI without actual controls behind it, for example.

  <pre>
   skin->beginRendering(rd);
       skin->renderWindow(rd, Rect2D::xywh(80, 70, 550, 250), false, "Window");
       skin->renderToolWindow(rd, Rect2D::xywh(500, 120, 50, 150), true, "Tools");
       skin->renderCheckBox(rd, Rect2D::xywh(100, 100, 20, 20), true, true, true, "Check box");
       skin->renderRadioButton(rd, Rect2D::xywh(100, 120, 20, 20), true, false, false, "Radio button");
       skin->renderButton(rd, Rect2D::xywh(100, 160, 80, 27), true, true, true, "Button");
       skin->setFont(timesFont, 12, Color3::black(), Color4::clear());
       skin->renderButton(rd, Rect2D::xywh(200, 160, 80, 27), true, true, true, "New Font");
       skin->setFont(arialFont, 10, Color3::black(), Color4::clear());
       skin->renderHorizontalSlider(rd, Rect2D::xywh(100, 200, 150, 27), 0.5f, true, false, "Slider");

       skin->setFont(iconFont, 18, Color3::black(), Color4::clear());
       int s = 30;
       skin->renderButton(rd, Rect2D::xywh(100 + s * 0, 230, 30, 30), true, false, false, "7");
       skin->renderButton(rd, Rect2D::xywh(100 + s * 1, 230, 30, 30), true, false, false, "4");
       skin->setFont(iconFont, 18, Color3::red(), Color4::clear());
       skin->renderButton(rd, Rect2D::xywh(100 + s * 2, 230, 30, 30), true, false, false, "=");
       skin->setFont(iconFont, 18, Color3::black(), Color4::clear());
       skin->renderButton(rd, Rect2D::xywh(100 + s * 3, 230, 30, 30), true, false, true, ";");
       skin->renderButton(rd, Rect2D::xywh(100 + s * 4, 230, 30, 30), true, false, false, "<");
       skin->renderButton(rd, Rect2D::xywh(100 + s * 5, 230, 30, 30), true, false, false, "8");
       skin->setFont(arialFont, 10, Color3::black(), Color4::clear());
    skin->endRendering(rd);
   </pre>
*/
class GuiSkin : public ReferenceCountedObject {
private:
    enum {SLIDER_WIDTH = 100};
    
    /** Used for delayed text rendering. */
    class Text {
    public:
        Vector2       position;
        std::string   text;
        GFont::XAlign xAlign;
        GFont::YAlign yAlign;
        float         size;
        Color4        color;
        Color4        outlineColor;
    };

    /** Delayed text, organized by the associated font.*/
    Table<GFontRef, Array<Text> >       delayedText;

    /** Number of values in delayedText's arrays.  Used to 
        detect when we need to enter font rendering mode. */
    int                                 delayedTextCount;


    /** Clears the delayedText array. */
    void drawDelayedText() const;

    /** Postpones rendering the specified text until later. Switching
        between the GUI texture and the font texture is relatively
        slow, so delaying text rendering until a few controls have
        been put down increases performance.

        Note that delayed text must be drawn before the clipping region is changed or another window is rendered.
    */
    void addDelayedText(GFontRef font, const std::string& text, const Vector2& position, float size, 
                        const Color4& color, const Color4& outlineColor, GFont::XAlign xalign, 
                        GFont::YAlign yalign = GFont::YALIGN_CENTER) const;

    enum {TEXTURE_UNIT = 0};

    enum StretchMode {TILE, STRETCH};

    // All internal coordinates are stored in texture coordinates

    /** */
    class Fill {
    public:
        StretchMode     horizontalMode;
        StretchMode     verticalMode;

        /** Source rectangle.  This does not have to correspond to the 
            dimensions of the destination rectangle at all.  When 
            rendered, mode will be used to fill the destination rectangle
            up to the correct dimensions if source is too small.  If too source
            is too large it is cropped on the bottom and right.
         */
        Rect2D          rect;

        void deserialize(const std::string& name, TextInput& b);
        void render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset) const;        
    };

    /** 
        Horizontal stretch--two caps and a center fill
     */
    class StretchRectH {
    public:
        Rect2D         left;
        Fill           center;
        Rect2D         right;

        /** Name is prepended onto Left, Center, Right */
        void deserialize(const std::string& name, TextInput& b);

        /** Bounds height must match left.height and right.height */
        void render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset) const;
        
        inline float height() const {
            return left.height();
        }
    };

    /** 
        Vertical stretch--two caps and a center fill
     */
    class StretchRectV {
    public:
        Rect2D         top;
        Rect2D         bottom;
        Fill           center;
        void deserialize(TextInput& b);
        void render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset) const;
    };
    
    /** Stretchable in horizontal and vertical direction. */
    class StretchRectHV {
    public:
        StretchRectH       top;
        Fill               centerLeft;
        Fill               centerCenter;
        Fill               centerRight;
        StretchRectH       bottom;

        void deserialize(const std::string& name, TextInput& b);
        void render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset) const;
    };

    class Button {
    public:
        /**
           General texture coordinates for any mode of the button
         */
        StretchRectHV        base;

        /**
           Displacement from bounds center.
         */
        Vector2              textOffset;

        class Pair {
        public:
            /** TexOffset from base of this image */
            Vector2          down;

            /** TexOffset from base of this image */
            Vector2          up;
            void deserialize(const std::string& name, TextInput& b);
        };

        class Focus {
        public:
            Pair      focused;
            Pair      defocused;
            void deserialize(const std::string& name, TextInput& b);
        };
    
        Focus         enabled;
        Pair          disabled;

        void deserialize(const std::string& name, TextInput& b);
        void render(RenderDevice* rd, const Rect2D& bounds, bool enabled, bool focused, bool pushed) const;
    };

    /** Used for radio and check boxes */
    class Checkable {
    public:
        class Pair {
        public:
            Rect2D    checked;
            Rect2D    unchecked;
            void deserialize(const std::string& name, TextInput& b);
        };

        class Focus {
        public:
            Pair      focused;
            Pair      defocused;
            void deserialize(const std::string& name, TextInput& b);
        };
    
        Focus         enabled;
        Pair          disabled;

        Vector2       textOffset;

        void deserialize(const std::string& name, TextInput& b);
        void render(RenderDevice* rd, const Rect2D& bounds, bool enabled, bool focused, bool checked) const;
        
        inline float width() const {
            return disabled.checked.width();
        }

        inline float height() const {
            return disabled.checked.height();
        }
    };

    class Pad {
    public:
        Vector2      topLeft;
        Vector2      bottomRight;
        
        void deserialize(const std::string& name, TextInput& b);
        
        /** Net width and height of the padding */
        inline Vector2 wh() const {
            return topLeft + bottomRight;
        }
    };

    class Window {
    public:

        StretchRectHV    base;

        /** Distance from edge to border */
        Pad              borderPad;

        /** Distance from border to client area */
        Pad              clientPad;

        Vector2          focused;
        Vector2          defocused;

        void deserialize(const std::string& name, TextInput& b);

        /** Pass the bounds outside the border; the borderPad will automatically be added. */
        void render(RenderDevice* rd, const Rect2D& bounds, bool focused) const;
    };

    class HSlider {
    public:
        class Bar {
        public:
            StretchRectH     base;
            Vector2          enabled;
            Vector2          disabled;
            void deserialize(const std::string& name, TextInput& b);
        };

        class Thumb {
        public:
            Rect2D           base;
            class Focus {
            public:
                Vector2      focused;
                Vector2      defocused;
                void deserialize(const std::string& name, TextInput& b);
            };

            Focus            enabled;
            Vector2          disabled;

            void deserialize(const std::string& name, TextInput& b);
        };

        Bar                  bar;
        Thumb                thumb;

        void deserialize(const std::string& name, TextInput& b);

        /** Renders along the center of the vertical bounds and stretches to fill horizontally.*/
        void render(RenderDevice* rd, const Rect2D& bounds, float thumbPos, bool enabled, bool focused) const;

        /** Given the bounds on the graphical part of the slider, returns the bounds on the track.*/
        Rect2D trackBounds(const Rect2D& sliderBounds) const;
        Rect2D thumbBounds(const Rect2D& sliderBounds, float pos) const;
    };

    class Pane {
    public:
        StretchRectHV    frame;
        Pad              clientPad;
        void deserialize(const std::string& name, TextInput& b);
    };

    Checkable         m_checkBox;
    Checkable         m_radioButton;
    Button            m_button;
    Window            m_window;
    Window            m_toolWindow;
    HSlider           m_hSlider;
    Pane              m_simplePane;
    Pane              m_ornatePane;

    TextureRef        texture;

    /**
       The transformation matrix used to scale texture coordinates
       to the resolution of the texture.
       
       OpenGL-style matrix
     */
    float             guiTextureMatrix[16];

    float             fontTextureMatrix[16];
    
    /** The font and gui share a single texture unit (this is faster than
        using two units and selectively disabling).
     */
    int               fontTextureID;

    /** True between beginRendering and endRendering */
    bool              inRendering;

    // Defaults:
    GFontRef          font;
    float             fontSize;
    Color4            fontColor;
    Color4            fontOutlineColor;

    RenderDevice*     rd;

    /** Used by push/popClientRect */
    Array<Rect2D>     scissorStack;

    /** Used by push/popClientRect */
    Array<CoordinateFrame> coordinateFrameStack;

    static StretchMode readStretchMode(TextInput& t);

    static void drawRect(const Rect2D& vertex, const Rect2D& texCoord, RenderDevice* rd);
    
    void drawCheckable(const Checkable& control, const Rect2D& bounds, bool enabled, bool focused,
                       bool selected, const GuiText& text) const;

    void drawWindow(const Window& window, const Rect2D& bounds, bool focused, const GuiText& text) const;

    static Rect2D readRect2D(const std::string& name, TextInput& b);

    static Vector2 readVector2(const std::string& name, TextInput& b);

    GuiSkin(const std::string& filename);
    
    void deserialize(const std::string& path, BinaryInput& b);

    /** Called from deserialize(BinaryInput) */
    void deserialize(const std::string& path, TextInput& t);

    /** Call before GFont::send2DQuads */
    void beginText() const;   
 
    /** Call after GFont::send2DQuads */
    void endText() const;    
    
    Rect2D horizontalSliderToSliderBounds(const Rect2D& bounds) const;

public:

    /** 
     May return a cached copy.  

     @param filename Name of the .skn file.
    */
    static GuiSkinRef fromFile(const std::string& filename);

    /** Set the values to be used for default GuiText parameters. */
    void setFont(const GFontRef& font, float size, const Color4& color, const Color4& outlineColor);

    /** Call before all other render methods. 
        @param offset Offset all positions by this amount (convenient for rendering 
        relative to a containing control or window.)
     */
    void beginRendering(class RenderDevice* rd);

    /** 
      Offsets all subsequent rendering by r.x0y0() and sets the clipping region to r.
      Call only between beginRendering and endRendering.
      */
    void pushClientRect(const Rect2D& r);
    void popClientRect();

    /** Call after all other render methods. */
    void endRendering();

    /** Only call between beginRendering and endRendering */
    void renderCheckBox(const Rect2D& bounds, bool enabled, bool focused, 
                        bool checked, const GuiText& text) const;

    /** Only call between beginRendering and endRendering */
    void renderRadioButton(const Rect2D& bounds, bool enabled, bool focused, 
                           bool checked, const GuiText& text) const;

    /** Only call between beginRendering and endRendering */
    void renderButton(const Rect2D& bounds, bool enabled, bool focused, 
                      bool pushed, const GuiText& text) const;

    /** Only call between beginRendering and endRendering.
        @param bounds Corresponds to the footprint of the window; dropshadows and glows may
         still render outside this area.*/
    void renderWindow(const Rect2D& bounds, bool focused, 
                      const GuiText& text) const;

    /** Given the bounds on a window's borders, returns the bounds of
     the area inside the window where controls will appear.*/
    Rect2D windowToClientBounds(const Rect2D& bounds) const;

    /** Given the area that controls should appear in for a window,
        returns the bounds that should be used to draw the window.
        Note that the window's shadow or other effects may exceed
        these bounds when rendering.*/
    Rect2D clientToWindowBounds(const Rect2D& bounds) const;
    Rect2D windowToTitleBounds(const Rect2D& bounds) const;

    /** Returns the position of the thumb button, which is needed for processing
        UI events for the slider. */
    Rect2D horizontalSliderToThumbBounds(const Rect2D& bounds, float pos) const;
    Rect2D horizontalSliderToTrackBounds(const Rect2D& bounds) const;

    Rect2D toolWindowToClientBounds(const Rect2D& bounds) const;
    Rect2D clientToToolWindowBounds(const Rect2D& bounds) const;
    Rect2D toolWindowToTitleBounds(const Rect2D& bounds) const;

    Rect2D ornatePaneToClientBounds(const Rect2D& bounds) const;
    Rect2D simplePaneToClientBounds(const Rect2D& bounds) const;

    /** Only call between beginRendering and endRendering */
    void renderToolWindow(const Rect2D& bounds, bool focused, 
                          const GuiText& text) const;

    /** Only call between beginRendering and endRendering.
        Label is on the right, slider is aligned with the left edge
        @param pos 0 = left edge, 1 = right edge*/
    void renderHorizontalSlider(const Rect2D& bounds, float pos, bool enabled, bool focused, 
                                const GuiText& text) const;

    /** Only call between beginRendering and endRendering */
    void renderLabel(const Rect2D& bounds, const GuiText& text, 
                     GFont::XAlign xalign, GFont::YAlign yalign) const;

    void renderSimplePane(const Rect2D& bounds) const;

    void renderOrnatePane(const Rect2D& bounds) const;
    
    /** 
        Create a .skn file from source files.  Used as a preprocess
        when creating new skins.  You probably don't need to call this
        because most people will want to use pre-existing skins
        (making a skin is a time-consuming artistic process!)

        @param sourceDir Must end in a slash. 
        @param whiteName Image of controls on a white background, must be in sourceDir
        @param blackName Image of controls on a black background, must be in sourceDir
        @param coordsFile Text file describing the location of controls
        @param destFile Output .skn file, not relative to sourceDir
    */
    static void makeSkinFromSourceFiles(
              const std::string& sourceDir,
              const std::string& whiteName,
              const std::string& blackName,
              const std::string& coordsFile,
              const std::string& destFile);

}; // GuiSkin

} // namespace G3D

#endif // G3D_GUISKIN_H
