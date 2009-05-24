/**
   @file Framebuffer.cpp

   @author Daniel Hilferty, djhilferty@users.sourceforge.net
   @maintainer Morgan McGuire
   
   Notes:
   <UL>
   <LI>http://oss.sgi.com/projects/ogl-sample/registry/EXT/framebuffer_object.txt
   </UL>
   
   @created 2006-01-07
   @edited  2009-04-11
*/

#include "GLG3D/Framebuffer.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/GLCaps.h"

namespace G3D {

Framebuffer::Framebuffer(
    const std::string&  name, 
    GLuint              framebufferID) : 
    m_name(name),
    m_framebufferID(framebufferID) {
}


Framebuffer::~Framebuffer () {
    if (m_framebufferID != 0) {
        glDeleteFramebuffersEXT(1, &m_framebufferID);
        m_framebufferID = 0;
    }
}


FramebufferRef Framebuffer::create(const std::string& _name) {
    GLuint _framebufferID;
    
    // Generate Framebuffer
    glGenFramebuffersEXT(1, &_framebufferID);
    debugAssertGLOk();

    return new Framebuffer(_name, _framebufferID);
}


bool Framebuffer::has(AttachmentPoint ap) const {
    bool found = false;
    find(ap, found);
    return found;
}


int Framebuffer::find(AttachmentPoint ap, bool& found) const {
    for (int i = 0; i < m_desired.size(); ++i) {
        if (m_desired[i]->m_point >= ap) {
            found = (m_desired[i]->m_point == ap);
            return i;
        }
    }

    found = false;
    return m_desired.size();
}


int Framebuffer::findCurrent(AttachmentPoint ap) const {
    for (int i = 0; i < m_current.size(); ++i) {
        if (m_current[i]->m_point >= ap) {
            return i;
        }
    }

    return m_current.size();
}


void Framebuffer::set(AttachmentPoint ap, const void* n) {
    (void)n;
    debugAssert(n == NULL);

    bool found = false;
    int i = find(ap, found);
    if (found) {
        m_desired.remove(i);
        m_currentOutOfSync = true;
    }
}


void Framebuffer::set(AttachmentPoint ap, const Texture::Ref& texture) {
    set(ap, texture, Texture::CUBE_NEG_X);
}

    
void Framebuffer::set(AttachmentPoint ap, const Texture::Ref& texture, Texture::CubeFace face) {
    if (texture.isNull()) {
        // We're in the wrong overload due to C++ static type dispatch
        set(ap, NULL);
        return;
    }

    if (m_desired.size() > 0) {
        if (! ((m_desired.size() == 1) &&
            (m_desired[0]->m_point == ap))) {
        debugAssertM(texture->vector2Bounds() == m_desired[0]->vector2Bounds(), 
                     format("Cannot attach a Texture of size %s to a Framebuffer"
                            " of size %s without calling Framebuffer::clear first.",
                            texture->vector2Bounds().toString().c_str(),
                            m_desired[0]->vector2Bounds().toString().c_str()));
        }
    }
    
    Attachment::Ref a = get(ap);
    if (a.isNull() || ! (a->equals(texture, face))) {
        // This is a change
        set(new Attachment(ap, texture, face));
        texture->invertY = true;
    }
}


void Framebuffer::set(AttachmentPoint ap, const Renderbuffer::Ref& b) {
    if (b.isNull()) {
        // We're in the wrong overload due to C++ static type dispatch
        set(ap, NULL);
        return;
    }

    if (m_desired.size() > 0) {
        debugAssertM(b->vector2Bounds() == m_desired[0]->vector2Bounds(), 
                     format("Cannot attach a Renderbuffer of size %s to a Framebuffer"
                            " of size %s without calling Framebuffer::clear first.",
                            b->vector2Bounds().toString().c_str(),
                            m_desired[0]->vector2Bounds().toString().c_str()));
    }
                    
    
    Attachment::Ref a = get(ap);
    if (a.isNull() || ! (a->equals(b))) {
        // This is a change
        set(new Attachment(ap, b));
    }
}


void Framebuffer::set(const Attachment::Ref& a) {
    bool found = false;
    int i = find(a->point(), found);
    if (found) {
        m_desired[i] = a;
    } else {
        m_desired.insert(i, a);
    }
    m_currentOutOfSync = true;
}


Framebuffer::Attachment::Ref Framebuffer::get(AttachmentPoint ap) const {
    bool found = false;
    int i = find(ap, found);
    if (! found) {
        return NULL;
    } else {
        return m_desired[i];
    }
}


bool Framebuffer::bind(bool alreadyBound) {
    if (! alreadyBound) {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, openGLID());
    }

    if (m_currentOutOfSync) {
        sync();
        return true;
    } else {
        return false;
    }
}


void Framebuffer::bindWindowBuffer() {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}


void Framebuffer::clear() {
    m_desired.clear();
    m_currentOutOfSync = true;
}


void Framebuffer::sync() {
    debugAssert(m_currentOutOfSync);
    int d = 0;
    int c = 0;

    // Walk both the desired and current arrays
    while ((d < m_desired.size()) && (c < m_current.size())) {
        // Cannot be const references since the code below mutates the
        // m_desired and m_current arrays.
        Attachment::Ref da = m_desired[d];
        Attachment::Ref ca = m_current[c];

        if (da->equals(ca)) {
            // Matched; nothing to do
            ++d;
            ++c;
        } else {
            // Mismatch
            if (da->point() >= ca->point()) {
                // Remove current.  Don't change 
                // c index because we just shrank m_current.
                detach(ca);
            }
            
            if (da->point() <= ca->point()) {
                // Add desired
                attach(da);
                ++d;

                // Increment c, since we just inserted at it or earlier
                ++c;
            }
        }
    }

    // Only one of the following two loops will execute. This one must be first because
    // it assumes that c is unchanged from the previous loop.
    while (c < m_current.size()) {
        // Don't change c index because we just shrank m_current.
        detach(m_current[c]);
    }

    while (d < m_desired.size()) {
        attach(m_desired[d]);
        ++d;
    }

    // We are now in sync
    m_currentOutOfSync = false;

    debugAssert(m_current.length() == m_desired.length());

    debugAssert(
        m_colorDrawBufferArray.length() >= m_current.length() - 2 && 
        m_colorDrawBufferArray.length() <= m_current.length());
}


void Framebuffer::attach(const Attachment::Ref& a) {
    int cIndex = findCurrent(a->point());
    if (a->point() < DEPTH) {
        m_colorDrawBufferArray.insert(cIndex, GLenum(a->point()));
    }
    m_current.insert(cIndex, a);
    a->attach();
}


void Framebuffer::detach(Attachment::Ref a) {
    int cIndex = findCurrent(a->point());
    if (a->point() < DEPTH) {
        m_colorDrawBufferArray.remove(cIndex);
    }
    m_current.remove(cIndex);
    a->detach();
}


int Framebuffer::width() const {
    if (m_desired.size() > 0) {
        return m_desired[0]->width();
    } else { 
        return 0;
    }
}
 

int Framebuffer::height() const {
    if (m_desired.size() > 0) {
        return m_desired[0]->height();
    } else { 
        return 0;
    }
}

    
Rect2D Framebuffer::rect2DBounds() const {
    if (m_desired.size() > 0) {
        return Rect2D::xywh(Vector2::zero(), m_desired[0]->vector2Bounds());
    } else {
        return Rect2D::xywh(0,0,0,0);
    }
}

    
Vector2 Framebuffer::vector2Bounds() const {
    if (m_desired.size() > 0) {
        return m_desired[0]->vector2Bounds();
    } else {
        return Vector2::zero();
    }
}

////////////////////////////////////////////////////////////////////

Framebuffer::Attachment::Attachment(AttachmentPoint ap, const RenderbufferRef& r) : 
    m_type(RENDERBUFFER),
    m_point(ap),
    m_renderbuffer(r),
    m_cubeFace(Texture::CUBE_NEG_X) {
}


Framebuffer::Attachment::Attachment(AttachmentPoint ap, const Texture::Ref& r, Texture::CubeFace c) : 
    m_type(TEXTURE), 
    m_point(ap),
    m_texture(r),
    m_cubeFace(c) {
    
    if (m_texture->settings().autoMipMap) {
        m_texture->setAutoMipMap(false);
    }
}

 
Vector2 Framebuffer::Attachment::vector2Bounds() const {
    if (m_type == TEXTURE) {
        return m_texture->vector2Bounds();
    } else {
        return m_renderbuffer->vector2Bounds();
    }
}


int Framebuffer::Attachment::width() const {
    if (m_type == TEXTURE) {
        return m_texture->width();
    } else {
        return m_renderbuffer->width();
    }
}
    

int Framebuffer::Attachment::height() const {
    if (m_type == TEXTURE) {
        return m_texture->height();
    } else {
        return m_renderbuffer->height();
    }
}


bool Framebuffer::Attachment::equals(const Texture::Ref& t, Texture::CubeFace c) const {
    return
        (m_type == TEXTURE) &&
        (m_texture == t) &&
        (m_cubeFace == c);
}


bool Framebuffer::Attachment::equals(const Renderbuffer::Ref& r) const {
    return
        (m_type == RENDERBUFFER) &&
        (m_renderbuffer == r);
}


bool Framebuffer::Attachment::equals(const Attachment::Ref& other) const {
    return
        (m_type == other->m_type) &&
        (m_point == other->m_point) &&
        (m_texture == other->m_texture) &&
        (m_cubeFace == other->m_cubeFace) &&
        (m_renderbuffer == other->m_renderbuffer);
}


void Framebuffer::Attachment::attach() const {
    if (m_type == TEXTURE) {
        bool isCubeMap = 
            (m_texture->dimension() == Texture::DIM_CUBE_MAP) ||
            (m_texture->dimension() == Texture::DIM_CUBE_MAP_NPOT);
        

        if (isCubeMap) {
            glFramebufferTexture2DEXT
                (GL_FRAMEBUFFER_EXT, GLenum(m_point),
                 GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)m_cubeFace, m_texture->openGLID(), 0);
        } else {
            glFramebufferTexture2DEXT
                (GL_FRAMEBUFFER_EXT, GLenum(m_point),
                 m_texture->openGLTextureTarget(), 
                 m_texture->openGLID(), 0);
        }
    } else {
        glFramebufferRenderbufferEXT
            (GL_FRAMEBUFFER_EXT, GLenum(m_point), 
             GL_RENDERBUFFER_EXT, m_renderbuffer->openGLID());
    }
}


void Framebuffer::Attachment::detach() const {
    if (m_type == TEXTURE) {
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GLenum(m_point), GL_TEXTURE_2D, 0, 0);
    } else {
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GLenum(m_point), GL_RENDERBUFFER_EXT, 0);
    }
}


const ImageFormat* Framebuffer::Attachment::format() const {
    if (m_type == TEXTURE) {
        return m_texture->format();
    } else {
        return m_renderbuffer->format();
    }
}

} // G3D
