/**
   This code sample shows how to use GLUT to initialize OpenGL.  It contains
   no G3D code.  This is primarily useful for tracking down GPU driver bugs
   or other issues for which you would like to rule out G3D's interactions.

   See also the "minGlut" G3D sample, which contains a small amount of 
   G3D code for support classes.

   Note that this project requires linking to GLUT, which is not
   formally distributed with G3D but is part of OS X and most Linux
   distributions.  The Windows version of GLUT is included in this
   directory.
 */
#include <stdio.h>
#include <string>
#include "glew.h"
#include "glut.h"
#ifdef _MSC_VER
#   include <windows.h>
#   pragma comment(lib, "glut32.lib")
#endif

static const int WIDTH = 640;
static const int HEIGHT = 400;

static void quitOnEscape(unsigned char key, int x, int y) {
    if (key == 27) { ::exit(0); }
}

static void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Put your rendering code here

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    
    // Initialize OpenGL
    glutInit(&argc, argv);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutCreateWindow("OpenGL");

    // Initialize OpenGL extensions
    glewInit();

    // Set GLUT callbacks
    glutKeyboardFunc(&quitOnEscape);
    glutDisplayFunc(&render);

    // Never returns
    glutMainLoop();

    return 0;
}


#ifdef _MSC_VER

// Make Microsoft Windows programs start from the main() entry point
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw) {
    const int argc = 1;
    char** argv = new char*[argc];
    argv[0] = "OpenGL";
    
    int r = main(argc, argv);
    delete[] argv;

    return r;
}

#endif
