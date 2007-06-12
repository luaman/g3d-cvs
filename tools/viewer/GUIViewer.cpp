/**
 @file GUIViewer.cpp
 
 Viewer for testing and examining .skn files
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#include "GUIViewer.h"


GUIViewer::GUIViewer() : addToApp(false), parentApp(NULL) {}


GUIViewer::~GUIViewer(){
	parentApp->removeWidget(window);
	parentApp->removeWidget(toolWindow);
}


void GUIViewer::onInit(const std::string& filename) {
	GuiSkinRef			skin;
	GuiPane*			pane;
	GuiPane*			ornatePane;
	GuiPane*			normalPane;

	skin = GuiSkin::fromFile(filename);
	skin->setFont(GFont::fromFile("X:/morgan/data/font/arial.fnt"), 11, Color3::black(), Color4::clear());
	window = GuiWindow::create(GuiText("Normal"), Rect2D::xywh(50,50,330,550), skin, GuiWindow::FRAME_STYLE, GuiWindow::IGNORE_CLOSE);
	toolWindow = GuiWindow::create(GuiText("Tool"), Rect2D::xywh(300,100,200,440), skin, GuiWindow::TOOL_FRAME_STYLE, GuiWindow::IGNORE_CLOSE);

	pane = window->pane();
	slider[0] = 1.5f;
	slider[1] = 1.8f;
	pane->addSlider("Slider", &slider[0], 1.0f, 2.2f);
	pane->addSlider("Slider Disabled", &slider[1], 1.0f, 2.2f)->setEnabled(false);
		normalPane = pane->addPane(GuiText("Simple Pane"), 170, GuiPane::SIMPLE_FRAME_STYLE);
			normalPane->addLabel(GuiText("Radio (Default)"));
			radio[0] = 1;
			radio[1] = 3;
			radio[2] = 5;
			radio[3] = 7;
			normalPane->addRadioButton("Selected, Disabled", 1, &radio[0])->setEnabled(false);
			normalPane->addRadioButton("Deselected, Disabled", 2, &radio[0])->setEnabled(false);
			normalPane->addRadioButton("Selected, Enabled", 3, &radio[1]);
			normalPane->addRadioButton("Deselected, Disabled", 4, &radio[1]);
		normalPane = pane->addPane(GuiText("Simple Pane 2"), 170, GuiPane::SIMPLE_FRAME_STYLE);
			normalPane->addLabel(GuiText("Button (Radio)"));
			normalPane->addRadioButton("Selected, Disabled", 5, &radio[2], GuiRadioButton::BUTTON_STYLE)->setEnabled(false);
			normalPane->addRadioButton("Deselected, Disabled", 6, &radio[2], GuiRadioButton::BUTTON_STYLE)->setEnabled(false);
			normalPane->addRadioButton("Selected, Enabled", 7, &radio[3], GuiRadioButton::BUTTON_STYLE);
			normalPane->addRadioButton("Deselected, Disabled", 8, &radio[3], GuiRadioButton::BUTTON_STYLE);
	pane->addButton("Button");
    pane->addTextBox("TextBox", &text);
    pane->addTextBox("TextBox (Dis)", &text)->setEnabled(false);


	pane = toolWindow->pane();
		ornatePane = pane->addPane(GuiText("Ornate Pane"), 200, GuiPane::ORNATE_FRAME_STYLE);
			ornatePane->addLabel(GuiText("Checkbox (Default)"));
			checkbox[0] = true;
			checkbox[1] = false;
			checkbox[2] = true;
			checkbox[3] = false;
			ornatePane->addCheckBox(GuiText("Selected, Enabled"), &checkbox[0]);
			ornatePane->addCheckBox(GuiText("Deselected, Enabled"), &checkbox[1]);
			ornatePane->addCheckBox(GuiText("Selected, Disabled"), &checkbox[2])->setEnabled(false);
			ornatePane->addCheckBox(GuiText("Deselected, Disabled"), &checkbox[3])->setEnabled(false);
		ornatePane = pane->addPane(GuiText("Ornate Pane 2"), 170, GuiPane::ORNATE_FRAME_STYLE);
			ornatePane->addLabel(GuiText("Button (Checkbox)"));
			checkbox[4] = true;
			checkbox[5] = false;
			checkbox[6] = true;
			checkbox[7] = false;
			ornatePane->addCheckBox(GuiText("Selected, Disabled"), &checkbox[4], GuiCheckBox::BUTTON_STYLE)->setEnabled(false);
			ornatePane->addCheckBox(GuiText("Deselected, Disabled"), &checkbox[5], GuiCheckBox::BUTTON_STYLE)->setEnabled(false);
			ornatePane->addCheckBox(GuiText("Selected, Enabled"), &checkbox[6], GuiCheckBox::BUTTON_STYLE);
			ornatePane->addCheckBox(GuiText("Deselected, Enabled"), &checkbox[7], GuiCheckBox::BUTTON_STYLE);
	pane->addButton("Disabled")->setEnabled(false);

	addToApp = true;
}


void GUIViewer::onGraphics(RenderDevice* rd, App* app) {
	// When we initialize, the App isn't available to call addWidget on, so we delay until onGraphics
	if (addToApp) {
		addToApp = false;
		parentApp = app;
		app->addWidget(window);
		app->addWidget(toolWindow);
	}
}
