/**
  @file rawOpenGL/main.cpp

  This demo shows how to use G3D to initialize the 3D system and then
  transfer control to raw OpenGL commands.  
  
  *** This is not a recommended method for most users *** 

  ...but is very helpful when teaching OpenGL programming in an
  introductory 3D graphics course.  You can then add in more
  high-level G3D routines as students become more sophisticated.

  This demo also shows how G3D can abstract the platform-specific
  aspect of creating a window and initializing OpenGL while still
  leaving you with full control over OpenGL.

  @author Morgan McGuire, http://graphics.cs.williams.edu
 */
#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

G3D_START_AT_MAIN();

void drawCube() {
    // Code from http://www.morrowland.com/apron/tutorials/gl/gl_rotating_cube.php
    glBegin(GL_QUADS);
    glColor3f(0.0f,1.0f,0.0f);// Color Blue
    glVertex3f( 1.0f, 1.0f,-1.0f);// Top Right Of The Quad (Top)
    glVertex3f(-1.0f, 1.0f,-1.0f);// Top Left Of The Quad (Top)
    glVertex3f(-1.0f, 1.0f, 1.0f);// Bottom Left Of The Quad (Top)
    glVertex3f( 1.0f, 1.0f, 1.0f);// Bottom Right Of The Quad (Top)

    glColor3f(1.0f,0.5f,0.0f);// Color Orange
    glVertex3f( 1.0f,-1.0f, 1.0f);// Top Right Of The Quad (Bottom)
    glVertex3f(-1.0f,-1.0f, 1.0f);// Top Left Of The Quad (Bottom)
    glVertex3f(-1.0f,-1.0f,-1.0f);// Bottom Left Of The Quad (Bottom)
    glVertex3f( 1.0f,-1.0f,-1.0f);// Bottom Right Of The Quad (Bottom)
    
    glColor3f(1.0f,0.0f,0.0f);// Color Red
    glVertex3f( 1.0f, 1.0f, 1.0f);// Top Right Of The Quad (Front)
    glVertex3f(-1.0f, 1.0f, 1.0f);// Top Left Of The Quad (Front)
    glVertex3f(-1.0f,-1.0f, 1.0f);// Bottom Left Of The Quad (Front)
    glVertex3f( 1.0f,-1.0f, 1.0f);// Bottom Right Of The Quad (Front)
    
    glColor3f(1.0f,1.0f,0.0f);// Color Yellow
    glVertex3f( 1.0f,-1.0f,-1.0f);// Top Right Of The Quad (Back)
    glVertex3f(-1.0f,-1.0f,-1.0f);// Top Left Of The Quad (Back)
    glVertex3f(-1.0f, 1.0f,-1.0f);// Bottom Left Of The Quad (Back)
    glVertex3f( 1.0f, 1.0f,-1.0f);// Bottom Right Of The Quad (Back)
    
    glColor3f(0.0f,0.0f,1.0f);// Color Blue
    glVertex3f(-1.0f, 1.0f, 1.0f);// Top Right Of The Quad (Left)
    glVertex3f(-1.0f, 1.0f,-1.0f);// Top Left Of The Quad (Left)
    glVertex3f(-1.0f,-1.0f,-1.0f);// Bottom Left Of The Quad (Left)
    glVertex3f(-1.0f,-1.0f, 1.0f);// Bottom Right Of The Quad (Left)
    
    glColor3f(1.0f,0.0f,1.0f);// Color Violet
    glVertex3f( 1.0f, 1.0f,-1.0f);// Top Right Of The Quad (Right)
    glVertex3f( 1.0f, 1.0f, 1.0f);// Top Left Of The Quad (Right)
    glVertex3f( 1.0f,-1.0f, 1.0f);// Bottom Left Of The Quad (Right)
    glVertex3f( 1.0f,-1.0f,-1.0f);// Bottom Right Of The Quad (Right)
    glEnd();
}

void drawFrame(int w, int h, int frameNum) {
    // Set up the camera
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0f, (GLfloat)w/(GLfloat)h, 0.1f, 100.0f);

    glClearColor(0.0f, 0.2f, 0.4f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_LIGHTING);

    float angle = frameNum * 2;

    // Move the cube in camera space
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -10.0f);

    // Rotate the cube around the Y axis
    glRotatef(angle, 0.0f, 1.0f, 0.0f);
    glRotatef(angle, 1.0f, 1.0f, 1.0f);

    drawCube();
}

int main(int argc, char** argv) {
    RenderDevice* rd = new RenderDevice();
    OSWindow::Settings settings;

    settings.width = 960;
    settings.height = 600;

    rd->init(settings);

    for (int i = 0; i < 100; ++i) {
        drawFrame(settings.width, settings.height, i);

        // Render at 30 fps
        System::sleep(1.0/30.0);

        // See also RenderDevice::beginFrame, RenderDevice::endFrame
        rd->swapBuffers();
    }

    rd->cleanup();
    delete rd;

    return 0;
}
