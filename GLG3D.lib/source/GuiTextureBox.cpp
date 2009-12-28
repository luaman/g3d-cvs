/**
 @file GLG3D/GuiTextureBox.cpp

 @created 2009-09-11
 @edited  2009-09-19

 G3D Library http://g3d-cpp.sf.net
 Copyright 2001-2009, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
*/
#include "GLG3D/GApp.h" // TODO: remove

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
    channels(c), documentGamma(g), min(mn), max(mx), showFormat(true) {
}


const GuiTextureBox::Settings& GuiTextureBox::Settings::sRGB() {
    static const Settings s(RGB, 2.2f, 0.0f, 1.0f);
    return s;
}


const GuiTextureBox::Settings& GuiTextureBox::Settings::unitVector() {
    static const Settings s(RGB, 1.0f, -1.0f, 1.0f);
    return s;
}


const GuiTextureBox::Settings& GuiTextureBox::Settings::depthBuffer() {
    static const Settings s(RasL, 9.0f, 0.2f, 1.0f);
    return s;
}


const GuiTextureBox::Settings& GuiTextureBox::Settings::bumpInAlpha() {
    static const Settings s(AasL, 1.0f, 0.0f, 1.0f);
    return s;
}


const GuiTextureBox::Settings& GuiTextureBox::Settings::defaults() {
    static const Settings s;
    return s;
}


bool GuiTextureBox::Settings::needsShader() const {
    return 
        (channels != RGB) ||
        (documentGamma != 2.2f) ||
        (min != 0.0f) ||
        (max != 1.0f);
}

//////////////////////////////////////////////////////////

GuiTextureBox::GuiTextureBox
(GuiContainer*       parent,
 const GuiText&      caption,
 const Texture::Ref& t,
 const Settings&     s,
 bool                embeddedMode) : 
    GuiContainer(parent, caption), 
    m_texture(t),
    m_settings(s), 
    m_showInfo(true), 
    m_dragging(false), 
    m_drawerOpen(embeddedMode),
    m_readbackXY(-1, -1),
    m_embeddedMode(embeddedMode) {

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

    m_drawerPane = new GuiPane(this, "", Rect2D::xywh(100, cs, 100, h), embeddedMode ? GuiTheme::NO_PANE_STYLE : GuiTheme::ORNATE_PANE_STYLE);
    m_drawerPane->setVisible(false);

    // Contents of the tools drawer:
    {
        static const char* infoIcon = "i";
        static const char* zoomIcon = "L";
        static const char* diskIcon = "\xcd";
        static const char* inspectorIcon = "\xa0";

        m_saveButton = m_drawerPane->addButton(GuiText(diskIcon, iconFont, h), 
                                                        Callback(this, &GuiTextureBox::save),
                                                        GuiTheme::TOOL_BUTTON_STYLE);
        m_saveButton->setSize(h, h);


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

        m_inspectorButton = m_drawerPane->addButton(GuiText(inspectorIcon, iconFont, h), 
            Callback(this, &GuiTextureBox::showInspector), GuiTheme::TOOL_BUTTON_STYLE);
        m_inspectorButton->setSize(h, h);
        m_inspectorButton->moveBy(h/3, 0);

        m_drawerPane->pack();
        // Add some padding
        m_drawerPane->setWidth(m_drawerPane->rect().width() + m_drawerButton->rect().width());

        if (embeddedMode) {
            m_inspectorButton->setVisible(false);
            m_inspectorButton->setEnabled(false);

            m_drawerButton->setVisible(false);
            m_drawerButton->setEnabled(false);

            infoButton->setVisible(false);
            infoButton->setEnabled(false);
        }
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

    filename = System::currentDateString() + "-" + filename;

    // Make sure this filename doesn't exist
    int i = 0;
    while (fileExists(format("%s-%d.png", filename.c_str(), i))) {
        ++i;
    }
    filename = format("%s-%d.png", filename.c_str(), i);

    if (FileDialog::create(window())->getFilename(filename)) {
        // save code
        Framebuffer::Ref fb = Framebuffer::create("GuiTextureBox: save");
        Texture::Ref color = Texture::createEmpty("GuiTextureBox: save", 
            m_texture->width(), m_texture->height(), ImageFormat::RGB8(), Texture::DIM_2D_NPOT, Texture::Settings::video());
        fb->set(Framebuffer::COLOR0, color);
        
        RenderDevice* rd = RenderDevice::lastRenderDeviceCreated;
        rd->push2D(fb);
        {
            rd->setColorClearValue(Color3::white());
            rd->clear();
            drawTexture(rd, rd->viewport());
        }
        rd->pop2D();

        color->toImage3()->save(filename);
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
        if (m_clipBounds.contains(Vector2(event.button.x, event.button.y))) {
            return true;
        } else {
            return false;
        }

    } else if (event.type == GEventType::MOUSE_MOTION) {
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
    float OPEN_Y = m_rect.height() - oldRect.height() - DRAWER_Y_OFFSET;
    if (m_embeddedMode) {
        OPEN_Y += 4.0f;
    }

    const float CLOSED_Y = m_rect.height() - oldRect.height() * 2.0f;
    m_drawerPane->setPosition(m_rect.width() - oldRect.width() - 2.0f, m_drawerOpen ? OPEN_Y : CLOSED_Y);
}


void GuiTextureBox::toggleDrawer() {
    Rect2D oldRect = m_drawerPane->rect();

    float OPEN_Y = m_rect.height() - oldRect.height() - DRAWER_Y_OFFSET;
    if (m_embeddedMode) {
        OPEN_Y += 4.0f;
    }

    float CLOSED_Y = m_rect.height() - oldRect.height() * 2.0f;

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

    GuiTextureBox*              m_textureBox;

    GuiWindow::Ref              m_parentWindow;

    GuiDropDownList*            m_modeDropDownList;

    mutable GuiLabel*           m_xyLabel;
    mutable GuiLabel*           m_uvLabel;
    mutable GuiLabel*           m_rgbaLabel;
    mutable GuiLabel*           m_ARGBLabel;

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
        valLabel->setWidth(130);
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
        m_textureBox = leftPane->addTextureBox(displayCaption, texture, m_settings, true);

        m_textureBox->setSize(screenBounds - Vector2(450, 275));
        m_textureBox->zoomToFit();

        // Place the preset list in the empty space next to the drawer, over the TextureBox control
        Array<std::string> presetList;

        // This list must be kept in sync with onEvent
        presetList.append("<Click to load>", "sRGB Image", "Radiance", "Reflectivity");
        presetList.append( "8-bit Normal/Dir", "Float Normal/Dir");
        presetList.append("Depth Buffer", "Bump Map (in Alpha)");

        m_modeDropDownList = leftPane->addDropDownList("Vis. Preset", presetList);
        m_modeDropDownList->moveBy(4, -21);
        leftPane->pack();

        //////////////////////////////////////////////////////////////////////

        GuiPane* visPane = leftPane->addPane("", GuiTheme::NO_PANE_STYLE);
        
        Array<std::string> channelList;
        // this order must match the order of the Channels enum
        channelList.append("RGB", "R", "G", "B");
        channelList.append("R as Luminance", "G as Luminance", "B as Luminance", "A as Luminance");
        channelList.append("RGB/3 as Luminance", "True Luminance");
        visPane->addDropDownList("Channels", channelList, (int*)&m_settings.channels);

        GuiLabel* documentCaption = visPane->addLabel("Document");
        documentCaption->setWidth(65.0f);
        GuiNumberBox<float>* gammaBox = 
            visPane->addNumberBox(GuiText("g", GFont::fromFile(System::findDataFile("greek.fnt"))), &m_settings.documentGamma, "", GuiTheme::LINEAR_SLIDER, 0.1f, 15.0f);
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
        m_xyLabel = addPair(dataPane, "xy =", "", 30);
        m_xyLabel->setWidth(70);
        m_uvLabel = addPair(dataPane, "uv =", "", 30, m_xyLabel);
        m_uvLabel->setWidth(120);
        if (texture->invertY) {
            dataPane->addLabel(GuiText("After Y-inversion", NULL, 8))->moveBy(Vector2(5, -5));
        }

        m_rgbaLabel = addPair(dataPane, "rgba =", "", captionWidth);
        m_ARGBLabel = addPair(dataPane, "ARGB =", "", captionWidth);
        dataPane->addLabel(GuiText("Before gamma correction", NULL, 8))->moveBy(Vector2(5, -5));
        dataPane->pack();  
        dataPane->moveRightOf(visPane);
        leftPane->pack();

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


    virtual void render(RenderDevice* rd) const {
        GuiWindow::render(rd);

        // Keep our display in sync with the original one when a GUI control changes
        m_textureBox->setSettings(m_settings);


        // Update the xy/uv/rgba labels
        Texture::Ref tex = m_textureBox->texture();
        float w = 1, h = 1;
        if (tex.notNull()) {
            w = tex->width();
            h = tex->height();
        }

        m_xyLabel->setCaption(format("(%d, %d)", m_textureBox->m_readbackXY.x, m_textureBox->m_readbackXY.y));
        m_uvLabel->setCaption(format("(%6.4f, %6.4f)", m_textureBox->m_readbackXY.x / w, m_textureBox->m_readbackXY.y / h));
        m_rgbaLabel->setCaption(format("(%6.4f, %6.4f, %6.4f, %6.4f)", m_textureBox->m_texel.r, 
            m_textureBox->m_texel.g, m_textureBox->m_texel.b, m_textureBox->m_texel.a));
        Color4uint8 c(m_textureBox->m_texel);
        m_ARGBLabel->setCaption(format("0x%02x%02x%02x%02x", c.a, c.r, c.g, c.b));
    }


    virtual bool onEvent(const GEvent& event) {
        if (GuiWindow::onEvent(event)) {
            return true;
        }
        
        switch (event.type) {
        case GEventType::KEY_DOWN:
            if (event.key.keysym.sym == GKey::ESCAPE) {
                // Cancel this window
                manager()->remove(this);
                return true;
            }
            break;

        case GEventType::GUI_ACTION:
            if ((event.gui.control == m_modeDropDownList) && (m_modeDropDownList->selectedIndex() > 0)) {
                std::string preset = m_modeDropDownList->selectedValue().text();
                if (preset == "sRGB Image") {
                    m_settings = GuiTextureBox::Settings::sRGB();
                } else if (preset == "Radiance") {
                    // Choose the maximum value
                    m_settings = GuiTextureBox::Settings::defaults();
                    Texture::Ref tex = m_textureBox->texture();
                    if (tex.notNull()) {
                        Color4 max = tex->max();
                        if (max.isFinite()) {
                            m_settings.max = G3D::max(max.r, max.g, max.b);
                        }
                    }
                } else if (preset == "Reflectivity") {
                    m_settings = GuiTextureBox::Settings::defaults();
                } else if (preset == "8-bit Normal/Dir") {
                    m_settings = GuiTextureBox::Settings::packedUnitVector();
                } else if (preset == "Float Normal/Dir") {
                    m_settings = GuiTextureBox::Settings::unitVector();
                } else if (preset == "Depth Buffer") {
                    m_settings = GuiTextureBox::Settings::depthBuffer();
                } else if (preset == "Bump Map (in Alpha)") {
                    m_settings = GuiTextureBox::Settings::bumpInAlpha();
                }

                // Switch back to <click to load>
                m_modeDropDownList->setSelectedIndex(0);
                return true;
            }
            break;

        default:;
        }

        return false;
    }
};


void GuiTextureBox::showInspector() {
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


void GuiTextureBox::drawTexture(RenderDevice* rd, const Rect2D& r) const {
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
            
            // GasL
            Matrix4(0, 1, 0, 0,
                    0, 1, 0, 0,
                    0, 1, 0, 0,
                    0, 0, 0, 0),

            // BasL
            Matrix4(0, 0, 1, 0,
                    0, 0, 1, 0,
                    0, 0, 1, 0,
                    0, 0, 0, 0),

            // AasL
            Matrix4(0, 0, 0, 1,
                    0, 0, 0, 1,
                    0, 0, 0, 1,
                    0, 0, 0, 0),

            // MeanRGBasL
            Matrix4(1, 1, 1, 0,
                    1, 1, 1, 0,
                    1, 1, 1, 0,
                    0, 0, 0, 0) * (1.0f/3.0f),

            // Luminance
            Matrix4(0.2126f, 0.7152f, 0.0722f, 0,
                    0.2126f, 0.7152f, 0.0722f, 0,
                    0.2126f, 0.7152f, 0.0722f, 0,
                    0, 0, 0, 0)
        };

        m_shader->args.set("texture", m_texture);
        m_shader->args.set("adjustGamma", m_settings.documentGamma / 2.2f);
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
}


void GuiTextureBox::render(RenderDevice* rd, const GuiTheme::Ref& theme) const {
    if (! m_visible) {
        return;
    }

    m_inspectorButton->setEnabled(m_texture.notNull());
    m_saveButton->setEnabled(m_texture.notNull());    

    int w = 0;
    int h = 0;

    m_drawerPane->setVisible(m_drawerOpen || m_drawerPane->morphing());

    const Rect2D& cvs = canvasRect();

    // Keep button on bottom of drawer, but always visible
    m_drawerButton->setPosition(m_drawerPane->rect().x1() - m_drawerButton->rect().width(), 
                                max(cvs.height() - 2.0f, m_drawerPane->rect().y1() - m_drawerButton->rect().height() - 1.0f));

    // Render size label
    if (m_texture.notNull()) {
        w = m_texture->width();
        h = m_texture->height();

        const std::string& fmt = toLower(m_texture->format()->name());

        if ((m_lastSize.x != w) || (m_lastSize.y != h) || (fmt != m_lastFormat)) {
            m_lastSize.x = w;
            m_lastSize.y = h;
            m_lastFormat = fmt;

            // Avoid computing this every frame
            std::string s;
            if (w == h) {
                // Use ASCII squared character
                s = format("(%d\xB2", w);
            } else {
                s = format("(%dx%d", w, h);
            }

            if (m_settings.showFormat) {
                s += " " + fmt + ")";
            } else {
                s += ")";
            }
            m_lastSizeCaption = s;
        }

        float regularCaptionWidth = theme->bounds(m_caption).x + 11;
        theme->renderLabel(Rect2D::xywh(m_rect.x0y0() + Vector2(regularCaptionWidth, 0), Vector2(46, 12)), 
                           m_lastSizeCaption, GFont::XALIGN_LEFT, GFont::YALIGN_TOP, m_enabled);
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
            // Merge with existing clipping region!
            Rect2D oldClip = rd->clip2D();
            // Scissor region ignores transformation matrix
            Rect2D newClip = m_clipBounds + matrix.translation.xy();

            rd->setClip2D(oldClip.intersect(newClip));

            drawTexture(rd, r);

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
                        mousePos *= Vector2(w, h) / r.wh();
                        //screenPrintf("w=%d h=%d", w, h);
                        int ix = iFloor(mousePos.x);
                        int iy = iFloor(mousePos.y);

                        if (ix >= 0 && ix < w && iy >= 0 && iy < h) {
                            if (m_readbackXY.x != ix || m_readbackXY.y != iy) {
                                m_readbackXY.x = ix;
                                m_readbackXY.y = iy;
                                m_texel = m_texture->readTexel(ix, iy, rd);
                            }

                            // Only display the values on-screen when we're not embedded
                            if (! m_embeddedMode) {
                                std::string s = format("xy:    (%d, %d)", ix, iy);                    
                                pos.y += font->draw2D(rd, s, pos, style.size, front, back).y * lineSpacing;
                                if (m_texture->invertY) {
                                    pos.y += font->draw2D(rd, "after y-inversion", pos + Vector2(20, 0), style.size * 0.75, front, back).y * lineSpacing;
                                }

                                Color4uint8 ci(m_texel);
                                pos.y += 
                                    font->draw2D(rd, 
                                                 format("rgba:(%.3f, %.3f, %.3f, %.3f)", 
                                                        m_texel.r, m_texel.g, m_texel.b, m_texel.a),
                                                        pos, style.size, front, back).y * lineSpacing;
                                if (m_settings.documentGamma != 2.2f) {
                                    pos.y += font->draw2D(rd, "before gamma correction", pos + Vector2(20, 0), style.size * 0.75, front, back).y * lineSpacing;
                                }
                            }
                        }
                    }
                }
            }

            Draw::rect2DBorder(r, rd, Color3::black(), 0, BORDER);          
        }
        theme->resumeRendering();
    }
}

#define ZOOM_FACTOR (sqrt(2.0f))

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

