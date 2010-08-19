/**
 */
#include <stdio.h>
#include <string>
#include "assert.h"
#include "glew.h"
#include "glut.h"
#include <algorithm>
#ifdef _MSC_VER
#   include <windows.h>
#   pragma comment(lib, "glut32.lib")
#   undef min
#   undef max
#endif


#define WIDTH (256)
#define HEIGHT (256)

const int shadowMapWidth = WIDTH;
const int shadowMapHeight = HEIGHT;
const GLenum shadowMapTextureTarget = GL_TEXTURE_2D;
GLuint shadowMapFramebuffer = 0;
GLuint shadowMapTexture = 0;    
    
GLuint shaderProgram = 0;

static const float squareVertices[] = {
    0.25f, 0.25f, 0, 
    0.25f, 0.75f, 0,
    0.75f, 0.75f, 0,
    0.75f, 0.25f, 0};
    
static const float fullScreenVertices[] = {
    0,0,0, 
    0,1,0,
    1,1,0,
    1,0,0};


void assertGLOK() {
    GLenum e;
    while ((e = glGetError()) != GL_NO_ERROR) {
        assert(e == GL_NO_ERROR);
    }
}

void quitOnEscape(unsigned char key, int x, int y) {
    if (key == 27) { ::exit(0); }
}

void render() {
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    glDrawBuffer(GL_BACK);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 1, 0, 0, 2);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw a full-screen quad, with 0-1 texture coordinates
    glVertexPointer(3, GL_FLOAT, 0, fullScreenVertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(float) * 2, fullScreenVertices);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glUseProgram(shaderProgram);
    static const GLint shadowMapUnit = 0;
    const GLint shadowMapLocation = glGetUniformLocation(shaderProgram, "shadowMap");
    assert(shadowMapLocation != -1);
    glUniform1iARB(shadowMapLocation, shadowMapUnit);
    glActiveTexture(GL_TEXTURE0 + shadowMapUnit);
    glBindTexture(shadowMapTextureTarget, shadowMapTexture);

    glUniform1fARB(glGetUniformLocation(shaderProgram, "width"), WIDTH);
    glUniform1fARB(glGetUniformLocation(shaderProgram, "height"), HEIGHT);

    glDrawArrays(GL_QUADS, 0, 4);
    glUseProgram(GL_NONE);
    assertGLOK();

    glutSwapBuffers();
}


void createShadowMap() {
    // Create and configure texture
    assertGLOK();
    glGenTextures(1, &shadowMapTexture);
    glBindTexture(shadowMapTextureTarget, shadowMapTexture);
    glTexParameteri(shadowMapTextureTarget, GL_GENERATE_MIPMAP_SGIS, GL_FALSE);
    glTexParameteri(shadowMapTextureTarget, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
    glTexParameteri(shadowMapTextureTarget, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
    glTexParameteri(shadowMapTextureTarget, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(shadowMapTextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(shadowMapTextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(shadowMapTextureTarget, 0, GL_DEPTH_COMPONENT32, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);    
    glBindTexture(shadowMapTextureTarget, GL_NONE);
    assertGLOK();

    // Create and configure framebuffer
    glGenFramebuffers(1, &shadowMapFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFramebuffer);
    assert(shadowMapTextureTarget == GL_TEXTURE_2D);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMapTextureTarget, shadowMapTexture, 0);
    glDrawBuffer(GL_NONE);
    GLenum e = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(e == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);


    assertGLOK();
}


void renderShadowMap() {
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFramebuffer);
    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDrawBuffer(GL_NONE);
    glClear(GL_DEPTH_BUFFER_BIT);
    assertGLOK();

    // Draw a square in the middle of the screen
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 1, 0, 0, 2);
    assertGLOK();

    // Draw the square
    glVertexPointer(3, GL_FLOAT, 0, squareVertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_QUADS, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    assertGLOK();
}

GLhandleARB compileShader(GLenum type, const char* source) {
    GLint sourceLengths[1];
    const GLchar* sourceStrings[1];

    GLint compiled = GL_FALSE;

    GLhandleARB shader = 0;
    shader = glCreateShaderObjectARB(type);

    sourceLengths[0] = strlen(source);
    sourceStrings[0] = static_cast<const GLchar*>(source);

    glShaderSource(shader, 1, sourceStrings, sourceLengths);
    glCompileShader(shader);

    glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);

    if (compiled != GL_TRUE) {
        GLint maxLength = 0;
        GLint actualLength = 0;
        glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &maxLength);
        GLchar* pInfoLog = new GLchar[maxLength];
        glGetInfoLogARB(shader, maxLength, &actualLength, pInfoLog);
        assert(compiled == GL_TRUE);
    }

    return shader;
}

void createShader() {
    const char* vertexSource =
        "#version 150 compatibility\n"
        "\n"
        "void main() {\n"
        "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
        "}\n";

    const char* pixelSource = 
        "#version 150 compatibility\n"
        "\n"
        "uniform sampler2DShadow shadowMap;"
        "out vec4 color;\n"
        "uniform float width;\n"
        "uniform float height;\n"
        "\n"
        "void main() {\n"
        "   const float zTest = 0.5;\n"
        "   float shadowResult = texture(shadowMap, vec3(gl_FragCoord.x / width, gl_FragCoord.y / height, zTest)).r;"
        "   color = vec4(vec3(shadowResult), 1.0);\n"
        "}\n";

    const GLhandleARB vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    const GLhandleARB fragmentShader = compileShader(GL_FRAGMENT_SHADER, pixelSource);

    shaderProgram = glCreateProgramObjectARB();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint linked = GL_FALSE;
    glGetObjectParameterivARB(shaderProgram, GL_OBJECT_LINK_STATUS_ARB, &linked);
    assert(linked == GL_TRUE);
    assertGLOK();
}


void savePGM(const char* filename, int width, int height, const float* data) {
    //http://netpbm.sourceforge.net/doc/pgm.html
    FILE* file = fopen(filename, "wt");
    fprintf(file, "P2 %d %d 255\n", width, height);                                                    
    for (int y = 0; y < height; ++y) {
        fprintf(file, "\n# y = %d\n", y);                                                                  
        for (int x = 0; x < width; ++x) {
            fprintf(file, "%d\n", std::min(255, std::max(0, int(255 * data[x + y * width]))));
        }
    }
    fflush(file);
    fclose(file);
}


void saveShadowMap() {

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFramebuffer);

    float* data = new float[shadowMapWidth * shadowMapHeight];
    glReadBuffer(GL_NONE);
    glReadPixels(0, 0, shadowMapWidth, shadowMapHeight, 
                    GL_DEPTH_COMPONENT, GL_FLOAT, data);
    assertGLOK();
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    glReadBuffer(GL_NONE);

    savePGM("shadowmap.pgm", shadowMapWidth, shadowMapHeight, data);
    /*
    int numNonZero = 0;
    for (int y = 0; y < shadowMapHeight; ++y) {
        for (int x = 0; x < shadowMapWidth; ++x) {
            if (data[x + y * shadowMapWidth] != 0.0f) {
                ++numNonZero;
            }
        }
    }*/

    delete[] data;
}

int main(int argc, char** argv) {
    
    // Initialize OpenGL
    glutInit(&argc, argv);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutCreateWindow("OpenGL - ESC to Quit");

    // Initialize OpenGL extensions
    glewInit();

    // Set GLUT callbacks
    glutKeyboardFunc(&quitOnEscape);
    glutDisplayFunc(&render);

    createShader();
    createShadowMap();
    renderShadowMap();

    saveShadowMap();

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
