/**
  @file FileDialog.h

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu

  @created 2007-10-01
  @edited  2008-03-10
*/
#ifndef G3D_FileDialog_H
#define G3D_FileDialog_H

#include "G3D/platform.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiTextBox.h"

namespace G3D {

typedef ReferenceCountedPointer<class FileDialog> FileDialogRef;

/** 
@param A G3D GUI dialog that prompts for a load or save file name.  

The dialog needs a default theme to use, so it requires either a GuiTheme or a GuiWindow
from which to copy the theme.

Example of typical use within a GApp subclass, where GApp::debugWindow is an inherited
GuiWindow*:

<pre>
std::string filename = "";
if (FileDialog::create(debugWindow)->getFilename(filename)) {
  // save code
}
</pre>


Example of a custom file dialog:

<pre>
class MyFileDialog : public FileDialog {
protected:
    
    bool m_saveAlpha;

    MyFileDialog(OSWindow* osWindow, GuiThemeRef skin) : FileDialog(osWindow, skin, "") {
        pane()->addCheckBox("Save Alpha", &m_saveAlpha);
    }

public:
    FileDialogRef create(GuiWindowRef parent) {
        return new MyFileDialog(parent->window(), parent->theme());
    }

    bool getFilename(std::string& filename, bool& saveAlpha, const std::string& caption = "Enter Filename") {
        m_saveAlpha = saveAlpha;

        FileDialog::getFilename(filename, caption);

        if (ok) { saveAlpha = m_saveAlpha; }
        return ok;
    }

}

...

std::string filename = "";
bool saveAlpha = true;

if (MyFileDialog::create(window)->->getFilename(filename, saveAlpha)) {
  // save code
}
</pre>
*/
class FileDialog : public GuiWindow {
protected:

    bool               ok;

    GuiButton*         okButton;
    GuiButton*         cancelButton;
    GuiTextBox*        textBox;

    std::string        m_filename;

    OSWindow*           m_osWindow;

    FileDialog(OSWindow* osWindow, GuiThemeRef theme, const std::string& note);

    void close();

public:

    static FileDialogRef create(OSWindow* osWindow, GuiThemeRef theme, const std::string& note = "") {
        return new FileDialog(osWindow, theme, note);
    }

    static FileDialogRef create(GuiWindowRef parent, const std::string& note = "") {
        return create(parent->window(), parent->theme(), note);
    }

    /**
       @param filename  This is the initial filename shown, and unless cancelled, receives the final filename as well.
       @return True unless cancelled
     */
    // filename is passed as a reference instead of a pointer because it will not be used after the
    // method ends, so there is no danger of the caller misunderstanding as there is with the GuiPane::addTextBox method.
    virtual bool getFilename(std::string& filename, const std::string& caption = "Enter Filename");

    virtual bool onEvent(const GEvent& e);

};

} // namespace

#endif
