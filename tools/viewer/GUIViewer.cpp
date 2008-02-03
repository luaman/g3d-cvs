/**
 @file GUIViewer.cpp
 
 Viewer for testing and examining .skn files
 
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2008-01-27
 */
#include "GUIViewer.h"


GUIViewer::GUIViewer(App* app) : parentApp(app) {

    if (fileExists("background1.jpg")) {
        background1 = Texture::fromFile("background1.jpg", TextureFormat::AUTO(),
                                        Texture::DIM_2D_NPOT, Texture::Settings::video());
    }
    
    if (fileExists("background2.jpg")) {
        background2 = Texture::fromFile("background2.jpg", TextureFormat::AUTO(),
                                        Texture::DIM_2D_NPOT, Texture::Settings::video());
    }
}


void GUIViewer::createGui(const std::string& filename) {
    GuiPane*			pane;
    
    skin = GuiSkin::fromFile(filename, parentApp->debugFont);
    
    window         = GuiWindow::create("Normal", skin, Rect2D::xywh(50,50,0,0),   
                                       GuiWindow::NORMAL_FRAME_STYLE, GuiWindow::IGNORE_CLOSE);
    toolWindow     = GuiWindow::create("Tool",   skin, Rect2D::xywh(300,100,0,0), 
                                       GuiWindow::TOOL_FRAME_STYLE,   GuiWindow::IGNORE_CLOSE);
    bgControl      = GuiWindow::create("Dialog", skin, Rect2D::xywh(550,100,0,0), 
                                       GuiWindow::DIALOG_FRAME_STYLE, GuiWindow::IGNORE_CLOSE);
    dropdownWindow = GuiWindow::create("Normal", skin, Rect2D::xywh(400,400,0,0), 
                                       GuiWindow::NORMAL_FRAME_STYLE, GuiWindow::IGNORE_CLOSE);

    text = "Hello";

    pane = window->pane();
    slider[0] = 1.5f;
    slider[1] = 1.8f;

    {
        GuiPane* p = pane->addPane("Pane (NO_FRAME_STYLE)", GuiPane::NO_FRAME_STYLE);
        p->addSlider("Slider", &slider[0], 1.0f, 2.2f);
        p->addSlider("Slider Disabled", &slider[1], 1.0f, 2.2f)->setEnabled(false);
    }
    {
        GuiPane* p = pane->addPane("Pane (SIMPLE_FRAME_STYLE)", GuiPane::SIMPLE_FRAME_STYLE);
        p->addLabel("RadioButton (RADIO_STYLE)");
        p->addRadioButton("Sel, Dis", 1, &radio[0])->setEnabled(false);
        p->addRadioButton("Desel, Dis", 2, &radio[0])->setEnabled(false);
        p->addRadioButton("Sel, Enabled", 3, &radio[1]);
        p->addRadioButton("Desel, Disabled", 4, &radio[1]);
    }

    {
        GuiPane* p = pane->addPane("Pane (SIMPLE_FRAME_STYLE)", GuiPane::SIMPLE_FRAME_STYLE);
        p->addLabel("RadioButton (BUTTON_STYLE)");
        p->addRadioButton("Selected, Disabled", 5, &radio[2], GuiRadioButton::BUTTON_STYLE)->setEnabled(false);
        p->addRadioButton("Deselected, Disabled", 6, &radio[2], GuiRadioButton::BUTTON_STYLE)->setEnabled(false);
        p->addRadioButton("Selected, Enabled", 7, &radio[3], GuiRadioButton::BUTTON_STYLE);
        p->addRadioButton("Deselected, Disabled", 8, &radio[3], GuiRadioButton::BUTTON_STYLE);
        p->addButton("Button");
    }

    pane = toolWindow->pane();
    {
        GuiPane* p = pane->addPane("Pane (ORNATE_FRAME_STYLE)", GuiPane::ORNATE_FRAME_STYLE);
        p->addLabel("Checkbox (CHECKBOX_SYLE)");
        checkbox[0] = true;
        checkbox[1] = false;
        checkbox[2] = true;
        checkbox[3] = false;
        p->addCheckBox("Selected, Enabled", &checkbox[0]);
        p->addCheckBox("Deselected, Enabled", &checkbox[1]);
        p->addCheckBox("Selected, Disabled", &checkbox[2])->setEnabled(false);
        p->addCheckBox("Deselected, Disabled", &checkbox[3])->setEnabled(false);
    }

    {
        GuiPane* p = pane->addPane("", GuiPane::SIMPLE_FRAME_STYLE);
        p->addLabel("Button (Checkbox)");
        checkbox[4] = true;
        checkbox[5] = false;
        checkbox[6] = true;
        checkbox[7] = false;
        p->addCheckBox("Selected, Disabled", &checkbox[4], GuiCheckBox::BUTTON_STYLE)->setEnabled(false);
        p->addCheckBox("Deselected, Disabled", &checkbox[5], GuiCheckBox::BUTTON_STYLE)->setEnabled(false);
        p->addCheckBox("Selected, Enabled", &checkbox[6], GuiCheckBox::BUTTON_STYLE);
        p->addCheckBox("Deselected, Enabled", &checkbox[7], GuiCheckBox::BUTTON_STYLE);
        p->addButton("Disabled")->setEnabled(false);
    }

    pane = dropdownWindow->pane();
    GuiButton* t1 = pane->addButton("Tool", GuiSkin::TOOL_BUTTON_STYLE);
    GuiButton* t2 = pane->addButton("Tool", GuiSkin::TOOL_BUTTON_STYLE);
    t2->moveRightOf(t1);
    t2->setEnabled(false);
    static bool check = false;
    GuiCheckBox* t3 = pane->addCheckBox("Check", &check, GuiCheckBox::TOOL_STYLE);
    t3->moveRightOf(t2);

    dropdownIndex[0] = 0;
    dropdownIndex[1] = 0;
    dropdown.append("Option 1");
    dropdown.append("Option 2");
    dropdown.append("Option 3");
    dropdownDisabled.append("Disabled");
    pane->addLabel("Dropdown List");
    pane->addDropDownList(GuiCaption("Enabled"), &dropdownIndex[0], &dropdown);
    pane->addDropDownList(GuiCaption("Disabled"), &dropdownIndex[1], &dropdownDisabled)->setEnabled(false);
    pane->addTextBox("TextBox", &text);
    pane->addTextBox("Disabled", &text)->setEnabled(false);

    pane = bgControl->pane();
    windowControl = BGIMAGE2;
    pane->addLabel("Background Color");
    pane->addRadioButton(GuiCaption("White"), WHITE, &windowControl);
    pane->addRadioButton(GuiCaption("Blue"), BLUE, &windowControl);
    pane->addRadioButton(GuiCaption("Black"), BLACK, &windowControl);
    pane->addRadioButton(GuiCaption("background1.jpg"), BGIMAGE1, &windowControl)->setEnabled(background1.notNull());
    pane->addRadioButton(GuiCaption("background2.jpg"), BGIMAGE2, &windowControl)->setEnabled(background2.notNull());

    // Gets rid of any empty, unused space in the windows
    window->pack();
    toolWindow->pack();
    bgControl->pack();
    dropdownWindow->pack();

    parentApp->addWidget(window);
    parentApp->addWidget(toolWindow);
    parentApp->addWidget(bgControl);
    parentApp->addWidget(dropdownWindow);
}


GUIViewer::~GUIViewer(){
    parentApp->removeWidget(window);
    parentApp->removeWidget(toolWindow);
    parentApp->removeWidget(bgControl);
    parentApp->removeWidget(dropdownWindow);

    parentApp->colorClear = Color3::blue();
}


void GUIViewer::onInit(const std::string& filename) {
    createGui(filename);
}


void GUIViewer::onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting) {
    switch (windowControl) {
    case WHITE:
        app->colorClear = Color3::white();
        break;
    case BLUE:
        app->colorClear = Color3::blue();
        break;
    case BLACK:
        app->colorClear = Color3::black();
        break;

    case BGIMAGE1:
        rd->setTexture(0, background1);
        rd->push2D();
        Draw::rect2D(rd->viewport(), rd);
        rd->pop2D();
        rd->setTexture(0, NULL);
        break;

    case BGIMAGE2:
        rd->setTexture(0, background2);
        rd->push2D();
        Draw::rect2D(rd->viewport(), rd);
        rd->pop2D();
        rd->setTexture(0, NULL);
        break;
    }

}
