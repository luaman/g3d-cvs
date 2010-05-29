/**
 @file Framebuffer.h

 @maintainer Morgan McGuire
 @cite Initial implementation by Daniel Hilferty, djhilferty@users.sourceforge.net

 Notes:
 <UL>
 <LI>http://oss.sgi.com/projects/ogl-sample/registry/EXT/framebuffer_object.txt
 </UL>

 @created 2006-01-07
 @edited  2009-09-17
*/

#ifndef GLG3D_Framebuffer_h
#define GLG3D_Framebuffer_h

#include "G3D/Array.h"
#include "G3D/Table.h"
#include "G3D/Rect2D.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Renderbuffer.h"
#include <string.h>

namespace G3D {

/** \deprecated */
typedef ReferenceCountedPointer<class Framebuffer> FramebufferRef;

/**
 @brief Holds a set of G3D::Textures or G3D::Renderbuffers for use as draw targets.
 
 Abstraction of OpenGL's Framebuffer Object.  This is an efficient way
 of rendering to Textures. 

 RenderDevice::setFramebuffer automatically configures the appropriate
 OpenGL draw buffers.  These are maintained even if the frame buffer
 is changed while set on the RenderDevice.  Inside a pixel shader
 gl_FragData[i] is the ith attached buffer, in number order.  For
 example, if there are attachments to buffer0 and buffer2, then
 gl_FragData[0] maps to buffer0 and gl_FragData[1] maps to buffer2.


 Basic Framebuffer Theory:
	Every OpenGL program has at least one Framebuffer.  This framebuffer is
 setup by the windowing system and its image format is that specified by the
 OS.  With the Framebuffer Object extension, OpenGL gives the developer
 the ability to create offscreen framebuffers that can be used to render 
 to textures of any specified format.

    The Framebuffer class is used in conjunction with the RenderDevice to
 set a render target.  The RenderDevice method setFramebuffer performs this
 action.  If a NULL argument is passed to setFramebuffer, the render target
 defaults to the window display framebuffer.

 The following example shows how to create a texture and bind it to Framebuffer
 for rendering.

 Framebuffer Example:

 <PRE>
    // Create Texture
	static Texture::Ref tex = Texture::createEmpty(256, 256, "Rendered Texture", ImageFormat::RGB8, Texture::CLAMP, Texture::NEAREST_NO_MIPMAP, Texture::DIM_2D);

	// Create a framebuffer that uses this texture as the color buffer
	static FramebufferRef fb = Framebuffer::create("Offscreen target");
	bool init = false;

	if (! init) {
		fb->set(Framebuffer::COLOR0, tex);
		init = true;
	}

	rd->pushState();
		rd->setFramebuffer(fb);
		rd->push2D(fb->rect2DBounds());

			// Set framebuffer as the render target

			// Draw on the texture
			Draw::rect2D(Rect2D::xywh(0,0,128,256), rd, Color3::white());
			Draw::rect2D(Rect2D::xywh(128,0,128,256), rd, Color3::red());

			// Restore renderdevice state (old frame buffer)
		rd->pop2D();
	rd->popState();

	app->renderDevice->setProjectionAndCameraMatrix(app->debugCamera);

	// Remove the texture from the framebuffer
	//	fb->set(Framebuffer::COLOR0, NULL);

	// Can now render from the texture

    
    // Cyan background
    app->renderDevice->setColorClearValue(Color3(.1f, .5f, 1));
    app->renderDevice->clear();

    app->renderDevice->push2D();
		rd->setTexture(0, tex);
		Draw::rect2D(Rect2D::xywh(10,10,256,256), rd);
	app->renderDevice->pop2D();
  </PRE>

 In addition to Textures, Renderbuffers may also be bound to the
 Framebuffer.  This is done in a very similar manner to the Texture
 object in the example.  Renderbuffer class contains an example of
 this operation.

 Note: Not any combination of images may be attached to a Framebuffer.
 OpenGL lays out some restrictions that must be considered:

 <ol>
   <li> In order to render to a Framebuffer, there must be at least
   one image (Renderbuffer or Texture) attached to an attachment point.
	
   <li> All images must have the same height and width.
   
   <li> All images attached to a COLOR_ATTACHMENT[n] point must have
   the same internal format (RGBA8, RGBA16...etc)
   
   <li> If RenderDevice->setDrawBuffer is used then the specified 
   attachment point must have a bound image.
	
   <li> The combination of internal formats of attached images does not
   violate some implementation-dependent set of restrictions (i.e., Your
   graphics card must completely implement all combinations that you
   plan to use!)
 </ol>

 If you create a Framebuffer with a single, depth Renderbuffer
 attached (e.g., for shadow map rendering) it is complete in the
 OpenGL sense, however you will receive a completeness error because
 the glDrawBuffer and glReadBuffer attached to that Framebuffer have
 incorrect defaults.  To fix this, call
 <code>glDrawBuffer(GL_NONE);glReadBuffer(GL_NONE);</code>
 <b>after</b> binding the Framebuffer to the RenderDevice but before
 rendering.
*/
class Framebuffer : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Framebuffer> Ref;
    /**
       Specifies which channels of the framebuffer the renderbuffer or texture will 
       define. These mirror
       the OpenGL definition as do their values.
       
       A DEPTH_STENCIL format renderbuffer or texture can be attached to either the 
       DEPTH_ATTACHMENT or the STENCIL_ATTACHMENT, or both simultaneously; Framebuffer will
       understand the format and use the appropriate channels.
    */
    enum AttachmentPoint {
        COLOR0 = GL_COLOR_ATTACHMENT0,  // = 0x8CE0, so all color attachments have lower value than depth
        /** @deprecated Use COLOR0 */
        COLOR_ATTACHMENT0   = GL_COLOR_ATTACHMENT0,

        COLOR1 = GL_COLOR_ATTACHMENT1,
        /** @deprecated */
        COLOR_ATTACHMENT1   = GL_COLOR_ATTACHMENT1,

        COLOR2 = GL_COLOR_ATTACHMENT2,
        /** @deprecated */
        COLOR_ATTACHMENT2   = GL_COLOR_ATTACHMENT2,

        COLOR3 = GL_COLOR_ATTACHMENT3,
        /** @deprecated */
        COLOR_ATTACHMENT3   = GL_COLOR_ATTACHMENT3,

        COLOR4 = GL_COLOR_ATTACHMENT4,
        /** @deprecated */
        COLOR_ATTACHMENT4   = GL_COLOR_ATTACHMENT4,

        COLOR5 = GL_COLOR_ATTACHMENT5,
        /** @deprecated */
        COLOR_ATTACHMENT5   = GL_COLOR_ATTACHMENT5,

        COLOR6 = GL_COLOR_ATTACHMENT6,
        /** @deprecated */
        COLOR_ATTACHMENT6   = GL_COLOR_ATTACHMENT6,

        COLOR7 = GL_COLOR_ATTACHMENT7,
        /** @deprecated */
        COLOR_ATTACHMENT7   = GL_COLOR_ATTACHMENT7,

        COLOR8 = GL_COLOR_ATTACHMENT8,
        /** @deprecated */
        COLOR_ATTACHMENT8   = GL_COLOR_ATTACHMENT8,

        COLOR9 = GL_COLOR_ATTACHMENT9,
        /** @deprecated */
        COLOR_ATTACHMENT9   = GL_COLOR_ATTACHMENT9,

        COLOR10 = GL_COLOR_ATTACHMENT10,
        /** @deprecated */
        COLOR_ATTACHMENT10  = GL_COLOR_ATTACHMENT10,

        COLOR11 = GL_COLOR_ATTACHMENT11,
        /** @deprecated */
        COLOR_ATTACHMENT11  = GL_COLOR_ATTACHMENT11,

        COLOR12 = GL_COLOR_ATTACHMENT12,
        /** @deprecated */
        COLOR_ATTACHMENT12  = GL_COLOR_ATTACHMENT12,

        COLOR13 = GL_COLOR_ATTACHMENT13,
        /** @deprecated */
        COLOR_ATTACHMENT13  = GL_COLOR_ATTACHMENT13,

        COLOR14 = GL_COLOR_ATTACHMENT14,
        /** @deprecated */
        COLOR_ATTACHMENT14  = GL_COLOR_ATTACHMENT14,

        COLOR15 = GL_COLOR_ATTACHMENT15,
        /** @deprecated */
        COLOR_ATTACHMENT15  = GL_COLOR_ATTACHMENT15,

        DEPTH = GL_DEPTH_ATTACHMENT,
        /** @deprecated */
        DEPTH_ATTACHMENT   = GL_DEPTH_ATTACHMENT,

        STENCIL = GL_STENCIL_ATTACHMENT,
        /** @deprecated */
        STENCIL_ATTACHMENT = GL_STENCIL_ATTACHMENT,
    };

    class Attachment : public ReferenceCountedObject {
    public:
        friend class Framebuffer;

        typedef ReferenceCountedPointer<Attachment> Ref;

        enum Type {TEXTURE, RENDERBUFFER};

    private:

        Type                        m_type;

        AttachmentPoint             m_point;

        Renderbuffer::Ref           m_renderbuffer;

        Texture::Ref                m_texture;

        /** If texture is a Texture::CUBE_MAP, this is the face that
            is attached. */
        Texture::CubeFace           m_cubeFace;
        
        /** Mip level being rendered to */
        int                         m_mipLevel;

        Attachment(AttachmentPoint ap, const Renderbuffer::Ref& r);

        Attachment(AttachmentPoint ap, const Texture::Ref& r, Texture::CubeFace c, int miplevel);
        
        /** Assumes the point is correct */
        bool equals(const Texture::Ref& t, Texture::CubeFace f, int miplevel) const;

        /** Assumes the point is correct */
        bool equals(const Renderbuffer::Ref& r) const;

        bool equals(const Attachment::Ref& other) const;

        /** Called from sync() to actually force this to be attached
            at the OpenGL level.  Assumes the framebuffer is already
            bound.*/
        void attach() const;

        /** Called from sync() to actually force this to be detached
            at the OpenGL level.  Assumes the framebuffer is already
            bound.*/
        void detach() const;

    public:
       
        inline Type type() const {
            return m_type;
        } 

        inline AttachmentPoint point() const {
            return m_point;
        }

        inline const Renderbuffer::Ref& renderbuffer() const {
            return m_renderbuffer;
        }

        inline const Texture::Ref& texture() const {
            return m_texture;
        }

        inline Texture::CubeFace cubeFace() const {
            return m_cubeFace;
        }

        inline int mipLevel() const {
            return m_mipLevel; 
        }

        const ImageFormat* format() const;

        Vector2 vector2Bounds() const;

        int width() const;

        int height() const;
    };
    
protected:

    /** Framebuffer name */
    std::string                     m_name;

    /** True when desiredAttachment != m_currentAttachment. 
        Set to true by set().  Set to false by sync(), which is
        called by RenderDevice::sync().
     */
    bool                            m_currentOutOfSync;

    /** What should be attached on this framebuffer, according to
        set() calls that have been made. */
    Array<Attachment::Ref>          m_desired;

    /** What is actually attached on this framebuffer as far as OpenGL
        is concerned. */
    Array<Attachment::Ref>          m_current;

    /** The GL buffer names of the m_currentAttachment, in increasing
        number order (parallel array to m_currentAttachment). Needed by
        RenderDevice for synchronizing the glDrawBuffers.*/
    Array<GLenum>                   m_colorDrawBufferArray;

    /** OpenGL Object ID */
    GLuint                          m_framebufferID;
    
    /** Adds \a a to m_desired. */
    void set(const Attachment::Ref& a);

    Framebuffer(const std::string& name, GLuint framebufferID);
    
    /** Returns the index in m_desired where ap is, or
        where it should be inserted if it is not present.*/
    int find(AttachmentPoint ap, bool& found) const;

    int findCurrent(AttachmentPoint ap) const;

    /** Executes the synchronization portion of bind() */
    void sync();

    /** Called from sync() to actually force \a a to be attached
        at the OpenGL level.  Assumes the framebuffer is already
        bound.
    */
    void attach(const Attachment::Ref& a);

    /** Called from sync() to actually force \a a to be detached
        at the OpenGL level.  Assumes the framebuffer is already
        bound.
        
        \param dIndex index in m_colorDrawBufferArray from which to
        remove the appropriate enum value.

        Argument can't be a & reference because it is being removed 
        from the only place holding it.
    */
    void detach(Attachment::Ref a);

public:
    
    /** Creates a Framebuffer object.
       
       \param name Name of framebuffer, for debugging purposes. */
    static Ref create(const std::string& name);

    /** Bind this framebuffer and force all of its attachments to
        actually be attached at the OpenGL level.  The latter step is
        needed because set() is lazy.

        <b>Primarily used by RenderDevice.  Developers should not need
        to explicitly call this method or glDrawBuffers.</b>

        After binding, you also have to set the glDrawBuffers to match
        the capabilities of the Framebuffer that is currently bound.

        \param alreadyBound If true, do not bother binding the FBO
        itself, just sync any out of date attachments.

        \return True if openGLDrawArray() was changed by this call
    */
    bool bind(bool alreadyBound = false);

    /** Bind the current context's default Framebuffer, instead of an
        application-created one. 

        <b>Primarily used by RenderDevice.  Developers should not need
        to explicitly call this method or glDrawBuffers.</b>
    */
    static void bindWindowBuffer();

    /** Returns the attachment currently at ap, or NULL if there is
     not one.  \sa has()*/
    Attachment::Ref get(AttachmentPoint ap) const;

    /**
     Number of currently bound attachments.  When this hits zero we can
     add attachments with new sizes.
     */
    inline int numAttachments() const {
        return m_desired.size();
    }

    /** The draw array for use with glDrawBuffers. This is not up to
        date until bind() is invoked.

        Note that DEPTH and STENCIL are never included in this list.

        RenderDevice automatically uses this.*/
    inline const Array<GLenum>& openGLDrawArray() const {
        return m_colorDrawBufferArray;
    }
    
    /** Reclaims OpenGL ID.  All buffers/textures are automatically
        detatched on destruction. */
    ~Framebuffer();
    
   
    /** Overload used when setting attachment points to NULL */
    void set(AttachmentPoint ap, const void* n);
    
    void set(AttachmentPoint ap, const Texture::Ref& texture);
    
    /**
       Set one of the attachment points to reference a Texture.  Set
       to NULL or call clear() to unset.  Auto-mipmap will
       automatically be disabled on set.
       
       Do not use a texture that is bound to the *current* Framebuffer
       as a source texture, however, you can render a surface using a
       Texture that is bound on a different Framebuffer.  In general,
       create one Framebuffer per set of textures you wish to render 
       to and just leave them bound at all times.

       All set() calls are lazy because OpenGL provides no mechanism
       for efficiently pushing and popping the Framebuffer. Thus all
       calls to actually set attachments must be delayed until the
       bind() call, when this Framebuffer is guaranteed to be bound.
       
       \param texture	 Texture to bind to the Framebuffer.
       \param ap	 Attachment point to bind texture to.
       \param mipLevel   Target MIP-map level to render to.
    */
    void set(AttachmentPoint ap, const Texture::Ref& texture, 
             Texture::CubeFace face, int mipLevel = 0);
    
    /**
       Set one of the attachment points to reference a renderbuffer.
       Set to NULL to unset.
       
       @param renderbuffer	Renderbuffer to bind to the framebuffer
       @param slot		Attachment point to bind renderbuffer to.
    */
    void set(AttachmentPoint ap, const Renderbuffer::Ref& renderbuffer);
    
    /** Returns true if this attachment is currently non-NULL.*/
    bool has(AttachmentPoint ap) const;
    
    /**
       The OpenGL ID of the underlying framebuffer object.
    */
    inline unsigned int openGLID() const {
        return m_framebufferID;
    }
    
    int width() const;
    
    int height() const;
    
    Rect2D rect2DBounds() const;
    
    Vector2 vector2Bounds() const;

    inline const std::string& name() const {
        return m_name;
    }

    /** Detach all attachments.  This is lazy; see set() for discussion.*/
    void clear();

}; // class Framebuffer 

typedef Framebuffer FrameBuffer;
/** @deprecated */
typedef FramebufferRef FrameBufferRef;

} //  G3D

#endif // GLG3D_FRAMEBUFFER_H

