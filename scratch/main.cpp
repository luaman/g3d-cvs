/**
  @file empty/main.cpp

  This is a sample main.cpp to get you started with G3D.  It is
  designed to make writing an application easy.  Although the GApp
  infrastructure is helpful for most projects, you are not restricted
  to using it--choose the level of support that is best for your
  project.  You can also customize GApp through its members and change
  its behavior by overriding methods.

  @author Morgan McGuire, morgan@cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

#if defined(G3D_VER) && (G3D_VER < 70000)
#   error Requires G3D 7.00
#endif


/**********************************************************/
/**
  @file VideoRecordDialog.h

  @maintainer Morgan McGuire, morgan@cs.williams.edu

  @created 2002-07-28
  @edited  2008-07-14
*/
#ifndef G3D_VideoRecordDialog_h
#define G3D_VideoRecordDialog_h

#include "G3D/platform.h"
#include "GLG3D/Widget.h"
#include "GLG3D/UprightSplineManipulator.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiLabel.h"
#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/GuiDropDownList.h"
#include "GLG3D/GuiTextBox.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/FirstPersonManipulator.h"

namespace G3D {

/**
   @brief A dialog that allows the user to launch recording of the
   on-screen image to a movie.

   The playback rate is the frames-per-second value to be stored in
   the movie file.  The record rate 1/G3D::GApp::simTimeStep.
 */
class VideoRecordDialog : public GuiWindow {
public:

    typedef ReferenceCountedPointer<class VideoRecordDialog> Ref;

protected:
    GApp*                        m_app;

    Array<VideoOutput::Settings> m_settingsTemplate;

    /** Parallel array to m_settingsTemplate of the descriptions for
        use with a drop-down list. */
    Array<std::string>           m_formatList;

    /** Index into m_settingsTemplate and m_formatList */
    int                          m_templateIndex;

    std::string                  m_filename;

    float                        m_playbackFPS;
    float                        m_recordFPS;

    bool                         m_halfSize;
    bool                         m_enableMotionBlur;
    int                          m_motionBlurFrames;

    /** Recording modifies the GApp::simTimeStep; this is the old value */
    float                        m_oldSimTimeStep;
    float                        m_oldDesiredFrameRate;
    
    /** Motion blur frames */
    GuiNumberBox<int>*           m_framesBox;

    /**
       When false, the screen is captured at the beginning of 
       Posed2DModel rendering from the back buffer, which may 
       slow down rendering.
       
       When true, the screen is captured from the the previous 
       frame, which will not introduce latency into rendering.
    */
    bool                         m_captureGUI;

    /** Draw a software cursor on the frame after capture, since the
     hardware cursor will not be visible.*/
    bool                         m_showCursor;

    GuiButton*                   m_recordButton;

    /** Key to start/stop recording even when the GUI is not
        visible.
      */
    GKey                         m_hotKey;
    GKeyMod                      m_hotKeyMod;

    /** Hotkey + mod as a human readable string */
    std::string                  m_hotKeyString;

    /** Non-NULL while recording */
    VideoOutput::Ref             m_video;

    VideoRecordDialog(const GuiThemeRef& theme, GApp* app);

public:

    /**
       @param app If not NULL, the VideoRecordDialog will set the app's
       simTimeStep.
     */
    static Ref create(const GuiThemeRef& theme, GApp* app = NULL);
    static Ref create(GApp* app);

    /** Automatically invoked when the record button is pressed. */
    void startRecording();
    void recordFrame(RenderDevice* rd);
    void stopRecording();

    virtual void onLogic();
    /*    virtual bool onEvent(const GEvent& event);
          virtual void onUserInput(UserInput*);*/
};

}

#endif
/*********************************************************/

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
    m_settingsTemplate.append(VideoOutput::Settings::DV(640, 680));

    // Remove unsupported formats and build a drop-down list
    for (int i = 0; i < m_settingsTemplate.size(); ++i) {
        if (! VideoOutput::supports(m_settingsTemplate[i].codec)) {
            m_settingsTemplate.remove(i);
            --i;
        } else {
            m_formatList.append(m_settingsTemplate[i].description);
        }
    }

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

    GuiNumberBox<float>* playbackBox = pane()->addNumberBox("Playback",    &m_playbackFPS, "fps", false, 1.0f, 120.0f, 0.1f);
    playbackBox->setCaptionSize(captionSize);

    GuiNumberBox<float>* recordBox   = pane()->addNumberBox("Record",      &m_recordFPS, "fps", false, 1.0f, 120.0f, 0.1f);
    recordBox->setCaptionSize(captionSize);

    const GWindow* window = GWindow::current();
    int w = window->width() / 2;
    int h = window->height() / 2;

    pane()->addCheckBox("Record GUI (PosedModel2D)", &m_captureGUI);

    pane()->addCheckBox(format("Half-size (%d x %d)", w, h), &m_halfSize);
    pane()->addCheckBox("Show cursor", &m_showCursor);

    m_recordButton = pane()->addButton("Record (" + m_hotKeyString + ")");
    m_recordButton->moveBy(pane()->rect().width() - m_recordButton->rect().width() - 5, 0);
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
    const GWindow* window = GWindow::current();
    settings.width = window->width();
    settings.height = window->height();
    if (settings.codec == VideoOutput::CODEC_ID_MPEG4) {
        settings.bitrate = (3000000 * 8 / 60) * (settings.width * settings.height) / (640 * 480);
    }
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
}


void VideoRecordDialog::recordFrame(RenderDevice* rd) {
    debugAssert(m_video.notNull());

    bool useBackBuffer = ! m_captureGUI;
    
    m_video->append(rd, useBackBuffer);

}


void VideoRecordDialog::stopRecording() {
    debugAssert(m_video.notNull());

    // Save the movie
    m_video->commit();
    m_video = NULL;

    // Make a new unique filename
    m_filename = generateFilenameBase("movie-");

    // Reset the GUI
    m_recordButton->setCaption("Record (" + m_hotKeyString + ")");
}











class App : public GApp {
public:
    LightingRef         lighting;
    SkyParameters       skyParameters;
    SkyRef              sky;
    BSPMapRef           map;
    VideoOutput::Ref    video;

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onLogic();
    virtual void onNetwork();
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
    virtual void onPose(Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D);
    virtual void onGraphics(RenderDevice* rd, Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D);
    virtual void onUserInput(UserInput* ui);
    virtual void onCleanup();
    virtual void onConsoleCommand(const std::string& cmd);
    void printConsoleHelp();
};

App::App(const GApp::Settings& settings) : GApp(settings) {
    catchCommonExceptions = false;
}


void App::onInit() {

    {
        Array<std::string> s;
        VideoOutput::getSupportedCodecs(s);
        printf ("Supported Codecs:\n");
        for (int i = 0; i < s.size(); ++i) {
            printf("  %s\n", s[i].c_str());
        }
    }

    setDesiredFrameRate(60);

//	map = BSPMap::fromFile("D:/morgan/data/quake3/AriaDeCapo/ariadecapo.pk3", "ariadecapo.bsp");
//	map = BSPMap::fromFile("D:/morgan/data/quake3/charon/map-charon3dm11v2.pk3", "charon3dm11v2.bsp");

    sky = Sky::fromFile(System::findDataFile("sky"));


    skyParameters = SkyParameters(G3D::toSeconds(11, 00, 00, AM));
    lighting = Lighting::fromSky(sky, skyParameters, Color3::white());

    // This simple demo has no shadowing, so make all lights unshadowed
    lighting->lightArray.append(lighting->shadowedLightArray);
    lighting->shadowedLightArray.clear();

    toneMap->setEnabled(false);

    static std::string text = "hi";
    debugPane->addTextBox("Text", &text);    // Indent and display a caption
    debugPane->addTextBox(" ", &text);       // Indent, but display no caption
    debugPane->addTextBox("", &text);        // Align the text box to the left
    debugWindow->setVisible(true);

    static float f = 0.5f;

    float low = 0.0f;
    float high = 100.0f;

    debugPane->addNumberBox("Log", &f, "s", GuiTheme::LOG_SLIDER, low, high);
    debugPane->addNumberBox("Linear", &f, "s", GuiTheme::LINEAR_SLIDER, low, high);


    static Array<std::string> list;
    list.append("First");
    for (int i = 0; i < 10; ++i) {
        list.append(format("Item %d", i + 2));
    }
    list.append("Last");
    static int index = 0;
    debugPane->addDropDownList("List", &index, &list);

    // NumberBox: textbox for numbers: label, ptr, suffix, slider?, min, max, roundToNearest
    // defaults:                        /   , ptr, "", false, -inf, inf, 0

    addWidget(VideoRecordDialog::create(this));
}


void App::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught
}

void App::onLogic() {
    // Add non-simulation game logic and AI code here
    if (video.notNull()) {
        static GImage frame;
        renderDevice->screenshotPic(frame);
        video->append(frame);
        screenPrintf("RECORDING");
    }
}

void App::onNetwork() {
    // Poll net messages here
}

void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    // Add physical simulation here.  You can make your time advancement
    // based on any of the three arguments.
}

void App::onUserInput(UserInput* ui) {
    if (ui->keyPressed(' ') && video.isNull()) {
        VideoOutput::Settings s = VideoOutput::Settings::rawAVI(window()->width(), window()->height());
        video = VideoOutput::create("test.avi", s);
    } else if (ui->keyPressed('x') && video.notNull()) {
        video->commit();
        video = NULL;
    } else if (ui->keyPressed('p')) {
        GImage im;
        renderDevice->screenshotPic(im);
        im.save("test.png");
        exit(0);
    }
}

void App::onConsoleCommand(const std::string& str) {
    // Add console processing here

    TextInput t(TextInput::FROM_STRING, str);
    if (t.hasMore() && (t.peek().type() == Token::SYMBOL)) {
        std::string cmd = toLower(t.readSymbol());
        if (cmd == "exit") {
            exit(0);
            return;
        } else if (cmd == "help") {
            printConsoleHelp();
            return;
        }

        // Add commands here
    }

    console->printf("Unknown command\n");
    printConsoleHelp();
}

void App::printConsoleHelp() {
    console->printf("exit          - Quit the program\n");
    console->printf("help          - Display this text\n\n");
    console->printf("~/ESC         - Open/Close console\n");
    console->printf("TAB           - Enable first-person camera control\n");
}

void App::onPose(Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {
    // Append any models to the array that you want rendered by onGraphics

}

void App::onGraphics(RenderDevice* rd, Array<PosedModelRef>& posed3D, Array<PosedModel2DRef>& posed2D) {
    Array<PosedModel::Ref>        opaque, transparent;
    LightingRef   localLighting = toneMap->prepareLighting(lighting);
    SkyParameters localSky      = toneMap->prepareSkyParameters(skyParameters);
    
    rd->setProjectionAndCameraMatrix(defaultCamera);

    rd->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));
    rd->clear(false, true, true);
    sky->render(rd, localSky);

    if (map.notNull()) {
        map->render(rd, defaultCamera);
    //    Draw::box(map->bounds(), rd);
    }
    PosedModel::sortAndRender(rd, defaultCamera, posed3D, localLighting);

    sky->renderLensFlare(rd, localSky);

    PosedModel2D::sortAndRender(rd, posed2D);

    // TODO: Draw "recording" on the screen
    rd->push2D();
        GFontRef font = GFont::fromFile("arial.fnt");
        /*
        if (! useBackBuffer) {
            rd->setDrawBuffer(RenderDevice::BUFFER_FRONT);
        }*/

        font->draw2D(rd, "REC", Vector2(rd->width() - 100, 5), 30, Color3::red());
    rd->pop2D();
}


G3D_START_AT_MAIN();



int main(int argc, char** argv) {
/*
    RenderDevice* rd = new RenderDevice();
    rd->init();
    GuiTheme::makeThemeFromSourceFiles("C:/Projects/data/source/themes/osx/", "white.png", "black.png", "osx.txt", "C:/Projects/G3D/data-files/gui/osx.skn");
//    return 0;
    */

    GApp::Settings set;
//    set.window.width = 1440;
//    set.window.height = 900;
//    set.window.fsaaSamples = 4;
    return App(set).run();
}
