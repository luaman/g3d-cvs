/**
 @file main.cpp
 
 App that allows viewing of 3D assets, either as a command line argument or after dragging and dropping
 onto the window
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "App.h"

#if defined(G3D_VER) && (G3D_VER < 70000)
#   error Requires G3D 7.00
#endif


G3D_START_AT_MAIN();


int main(int argc, char** argv) {
	std::string filename;

	if (argc > 1) {
		filename = argv[1];
	}

	GApp2::Settings s = GApp2::Settings();
	s.window.resizable = true;
    s.window.width = 640;
    s.window.height = 480;

    return App(s, filename).run();
}
