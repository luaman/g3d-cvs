/**
@file FileDialog.cpp

@maintainer Morgan McGuire, morgan@cs.williams.edu

@created 2007-10-01
@edited  2008-03-10
*/

#include "GLG3D/FileDialog.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

FileDialog::FileDialog(GWindow* osWindow, GuiThemeRef skin, const std::string& note) : 
    GuiWindow("", skin, Rect2D::xywh(150, 100, 10, 10), GuiTheme::DIALOG_WINDOW_STYLE, HIDE_ON_CLOSE), 
    ok(false), m_osWindow(osWindow) {

    GuiPane* rootPane = pane();

    textBox = rootPane->addTextBox("Filename", &m_filename, GuiTextBox::IMMEDIATE_UPDATE);
    textBox->setSize(textBox->rect().wh() + Vector2(70, 0));
    textBox->setFocused(true);

    cancelButton = rootPane->addButton("Cancel");
    okButton = rootPane->addButton("Ok");
    okButton->moveRightOf(cancelButton);

    okButton->setEnabled(false);

    if (note != "") {
        rootPane->addLabel(note);
    }

    pack();
}


bool FileDialog::getFilename(std::string& filename, const std::string& caption) {
    setCaption(caption);
    m_filename = filename;

    showModal(m_osWindow);

    if (ok) {
        filename = m_filename;
    }
    return ok;
}


bool FileDialog::onEvent(const GEvent& e) {
    if (GuiWindow::onEvent(e)) {
        return true;
    }

    okButton->setEnabled(trimWhitespace(m_filename) != "");

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
