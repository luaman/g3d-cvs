/**
 @file GUIViewer.h
 
 Viewer for testing and examining .skn files
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-14
 */
#ifndef GUIVIEWER_H
#define GUIVIEWER_H

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class GUIViewer : public Viewer {
private:
    GuiWindow::Ref		window;
    GuiWindow::Ref		toolWindow;
    GuiWindow::Ref		bgControl;
    GuiWindow::Ref          dropdownWindow;
    App*			parentApp;
    GuiThemeRef	        skin;
    TextureRef		background1;
    TextureRef	        background2;
    
    enum WindowBackground {WHITE = 1, BLUE = 2, BLACK = 3, 
                           BGIMAGE1 = 4, BGIMAGE2 = 5};
    
    WindowBackground	windowControl;

    /**
       A typical GUI has actual state that it is connected to.
       We're just showing off the GUI and not changing any real
       state, so we make up some variables to hold the values.  In a
       normal program, for example, you should not name your
       variables "checkbox", but instead hook a GuiCheckBox up to an
       existing bool that has a useful name.
    */
    Array<std::string>	dropdown;
    Array<std::string>	dropdownDisabled;
    int                 dropdownIndex[2];
    bool                checkbox[8];
    int                 radio[4];
    float               slider[2];
    std::string         text;

    void createGui(const std::string& filename);

public:
	GUIViewer(App* app);
	~GUIViewer();
	virtual void onInit(const std::string& filename);
    virtual void onGraphics(RenderDevice* rd, App* app, const LightingRef& lighting);

};

#endif
