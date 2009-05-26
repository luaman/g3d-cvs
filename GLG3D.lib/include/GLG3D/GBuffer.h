/**
  @file GBuffer.h
  @author Morgan McGuire, morgan@cs.williams.edu
 */
#ifndef GLG3D_GBuffer_h
#define GLG3D_GBuffer_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Shader.h"
#include "GLG3D/GenericSurface.h"
#include "GLG3D/Surface.h"

namespace G3D {

class RenderDevice;

/** \brief Saito and Takahashi's Geometry Buffers for deferred shading. 
    Contains position, normal, depth, and BSDF parameters.

    Used for rendering a G3D::UberBSDF with deferred shading. 

    Requires GBuffer.pix and GBuffer.vrt at runtime, which can be found
    in the G3D/data-files/SuperShader directory of the G3D distribution.

    \beta This build of G3D only supports GenericSurface with GBuffer.

    Requires glGetInteger(GL_MAX_COLOR_ATTACHMENTS_EXT) >= 5 and 
    pixel and vertex shaders.
*/
class GBuffer : public ReferenceCountedObject {
public:

    typedef ReferenceCountedPointer<GBuffer> Ref;

private:

    std::string                 m_name;

    const ImageFormat*          m_depthFormat;

    /** Renders the GenericSurface Material UberBSDF coefficients
      to the color attachments. */
    Shader::Ref                 m_shader;

    /** The other buffers are permanently bound to this framebuffer */
    Framebuffer::Ref            m_framebuffer;

    /** RGB = diffuse reflectance (Fresnel is not applied), A = undefined */
    Texture::Ref                m_lambertian;

    /** RGB = F0, A = \f$\sigma\f$ (packed glossy exponent).  Fresnel
        is not applied */
    Texture::Ref                m_specular;

    /** RGB = T0, A = eta.  Fresnel is not applied */
    Texture::Ref                m_transmissive;

    /** World-space position */
    Texture::Ref                m_position;

    /** World-space unit normal. */
    Texture::Ref                m_normal;

    /** Depth texture. */
    Texture::Ref                m_depth;

    GBuffer(const std::string& name, const ImageFormat* depthFormat);

    void computeGeneric
    (RenderDevice* rd, 
     const GenericSurface::Ref& model) const;

    void computeNonGeneric
    (RenderDevice* rd, 
     const Surface::Ref& model) const;

    void computeGenericArray
    (RenderDevice* rd, 
     const Array<GenericSurface::Ref>& model) const;

    void computeNonGenericArray
    (RenderDevice* rd, 
     const Array<Surface::Ref>& model) const;

public:

    /** \brief Returns true if GBuffer is supported on this GPU */
    static bool supported();

    static Ref create
    (const std::string& name,
     const ImageFormat* depthFormat = ImageFormat::DEPTH24());

    virtual ~GBuffer();

    int width() const;

    int height() const;

    Rect2D rect2DBounds() const;

    /** The other buffers are permanently bound to this framebuffer */
    inline Framebuffer::Ref framebuffer() const {
        return m_framebuffer;
    }

    /** RGB = diffuse reflectance (Fresnel is not applied), A = undefined */
    inline Texture::Ref lambertian() const {
        return m_lambertian;
    }

    /** RGB = F0, A = \f$\sigma\f$ (packed glossy exponent).  Fresnel
        is not applied */
    inline Texture::Ref specular() const {
        return m_specular;
    }

    /** RGB = T0, A = eta.  Fresnel is not applied */
    inline Texture::Ref transmissive() const {
        return m_transmissive;
    }

    /** World-space position */
    inline Texture::Ref position() const {
        return m_position;
    }

    /** World-space unit normal. */
    inline Texture::Ref normal() const {
        return m_normal;
    }

    /** Depth texture. */
    inline Texture::Ref depth() const {
        return m_depth;
    }

    inline const std::string& name() const {
        return m_name;
    }

    /** Reallocate all buffers to this size if they are not already */
    virtual void resize(int width, int height);

    /** Render the models to this GBuffer set (clearing first).  Depth
        is only cleared if RenderDevice::depthWrite is true.

        Assumes that \a modelArray has already been culled and sorted
        for the camera.

        Performs alpha testing using Lambertian.a.

        Elements of \a modelArray that are not GenericSurface
        will be rendered twice to extract their lambertian component
        and will have no other components.  GenericSurface instances
        render efficiently and will have full BSDF components.
    */
    void compute
    (RenderDevice*                  rd, 
     const GCamera&                 camera,
     const Array<Surface::Ref>&  modelArray) const;
};

} // namespace G3D

#endif
