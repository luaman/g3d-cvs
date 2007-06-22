/**
 @file GUIViewer.cpp
 
 Viewer for testing and examining .skn files
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#include "GUIViewer.h"


GUIViewer::GUIViewer(App* app) : parentApp(app) {

    if (fileExists("background1.jpg")) {
	    background1 = Texture::fromFile("background1.jpg", TextureFormat::AUTO,
							    Texture::DIM_2D_NPOT, Texture::Settings::video());
    }

    if (fileExists("background2.jpg")) {
	    background2 = Texture::fromFile("background2.jpg", TextureFormat::AUTO,
							    Texture::DIM_2D_NPOT, Texture::Settings::video());
    }
}


void GUIViewer::createGui(const std::string& filename) {
	GuiPane*			pane;
	GuiPane*			ornatePane;
	GuiPane*			normalPane;

	skin = GuiSkin::fromFile(filename, parentApp->debugFont);

	window         = GuiWindow::create("Normal", skin, Rect2D::xywh(50,50,0,0),   GuiWindow::FRAME_STYLE,        GuiWindow::IGNORE_CLOSE);
	toolWindow     = GuiWindow::create("Tool",   skin, Rect2D::xywh(300,100,0,0), GuiWindow::TOOL_FRAME_STYLE,   GuiWindow::IGNORE_CLOSE);
	bgControl      = GuiWindow::create("Dialog", skin, Rect2D::xywh(550,100,0,0), GuiWindow::DIALOG_FRAME_STYLE, GuiWindow::IGNORE_CLOSE);
	dropdownWindow = GuiWindow::create("Normal", skin, Rect2D::xywh(400,400,0,0), GuiWindow::FRAME_STYLE,        GuiWindow::IGNORE_CLOSE);

    text = "Hello";

	pane = window->pane();
	slider[0] = 1.5f;
	slider[1] = 1.8f;
	pane->addSlider("Slider", &slider[0], 1.0f, 2.2f);
	pane->addSlider("Slider Disabled", &slider[1], 1.0f, 2.2f)->setEnabled(false);
		normalPane = pane->addPane(GuiCaption(""), 170, GuiPane::ORNATE_FRAME_STYLE);
			normalPane->addLabel(GuiCaption("Ornate Pane"));
			normalPane->addLabel(GuiCaption("Radio (Default)"));
			radio[0] = 1;
			radio[1] = 3;
			radio[2] = 5;
			radio[3] = 7;
			normalPane->addRadioButton("Selected, Disabled", 1, &radio[0])->setEnabled(false);
			normalPane->addRadioButton("Deselected, Disabled", 2, &radio[0])->setEnabled(false);
			normalPane->addRadioButton("Selected, Enabled", 3, &radio[1]);
			normalPane->addRadioButton("Deselected, Disabled", 4, &radio[1]);
		normalPane = pane->addPane(GuiCaption(""), 170, GuiPane::SIMPLE_FRAME_STYLE);
			normalPane->addLabel(GuiCaption("Simple Pane"));
			normalPane->addLabel(GuiCaption("Button (Radio)"));
			normalPane->addRadioButton("Selected, Disabled", 5, &radio[2], GuiRadioButton::BUTTON_STYLE)->setEnabled(false);
			normalPane->addRadioButton("Deselected, Disabled", 6, &radio[2], GuiRadioButton::BUTTON_STYLE)->setEnabled(false);
			normalPane->addRadioButton("Selected, Enabled", 7, &radio[3], GuiRadioButton::BUTTON_STYLE);
			normalPane->addRadioButton("Deselected, Disabled", 8, &radio[3], GuiRadioButton::BUTTON_STYLE);
	pane->addButton("Button");

	pane = toolWindow->pane();
		ornatePane = pane->addPane(GuiCaption(""), 200, GuiPane::ORNATE_FRAME_STYLE);
			ornatePane->addLabel(GuiCaption("Ornate Pane"));
			ornatePane->addLabel(GuiCaption("Checkbox (Default)"));
			checkbox[0] = true;
			checkbox[1] = false;
			checkbox[2] = true;
			checkbox[3] = false;
			ornatePane->addCheckBox(GuiCaption("Selected, Enabled"), &checkbox[0]);
			ornatePane->addCheckBox(GuiCaption("Deselected, Enabled"), &checkbox[1]);
			ornatePane->addCheckBox(GuiCaption("Selected, Disabled"), &checkbox[2])->setEnabled(false);
			ornatePane->addCheckBox(GuiCaption("Deselected, Disabled"), &checkbox[3])->setEnabled(false);
		ornatePane = pane->addPane(GuiCaption(""), 170, GuiPane::SIMPLE_FRAME_STYLE);
			ornatePane->addLabel(GuiCaption("Simple Pane"));
			ornatePane->addLabel(GuiCaption("Button (Checkbox)"));
			checkbox[4] = true;
			checkbox[5] = false;
			checkbox[6] = true;
			checkbox[7] = false;
			ornatePane->addCheckBox(GuiCaption("Selected, Disabled"), &checkbox[4], GuiCheckBox::BUTTON_STYLE)->setEnabled(false);
			ornatePane->addCheckBox(GuiCaption("Deselected, Disabled"), &checkbox[5], GuiCheckBox::BUTTON_STYLE)->setEnabled(false);
			ornatePane->addCheckBox(GuiCaption("Selected, Enabled"), &checkbox[6], GuiCheckBox::BUTTON_STYLE);
			ornatePane->addCheckBox(GuiCaption("Deselected, Enabled"), &checkbox[7], GuiCheckBox::BUTTON_STYLE);
	pane->addButton("Disabled")->setEnabled(false);

	pane = dropdownWindow->pane();
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
    	pane->addLabel("Background Color");
		windowControl = BLUE;
		pane->addRadioButton(GuiCaption("White"), WHITE, &windowControl);
		pane->addRadioButton(GuiCaption("Blue"), BLUE, &windowControl);
		pane->addRadioButton(GuiCaption("Black"), BLACK, &windowControl);
		pane->addRadioButton(GuiCaption("background1.jpg"), BGIMAGE1, &windowControl)->setEnabled(background1.notNull());
		pane->addRadioButton(GuiCaption("background2.jpg"), BGIMAGE2, &windowControl)->setEnabled(background1.notNull());

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


void GUIViewer::onGraphics(RenderDevice* rd, App* app) {
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
