/**
 @file GLG3D/GuiTextureBox.cpp

 @created 2009-09-11
 @edited  2009-09-19

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2009, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
*/

#include "GLG3D/GuiTextureBox.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/Draw.h"
#include "GLG3D/FileDialog.h"
#include "G3D/fileutils.h"

namespace G3D {

/** Pixels the drawer is pushed up to make it appear to hang off the canvas */
static const float DRAWER_Y_OFFSET = 5.0f;

WeakReferenceCountedPointer<Shader> GuiTextureBox::g_cachedShader;

GuiTextureBox::Settings::Settings(Channels c, float g, float mn, float mx) : 
    channels(c), documentGamma(g), min(mn), max(mx) {
}

const GuiTextureBox::Settings& GuiTextureBox::Settings::image() {
    static const Settings s(RGB, 2.1f, 0.0f, 1.0f);
    return s;
}


const GuiTextureBox::Settings& GuiTextureBox::Settings::unitVector() {
    static const Settings s(RGB, 1.0f, -1.0f, 1.0f);
    return s;
}


const GuiTextureBox::Settings& GuiTextureBox::Settings::zBuffer() {
    static const Settings s(RasL, 1.0f, 0.1f, 1.0f);
    return s;
}


const GuiTextureBox::Settings& GuiTextureBox::Settings::bumpInAlpha() {
    static const Settings s(AasL, 1.0f, 0.0f, 1.0f);
    return s;
}

bool GuiTextureBox::Settings::needsShader() const {
    return 
        (channels != RGB) ||
        (documentGamma != 2.1f) ||
        (min != 0.0f) ||
        (max != 1.0f);
}

//////////////////////////////////////////////////////////

GuiTextureBox::GuiTextureBox
(GuiContainer*       parent,
 const GuiText&      caption,
 const Texture::Ref& t,
 const Settings&     s) : 
    GuiContainer(parent, caption), m_texture(t), m_settings(s), m_showInfo(true), 
    m_dragging(false), m_needReadback(true) {

    // Height of caption and button bar
    const float cs = TOP_CAPTION_SIZE;

    // Height of the drawer
    const float h = cs - 1;

    setTexture(t);
    setSettings(s);

    GFont::Ref iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));

    // Button for opening tools drawer
    m_drawerCollapseCaption = GuiText("5", iconFont);
    m_drawerExpandCaption = GuiText("6", iconFont);
    m_drawerButton = new GuiButton(this, Callback(this, &GuiTextureBox::toggleDrawer), 
                                   m_drawerExpandCaption, GuiTheme::TOOL_BUTTON_STYLE);
    m_drawerButton->setCaption(m_drawerExpandCaption);
    m_drawerButton->setSize(12, 9);

    m_drawerOpen = false;

    m_drawerPane = new GuiPane(this, "", Rect2D::xywh(100, cs, 100, h), GuiTheme::ORNATE_PANE_STYLE);
    m_drawerPane->setVisible(false);

    // Contents of the tools drawer:
    {
        static const char* infoIcon = "i";
        static const char* zoomIcon = "L";
        static const char* diskIcon = "\xcd";
        static const char* inspectorIcon = "\xa0";

        GuiButton* saveButton = m_drawerPane->addButton(GuiText(diskIcon, iconFont, h), 
                                                        Callback(this, &GuiTextureBox::save),
                                                        GuiTheme::TOOL_BUTTON_STYLE);
        saveButton->setSize(h, h);


        GuiButton* zoomInButton = m_drawerPane->addButton(GuiText(zoomIcon, iconFont, h), 
                                                          Callback(this, &GuiTextureBox::zoomIn), 
                                                          GuiTheme::TOOL_BUTTON_STYLE);
        zoomInButton->setSize(h, h);
        zoomInButton->moveBy(h/3, 0);

        GuiButton* fitToWindowButton = m_drawerPane->addButton(GuiText("fit", NULL, h - 7), 
                                                               Callback(this, &GuiTextureBox::zoomToFit), GuiTheme::TOOL_BUTTON_STYLE);
        fitToWindowButton->setSize(h, h);

        GuiButton* zoom100Button = m_drawerPane->addButton(GuiText("1:1", NULL, h - 8), 
                                                           Callback(this, &GuiTextureBox::zoomTo1), GuiTheme::TOOL_BUTTON_STYLE);
        zoom100Button->setSize(h, h);

        GuiButton* zoomOutButton = m_drawerPane->addButton(GuiText(zoomIcon, iconFont, h/2), 
                                                           Callback(this, &GuiTextureBox::zoomOut),
                                                           GuiTheme::TOOL_BUTTON_STYLE);
        zoomOutButton->setSize(h, h);

        GuiCheckBox* infoButton = m_drawerPane->addCheckBox(GuiText(infoIcon, iconFont, h - 1),
                                                            &m_showInfo, GuiTheme::TOOL_CHECK_BOX_STYLE);
        infoButton->setSize(h, h);
        infoButton->moveBy(h/3, 0);
        GuiButton* inspectorButton = m_drawerPane->addButton(GuiText(inspectorIcon, iconFont, h), 
            Callback(this, &GuiTextureBox::launchInspector), GuiTheme::TOOL_BUTTON_STYLE);
        inspectorButton->setSize(h, h);
        inspectorButton->moveBy(h/3, 0);

        m_drawerPane->pack();
        // Add some padding
        m_drawerPane->setWidth(m_drawerPane->rect().width() + m_drawerButton->rect().width());
    }

    setCaptionSize(h);
    float aspect = 1440.0f / 900.0f;
    setSize(Vector2(190, 190 / aspect));

    zoomToFit();
}


GuiTextureBox::~GuiTextureBox() {
    delete m_drawerPane;
    delete m_drawerButton;
}


void GuiTextureBox::save() {
    std::string filename;
    
    // Make a sample filename, removing illegal or undesirable characters
    std::string temp = m_caption.text();
    for (int i = 0; i < (int)temp.size(); ++i) {
        switch (temp[i]) {
        case ' ':
        case '\r':
        case '\n':
        case '\t':
        case '.':
        case ':':
        case '/':
        case '\\':
        case '\'':
        case '\"':
            filename += "_";
            break;

        default:
            filename += temp[i];
        }
    }

    if (filename == "") {
        filename = "image";
    }

    // Make sure this filename doesn't exist
    int i = 0;
    while (fileExists(format("%s%d.png", filename.c_str(), i))) {
        ++i;
    }
    filename = format("%s%d.png", filename.c_str(), i);

    if (FileDialog::create(window())->getFilename(filename)) {
        // save code
        // TODO: render to texture
        // TODO: readback texture
        // TODO: save texture
    }
}


void GuiTextureBox::setSizeFromInterior(const Vector2& dims) {
    // Find out how big the canvas inset is
    Rect2D big = Rect2D::xywh(0, 0, 100, 100);
    Rect2D small = theme()->canvasToClientBounds(big, m_captionSize);
    
    // Offset is now big - small
    setSize(dims + big.wh() - small.wh() + Vector2(BORDER, BORDER) * 2.0f);
}


bool GuiTextureBox::onEvent(const GEvent& event) {
    if (! m_visible) {
        return false;
    }

    m_needReadback = true;

    if (! m_enabled) {
        return false;
    }

    if (GuiContainer::onEvent(event)) {
        // Event was handled by base class
        return true;
    }

    if ((event.type == GEventType::MOUSE_BUTTON_DOWN) && 
        m_clipBounds.contains(Vector2(event.button.x, event.button.y))) {

        m_dragStart = Vector2(event.button.x, event.button.y);
        m_dragging = true;
        m_offsetAtDragStart = m_offset;
        return true;

    } else if (event.type == GEventType::MOUSE_BUTTON_UP) {

        // Stop drag
        m_dragging = false;
        return true;

    } else if (event.type == GEventType::MOUSE_MOTION) {
        m_needReadback = true;
        if (m_dragging) {
            Vector2 mouse(event.motion.x, event.motion.y);
            
            // Move point, clamping adjacents        
            Vector2 delta = mouse - m_dragStart;

            // Hide weird mouse event delivery
            if (delta.squaredLength() < 100000) {
                m_offset = m_offsetAtDragStart + delta / m_zoom;
                return true;
            }
        }
    }

    return false;
}


void GuiTextureBox::setRect(const Rect2D& rect) {
    GuiContainer::setRect(rect);

    m_clipBounds = theme()->canvasToClientBounds(canvasRect(), m_captionSize);

    Rect2D oldRect = m_drawerPane->rect();
    const float OPEN_Y = m_rect.height() - oldRect.height() - DRAWER_Y_OFFSET;
    const float CLOSED_Y = m_rect.height() - oldRect.height() * 2.0f;
    m_drawerPane->setPosition(m_rect.width() - oldRect.width() - 2.0f, m_drawerOpen ? OPEN_Y : CLOSED_Y);
}


void GuiTextureBox::toggleDrawer() {
    Rect2D oldRect = m_drawerPane->rect();

    const float OPEN_Y = m_rect.height() - oldRect.height() - DRAWER_Y_OFFSET;
    const float CLOSED_Y = m_rect.height() - oldRect.height() * 2.0f;

    if (m_drawerOpen) {
        m_drawerButton->setCaption(m_drawerExpandCaption);
        m_drawerPane->morphTo(Rect2D::xywh(oldRect.x0(), CLOSED_Y, oldRect.width(), oldRect.height()));
    } else {
        m_drawerButton->setCaption(m_drawerCollapseCaption);
        m_drawerPane->morphTo(Rect2D::xywh(oldRect.x0(), OPEN_Y, oldRect.width(), oldRect.height()));
    }
    m_drawerOpen = ! m_drawerOpen;
}


Rect2D GuiTextureBox::canvasRect() const {
    // Use textbox borders, but reserve space for the button bar
    return Rect2D::xywh(m_rect.x0y0(), m_rect.wh() - Vector2(0, m_drawerPane->rect().height() - DRAWER_Y_OFFSET));
}


class GuiTextureBoxInspector : public GuiWindow {
protected:

    /** Settings of the original GuiTextureBox */
    GuiTextureBox::Settings&    m_settings;

    GuiWindow::Ref              m_parentWindow;

    /** Adds two labels to create a two-column display and returns a pointer to the second label. */
    static GuiLabel* addPair(GuiPane* p, const GuiText& key, const GuiText& val, int captionWidth = 130, GuiLabel* nextTo = NULL, int moveDown = 0) {
        GuiLabel* keyLabel = p->addLabel(key);
        if (nextTo) {
            keyLabel->moveRightOf(nextTo);
        }
        if (moveDown != 0) {
            keyLabel->moveBy(0, moveDown);
        }
        keyLabel->setWidth(captionWidth);
        GuiLabel* valLabel = p->addLabel(val);
        valLabel->moveRightOf(keyLabel);
        valLabel->setWidth(120);
        return valLabel;
    }


    static std::string valToText(const Color4& val) {
        if (val.isFinite()) {
            return format("(%6.3f, %6.3f, %6.3f, %6.3f)", val.r, val.g, val.b, val.a);
        } else {
            return "Unknown";
        }
    }

public:

    /** \param parentWindow Hold a pointer to the window containing the original 
        GuiTextureBox so that it is not collected while we have its Settings&. */
    GuiTextureBoxInspector(const GuiText& displayCaption, const Texture::Ref& texture, GuiTextureBox::Settings& settings, const GuiWindow::Ref& parentWindow) :
        GuiWindow("Inspecting \"" + texture->name() + "\"", 
            parentWindow->theme(), 
            Rect2D::xywh(0,0, 100, 100),
            GuiTheme::NORMAL_WINDOW_STYLE, 
            REMOVE_ON_CLOSE),
        m_settings(settings),
        m_parentWindow(parentWindow) {

        Vector2 screenBounds(parentWindow->window()->width(), parentWindow->window()->height());

        GuiPane* p = pane();
        GuiPane* leftPane = p->addPane("", GuiTheme::NO_PANE_STYLE);

        GuiTextureBox::Settings s = GuiTextureBox::Settings(GuiTextureBox::RGB, 0.01f, 0.0f, 1.0f);
        GuiTextureBox* t = leftPane->addTextureBox(displayCaption, texture, m_settings);
        t->setSize(screenBounds - Vector2(450, 275));
        t->zoomToFit();
        leftPane->pack();

        //////////////////////////////////////////////////////////////////////

        GuiPane* visPane = leftPane->addPane("", GuiTheme::NO_PANE_STYLE);
        
        Array<std::string> channelList;
        channelList.append("RGB", "R", "G", "B");
        channelList.append("R as Luma", "G as Luma", "B as Luma", "A as Luma");
        channelList.append("Luminance");
        visPane->addDropDownList("Channels", channelList);

        GuiLabel* documentCaption = visPane->addLabel("Document");
        documentCaption->setWidth(65.0f);
        GuiNumberBox<float>* gammaBox = 
            visPane->addNumberBox(GuiText("g", GFont::fromFile(System::findDataFile("greek.fnt"))), &m_settings.documentGamma, "", GuiTheme::LINEAR_SLIDER, 0.1f, 10.0f);
        gammaBox->setCaptionSize(15.0f);
        gammaBox->setUnitsSize(5.0f);
        gammaBox->setWidth(150.0f);
        gammaBox->moveRightOf(documentCaption);

        GuiNumberBox<float>* minBox;
        GuiNumberBox<float>* maxBox;
        minBox = visPane->addNumberBox("Range", &m_settings.min);
        minBox->setUnitsSize(0.0f);
        minBox->setWidth(145.0f);
        
        maxBox = visPane->addNumberBox("-", &m_settings.max);
        maxBox->setCaptionSize(10.0f);
        maxBox->moveRightOf(minBox);
        visPane->pack();
        visPane->setWidth(230);

        GuiPane* dataPane = leftPane->addPane("", GuiTheme::NO_PANE_STYLE);

        int captionWidth = 55;
        GuiLabel* xyLabel = addPair(dataPane, "xy =", "(400, 300)", 30);
        xyLabel->setWidth(100);
        GuiLabel* uvLabel = addPair(dataPane, "uv =", "(0.1111, 0.3111)", 30, xyLabel);
        uvLabel->setWidth(100);
        addPair(dataPane, "rgba* =", "(0.2001, 0.2001, 3.2001, 1.2001)", captionWidth);
        addPair(dataPane, "ARGB* =", "0xFF3029AA", captionWidth);
        dataPane->addLabel(GuiText("* Before gamma correction", NULL, 8))->moveBy(Vector2(0, -5));
        dataPane->pack();  
        dataPane->moveRightOf(visPane);
        leftPane->pack();

        //////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////
        GuiPane* infoPane = p->addPane("", GuiTheme::NO_PANE_STYLE);
        const Texture::Settings& textureSettings = texture->settings();

        addPair(infoPane, "Invert Y:", (texture->invertY ? "true" : "false"));
        addPair(infoPane, "Format:", texture->format()->name());

        addPair(infoPane, "Wrap Mode:", textureSettings.wrapMode.toString());
        std::string dim;
        switch (texture->dimension()) {
        case Texture::DIM_2D: dim = "DIM_2D"; break;
        case Texture::DIM_3D: dim = "DIM_3D"; break;
        case Texture::DIM_2D_RECT: dim = "DIM_2D_RECT"; break;
        case Texture::DIM_CUBE_MAP: dim = "DIM_CUBE_MAP"; break;
        case Texture::DIM_2D_NPOT: dim = "DIM_2D_NPOT"; break;
        case Texture::DIM_CUBE_MAP_NPOT: dim = "DIM_CUBE_MAP_NPOT"; break;
        case Texture::DIM_3D_NPOT: dim = "DIM_3D_NPOT"; break;
        }
        addPair(infoPane, "Dimension:", dim);

        std::string dr;
        switch (textureSettings.depthReadMode) {
        case Texture::DEPTH_NORMAL: dr = "DEPTH_NORMAL"; break;
        case Texture::DEPTH_LEQUAL: dr = "DEPTH_LEQUAL"; break;
        case Texture::DEPTH_GEQUAL: dr = "DEPTH_GEQUAL"; break;
        }
        addPair(infoPane, "Depth Read Mode:", dr);

        std::string interp;
        switch (textureSettings.interpolateMode) {
        case Texture::TRILINEAR_MIPMAP: interp = "TRILINEAR_MIPMAP"; break;
        case Texture::BILINEAR_MIPMAP: interp = "BILINEAR_MIPMAP"; break;
        case Texture::NEAREST_MIPMAP: interp = "NEAREST_MIPMAP"; break;
        case Texture::BILINEAR_NO_MIPMAP: interp = "BILINEAR_NO_MIPMAP"; break;
        case Texture::NEAREST_NO_MIPMAP: interp = "NEAREST_NO_MIPMAP"; break;
        }
        addPair(infoPane, "Interpolate Mode:", interp, 130, NULL, 20);

        addPair(infoPane, "Autoupdate MIP-map:", (textureSettings.autoMipMap ? "true" : "false"));

        addPair(infoPane, "Min MIP-level:", 
            format("%-5d (%d x %d)", textureSettings.minMipMap, 
            max(1, texture->width() / pow2(max(0, textureSettings.minMipMap))), 
            max(1, texture->height() / pow2(max(0, textureSettings.minMipMap)))));
        addPair(infoPane, "Max MIP-level:", 
            format("%-5d (%d x %d)", textureSettings.maxMipMap, 
            max(1, texture->width() / pow2(textureSettings.maxMipMap)), 
            max(1, texture->height() / pow2(textureSettings.maxMipMap))));
                            
        addPair(infoPane, "Max Anisotropy:", format("%g", textureSettings.maxAnisotropy));

        addPair(infoPane, "Min Value:",  valToText(texture->min()), 80, NULL, 20);
        addPair(infoPane, "Mean Value:", valToText(texture->mean()), 80);
        addPair(infoPane, "Max Value:",  valToText(texture->max()), 80);

        infoPane->pack();
        infoPane->setWidth(300);
        infoPane->moveRightOf(leftPane);
        infoPane->moveBy(0, -3);
        
        pack();
        moveTo(screenBounds / 2.0f - rect().center());
        setVisible(true);
    }

    virtual bool onEvent(const GEvent& event) {
        if (GuiWindow::onEvent(event)) {
            return true;
        } else if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::ESCAPE)) {
            // Cancel this window
            manager()->remove(this);
            return true;
        } else {
            return false;
        }
    }
};


void GuiTextureBox::launchInspector() {

    GuiWindow::Ref myWindow = window();
    WidgetManager::Ref manager = myWindow->manager();

    ReferenceCountedPointer<GuiTextureBoxInspector> ins = m_inspector.createStrongPtr();
    if (ins.isNull()) {
        ins = new GuiTextureBoxInspector(caption(), m_texture, m_settings, myWindow);
        m_inspector = ins;

        manager->add(ins);
    }

    manager->setFocusedWidget(ins);
}


void GuiTextureBox::render(RenderDevice* rd, const GuiTheme::Ref& theme) const {
    if (! m_visible) {
        return;
    }

    int w = 0;
    int h = 0;

    m_drawerPane->setVisible(m_drawerOpen || m_drawerPane->morphing());

    const Rect2D& cvs = canvasRect();

    // Keep button on bottom of drawer, but always visible
    m_drawerButton->setPosition(m_drawerPane->rect().x1() - m_drawerButton->rect().width(), 
                                max(cvs.height() - 2.0f, m_drawerPane->rect().y1() - m_drawerButton->rect().height() - 1.0f));

    GuiTextureBox* me = const_cast<GuiTextureBox*>(this);

    // Render size label
    if (m_texture.notNull()) {
        w = m_texture->width();
        h = m_texture->height();

        if ((m_lastSize.x != w) || (m_lastSize.y != h)) {
            // Avoid computing this every frame
            std::string s;
            if (w == h) {
                // Use ASCII squared character
                m_lastSizeCaption = format("%d\xB2", w);
            } else {
                m_lastSizeCaption = format("%dx%d", w, h);
            }
        }
        theme->renderLabel(Rect2D::xywh(m_rect.x1y0() + Vector2(-50, 0), Vector2(46, 12)), 
                           m_lastSizeCaption, GFont::XALIGN_RIGHT, GFont::YALIGN_TOP, m_enabled);
    }

    // Render child controls so that they slide under the canvas
    theme->pushClientRect(m_clientRect);
    {
        m_drawerButton->render(rd, theme);
        m_drawerPane->render(rd, theme);
    }
    theme->popClientRect();

    theme->renderCanvas(cvs, m_enabled, focused(), m_caption, m_captionSize);

    const CoordinateFrame& matrix = rd->objectToWorldMatrix();


    if (m_texture.notNull()) {
        // Shrink by the border size to save space for the border,
        // and then draw the largest rect that we can fit inside.
        Rect2D r = m_texture->rect2DBounds();
        r = r + (m_offset - r.center());
        r = r * m_zoom;
        r = r + m_clipBounds.center();
        
        theme->pauseRendering();
        {
            // Scissor region ignores transformation matrix
// TODO: Merge with existing clipping region!
            rd->setClip2D(m_clipBounds + matrix.translation.xy());

            // TODO: Draw"transparent" background
            rd->setAlphaTest(RenderDevice::ALPHA_ALWAYS_PASS, 0);
            rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);

            // Draw texture
            if (m_settings.needsShader()) {
                static const Matrix4 colorShift[] = {
                    // RGB
                    Matrix4(1, 0, 0, 0,
                            0, 1, 0, 0,
                            0, 0, 1, 0,
                            0, 0, 0, 0),

                    // R
                    Matrix4(1, 0, 0, 0,
                            0, 0, 0, 0,
                            0, 0, 0, 0,
                            0, 0, 0, 0),

                    // G
                    Matrix4(0, 0, 0, 0,
                            0, 1, 0, 0,
                            0, 0, 0, 0,
                            0, 0, 0, 0),

                    // B
                    Matrix4(0, 0, 0, 0,
                            0, 0, 0, 0,
                            0, 0, 1, 0,
                            0, 0, 0, 0),

                    // RasL
                    Matrix4(1, 0, 0, 0,
                            1, 0, 0, 0,
                            1, 0, 0, 0,
                            0, 0, 0, 0),
                    
                    // AasL
                    Matrix4(0, 0, 0, 1,
                            0, 0, 0, 1,
                            0, 0, 0, 1,
                            0, 0, 0, 0),

                    // RGBasL
                    Matrix4(1, 1, 1, 0,
                            1, 1, 1, 0,
                            1, 1, 1, 0,
                            0, 0, 0, 0) * (1.0f/3.0f)
                };

                m_shader->args.set("texture", m_texture);
                m_shader->args.set("adjustGamma", m_settings.documentGamma / 2.1f);
                m_shader->args.set("bias", -m_settings.min);
                m_shader->args.set("scale", 1.0f / (m_settings.max - m_settings.min));
                m_shader->args.set("colorShift", colorShift[m_settings.channels]);

                rd->setShader(m_shader);
                debugAssert(m_shader.notNull());

            } else {
                rd->setTexture(0, m_texture);
            }
            Draw::fastRect2D(r, rd);
            rd->setShader(NULL);
            rd->setTexture(0, NULL);

            if (m_texture.notNull()) {
                GuiTheme::TextStyle style = theme->defaultStyle();
                if (min(m_clipBounds.width(), m_clipBounds.height()) <= 128) {
                    style.size = 9;
                } else {
                    style.size = 12;
                }
                GFont::Ref font = style.font;
                
                Color4 front = Color3::black();
                Color4 back(1,1,1,0.70f);

                Vector2 pos = m_clipBounds.x0y0() + Vector2(4, 1);
                const float lineSpacing = 0.8f;

                // Display coords and value when requested
                if (m_showInfo && (window()->window()->mouseHideCount() < 1)) {
                    // Find the mouse position
                    Vector2 mousePos;
                    uint8 ignore;
                    window()->window()->getRelativeMouseState(mousePos, ignore);
                    // Make relative to the control
                    mousePos -= matrix.translation.xy();
                    
                    if (m_clipBounds.contains(mousePos) && r.contains(mousePos)) {
                        mousePos -= r.x0y0();
                        // Convert to texture coordinates
                        mousePos *= Vector2(w - 1, h - 1) / (r.wh() - Vector2(1, 1));
                        int ix = iFloor(mousePos.x);
                        int iy = iFloor(mousePos.y);
                        std::string s = format("xy:    (%d, %d)", ix, iy);
                    
                        pos.y += font->draw2D(rd, s, pos, style.size, front, back).y * lineSpacing;
                        if (m_texture->invertY) {
                            pos.y += font->draw2D(rd, "after y-inversion", pos + Vector2(20, 0), style.size * 0.75, front, back).y * lineSpacing;
                        }

                        if (m_needReadback) {
                            me->m_texel = m_texture->readTexel(ix, iy, rd);
                            me->m_needReadback = false;
                        }
                        Color4uint8 ci(m_texel);
                        pos.y += 
                            font->draw2D(rd, 
                                         format("rgba:(%.3f, %.3f, %.3f, %.3f)", 
                                                m_texel.r, m_texel.g, m_texel.b, m_texel.a),
                                         pos, style.size, front, back).y * lineSpacing;
                        if (m_settings.documentGamma != 2.1f) {
                            pos.y += font->draw2D(rd, "before gamma correction", pos + Vector2(20, 0), style.size * 0.75, front, back).y * lineSpacing;
                        }
                    }
                }
            }

            Draw::rect2DBorder(r, rd, Color3::black(), 0, BORDER);          
        }
        theme->resumeRendering();
    }
}

#define ZOOM_FACTOR (1.5f)

void GuiTextureBox::zoomIn() {
    m_zoom *= ZOOM_FACTOR;
}


void GuiTextureBox::zoomOut() {
    m_zoom /= ZOOM_FACTOR;
}


void GuiTextureBox::setViewZoom(float z) {
    m_zoom = z;
}


void GuiTextureBox::setViewOffset(const Vector2& x) {
    m_offset = x;
}


void GuiTextureBox::zoomToFit() {
    if (m_texture.notNull()) {
        Vector2 w = m_texture->vector2Bounds();
        Rect2D r = m_clipBounds.expand(-BORDER).largestCenteredSubRect(w.x, w.y);
        m_zoom = r.width() / w.x;
        m_offset = Vector2::zero();
    } else {
        zoomTo1();
    }
}


void GuiTextureBox::zoomTo1() {
    m_zoom = 1.0;
    m_offset = Vector2::zero();
}
    

void GuiTextureBox::findControlUnderMouse(Vector2 mouse, GuiControl*& control) const {
    if (! m_enabled || ! m_rect.contains(mouse) || ! m_visible) {
        return;
    }

    control = const_cast<GuiTextureBox*>(this);

    mouse -= m_clientRect.x0y0();
    if (m_drawerButton->clickRect().contains(mouse) && m_drawerButton->visible() && m_drawerButton->enabled()) {
        control = m_drawerButton;
    } else {
        m_drawerPane->findControlUnderMouse(mouse, control);        
    }
}

    
void GuiTextureBox::setTexture(const Texture::Ref& t) {
    m_texture = t;
    ReferenceCountedPointer<GuiTextureBoxInspector> ins = m_inspector.createStrongPtr();
    if (ins.notNull()) {
        // The inspector now has the wrong texture in it and it would require a 
        // lot of GUI changes to update it, so we simply close that window.
        window()->manager()->remove(ins);
    }
}


void GuiTextureBox::setSettings(const Settings& s) {
    // Check the settings for this computer
    m_settings = s;
    if (m_settings.needsShader()) {

        alwaysAssertM(GLCaps::supports_GL_ARB_shading_language_100(), 
                      "GuiTextureBox requires GLSL shader support for these GuiTextureBox::Settings");

        if (m_shader.isNull()) {
            // Load the shader
            m_shader = g_cachedShader.createStrongPtr();
            if (m_shader.isNull()) {
                // Load the global shader
                m_shader = Shader::fromStrings
                    ("",
                     STR(
                         uniform sampler2D texture;
                         uniform float     adjustGamma;
                         uniform mat4      colorShift;
                         uniform float     bias;
                         uniform float     scale;

                         void main(void) {
                             vec4 c = texture2D(texture, gl_TexCoord[g3d_Index(texture)].xy);
                             c = pow((c + bias) * scale, vec4(adjustGamma));
                             gl_FragColor.rgb = (colorShift * c).rgb;
                             gl_FragColor.a = 1.0;
                             }));
                g_cachedShader = m_shader;
            }
        }
    }

    m_settings = s;
}



} // G3D

