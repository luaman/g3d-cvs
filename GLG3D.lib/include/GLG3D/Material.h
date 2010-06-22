/**
 @file   Material.h
 @author Morgan McGuire, http://graphics.cs.williams.edu
 @date   2008-08-10
 @edited 2010-01-29
*/
#ifndef GLG3D_Material_h
#define GLG3D_Material_h

#include "G3D/platform.h"
#include "G3D/HashTrait.h"
#include "G3D/constants.h"
#include "GLG3D/Component.h"
#include "GLG3D/SuperBSDF.h"
#include "GLG3D/BumpMap.h"
#include "GLG3D/Shader.h"

namespace G3D {

class Any;

/** 
  \brief Description of a surface for rendering purposes.

  Encodes a BSDF, bump map, and emission function.

  \beta

  The G3D::Material::SimilarHashCode and G3D::Material::SimilarTo traits are provided
  to help identify when two G3D::Material have the same non-zero terms (similar to
  G3D::SuperBSDF::Factors).  G3D::SuperShader uses these to reduce the number of different
  shaders that need to be constructed.

  Note that for real-time rendering most translucent surfaces should be two-sided and have
  comparatively low diffuse terms.  They should also be applied to
  convex objects (subdivide non-convex objects) to prevent rendering
  surfaces out of order.  For ray tracing, implement translucent surfaces as two single-sided surfaces:
  one for entering the material and one for exiting it (i.e., the "backfaces").  The eta of the exiting surface
  should be that of the medium that is being exited into--typically, air.  So a glass sphere is 
  a set of front faces with eta ~= 1.3 and a set of backfaces with eta = 1.0.
    
  @sa G3D::SuperShader, G3D::BSDF, G3D::Component, G3D::Texture, G3D::BumpMap, G3D::ArticulatedModel
  */
class Material : public ReferenceCountedObject {
public:

    typedef ReferenceCountedPointer<Material> Ref;

    /** @brief Specification of a material; used for loading.  
    
        Can be written to a file or constructed from a series of calls.
        
      The following terminology for photon scattering is used in the G3D::Material::Specification and G3D::BSDF classes and 
      their documentation:
      \image html scatter-terms.png        
        */
    class Specification {
    private:
        friend class Material;

        float           m_depthWriteHintDistance;

        Texture::Specification m_lambertian;
        Color4          m_lambertianConstant;

        Texture::Specification m_specular;
        Color3          m_specularConstant;

        Texture::Specification m_shininess;
        float           m_shininessConstant;

        Texture::Specification m_transmissive;
        Color3          m_transmissiveConstant;

        float           m_etaTransmit;
        float           m_extinctionTransmit;

        float           m_etaReflect;
        float           m_extinctionReflect;

        Texture::Specification m_emissive;
        Color3          m_emissiveConstant;

        std::string     m_customShaderPrefix;

        BumpMap::Specification m_bump;

        Component4 loadLambertian() const;
        Component4 loadSpecular() const;
        Component3 loadTransmissive() const;
        Component3 loadEmissive() const;

        /** Preferred level of refraction quality. The actual level available depends on the renderer.*/
        RefractionQuality       m_refractionHint;

        /** Preferred level of mirror reflection quality. The actual level available depends on the renderer.*/
        MirrorQuality           m_mirrorHint;

    public:

        Specification();
        
        /** The parsing has been under significant revision and so is intentionally undocumented; see
           the Material_Specification.cpp source code for the latest format.
         \beta */
        Specification(const Any& any);

        bool operator==(const Specification& s) const;
        operator Any() const;

        inline bool operator!=(const Specification& s) const {
            return !((*this) == s);
        }

        /** Load from a file created by save(). */
        void load(const std::string& filename);

        void setCustomShaderPrefix(const std::string& s) {
            m_customShaderPrefix = s;
        }

        /** Distance below which Surface::depthWriteHint returns true.
        
           - inf() = Always depth write
           - -inf() = Never depth write
           - nan() [default]: depth write at any distance for opaque and never depth write for transmissive surfaces.
        */
        void setDepthWriteHintDistance(float hint) {
            m_depthWriteHintDistance = hint;
        }

        /** Filename of Lambertian (diffuse) term, empty if none. The
            alpha channel is a mask that will be applied to all maps
            for coverage.  That is, alpha = 0 indicates holes in the
            surface.  Alpha is for partial coverage. Do not use alpha
            for transparency; set transmissiveFilename instead.*/
        void setLambertian(const std::string& filename,
                           const Color4& constant = Color4::one());

        void setLambertian(const Texture::Specification& spec);
        
        inline void setLambertian(const std::string& filename, float c) {
            setLambertian(filename, Color4(Color3(c), 1.0f));
        }

        void setLambertian(const Color4& constant);

        inline void setLambertian(float c) {
            setLambertian(Color4(Color3(c), 1.0f));
        }
        
        /** Makes the surface opaque black. */
        void removeLambertian();

        void setEmissive(const std::string& filename, const Color3& constant = Color3::one());
        
        void setEmissive(const Color3& constant);

        void setEmissive(const Texture::Specification& spec);
        
        void removeEmissive();

        /** Mirror reflection or glossy reflection.
            This actually specifies 
            the \f$F_0\f$ term, which is the minimum reflectivity of the surface.  At 
            glancing angles it will increase towards white.*/
        void setSpecular(const std::string& filename, const Color3& constant = Color3::one());
        
        void setSpecular(const Color3& constant);

        void setSpecular(const Texture::Specification& spec);

        /**  */
        void removeSpecular();

        /**
         The constant multiplies packed values stored in the file. 
         */
        void setShininess(const std::string& filename, float constant = 1.0f);

        /**
           If a specular filename is set as well, the specular specification
           overrides all of the settings except for the filename itself.
         */
        void setShininess(const Texture::Specification& spec);
        
        /** \brief Packed sharpness of the specular highlight.
            
            - SuperBSDF::packedSpecularNone() = no specular term (also forces specular color to black)
            - SuperBSDF::packedSpecularMirror() = mirror reflection. 
            - SuperBSDF::packSpecularExponent(e) affects the size of the glossy hilight, where 1 is dull, 128 is sharp.
            */
        void setShininess(float constant);

        /** Same as <code>setShininess(SuperBSDF::packedSpecularMirror())</code> */
        void setMirrorShininess() {
            setShininess(SuperBSDF::packedSpecularMirror());
        }

        /** Same as <code>setShininess(SuperBSDF::packSpecularExponent(e))</code> */
        void setGlossyExponentShininess(int e) {
            setShininess(SuperBSDF::packSpecularExponent(float(e)));
        }

        /** This is an approximation of attenuation due to extinction
           while traveling through a translucent material.  Note that
           no real material is transmissive without also being at
           least slightly glossy */
        void setTransmissive(const std::string& filename, const Color3& constant = Color3::one());
        
        void setTransmissive(const Color3& constant);

        void setTransmissive(const Texture::Specification& spec);
        
        void removeTransmissive();

        /** Set the index of refraction. Not used unless transmissive is non-zero. */
        void setEta(float etaTransmit, float etaReflect);

        /**
           @param normalMapWhiteHeightInPixels When loading normal
              maps, argument used for G3D::GImage::computeNormalMap()
              whiteHeightInPixels.  Default is -0.02f
              \deprecated
        */
        void setBump(
            const std::string& filename,
            const BumpMap::Settings& settings = BumpMap::Settings(),
            float normalMapWhiteHeightInPixels = -0.02f);

        void setBump(const BumpMap::Specification& bump) {
            m_bump = bump;
        }

        void removeBump();

        /** Defaults to G3D::RefractionQuality::DYNAMIC_FLAT */
        void setRefractionHint(RefractionQuality q) {
            m_refractionHint = q;
        }

        /** Defaults to G3D::MirrorQuality::STATIC_ENV */
        void setMirrorHint(MirrorQuality q) {
            m_mirrorHint = q;
        }

        size_t hashCode() const;
    };

    /** \deprecated */
    typedef Specification Settings;

protected:

    /** Scattering function */
    SuperBSDF::Ref              m_bsdf;

    /** Emission map. */
    Component3                  m_emissive;

    /** Bump map */
    BumpMap::Ref                m_bump;

    /** For experimentation.  This is automatically passed to the
        shaders if not NULL.*/
    MapComponent<Image4>::Ref   m_customMap;

    /** For experimentation.  This is automatically passed to the
        shaders if finite.*/
    Color4                      m_customConstant;

    /** For experimentation.  This code (typically macro definitions) is injected into the
        shader code after the material constants.
      */
    std::string                 m_customShaderPrefix;

    /** Preferred level of refraction quality. The actual level available depends on the renderer.*/
    RefractionQuality           m_refractionHint;

    /** Preferred level of mirror reflection quality. The actual level available depends on the renderer.*/
    MirrorQuality               m_mirrorHint;

    float                       m_depthWriteHintDistance;

    Material();

public:

    /** \brief Constructs an empty Material, which has no BSDF. This is provided mainly 
       for efficiency when constructing a Material manually. Use Material::create() to 
       create a default material. */
    static Material::Ref createEmpty();

    /** The Material::create(const Settings& settings) factor method is recommended 
       over this one because it performs caching and argument validation. */ 
    static Material::Ref create(
        const SuperBSDF::Ref&               bsdf,
        const Component3&                   emissive        = Component3(),
        const BumpMap::Ref&                 bump            = NULL,
        const MapComponent<Image4>::Ref&    customMap       = NULL,
        const Color4&                       customConstant  = Color4::inf(),
        const std::string&                  customShaderPrefix = "");

    /**
       Caches previously created Materials, and the textures 
       within them, to minimize loading time.

       Materials are initially created with all data stored exclusively 
       on the GPU. Call setStorage() to move or copy data to the CPU 
       (note: it will automatically copy to the CPU as needed, but that 
       process is not threadsafe).
     */
    static Material::Ref create(const Specification& settings = Specification());

    /**
     Create a G3D::Material using a Lambertian (pure diffuse) G3D::BSDF with color @a p_Lambertian.
     */
    static Material::Ref createDiffuse(const Color3& p_Lambertian);
    static Material::Ref createDiffuse(const std::string& textureFilename);

    void setStorage(ImageStorage s) const;

    /** Never NULL */
    SuperBSDF::Ref bsdf() const {
        return m_bsdf;
    }

    /** May be NULL */
    BumpMap::Ref bump() const {
        return m_bump;
    }

    /** \copydoc Material::Specification::setDepthWriteHintDistance */
    float depthWriteHintDistance() const {
        return m_depthWriteHintDistance;
    }

    /** \copydoc m_customShaderPrefix */
    const std::string& customShaderPrefix() const {
        return m_customShaderPrefix;
    }

    /** An emission function.

        Dim emission functions are often used for "glow", where a surface is bright
        independent of external illumination but does not illuminate other surfaces.

        Bright emission functions are used for light sources under the photon mapping
        algorithm.

        The result is not a pointer because G3D::Component3 is immutable and 
        already indirects the G3D::Component::MapComponent inside of it by a
        pointer.*/
    inline const Component3& emissive() const {
        return m_emissive;
    }

    inline const Color4& customConstant() const {
        return m_customConstant;
    }

    /** Appends a string of GLSL macros (e.g., "#define LAMBERTIANMAP\n") to
        @a defines that
        describe the specified components of this G3D::Material, as used by 
        G3D::SuperShader.
      */
    void computeDefines(std::string& defines) const;

    /** Configure the properties of this material as optional
        arguments for a shader (e.g. G3D::SuperShader).  If an
        emissive map or reflectivity map is used then the constant
        will also be specified for those two fields; the lighting
        environment should take care of multiplying those two fields
        by the lighting.emissiveScale and lighting.environmentConstant
        as needed (e.g., for some tone-mapping algorithms.)
      */
    void configure(VertexAndPixelShader::ArgList& a) const;

    /** Returns true if this material has an alpha mask */
    bool hasAlphaMask() const;

    /** Returns true if @a this material uses similar terms to @a
        other (used by G3D::SuperShader), although the actual textures
        may differ. */
    bool similarTo(const Material& other) const;
    bool similarTo(const Material::Ref& other) const {
        return similarTo(*other);
    }

    /** 
     To be identical, two materials must not only have the same images in their
     textures but must share pointers to the same underlying G3D::Texture objects.
     */
    bool operator==(const Material& other) const {
        return 
            (this == &other) ||
            ((m_bsdf == other.m_bsdf) &&
             (m_emissive == other.m_emissive) &&
             (m_bump == other.m_bump) &&
             (m_customMap == other.m_customMap) &&
             (m_customConstant == other.m_customConstant));
    }

    /** Can be used with G3D::Table as an Equals function */
    class SimilarTo {
    public:
        static bool equals(const Material& a, const Material& b) {
            return a.similarTo(b);
        }
        static bool equals(const Material::Ref& a, const Material::Ref& b) {
            return a->similarTo(*b);
        }
    };

    /** Can be used with G3D::Table as a hash function; if two Materials
        have the same SimilarHashCode then they are SimilarTo each other.*/
    class SimilarHashCode {
    public:
        static size_t hashCode(const Material& mat);
        inline static size_t hashCode(const Material::Ref& mat) {
            return hashCode(*mat);
        }
    };


    /** Preferred level of refraction quality. The actual level available depends on the renderer.*/
    RefractionQuality refractionHint() const {
        return m_refractionHint;
    }

    /** Preferred level of mirror reflection quality. The actual level available depends on the renderer.*/
    MirrorQuality mirrorHint() const {
        return m_mirrorHint;
    }
};

}

template <>
struct HashTrait<G3D::Material::Settings> {
    static size_t hashCode(const G3D::Material::Settings& key) {
        return key.hashCode();
    }
};

#endif
