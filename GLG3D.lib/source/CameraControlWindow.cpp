/**
  @file CameraControlWindow.cpp

  @maintainer Morgan McGuire, morgan@cs.williams.edu

  @created 2007-06-01
  @edited  2009-09-14
*/
#include "G3D/platform.h"
#include "G3D/GCamera.h"
#include "G3D/prompt.h"
#include "G3D/Rect2D.h"
#include "G3D/fileutils.h"
#include "G3D/AnyVal.h"
#include "GLG3D/CameraControlWindow.h"
#include "GLG3D/FileDialog.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

enum {FILM_PANE_SIZE = 60};
const Vector2 CameraControlWindow::smallSize(286 + 16, 46);
const Vector2 CameraControlWindow::bigSize(286 + 16, 155 + FILM_PANE_SIZE);

static const std::string noSpline = "< None >";
static const std::string untitled = "< Unsaved >";

#ifdef DELETE
#    undef DELETE
#endif

typedef ReferenceCountedPointer<class BookmarkDialog> BookmarkDialogRef;
class BookmarkDialog : public GuiWindow {
public:
    enum Result {OK, CANCEL, DELETE};

    bool               ok;

    Result&            m_result;
    std::string&       m_name;
    std::string        m_originalName;
    GuiButton*         m_okButton;
    GuiButton*         m_deleteButton;

    /** Name */
    GuiTextBox*        m_textBox;

    OSWindow*           m_osWindow;
    
    BookmarkDialog(OSWindow* osWindow, const Vector2& position, GuiThemeRef skin, 
                   std::string& name, Result& result,
                   const std::string& note) : 
        GuiWindow("Bookmark Properties", skin, Rect2D::xywh(position - Vector2(160, 0), Vector2(300, 100)), 
                  GuiTheme::DIALOG_WINDOW_STYLE, GuiWindow::NO_CLOSE),
        m_result(result),
        m_name(name),
        m_originalName(name) {

        m_textBox = pane()->addTextBox("Name", &name, GuiTextBox::IMMEDIATE_UPDATE);
        
        GuiLabel* loc = pane()->addLabel("Location");
        loc->setWidth(84);
        GuiLabel* locDisplay = pane()->addLabel(note);
        locDisplay->moveRightOf(loc);

        m_okButton = pane()->addButton("Ok");
        m_okButton->moveBy(130, 20);
        m_deleteButton = pane()->addButton("Delete");
        m_deleteButton->moveRightOf(m_okButton);
        setRect(Rect2D::xywh(rect().x0y0(), Vector2::zero()));
        pane()->setSize(0, 0);
        pane()->pack();
        sync();

        m_textBox->setFocused(true);
    }

protected:

    void close(Result r) {
        setVisible(false);
        m_manager->remove(this);
        m_result = r;
    }

    /** Update enables/captions */
    void sync() {
        if ((m_originalName != m_name) || (m_name == "")) {
            m_deleteButton->setCaption("Cancel");
        }

        m_okButton->setEnabled(m_name != "");
    }

public:

    virtual bool onEvent(const GEvent& e) {
        if (GuiWindow::onEvent(e)) {
            return true;
        }

        sync();

        if (e.type == GEventType::GUI_ACTION) {
            if (e.gui.control == m_okButton) {
                close(OK);
                return true;
            } else if (e.gui.control == m_deleteButton) {
                if (m_deleteButton->caption().text() == "Cancel") {
                    close(CANCEL);
                } else {
                    close(DELETE);
                }
                return true;
            }
        }

        if ((e.type == GEventType::KEY_DOWN) && (e.key.keysym.sym == GKey::ESCAPE)) {
            close(CANCEL);
            return true;
        }

        return false;
    }

};

static bool hasRoll = false;

CameraControlWindow::CameraControlWindow(
    const FirstPersonManipulatorRef&      manualManipulator, 
    const UprightSplineManipulatorRef&    trackManipulator, 
    const Pointer<Manipulator::Ref>&      cameraManipulator,
    const Film::Ref&                      film,
    const GuiThemeRef&                    skin) : 
    GuiWindow("Camera Control", 
              skin, 
              Rect2D::xywh(5, 54, 200, 0),
              GuiTheme::TOOL_WINDOW_STYLE,
              GuiWindow::HIDE_ON_CLOSE),
    trackFileIndex(0),
    m_bookmarkSelection(NO_BOOKMARK),
    cameraManipulator(cameraManipulator),
    manualManipulator(manualManipulator),
    trackManipulator(trackManipulator),
    drawerButton(NULL),
    drawerButtonPane(NULL),
    m_expanded(false)
{

    manualOperation = manualManipulator->active();

    updateTrackFiles();

    GuiPane* pane = GuiWindow::pane();

    const char* DOWN = "6";
    const char* CHECK = "\x98";
    const char* CLIPBOARD = "\xA4";
    float w = 18;
    float h = 20;
    GFontRef iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));
    GFontRef greekFont = GFont::fromFile(System::findDataFile("greek.fnt"));

    // The default G3D textbox label doesn't support multiple fonts
    if (hasRoll) {
        pane->addLabel(GuiText("q q q", greekFont, 12))->setRect(Rect2D::xywh(19, 6, 10, 15));
        pane->addLabel(GuiText("y  x  z", NULL, 9))->setRect(Rect2D::xywh(24, 12, 10, 9));
    } else {
        pane->addLabel(GuiText("q q", greekFont, 12))->setRect(Rect2D::xywh(19, 6, 10, 15));
        pane->addLabel(GuiText("y  x", NULL, 9))->setRect(Rect2D::xywh(24, 12, 10, 9));
    }
    cameraLocationTextBox = 
        pane->addTextBox("xyz",
        Pointer<std::string>(this, &CameraControlWindow::cameraLocation, 
                                   &CameraControlWindow::setCameraLocation));
    cameraLocationTextBox->setRect(Rect2D::xywh(0, 2, 246 + (hasRoll ? 20.0f : 0.0f), 24));
    cameraLocationTextBox->setCaptionSize(38 + (hasRoll ? 12.0f : 0.0f));

    // Change to black "r" (x) for remove
    GuiButton* bookMarkButton = 
        pane->addButton(GuiText(CHECK, iconFont, 16, Color3::blue() * 0.8f), 
            GuiControl::Callback(this, &CameraControlWindow::onBookmarkButton),
            GuiTheme::TOOL_BUTTON_STYLE);
    bookMarkButton->setSize(w, h);
    bookMarkButton->moveRightOf(cameraLocationTextBox);
    bookMarkButton->moveBy(-2, 2);

    m_showBookmarksButton = pane->addButton(GuiText(DOWN, iconFont, 18), 
        GuiTheme::TOOL_BUTTON_STYLE);

    m_showBookmarksButton->setSize(w, h);

    GuiButton* copyButton = pane->addButton(GuiText(CLIPBOARD, iconFont, 16), GuiControl::Callback(this, &CameraControlWindow::copyToClipboard), GuiTheme::TOOL_BUTTON_STYLE);
    copyButton->setSize(w, h);

    /////////////////////////////////////////////////////////////////////////////////////////

    GuiPane* filmPane = pane->addPane();
    filmPane->moveBy(-9, 0);
    {
        GuiNumberBox<float>* gamma = NULL;
        GuiNumberBox<float>* exposure = NULL;
        float maxExposure = 10.0f;
        if (film.notNull()) {
            gamma = filmPane->addNumberBox("Gamma", Pointer<float>(film, &Film::gamma, &Film::setGamma), 
                                           "", GuiTheme::LOG_SLIDER, 0.5f, 7.0f, 0.001f);
            gamma->moveBy(0, 2);
            exposure = filmPane->addNumberBox("Exposure",      
                                              Pointer<float>(film, &Film::exposure, &Film::setExposure), 
                                              "", GuiTheme::LOG_SLIDER, 0.001f, maxExposure);
        } else {
            static float g = 1.0f;
            static float e = 1.0f;
            gamma = filmPane->addNumberBox("Gamma", &g, "", GuiTheme::LOG_SLIDER, 0.5f, 7.0f, 0.001f);
            gamma->moveBy(0, 2);
            exposure = filmPane->addNumberBox("Exposure", &e, 
                                              "", GuiTheme::LOG_SLIDER, 0.001f, maxExposure);
            gamma->setEnabled(false);
            exposure->setEnabled(false);            
        }
        gamma->setWidth(290);
        exposure->setWidth(290);
    }
    /////////////////////////////////////////////////////////////////////////////////////////

    GuiPane* manualPane = pane->addPane();
    manualPane->moveBy(-8, 0);

    manualPane->addCheckBox("Manual Control (F2)", &manualOperation)->moveBy(-2, 3);

    trackList = manualPane->addDropDownList("Path", trackFileArray, &trackFileIndex);
    trackList->setRect(Rect2D::xywh(Vector2(0, trackList->rect().y1() - 25), Vector2(180, trackList->rect().height())));
    trackList->setCaptionSize(34);

    visibleCheckBox = manualPane->addCheckBox("Visible", 
        Pointer<bool>(trackManipulator, 
                      &UprightSplineManipulator::showPath, 
                      &UprightSplineManipulator::setShowPath));

    visibleCheckBox->moveRightOf(trackList);
    visibleCheckBox->moveBy(6, 0);
    
    Vector2 buttonSize = Vector2(20, 20);
    recordButton = manualPane->addRadioButton
        (GuiText::Symbol::record(), 
         UprightSplineManipulator::RECORD_KEY_MODE, 
         trackManipulator.pointer(),
         &UprightSplineManipulator::mode,
         &UprightSplineManipulator::setMode,
         GuiTheme::TOOL_RADIO_BUTTON_STYLE);
    recordButton->moveBy(32, 2);
    recordButton->setSize(buttonSize);
    
    playButton = manualPane->addRadioButton
        (GuiText::Symbol::play(), 
         UprightSplineManipulator::PLAY_MODE, 
         trackManipulator.pointer(),
         &UprightSplineManipulator::mode,
         &UprightSplineManipulator::setMode,
         GuiTheme::TOOL_RADIO_BUTTON_STYLE);
    playButton->setSize(buttonSize);

    stopButton = manualPane->addRadioButton
        (GuiText::Symbol::stop(), 
         UprightSplineManipulator::INACTIVE_MODE, 
         trackManipulator.pointer(),
         &UprightSplineManipulator::mode,
         &UprightSplineManipulator::setMode,
         GuiTheme::TOOL_RADIO_BUTTON_STYLE);
    stopButton->setSize(buttonSize);

    saveButton = manualPane->addButton("Save...");
    saveButton->moveRightOf(stopButton);
    saveButton->setSize(saveButton->rect().wh() - Vector2(20, 1));
    saveButton->moveBy(20, -3);
    saveButton->setEnabled(false);

    cyclicCheckBox = manualPane->addCheckBox("Cyclic", 
        Pointer<bool>(trackManipulator, 
                      &UprightSplineManipulator::cyclic, 
                      &UprightSplineManipulator::setCyclic));

    cyclicCheckBox->setPosition(visibleCheckBox->rect().x0(), saveButton->rect().y0() + 1);

#   ifdef G3D_OSX
        manualHelpCaption = GuiText("W,A,S,D and shift+left mouse to move.", NULL, 10);
#   else
        manualHelpCaption = GuiText("W,A,S,D and right mouse to move.", NULL, 10);
#   endif

    autoHelpCaption = "";
    playHelpCaption = "";

    recordHelpCaption = GuiText("Spacebar to place a control point.", NULL, 10);

    helpLabel = manualPane->addLabel(manualHelpCaption);
    helpLabel->moveBy(0, 2);

    manualPane->pack();
    filmPane->setWidth(manualPane->rect().width());
    pack();
    // Set the width here so that the client rect is correct below
    setRect(Rect2D::xywh(rect().x0y0(), bigSize));

    // Make the pane width match the window width
    manualPane->setPosition(0, manualPane->rect().y0());
    manualPane->setSize(clientRect().width(), manualPane->rect().height());

    // Have to create the drawerButton last, otherwise the setRect
    // code for moving it to the bottom of the window will cause
    // layout to become broken.
    drawerCollapseCaption = GuiText("5", iconFont);
    drawerExpandCaption = GuiText("6", iconFont);
    drawerButtonPane = pane->addPane("", GuiTheme::NO_PANE_STYLE);
    drawerButton = drawerButtonPane->addButton(drawerExpandCaption, GuiTheme::TOOL_BUTTON_STYLE);
    drawerButton->setRect(Rect2D::xywh(0, 0, 12, 10));
    drawerButtonPane->setSize(12, 10);
    
    // Resize the pane to include the drawer button so that it is not clipped
    pane->setSize(clientRect().wh());

    setBookmarkFile("g3d-bookmarks.txt");        
    if (m_bookmarkName.size() == 0) {
        // Make a default home bookmark
        m_bookmarkName.append("Home");
        m_bookmarkPosition.append(CoordinateFrame::fromXYZYPRDegrees(0,1,7,0,-15,0));
    }

    setRect(Rect2D::xywh(rect().x0y0(), smallSize));
    sync();
}


CameraControlWindow::Ref CameraControlWindow::create(
    const FirstPersonManipulatorRef&   manualManipulator,
    const UprightSplineManipulatorRef& trackManipulator,
    const Pointer<Manipulator::Ref>&   cameraManipulator,
    const Film::Ref&                   film,
    const GuiThemeRef&                 skin) {

    return new CameraControlWindow(manualManipulator, trackManipulator, cameraManipulator, film, skin);
}


void CameraControlWindow::setManager(WidgetManager* manager) {
    GuiWindow::setManager(manager);
    if (manager) {
        // Move to the upper right
        float osWindowWidth = manager->window()->dimensions().width();
        setRect(Rect2D::xywh(osWindowWidth - rect().width(), 40, rect().width(), rect().height()));
    }
}


std::string CameraControlWindow::cameraLocation() const {
    CoordinateFrame cframe;
    trackManipulator->camera()->getCoordinateFrame(cframe);
    UprightFrame uframe(cframe);
    
    // \xba is the character 186, which is the degree symbol
    return format("(% 5.1f, % 5.1f, % 5.1f), % 5.1f\xba, % 5.1f\xba", 
                  uframe.translation.x, uframe.translation.y, uframe.translation.z, 
                  toDegrees(uframe.yaw), toDegrees(uframe.pitch));
}


std::string CameraControlWindow::cameraLocationCode() const {
    CoordinateFrame cframe;
    trackManipulator->camera()->getCoordinateFrame(cframe);
    UprightFrame uframe(cframe);
    
    return format("CFrame::fromXYZYPRDegrees(% 5.1ff, % 5.1ff, % 5.1ff, % 5.1ff, % 5.1ff, % 5.1ff)", 
                  uframe.translation.x, uframe.translation.y, uframe.translation.z, 
                  toDegrees(uframe.yaw), toDegrees(uframe.pitch), 0.0f);
}



void CameraControlWindow::setCameraLocation(const std::string& s) {
    TextInput t(TextInput::FROM_STRING, s);
    try {
        UprightFrame uframe;
        Token first = t.peek();
        if (first.string() == "CFrame") {
            // Code version
            t.readSymbols("CFrame", "::", "fromXYZYPRDegrees");
            t.readSymbol("(");
            uframe.translation.x = t.readNumber();
            t.readSymbol(",");
            uframe.translation.y = t.readNumber();
            t.readSymbol(",");
            uframe.translation.z = t.readNumber();
            t.readSymbol(",");
            uframe.yaw = toRadians(t.readNumber());
            t.readSymbol(",");
            uframe.pitch = toRadians(t.readNumber());
        } else {
            // Pretty-printed version
            uframe.translation.deserialize(t);
            t.readSymbol(",");
            uframe.yaw = toRadians(t.readNumber());
            std::string DEGREE = "\xba";
            if (t.peek().string() == DEGREE) {
                t.readSymbol();
            }
            t.readSymbol(",");
            uframe.pitch = toRadians(t.readNumber());
            if (t.peek().string() == DEGREE) {
                t.readSymbol();
            }
        }        
        CoordinateFrame cframe = uframe;

        trackManipulator->camera()->setCoordinateFrame(cframe);
        manualManipulator->setFrame(cframe);

    } catch (const TextInput::TokenException& e) {
        // Ignore the incorrectly formatted value
        (void)e;
    }
}


void CameraControlWindow::copyToClipboard() {
    System::setClipboardText(cameraLocationCode());
}


void CameraControlWindow::showBookmarkList() {
    if (m_bookmarkName.size() > 0) {
        m_menu = GuiMenu::create(theme(), &m_bookmarkName, &m_bookmarkSelection);
        manager()->add(m_menu);
        m_menu->show(manager(), this, NULL, cameraLocationTextBox->toOSWindowCoords(cameraLocationTextBox->clickRect().x0y1() + Vector2(45, 8)), false);
    }
}


void CameraControlWindow::onBookmarkButton() {
    std::string name;
    BookmarkDialog::Result result = BookmarkDialog::CANCEL;

    BookmarkDialogRef dialog = 
        new BookmarkDialog(m_manager->window(), rect().center() + Vector2(0, 100), 
                           theme(), name, result, cameraLocation());

    dialog->showModal(m_manager->window());

    dialog = NULL;

    switch (result) {
    case BookmarkDialog::CANCEL:
        break;

    case BookmarkDialog::OK:
        {
            CoordinateFrame frame;
            trackManipulator->camera()->getCoordinateFrame(frame);
            setBookmark(name, frame);
        }
        break;

    case BookmarkDialog::DELETE:
        removeBookmark(name);
        break;
    }
}


void CameraControlWindow::saveBookmarks() {
    AnyVal all(AnyVal::TABLE);
    for (int i = 0; i < m_bookmarkName.size(); ++i) {
        all[m_bookmarkName[i]] = m_bookmarkPosition[i];
    }
    all.save(m_bookmarkFilename);
}


void CameraControlWindow::setBookmarkFile(const std::string& filename) {
    m_bookmarkPosition.clear();
    m_bookmarkName.clear();
    m_bookmarkFilename = filename;

    if (fileExists(m_bookmarkFilename)) {
        // Load bookmarks
        AnyVal all;
        try {
            all.load(m_bookmarkFilename);
            if (all.type() != AnyVal::TABLE) {
                throw std::string("Was not a table");
            }
        } catch (...) {
            msgBox(m_bookmarkFilename + " is corrupt.");
            return;
        }

        all.getKeys(m_bookmarkName);
        m_bookmarkPosition.resize(m_bookmarkName.size());
        for (int i = 0; i < m_bookmarkName.size(); ++i) {
            m_bookmarkPosition[i] = all.get(m_bookmarkName[i], CoordinateFrame()).coordinateFrame();
        }
    }
}


void CameraControlWindow::setBookmark(const std::string& name, 
                                      const CoordinateFrame& frame) {
    for (int i = 0; i < m_bookmarkName.size(); ++i) {
        if (m_bookmarkName[i] == name) {
            m_bookmarkPosition[i] = frame;
            saveBookmarks();
            return;
        }
    }

    m_bookmarkName.append(name);
    m_bookmarkPosition.append(frame);
    saveBookmarks();
}


void CameraControlWindow::removeBookmark(const std::string& name) {
    for (int i = 0; i < m_bookmarkName.size(); ++i) {
        if (m_bookmarkName[i] == name) {
            m_bookmarkName.remove(i);
            m_bookmarkPosition.remove(i);
            saveBookmarks();
            return;
        }
    }
}


CoordinateFrame CameraControlWindow::bookmark
(const std::string& name, 
 const CoordinateFrame& defaultValue) const {

    for (int i = 0; i < m_bookmarkName.size(); ++i) {
        if (m_bookmarkName[i] == name) {
            return m_bookmarkPosition[i];
        }
    }
    return defaultValue;
}


void CameraControlWindow::setRect(const Rect2D& r) {
    GuiWindow::setRect(r);
    if (drawerButtonPane) {
        const Rect2D& r = clientRect();
        drawerButtonPane->setPosition((r.width() - drawerButtonPane->rect().width()) / 2.0f, r.height() - drawerButtonPane->rect().height());
    }
}


void CameraControlWindow::updateTrackFiles() {
    trackFileArray.fastClear();
    trackFileArray.append(noSpline);
    getFiles("*.trk", trackFileArray);

    // Element 0 is <unsaved>, so skip it
    for (int i = 1; i < trackFileArray.size(); ++i) {
        trackFileArray[i] = trackFileArray[i].substr(0, trackFileArray[i].length() - 4);
    }
    trackFileIndex = iMin(trackFileArray.size() - 1, trackFileIndex);
}


void CameraControlWindow::onUserInput(UserInput* ui) {
    GuiWindow::onUserInput(ui);

    if (manualOperation && (trackManipulator->mode() == UprightSplineManipulator::PLAY_MODE)) {
        // Keep the FPS controller in sync with the spline controller
        CoordinateFrame cframe;
        trackManipulator->getFrame(cframe);
        manualManipulator->setFrame(cframe);
        trackManipulator->camera()->setCoordinateFrame(cframe);
    }

    if (m_bookmarkSelection != NO_BOOKMARK) {
        // Clicked on a bookmark
        const CoordinateFrame& cframe = m_bookmarkPosition[m_bookmarkSelection];
        trackManipulator->camera()->setCoordinateFrame(cframe);
        manualManipulator->setFrame(cframe);

        m_bookmarkSelection = NO_BOOKMARK;
        // TODO: change "bookmark" caption to "edit"
    }
}


bool CameraControlWindow::onEvent(const GEvent& event) {

    // Allow super class to process the event
    if (GuiWindow::onEvent(event)) {
        return true;
    }
    
    // Accelerator key for toggling camera control.  Active even when the window is hidden.
    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::F2)) {
        manualOperation = ! manualOperation;
        sync();
        return true;
    }

    if (! visible()) {
        return false;
    }

    // Special buttons
    if (event.type == GEventType::GUI_ACTION) {
        GuiControl* control = event.gui.control;

        if ((control == m_showBookmarksButton) && (m_menu.isNull() || ! m_menu->visible())) {
            showBookmarkList();
            return true;
        } else if (control == drawerButton) {

            // Change the window size
            m_expanded = ! m_expanded;
            morphTo(Rect2D::xywh(rect().x0y0(), m_expanded ? bigSize : smallSize));
            drawerButton->setCaption(m_expanded ? drawerCollapseCaption : drawerExpandCaption);

        } else if (control == trackList) {
            
            if (trackFileArray[trackFileIndex] != untitled) {
                // Load the new spline
                loadSpline(trackFileArray[trackFileIndex] + ".trk");

                // When we load, we lose our temporarily recorded spline,
                // so remove that display from the menu.
                if (trackFileArray.last() == untitled) {
                    trackFileArray.remove(trackFileArray.size() - 1);
                }
            }

        } else if (control == playButton) {

            // Take over manual operation
            manualOperation = true;
            // Restart at the beginning of the path
            trackManipulator->setTime(0);

        } else if ((control == recordButton) || (control == cameraLocationTextBox)) {

            // Take over manual operation and reset the recording
            manualOperation = true;
            trackManipulator->clear();
            trackManipulator->setTime(0);

            // Select the untitled path
            if ((trackFileArray.size() == 0) || (trackFileArray.last() != untitled)) {
                trackFileArray.append(untitled);
            }
            trackFileIndex = trackFileArray.size() - 1;

            saveButton->setEnabled(true);

        } else if (control == saveButton) {

            // Save
            std::string saveName;

            if (FileDialog::create(this)->getFilename(saveName)) {
                saveName = filenameBaseExt(trimWhitespace(saveName));

                if (saveName != "") {
                    saveName = saveName.substr(0, saveName.length() - filenameExt(saveName).length());

                    BinaryOutput b(saveName + ".trk", G3D_LITTLE_ENDIAN);
                    trackManipulator->spline().serialize(b);
                    b.commit();

                    updateTrackFiles();

                    // Select the one we just saved
                    trackFileIndex = iMax(0, trackFileArray.findIndex(saveName));
                    
                    saveButton->setEnabled(false);
                }
            }
        }
        sync();

    } else if (trackManipulator->mode() == UprightSplineManipulator::RECORD_KEY_MODE) {
        // Check if the user has added a point yet
        sync();
    }

    return false;
}


void CameraControlWindow::loadSpline(const std::string& filename) {
    saveButton->setEnabled(false);
    trackManipulator->setMode(UprightSplineManipulator::INACTIVE_MODE);

    if (filename == noSpline) {
        trackManipulator->clear();
        return;
    }

    if (! fileExists(filename)) {
        trackManipulator->clear();
        return;
    }

    UprightSpline spline;

    BinaryInput b(filename, G3D_LITTLE_ENDIAN);
    spline.deserialize(b);

    trackManipulator->setSpline(spline);
    manualOperation = true;
}


void CameraControlWindow::sync() {

    if (m_expanded) {
        bool hasTracks = trackFileArray.size() > 0;
        trackList->setEnabled(hasTracks);

        bool hasSpline = trackManipulator->splineSize() > 0;
        visibleCheckBox->setEnabled(hasSpline);
        cyclicCheckBox->setEnabled(hasSpline);
        playButton->setEnabled(hasSpline);

        if (manualOperation) {
            switch (trackManipulator->mode()) {
            case UprightSplineManipulator::RECORD_KEY_MODE:
            case UprightSplineManipulator::RECORD_INTERVAL_MODE:
                helpLabel->setCaption(recordHelpCaption);
                break;

            case UprightSplineManipulator::PLAY_MODE:
                helpLabel->setCaption(playHelpCaption);
                break;

            case UprightSplineManipulator::INACTIVE_MODE:
                helpLabel->setCaption(manualHelpCaption);
            }
        } else {
            helpLabel->setCaption(autoHelpCaption);
        }
    }

    if (manualOperation) {
        // User has control
        bool playing = trackManipulator->mode() == UprightSplineManipulator::PLAY_MODE;
        manualManipulator->setActive(! playing);
        if (playing) {
            *cameraManipulator = trackManipulator;
        } else {
            *cameraManipulator = manualManipulator;
        }
    } else {
        // Program has control
        manualManipulator->setActive(false);
        *cameraManipulator = Manipulator::Ref(NULL);
        trackManipulator->setMode(UprightSplineManipulator::INACTIVE_MODE);
    }
}

}
