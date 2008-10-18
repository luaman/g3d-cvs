/**
  @file VideoRecordDialog.cpp
  
  @maintainer Morgan McGuire, matrix@graphics3d.com

  @created 2008-10-18
  @edited  2008-10-18
 */ 
#include "G3D/platform.h"
#include "G3D/fileutils.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/VideoRecordDialog.h"
#include "GLG3D/GApp.h"


namespace G3D {

VideoRecordDialog::Ref VideoRecordDialog::create(const GuiThemeRef& theme, GApp* app) {
    return new VideoRecordDialog(theme, app);
}


VideoRecordDialog::Ref VideoRecordDialog::create(GApp* app) {
    alwaysAssertM(app, "GApp may not be NULL");
    return new VideoRecordDialog(app->debugWindow->theme(), app);
}


VideoRecordDialog::VideoRecordDialog(const GuiThemeRef& theme, GApp* app) : 
    GuiWindow("Record Video", theme, Rect2D::xywh(0, 100, 310, 200),
              GuiTheme::DIALOG_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE),
    m_app(app),
    m_templateIndex(0),
    m_playbackFPS(30),
    m_recordFPS(30),
    m_halfSize(false),
    m_enableMotionBlur(false),
    m_motionBlurFrames(10),
    m_framesBox(NULL),
    m_showCursor(false) {

    m_hotKey = GKey::F4;
    m_hotKeyMod = GKeyMod::NONE;
    m_hotKeyString = m_hotKey.toString();
 
    m_settingsTemplate.append(VideoOutput::Settings::MPEG4(640, 680));
    m_settingsTemplate.append(VideoOutput::Settings::WMV(640, 680));
    m_settingsTemplate.append(VideoOutput::Settings::AVI(640, 680));
    m_settingsTemplate.append(VideoOutput::Settings::rawAVI(640, 680));
    // Generates an error
    // m_settingsTemplate.append(VideoOutput::Settings::DV(640, 680));

    // Remove unsupported formats and build a drop-down list
    for (int i = 0; i < m_settingsTemplate.size(); ++i) {
        if (! VideoOutput::supports(m_settingsTemplate[i].codec)) {
            m_settingsTemplate.remove(i);
            --i;
        } else {
            m_formatList.append(m_settingsTemplate[i].description);
        }
    }

#   ifdef G3D_WIN32
    {
        // On Windows, default to raw AVI if available on this machine since
        // media player can't play MP4 files and WMV quality seems to be 
        // very low.
        for (int i = 0; i < m_settingsTemplate.size(); ++i) {
            if (m_settingsTemplate[i].codec == VideoOutput::CODEC_ID_RAWVIDEO) {
                m_templateIndex = i;
                break;
            }
        }
    }
#   endif

    m_filename = generateFilenameBase("movie-");
    GuiTextBox* filenameBox = pane()->addTextBox("Save as", &m_filename);
    GuiDropDownList* formatList = pane()->addDropDownList("Format", &m_templateIndex, &m_formatList);

    int width = 300;
    // Increase caption size to line up with the motion blur box
    int captionSize = 90;
    filenameBox->setWidth(width);
    filenameBox->setCaptionSize(captionSize);

    formatList->setWidth(width);
    formatList->setCaptionSize(captionSize);
    
    GuiCheckBox*  motionCheck = pane()->addCheckBox("Motion Blur",  &m_enableMotionBlur);
    m_framesBox   = pane()->addNumberBox("", &m_motionBlurFrames, "frames", true, 2, 20);
    m_framesBox->setUnitsSize(46);
    m_framesBox->moveRightOf(motionCheck);
    m_framesBox->setWidth(210);

    // TODO: Remove for future expansion
    motionCheck->setVisible(false);
    m_framesBox->setVisible(false);

    GuiNumberBox<float>* playbackBox = pane()->addNumberBox("Playback",    &m_playbackFPS, "fps", false, 1.0f, 120.0f, 0.1f);
    playbackBox->setCaptionSize(captionSize);

    GuiNumberBox<float>* recordBox   = pane()->addNumberBox("Record",      &m_recordFPS, "fps", false, 1.0f, 120.0f, 0.1f);
    recordBox->setCaptionSize(captionSize);

    const GWindow* window = GWindow::current();
    int w = window->width() / 2;
    int h = window->height() / 2;

    pane()->addCheckBox("Record GUI (PosedModel2D)", &m_captureGUI);

    // TODO: For future expansion
    // pane()->addCheckBox(format("Half-size (%d x %d)", w, h), &m_halfSize);
    // pane()->addCheckBox("Show cursor", &m_showCursor);    

    m_recordButton = pane()->addButton("Record (" + m_hotKeyString + ")");
    m_recordButton->moveBy(pane()->rect().width() - m_recordButton->rect().width() - 5, 0);

    m_font = GFont::fromFile(System::findDataFile("arial.fnt"));
    
    m_recorder = new Recorder();
    m_recorder->dialog = this;
}


void VideoRecordDialog::onPose (Array<PosedModelRef> &posedArray, Array< PosedModel2DRef > &posed2DArray) {
    GuiWindow::onPose(posedArray, posed2DArray);
    if (m_video.notNull()) {
        posed2DArray.append(m_recorder);
    }
}


void VideoRecordDialog::onLogic () {
    m_framesBox->setEnabled(m_enableMotionBlur);

    // Fix filename extension based on settings
    m_filename = filenameBase(m_filename) + "." + m_settingsTemplate[m_templateIndex].extension;
}


void VideoRecordDialog::startRecording() {
    debugAssert(m_video.isNull());

    // Create the video file
    VideoOutput::Settings settings = m_settingsTemplate[m_templateIndex];
    GWindow* window = const_cast<GWindow*>(GWindow::current());
    settings.width = window->width();
    settings.height = window->height();
    settings.bitrate = iRound((3000000.0 * 8 / 60) * (settings.width * settings.height) / (640 * 480));
    settings.fps = m_playbackFPS;

    m_video = VideoOutput::create(m_filename, settings);

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
    
    m_video->append(rd, useBackBuffer);

    //  Draw "REC" on the screen.
    rd->push2D();
    {
        if (! useBackBuffer) {
            // Draw directly to the front buffer so that the message does not appear in the 
            // recording of the next frame.
            rd->setDrawBuffer(RenderDevice::BUFFER_FRONT);
        }
        
        static RealTime t0 = System::time();
        bool toggle = isEven((int)((System::time() - t0) * 2));
        m_font->draw2D(rd, "REC", Vector2(rd->width() - 100, 5), 35, toggle ? Color3::black() : Color3::white(), Color3::black());
        m_font->draw2D(rd, m_hotKeyString + " to stop", Vector2(rd->width() - 100, 45), 16, Color3::white(), Color4(0,0,0,0.45f));
    }
    rd->pop2D();

    // TODO: stop if too many frames have been drawn
}


void VideoRecordDialog::stopRecording() {
    debugAssert(m_video.notNull());

    // Save the movie
    m_video->commit();
    debugPrintf("Saved %s\n", m_video->filename().c_str());
    m_video = NULL;

    // Make a new unique filename
    m_filename = generateFilenameBase("movie-");

    if (m_app) {
        // Restore the app state
        m_app->setDesiredFrameRate(m_oldDesiredFrameRate);
        m_app->setSimTimeStep(m_oldSimTimeStep);
    }

    // Reset the GUI
    m_recordButton->setCaption("Record (" + m_hotKeyString + ")");

    // Restore the window caption as well
    GWindow* window = const_cast<GWindow*>(GWindow::current());
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

    bool buttonClicked = (event.type == GEventType::GUI_ACTION) && (event.gui.control == m_recordButton);
    bool hotKeyPressed = (event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == m_hotKey) && (event.key.keysym.mod == m_hotKeyMod);
    if (buttonClicked || hotKeyPressed) {
        if (m_video.notNull()) {
            stopRecording();
        } else {
            startRecording();
        }
        return true;
    }

    return false;
}


void VideoRecordDialog::Recorder::render(RenderDevice* rd) const {
    if (dialog->m_video.notNull()) {
        dialog->recordFrame(rd);
    }
}

}
