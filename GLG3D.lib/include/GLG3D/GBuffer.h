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
#include "GLG3D/SuperSurface.h"
#include "GLG3D/Surface.h"

namespace G3D {

class RenderDevice;

/** \brief Saito and Takahashi's Geometry Buffers for deferred shading. 
    Contains position, normal, depth, and BSDF parameters.

    Used for rendering a G3D::SuperBSDF with deferred shading. 

    Requires GBuffer.pix and GBuffer.vrt at runtime, which can be found
    in the G3D/data-files/SuperShader directory of the G3D distribution.

    \beta This build of G3D only supports SuperSurface with GBuffer.

    Requires glGetInteger(GL_MAX_COLOR_ATTACHMENTS_EXT) >= 5 and 
    pixel and vertex shaders.
*/
class GBuffer : public ReferenceCountedObject {
public:

    typedef ReferenceCountedPointer<GBuffer> Ref;

    /**
       Storage method 
     */
    enum Mode {
        /**
           Channels are packed into RGBA8 textures as follows:

           24-bit fixed point camera space depth:

           Decode the camera space z by:
           <pre>
           z = -dot(zpack.rgb, vec3(256.0, 1.0, 1.0/256.0))
           </pre>

           It was encoded as: (TODO:need to use the last bit...)
           <pre>
           z = -z
           b = frac(z); z = (z - b) / 256;
           g = frac(z); z = (z - g) / 256;
           r = min(1.0, z);
           </pre>
 
           <table>
           <tr><td>Texture</td><td colspan=4>Channels</td></tr>
           <tr><td></td><td>0</td><td>1</td><td>2</td><td>3</td></tr>
           <td>zpack</td><td>z<sub>2</sub></td><td>z<sub>1</sub></td><td>z<sub>0</sub></td><td>u</td></tr>
           <td>normal</td><td colspan=3>n<sub>x,y,z</sub></td><td>v</td></tr>

           <td>reflect</td><td>diffuse</td><td>glossy</td><td>specular</td><td>log<sub>1024</sub>(glossyExp)</td></tr>
           <td>spectrum</td><td>spectrum<sub>r,g,b</sub></td><td>metalicity</td></tr>

           <td>transmit</td><td>transmit<sub>r,g,b</sub></td><td>eta</td></tr>
           <td>emit</td><td>emit<sub>r,g,b</sub></td><td><i>reserved</i></td></tr>
           </table>

           Depth stores a depth buffer in the requested format.
           
           Normals can be specified in either either camera or world
           space and are always unit length.           
           
           (u, v) is the texture coordinate.
         */
        PACKED_RGBA8_MODE,

        FLAT16F_MODE
    };

    enum Space {CAMERA_SPACE, WORLD_SPACE};

private:

    std::string                 m_name;

    const ImageFormat*          m_format;
    const ImageFormat*          m_depthFormat;

    /** Renders the SuperSurface Material SuperBSDF coefficients
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

    GBuffer(const std::string& name, const ImageFormat* depthFormat, const ImageFormat* otherFormat);

    void computeGeneric
    (RenderDevice* rd, 
     const SuperSurface::Ref& model) const;

    void computeNonGeneric
    (RenderDevice* rd, 
     const Surface::Ref& model) const;

    void computeGenericArray
    (RenderDevice* rd, 
     const Array<SuperSurface::Ref>& model) const;

    void computeNonGenericArray
    (RenderDevice* rd, 
     const Array<Surface::Ref>& model) const;

public:

    /** \brief Returns true if GBuffer is supported on this GPU */
    static bool supported();

    static Ref create
    (const std::string& name,
     const ImageFormat* depthFormat = ImageFormat::DEPTH24(),     
     const ImageFormat* otherFormat = ImageFormat::RGBA16F());

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

        Elements of \a modelArray that are not SuperSurface
        will be rendered twice to extract their lambertian component
        and will have no other components.  SuperSurface instances
        render efficiently and will have full BSDF components.
    */
    void compute
    (RenderDevice*                  rd, 
     const GCamera&                 camera,
     const Array<Surface::Ref>&  modelArray) const;
};

} // namespace G3D

#endif
