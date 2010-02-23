/**
  @file GBuffer.h
  @author Morgan McGuire, http://graphics.cs.williams.edu
 */
#ifndef GLG3D_GBuffer_h
#define GLG3D_GBuffer_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/ImageFormat.h"
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

    Requires SS_GBuffer.pix, SS_GBufferPosition.pix, and
    SS_NonShadowedPass.vrt at runtime, which can be found in the
    G3D/data-files/SuperShader directory of the G3D distribution.
*/
class GBuffer : public ReferenceCountedObject {
public:

    class Specification {
    public:
        /** World space shading normal in RGB (after bump mapping). */
        bool                  normal;
        bool                  lambertian;
        bool                  specular;
        bool                  transmissive;
        bool                  emissive;

        /** World space triangle normal in RGB. */
        bool                  faceNormal;

        /** Packed camera space depth */
        bool                  packedDepth;

        /** The Material "custom" channel */
        bool                  custom;

        /** World space position in RGB. */
        bool                  position;

        /** Must contain four channels */
        const ImageFormat*    format;

        const ImageFormat*    depthFormat;

        /** Must have at least three channels */
        const ImageFormat*    positionFormat;

        Specification() : 
            normal(true),
            lambertian(true),
            specular(true),
            transmissive(false),
            emissive(false),
            faceNormal(false),
            packedDepth(true),
            custom(false),
            position(false),
            format(ImageFormat::RGBA8()),
            depthFormat(ImageFormat::DEPTH24()), 
            positionFormat(ImageFormat::RGB16F()) {
        }

        size_t hashCode() const {
            return 
                int(normal) |
                (int(lambertian)  << 1) |
                (int(specular)    << 2) |
                (int(transmissive) << 4) |
                (int(emissive)    << 5) |
                (int(faceNormal)  << 6) |
                (int(packedDepth) << 7) |
                (int(position)    << 8);
                (int(custom)      << 9);
        }

        /** Can be used with G3D::Table as an Equals function */
        class Similar {
        public:
            static bool equals(const Specification& a, const Specification& b) {
                return hashCode(a) == hashCode(b);
            }
            static size_t hashCode(const Specification& s) {
                return s.hashCode();
            }
        };
    };

    typedef ReferenceCountedPointer<GBuffer> Ref;

private:

    class Indices {
    public:
        int L;
        int s;
        int t;
        int e;
        int n;
        int f;
        int z;
        int c;

        int numAttach;
        
        /** Indices of the FBO fields.  */
        Indices(const Specification& spec);
        std::string computeDefines() const;
    };

    std::string                 m_name;

    const Specification         m_specification;

    const Indices               m_indices;

    Shader::Ref                 m_positionShader;
    
    mutable GCamera             m_camera;

    /** The other buffers are permanently bound to this framebuffer */
    Framebuffer::Ref            m_framebuffer;

    Framebuffer::Ref            m_positionFramebuffer;

    /** RGB = diffuse reflectance (Fresnel is not applied), A = alpha */
    Texture::Ref                m_lambertian;

    /** RGB = F0, A = \f$\sigma\f$ (packed glossy exponent).  Fresnel
        is not applied */
    Texture::Ref                m_specular;

    /** RGB = T0, A = eta.  Fresnel is not applied */
    Texture::Ref                m_transmissive;

    Texture::Ref                m_emissive;

    /** World-space unit normal. */
    Texture::Ref                m_normal;

    Texture::Ref                m_faceNormal;

    Texture::Ref                m_packedDepth;

    /** World-space position */
    Texture::Ref                m_position;

    /** Depth texture. */
    Texture::Ref                m_depth;

    /** Returns the appropriate shader for this combination of
        specification and material, checking against a cache of
        previously compiled shaders.  The shader is not yet configured
        for the material.*/
    static Shader::Ref getShader(const Specification& specificiation, const Indices& indices, const Material::Ref& material);

    GBuffer(const std::string& name, const Specification& specification);

    void computeGeneric
    (RenderDevice* rd, 
     const SuperSurface::Ref& model) const;

    void computeGenericArray
    (RenderDevice* rd, 
     const Array<SuperSurface::Ref>& model) const;

public:

    /** \brief Returns true if GBuffer is supported on this GPU */
    static bool supported();

    static Ref create
    (const std::string& name = "GBuffer",
     const Specification& specification = Specification());

    virtual ~GBuffer();

    int width() const;

    int height() const;

    Rect2D rect2DBounds() const;

    /** The other buffers are permanently bound to this framebuffer */
    Framebuffer::Ref framebuffer() const {
        return m_framebuffer;
    }

    Framebuffer::Ref positionFramebuffer() const {
        return m_positionFramebuffer;
    }

    /** The camera from which these buffers were rendered */
    const GCamera& camera() const {
        return m_camera;
    }

    /** RGB = diffuse reflectance (Fresnel is not applied), A =
        partial coverage. */
    Texture::Ref lambertian() const {
        return m_lambertian;
    }

    /** RGB = F0, A = \f$\sigma\f$ (packed glossy exponent).  Fresnel
        is not applied */
    Texture::Ref specular() const {
        return m_specular;
    }

    /** RGB = T0, A = eta.  Fresnel is not applied */
    Texture::Ref transmissive() const {
        return m_transmissive;
    }

    /** RGB = T0, A = eta.  */
    Texture::Ref emissive() const {
        return m_emissive;
    }

    /** World-space position */
    Texture::Ref position() const {
        return m_position;
    }

    /** World-space unit shading normal, after bump mapping. */
    Texture::Ref normal() const {
        return m_normal;
    }

    /** Geometric normal */
    Texture::Ref faceNormal() const {
        return m_faceNormal;
    }

    /** Camera space depth */
    Texture::Ref packedDepth() const {
        return m_packedDepth;
    }

    /** Depth texture. */
    Texture::Ref depth() const {
        return m_depth;
    }

    const std::string& name() const {
        return m_name;
    }

    /** Reallocate all buffers to this size if they are not already */
    virtual void resize(int width, int height);

    /** Render the models to this GBuffer set (clearing first).  Depth
        is only cleared if RenderDevice::depthWrite is true.

        Assumes that \a modelArray has already been culled and sorted
        for the camera.

        Performs binary alpha testing using Lambertian.a.

        Only renders elements of \a modelArray that are SuperSurface
        instances.
    */
    void compute
    (RenderDevice*                  rd, 
     const GCamera&                 camera,
     const Array<Surface::Ref>&      modelArray) const;
};

} // namespace G3D

#endif
