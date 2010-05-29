/**
@file Renderbuffer.cpp

@maintainer Daniel Hilferty, djhilferty@users.sourceforge.net

@cite http://oss.sgi.com/projects/ogl-sample/registry/EXT/framebuffer_object.txt

@created 2006-01-07
@edited  2006-02-14
*/

#include "GLG3D/Renderbuffer.h"
#include "GLG3D/glcalls.h"
#include "G3D/ImageFormat.h"

namespace G3D {


/////////////////////////////////////////////////////////////////////////////

Renderbuffer::Renderbuffer (	
    const std::string&          _name, 
    const GLuint                _imageID,
    const G3D::ImageFormat*   _format, 
    const int                   _width, 
    const int                   _height) : 
    mName(_name),
    mImageID(_imageID),
    mFormat(_format),
    mWidth(_width),
    mHeight(_height) {

    // Intentionally empty
}


Renderbuffer::~Renderbuffer () {
    glDeleteRenderbuffers(1, &mImageID);
}


RenderbufferRef Renderbuffer::fromGLRenderbuffer(
    const std::string&		 _name, 
    const GLuint             _imageID,
    const G3D::ImageFormat*  _format) {

    GLint w, h;

    // Extract the width and height
    glGetRenderbufferParameteriv(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_WIDTH_EXT, &w);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_WIDTH_EXT, &h);
    debugAssertGLOk();

    // Create new renderbuffer
    return new Renderbuffer(_name, _imageID, _format, w, h);
}


RenderbufferRef Renderbuffer::createEmpty(
    const std::string&      _name, 
    const int               _width, 
    const int               _height,
    const G3D::ImageFormat* _format) { 
    // New Renderbuffer ID
    GLuint _imageID;

    // Save old renderbuffer state
    GLint origBuffer;
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &origBuffer);

    // Generate buffer
    glGenRenderbuffers(1, &_imageID);
    debugAssertGLOk();

    // Bind the buffer
    glBindRenderbuffer(GL_RENDERBUFFER, _imageID);
    debugAssertGLOk();

    // Allocate storage for it
    glRenderbufferStorage(GL_RENDERBUFFER, _format->openGLFormat, _width, _height);

    // Check for successful generation (ie, no INVALID_OPERATION)
    debugAssertGLOk();

    // Restore renderbuffer state
    glBindRenderbuffer(GL_RENDERBUFFER, origBuffer);
    debugAssertGLOk();

    // Create new renderbuffer
    return new Renderbuffer(_name, _imageID, _format, _width, _height);
}

} // End Namespace G3D
