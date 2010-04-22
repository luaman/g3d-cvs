/**
   This code sample shows how to use GLUT to initialize OpenGL.  This provides
   fewer features and much less support than the regular G3D initialization,
   but provides a convenient minimal base when tracking down GPU driver bugs
   or other issues for which you would like to rule out G3D's interactions.

   The provided Image class allows simple setPixel() access and can be saved
   to disk or shown on screen (both gamma corrected) for debugging.

   Note that this project requires linking to GLUT, which is not distributed
   with G3D.
 */
#include "supportclasses.h"

int main(int argc, char** argv) {
    Image im(200, 300);
    for (int i = 0; i < im.width(); ++i) {
        for (int j = 0; j < im.height(); ++j) {
            // Checkerboard gradient
            im.set(i, j, (((i + j) & 1) == 0) ? Color3::white() * float(j) / im.height() : Color3::black());
        }
    }

    // Save to disk:
    im.save("output.ppm");

    // Show on screen (never returns):
    im.display();

    return 0;
}
