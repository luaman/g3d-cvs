/**
@file Framebuffer.cpp

@author Daniel Hilferty, djhilferty@users.sourceforge.net
@maintainer Morgan McGuire

Notes:
<UL>
<LI>http://oss.sgi.com/projects/ogl-sample/registry/EXT/framebuffer_object.txt
</UL>

@created 2006-01-07
@edited  2007-08-10
*/

#include "GLG3D/Framebuffer.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/GLCaps.h"

namespace G3D {

Framebuffer::Framebuffer(
    const std::string&  _name, 
    GLuint              _framebufferID) : 
    framebufferID(_framebufferID),
    m_name(_name),
    m_height(0),
    m_width(0) {
}


Framebuffer::~Framebuffer () {
    if (framebufferID != 0) {
        glDeleteFramebuffersEXT(1, &framebufferID);
        framebufferID = 0;
    }
}


FramebufferRef Framebuffer::fromGLFramebuffer(const std::string& _name, GLuint _framebufferID) {
    alwaysAssertM(_framebufferID == 0 || GLCaps::supports_GL_EXT_framebuffer_object(), 
        "Framebuffer Object not supported!");

	// TODO: If there are existing attachments, find their size
    return new Framebuffer(_name, _framebufferID);
}


FramebufferRef Framebuffer::create(const std::string& _name) {
    GLuint _framebufferID;
    
    // Generate Framebuffer
    glGenFramebuffersEXT(1, &_framebufferID);
    debugAssertGLOk();

    return new Framebuffer(_name, _framebufferID);
}


bool Framebuffer::has(AttachmentPoint ap) const {
    return attachmentTable.containsKey(ap);
}


void Framebuffer::set(AttachmentPoint ap, const void* n) {
    debugAssert(n == NULL);

    // Get current framebuffer
    GLint origFB = glGetInteger(GL_FRAMEBUFFER_BINDING_EXT);
    debugAssertGLOk();

    // If we aren't already bound, bind us now
    if (origFB != (GLint)openGLID()) {
        // Bind this framebuffer
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, openGLID());
        debugAssertGLOk();
    }

    if (attachmentTable.containsKey(ap)) { 
        // Detach
        if (attachmentTable[ap].type == Attachment::TEXTURE) {

            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, ap, 
                                      GL_TEXTURE_2D, 0, 0);
            debugAssertGLOk();

            if (attachmentTable[ap].hadAutoMipMap) {
                attachmentTable[ap].texture->setAutoMipMap(true);
            }
        } else {
            glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, ap, 
                                         GL_RENDERBUFFER_EXT, 0);
			debugAssertGLOk();
        }

        // Wipe our record for that slot
        attachmentTable.remove(ap);

    }

    // If we were already bound, don't bother restoring
    if (origFB != (GLint)openGLID()) {
        // Bind original framebuffer
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, origFB);
        debugAssertGLOk();
    }
}


void Framebuffer::set(AttachmentPoint ap, const Texture::Ref& texture) {
    set(ap, texture, Texture::CUBE_NEG_X);
}

    
void Framebuffer::set(AttachmentPoint ap, const Texture::Ref& texture, Texture::CubeFace face) {

    if (texture.isNull()) {
        // We're in the wrong overload
        set(ap, (void*)NULL);
        return;
    }

    // Get current framebuffer
    GLint origFB = glGetInteger(GL_FRAMEBUFFER_BINDING_EXT);

    // If we aren't already bound, bind us now
    if (origFB != (GLint)openGLID()) {
        // Bind this framebuffer
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, openGLID());
        debugAssertGLOk();
    }

    // Remove the attachment that is being replaced so it does not affect size
    attachmentTable.remove(ap);

    // Check for completeness
    if (numAttachments() == 0) {
        // This is the first attachment.
        // Set texture height/width
        m_width  = texture->texelWidth();
        m_height = texture->texelHeight();
    } else {
        // Verify same dimensions
        debugAssertM((texture->texelWidth() == width()) && 
                     (texture->texelHeight() == height()), 
           "All attachments bound to a Framebuffer must "
                     "have identical dimensions!");
    }
    
    attachmentTable.set(ap, Attachment(texture, face));

    bool isCubeMap = (texture->dimension() == Texture::DIM_CUBE_MAP) ||
        (texture->dimension() == Texture::DIM_CUBE_MAP_NPOT);

    // Bind texture to framebuffer
    if (isCubeMap) {
        glFramebufferTexture2DEXT(
            GL_FRAMEBUFFER_EXT, 
            ap,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, 
            texture->openGLID(), 0);
    } else {
        glFramebufferTexture2DEXT(
            GL_FRAMEBUFFER_EXT, 
            ap, 
            texture->openGLTextureTarget(), 
            texture->openGLID(), 0);
    }
    debugAssertGLOk();

    // If we were already bound, don't bother restoring
    if (origFB != (GLint)openGLID()) {
        // Bind original framebuffer
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, origFB);
    }

    texture->invertY = true;

    debugAssertGLOk();
}


void Framebuffer::set(          
    AttachmentPoint ap,
    const RenderbufferRef& renderbuffer) {

    if (renderbuffer.isNull()) {
        // We're in the wrong overload
        set(ap, (void*)NULL);
        return;
    }

    // Get current framebuffer
    GLint origFB = glGetInteger(GL_FRAMEBUFFER_BINDING_EXT);

    // If we aren't already bound, bind us now
    if (origFB != (GLint)openGLID()) {
        // Bind this framebuffer
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, openGLID());
        debugAssertGLOk();
    }

    // Check for completeness
    if (numAttachments() == 0) {
        // This is the first attachment.
        // Set texture height/width
        m_width  = renderbuffer->width();
        m_height = renderbuffer->height();
    } else {
        // Verify same dimensions
        debugAssertM((renderbuffer->width()  == width()) && 
                     (renderbuffer->height() == height()), 
           "All attachments bound to a Framebuffer must have identical dimensions!");
    }
    
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, ap, GL_RENDERBUFFER_EXT, renderbuffer->openGLID());

    attachmentTable.set(ap, Attachment(renderbuffer));

    // If we were already bound, don't bother restoring
    if (origFB != (GLint)openGLID()) {
        // Bind original framebuffer
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, origFB);
    }

    debugAssertGLOk();
}


void Framebuffer::clear() {
    // Get current framebuffer
    GLint origFB = glGetInteger(GL_FRAMEBUFFER_BINDING_EXT);
    debugAssertGLOk();

    // If we aren't already bound, bind us now
    if (origFB != (GLint)openGLID()) {
        // Bind this framebuffer
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, openGLID());
        debugAssertGLOk();
    }

    for (int i = 0; i < 18; ++i) {
        int ap;
        if (i < 16) {
            ap = COLOR_ATTACHMENT0 + i;
        } else if (i == 16) {
            ap = DEPTH_ATTACHMENT;
        } else if (i == 17) {
            ap = STENCIL_ATTACHMENT;
        }

        if (attachmentTable.containsKey(ap)) { 
            // Detach
            if (attachmentTable[ap].type == Attachment::TEXTURE) {
                
                glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, ap, 
                                          GL_TEXTURE_2D, 0, 0);
                debugAssertGLOk();
                
                if (attachmentTable[ap].hadAutoMipMap) {
                    attachmentTable[ap].texture->setAutoMipMap(true);
                }
            } else {
                glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, ap, 
                                             GL_RENDERBUFFER_EXT, 0);
                debugAssertGLOk();
            }

            // Wipe our record for that slot
            attachmentTable.remove(ap);
        } 

    }

    // If we were already bound, don't bother restoring
    if (origFB != (GLint)openGLID()) {
        // Bind original framebuffer
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, origFB);
        debugAssertGLOk();
    }
}

} // G3D

