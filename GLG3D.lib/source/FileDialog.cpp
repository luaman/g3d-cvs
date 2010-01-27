/**
@file FileDialog.cpp

@maintainer Morgan McGuire, http://graphics.cs.williams.edu

@created 2007-10-01
@edited  2008-03-10
*/

#include "GLG3D/FileDialog.h"
#include "GLG3D/GuiPane.h"

namespace G3D {

FileDialog::FileDialog(OSWindow* osWindow, GuiThemeRef skin, const std::string& note) : 
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

void FileDialog::close() {
    setVisible(false);
    m_manager->remove(this);
}

bool FileDialog::onEvent(const GEvent& e) {
    if (GuiWindow::onEvent(e)) {
        return true;
    }

    okButton->setEnabled(trimWhitespace(m_filename) != "");

    if ((e.type == GEventType::GUI_ACTION) && 
        ((e.gui.control == cancelButton) ||
         (e.gui.control == textBox) ||
         (e.gui.control == okButton))) {
        ok = (e.gui.control != cancelButton);
        close();
        return true;
    } else if ((e.type == GEventType::KEY_DOWN) && (e.key.keysym.sym == GKey::ESCAPE)) {
        // Cancel the dialog
        ok = false;
        close();
        return true;
    }

    return false;
}

} // namespace G3D
