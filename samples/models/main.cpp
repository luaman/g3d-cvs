/**
  @file models/main.cpp

  @author Morgan McGuire, http://graphics.cs.williams.edu
 */

#include <G3D/G3DAll.h>
#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    GApp::Settings settings;

    settings.window.caption     = "G3D Models Demo";
    settings.window.width       = 960; 
    settings.window.height      = 600;

#   ifdef G3D_WIN32
        if (! FileSystem::exists("G3D-logo-tiny-alpha.tga")) {
            // Running under visual studio from the wrong directory
            chdir("../samples/models/data-files");
        }
#   endif

    alwaysAssertM(FileSystem::exists("G3D-logo-tiny-alpha.tga"), 
        "Cannot find runtime data files.");

    // Uncomment the next line to see what articulated model will look like
    // on a legacy OpenGL 1.x graphics card.
    // ArticulatedModel::setProfile(ArticulatedModel::FIXED_FUNCTION);

    return App(settings).run();
}
