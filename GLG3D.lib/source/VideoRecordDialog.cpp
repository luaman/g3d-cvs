/**
  @file VideoRecordDialog.cpp
  
  @maintainer Morgan McGuire, morgan@cs.williams.edu

  @created 2008-10-18
  @edited  2009-09-25
 */ 
#include "G3D/platform.h"
#include "G3D/fileutils.h"
#include "G3D/Log.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/VideoRecordDialog.h"
#include "GLG3D/GApp.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Draw.h"

namespace G3D {

VideoRecordDialog::Ref VideoRecordDialog::create(const GuiThemeRef& theme, GApp* app) {
    return new VideoRecordDialog(theme, app);
}


VideoRecordDialog::Ref VideoRecordDialog::create(GApp* app) {
    alwaysAssertM(app, "GApp may not be NULL");
    return new VideoRecordDialog(app->debugWindow->theme(), app);
}


VideoRecordDialog::VideoRecordDialog(const GuiThemeRef& theme, GApp* app) : 
    GuiWindow("Screen Capture", theme, Rect2D::xywh(0, 100, 310, 200),
              GuiTheme::DIALOG_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE),
    m_app(app),
    m_templateIndex(0),
    m_playbackFPS(30),
    m_recordFPS(30),
    m_halfSize(false),
    m_enableMotionBlur(false),
    m_motionBlurFrames(10),
    m_screenshotPending(false),
    m_framesBox(NULL),
    m_showCursor(false) {

    m_hotKey = GKey::F6;
    m_hotKeyMod = GKeyMod::NONE;
    m_hotKeyString = m_hotKey.toString();

    m_ssHotKey = GKey::F4;
    m_ssHotKeyMod = GKeyMod::NONE;
    m_ssHotKeyString = m_ssHotKey.toString();

    m_settingsTemplate.append(VideoOutput::Settings::MPEG4(640, 680));
    m_settingsTemplate.append(VideoOutput::Settings::WMV(640, 680));
    //m_settingsTemplate.append(VideoOutput::Settings::CinepakAVI(640, 680));
    m_settingsTemplate.append(VideoOutput::Settings::rawAVI(640, 680));

    // Remove unsupported formats and build a drop-down list
    for (int i = 0; i < m_settingsTemplate.size(); ++i) {
        if (! VideoOutput::supports(m_settingsTemplate[i].codec)) {
            m_settingsTemplate.remove(i);
            --i;
        } else {
            m_formatList.append(m_settingsTemplate[i].description);
        }
    }

    m_templateIndex = 0;
    // Default to MPEG4 since that combines quality and size
    for (int i = 0; i < m_settingsTemplate.size(); ++i) {
        if (m_settingsTemplate[i].codec == VideoOutput::CODEC_ID_MPEG4) {
            m_templateIndex = i;
            break;
        }
    }

    m_font = GFont::fromFile(System::findDataFile("arial.fnt"));

    makeGUI();
    
    m_recorder = new Recorder();
    m_recorder->dialog = this;
}


std::string VideoRecordDialog::nextFilenameBase() {
    return generateFilenameBase("", "_" + System::appName());
}


void VideoRecordDialog::makeGUI() {
    pane()->addLabel(GuiText("Video", NULL, 12));
    GuiPane* moviePane = pane()->addPane("", GuiTheme::ORNATE_PANE_STYLE);

    GuiLabel* label = NULL;
    GuiDropDownList* formatList = moviePane->addDropDownList("Format", m_formatList, &m_templateIndex);

    int width = 300;
    // Increase caption size to line up with the motion blur box
    int captionSize = 90;

    formatList->setWidth(width);
    formatList->setCaptionSize(captionSize);
    
    if (false) {
        // For future expansion
        GuiCheckBox*  motionCheck = moviePane->addCheckBox("Motion Blur",  &m_enableMotionBlur);
        m_framesBox = moviePane->addNumberBox("", &m_motionBlurFrames, "frames", true, 2, 20);
        m_framesBox->setUnitsSize(46);
        m_framesBox->moveRightOf(motionCheck);
        m_framesBox->setWidth(210);
    }

    GuiNumberBox<float>* playbackBox = moviePane->addNumberBox("Playback",    &m_playbackFPS, "fps", false, 1.0f, 120.0f, 0.1f);
    playbackBox->setCaptionSize(captionSize);

    GuiNumberBox<float>* recordBox   = moviePane->addNumberBox("Record",      &m_recordFPS, "fps", false, 1.0f, 120.0f, 0.1f);
    recordBox->setCaptionSize(captionSize);

    moviePane->addCheckBox("Record GUI (PosedModel2D)", &m_captureGUI);

    if (GLCaps::supports_GL_ARB_texture_non_power_of_two() && GLCaps::supports_GL_EXT_framebuffer_object()) {
        const OSWindow* window = OSWindow::current();
        int w = window->width() / 2;
        int h = window->height() / 2;
        moviePane->addCheckBox(format("Half-size (%d x %d)", w, h), &m_halfSize);
    }

    if (false) {
        // For future expansion
        moviePane->addCheckBox("Show cursor", &m_showCursor);
    }

    label = moviePane->addLabel("Hot key:");
    label->setWidth(captionSize);
    moviePane->addLabel(m_hotKeyString)->moveRightOf(label);

    m_recordButton = moviePane->addButton("Record Now (" + m_hotKeyString + ")");
    m_recordButton->moveBy(pane()->rect().width() - m_recordButton->rect().width() - 5, -27);

    moviePane->pack();
    moviePane->setWidth(pane()->rect().width());

    ///////////////////////////////////////////////////////////////////////////////////
    pane()->addLabel(GuiText("Screenshot", NULL, 12));
    GuiPane* ssPane = pane()->addPane("", GuiTheme::ORNATE_PANE_STYLE);

    m_ssFormatList.append("JPG", "PNG", "BMP", "TGA");
    GuiDropDownList* ssFormatList = ssPane->addDropDownList("Format", m_ssFormatList, &m_ssFormatIndex);
    m_ssFormatIndex = 0;

    ssFormatList->setWidth(width);
    ssFormatList->setCaptionSize(captionSize);

    label = ssPane->addLabel("Hot key:");
    label->setWidth(captionSize);
    ssPane->addLabel(m_ssHotKeyString)->moveRightOf(label);

    ssPane->pack();
    ssPane->setWidth(pane()->rect().width());

    ///////////////////////////////////////////////////////////////////////////////////

    pack();
    setRect(Rect2D::xywh(rect().x0(), rect().y0(), rect().width() + 5, rect().height() + 2));
}


void VideoRecordDialog::onPose (Array<PosedModelRef> &posedArray, Array< PosedModel2DRef > &posed2DArray) {
    GuiWindow::onPose(posedArray, posed2DArray);
    if (m_video.notNull() || m_screenshotPending) {
        posed2DArray.append(m_recorder);
    }
}


void VideoRecordDialog::onAI () {
    if (m_framesBox) {
        m_framesBox->setEnabled(m_enableMotionBlur);
    }
}


void VideoRecordDialog::startRecording() {
    debugAssert(m_video.isNull());

    // Create the video file
    VideoOutput::Settings settings = m_settingsTemplate[m_templateIndex];
    OSWindow* window = const_cast<OSWindow*>(OSWindow::current());
    settings.width = window->width();
    settings.height = window->height();

    if (m_halfSize) {
        settings.width /= 2;
        settings.height /= 2;
    }

    double kps = 1000;
    double baseRate = 1500;
    if (settings.codec == VideoOutput::CODEC_ID_WMV2) {
        // WMV is lower quality
        baseRate = 3000;
    }
    settings.bitrate = iRound(baseRate * kps * settings.width * settings.height / (640 * 480));
    settings.fps = m_playbackFPS;

    std::string filename = nextFilenameBase() + "." + m_settingsTemplate[m_templateIndex].extension;

    m_video = VideoOutput::create(filename, settings);

    // TODO: motion blur support

    if (m_app) {
        m_oldSimTimeStep = m_app->simTimeStep();
        m_oldDesiredFrameRate = m_app->desiredFrameRate();

        m_app->setSimTimeStep(1.0f / m_recordFPS);
        m_app->setDesiredFrameRate(m_recordFPS);
    }

    m_recordButton->setCaption("Stop (" + m_hotKeyString + ")");
    setVisible(false);

    // Change the window caption as well
    std::string c = window->caption();
    std::string appendix = " - Recording " + m_hotKeyString + " to stop";
    if (! endsWith(c, appendix)) {
        window->setCaption(c + appendix);
    }
}


void VideoRecordDialog::recordFrame(RenderDevice* rd) {
    debugAssert(m_video.notNull());

    bool useBackBuffer = ! m_captureGUI;

    // TODO: motion blur support
    // TODO: show cursor
    // TODO: half size
    if (m_halfSize) {
        // Half-size: downsample
        if (m_downsampleSrc.isNull()) {
            Texture::Settings settings = Texture::Settings::video();
            // Need bilinear for fast downsample
            settings.interpolateMode = Texture::BILINEAR_NO_MIPMAP;
            m_downsampleSrc = Texture::createEmpty("Downsample Source", 16, 16, TextureFormat::RGB8(), Texture::DIM_2D_NPOT, settings);
        }
        RenderDevice::ReadBuffer old = rd->readBuffer();
        m_downsampleSrc->copyFromScreen(Rect2D::xywh(0,0,rd->width(), rd->height()));
        rd->setReadBuffer(old);

        if (m_downsampleFBO.isNull()) {
            m_downsampleFBO = Framebuffer::create("Downsample Framebuffer");
        }
        if (m_downsampleDst.isNull() || 
            (m_downsampleDst->width() != m_downsampleSrc->width() / 2) || 
            (m_downsampleDst->height() != m_downsampleSrc->height() / 2)) {
            // Allocate destination
            m_downsampleDst = Texture::createEmpty("Downsample Destination", m_downsampleSrc->width() / 2, m_downsampleSrc->height() / 2, 
                ImageFormat::RGB8(), Texture::DIM_2D_NPOT, Texture::Settings::video());
            m_downsampleFBO->set(Framebuffer::COLOR0, m_downsampleDst);
        }
        // Downsample (use bilinear for fast filtering
        rd->push2D(m_downsampleFBO);
        {
            rd->setTexture(0, m_downsampleSrc);
            const Vector2& halfPixelOffset = Vector2(0.5f, 0.5f) / m_downsampleDst->vector2Bounds();
            Draw::fastRect2D(m_downsampleDst->rect2DBounds() + halfPixelOffset, rd);
        }
        rd->pop2D();

        // Write downsampled texture to the video
        m_video->append(m_downsampleDst);
    } else {
        // Full-size: grab directly from the screen
        m_video->append(rd, useBackBuffer);
    }

    //  Draw "REC" on the screen.
    rd->push2D();
    {
        if (! useBackBuffer && ! m_halfSize) {
            // Draw directly to the front buffer so that the message does not appear in the 
            // recording of the next frame.
            rd->setDrawBuffer(RenderDevice::DRAW_FRONT);
        }
        
        static RealTime t0 = System::time();
        bool toggle = isEven((int)((System::time() - t0) * 2));
        m_font->draw2D(rd, "REC", Vector2(rd->width() - 100, 5), 35, toggle ? Color3::black() : Color3::white(), Color3::black());
        m_font->draw2D(rd, m_hotKeyString + " to stop", Vector2(rd->width() - 100, 45), 16, Color3::white(), Color4(0,0,0,0.45f));
    }
    rd->pop2D();

    // TODO: stop if too many frames have been drawn
}


static void saveMessage(const std::string& filename) {
    debugPrintf("Saved %s\n", filename.c_str());
    logPrintf("Saved %s\n", filename.c_str());
    consolePrintf("Saved %s\n", filename.c_str());
}


void VideoRecordDialog::stopRecording() {
    debugAssert(m_video.notNull());

    // Save the movie
    m_video->commit();

    saveMessage(m_video->filename());
    m_video = NULL;

    if (m_app) {
        // Restore the app state
        m_app->setDesiredFrameRate(m_oldDesiredFrameRate);
        m_app->setSimTimeStep(m_oldSimTimeStep);
    }

    // Reset the GUI
    m_recordButton->setCaption("Record Now (" + m_hotKeyString + ")");

    // Restore the window caption as well
    OSWindow* window = const_cast<OSWindow*>(OSWindow::current());
    std::string c = window->caption();
    std::string appendix = " - Recording " + m_hotKeyString + " to stop";
    if (endsWith(c, appendix)) {
        window->setCaption(c.substr(0, c.size() - appendix.size()));
    }
}


bool VideoRecordDialog::onEvent(const GEvent& event) {
    if (GuiWindow::onEvent(event)) {
        // Base class handled the event
        return true;
    }

    if (enabled()) {
        // Video
        bool buttonClicked = (event.type == GEventType::GUI_ACTION) && (event.gui.control == m_recordButton);
        bool hotKeyPressed = (event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == m_hotKey) && (event.key.keysym.mod == m_hotKeyMod);

        // for debugging key events
        //if (event.type == GEventType::KEY_DOWN) {
        //    debugPrintf("F4 = %d, received %d, mod = %d, pressed = %d\n", (int)m_hotKey, (int)event.key.keysym.sym, (int)event.key.keysym.mod, hotKeyPressed);
        //}
        if (buttonClicked || hotKeyPressed) {
            if (m_video.notNull()) {
                stopRecording();
            } else {
                startRecording();
            }
            return true;
        }

        bool ssHotKeyPressed = (event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == m_ssHotKey) && (event.key.keysym.mod == m_ssHotKeyMod);

        if (ssHotKeyPressed) {
            takeScreenshot();
            return true;
        }

    }

    return false;
}


void VideoRecordDialog::takeScreenshot() {
    m_screenshotPending = true;
}


void VideoRecordDialog::maybeRecord(RenderDevice* rd) {
    if (m_video.notNull()) {
        recordFrame(rd);
    }

    if (m_screenshotPending) {
        screenshot(rd);
        m_screenshotPending = false;
    }
}


void VideoRecordDialog::screenshot(RenderDevice* rd) {
    GImage screen;

    rd->pushState();
    bool useBackBuffer = ! m_captureGUI;
    if (useBackBuffer) {
        rd->setReadBuffer(RenderDevice::READ_BACK);
    } else {
        rd->setReadBuffer(RenderDevice::READ_FRONT);
    }
    rd->screenshotPic(screen);
    rd->popState();

    std::string filename = nextFilenameBase() + "." + toLower(m_ssFormatList[m_ssFormatIndex]);
    screen.save(filename);
    saveMessage(filename);
}


void VideoRecordDialog::Recorder::render(RenderDevice* rd) const {
    dialog->maybeRecord(rd);
}

}
