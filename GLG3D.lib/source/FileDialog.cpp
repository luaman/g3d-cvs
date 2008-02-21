/**
@file FileDialog.cpp

@maintainer Morgan McGuire, morgan@cs.williams.edu

@created 2007-10-01
@edited  2008-02-10
*/

#include "GLG3D/FileDialog.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

FileDialog::FileDialog(std::string& saveName, GuiSkinRef skin, const std::string& caption) : 
    GuiWindow(caption, skin, Rect2D::xywh(150, 100, 10, 10), DIALOG_FRAME_STYLE, HIDE_ON_CLOSE), 
    ok(false), saveName(saveName) {
    GuiPane* rootPane = pane();

    textBox = rootPane->addTextBox("Filename", &saveName, GuiTextBox::IMMEDIATE_UPDATE);
    textBox->setSize(textBox->rect().wh() + Vector2(70, 0));
    textBox->setFocused(true);

    cancelButton = rootPane->addButton("Cancel");
    okButton = rootPane->addButton("Ok");
    okButton->moveRightOf(cancelButton);

    okButton->setEnabled(trimWhitespace(saveName) != "");

    pack();
}


bool FileDialog::getFilename(std::string& saveName, GWindow* osWindow, GuiSkinRef skin, const std::string& caption) {
    ReferenceCountedPointer<FileDialog> guiWindow = new FileDialog(saveName, skin, caption);
    guiWindow->showModal(osWindow);
    return guiWindow->ok;
}


bool FileDialog::getFilename(std::string& saveName, GuiWindowRef parent, const std::string& caption) {
    return getFilename(saveName, parent->window(), parent->skin(), caption);
}


bool FileDialog::onEvent(const GEvent& e) {
    if (GuiWindow::onEvent(e)) {
        return true;
    }

    okButton->setEnabled(trimWhitespace(saveName) != "");

    if (e.type == GEventType::GUI_ACTION) {
        // Text box, cancel button and ok button are the 
        // only choices.  Anything but cancel means ok,
        // and all mean close the dialog.
        ok = (e.gui.control != cancelButton);
        setVisible(false);
    } else if ((e.type == GEventType::KEY_DOWN) && (e.key.keysym.sym == GKey::ESCAPE)) {
        // Cancel the dialog
        ok = false;
        setVisible(false);
    }

    return false;
}

} // namespace G3D
