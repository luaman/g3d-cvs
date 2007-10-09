/**
  @file FileDialog.h

  @maintainer Morgan McGuire, morgan@cs.williams.edu

  @created 2007-10-01
  @edited  2007-10-10
*/
#ifndef G3D_FileDialog_H
#define G3D_FileDialog_H

#include "G3D/platform.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GuiWindow.h"

namespace G3D {

/** Example:         
<pre>
if (getFilename("Save File", mywidow)) {
  save code
}

</pre>
*/
class FileDialog : public GuiWindow {
protected:

    bool               ok;

    GuiButton*         okButton;
    GuiButton*         cancelButton;
    GuiTextBox*        textBox;

    std::string&       saveName;

    FileDialog(std::string& saveName, GuiSkinRef skin);

public:
    
    /**
       @param saveName Initial value of the box and what will be returned in the event
       that the user presses ok.

       @return True unless cancelled
     */
    static bool getFilename(std::string& saveName, GWindow* osWindow, GuiSkinRef skin);

    static bool getFilename(std::string& saveName, GuiWindowRef parent);

    virtual bool onEvent(const GEvent& e);

};

} // namespace

#endif
