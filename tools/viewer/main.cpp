/**
 @file main.cpp
 
 App that allows viewing of 3D assets, either as a command line argument or after dragging and dropping
 onto the window
 
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2010-02-25
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "App.h"

#if defined(G3D_VER) && (G3D_VER < 80000)
#   error Requires G3D 8.00
#endif


G3D_START_AT_MAIN();


int main(int argc, char** argv) {
    std::string filename;
    
    if (argc > 1) {
        filename = argv[1];
    }
    
    GApp::Settings settings;

    settings.window.resizable = true;

    settings.window.width = 1024;
    settings.window.height = 768;

    return App(settings, filename).run();
}
