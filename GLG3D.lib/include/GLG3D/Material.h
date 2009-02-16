/** 
  @file Material.h

  @created 2005-01-01
  @edited  2008-07-18
  @author  Morgan McGuire, morgan@cs.williams.edu
 */

#ifndef MATERIAL_H
#define MATERIAL_H

#include "G3D/Color3.h"
#include "G3D/Color4.h"
#include "G3D/Vector4.h"
#include "GLG3D/Shader.h"
#include "GLG3D/Texture.h"

namespace G3D {

/**
    @brief A surface material description (BSDF + Emission function +
    rendering parameters) including transmission, Lambertian,
    Blinn-Phong specular, and mirror reflection terms.

    Describes the material properties of a 3D surface. This includes a

    - Bidirectional Scattering Distribution Function (BSDF) describing how
    light reflects and transmits through the surface, 

    - an emission function describing the light emitted from the surface

    - a normal (a.k.a. displacement, bump) map describing small-scale
      detail and some parameters controling the accuracy of the
      displacement mapping algorithm.

    Each of the Component fields can be set to a scalar constant, a
    color, a texture map, or the product of a color and a texture map.
    If used with G3D::SuperShader, the application of the shading terms
    in the pixel shader will be optimized based on which components are
    present.

    An alpha mask is only allowed on the diffuse component.

    Illumination Equation:

    dst1 = underlying value in frame buffer
    evt = environment map
    ambUp, ambDn = ambient map up and down values (ideally, environment map convolved with a hemisphere)

    dst2 = dst1 * transmission + 
           evt[n] * reflection +
           lerp(ambDn, ambUp, n.y/2 + 0.5) * diffuse +
           emissive +
           SUM OVER LIGHTS {
             light * (diffuse * NdotL +
                      specular * NdotH^specularExponent)}

    When choosing material properties, the transmission, diffuse, and
    specular terms should sum to less than 1.

    Note that most translucent surfaces should be two-sided and have
    comparatively low diffuse terms.  They should also be applied to
    convex objects (subdivide non-convex objects) to prevent rendering
    surfaces out of order.
    
    @beta
    
    @sa G3D::SuperShader
  */
class Material : public ReferenceCountedObject {
public:

    typedef ReferenceCountedPointer<Material> Ref;

    /** Material property coefficients are specified as 
        a constant color times a texture map.  If the color
        is white the texture map entirely controls the result.
        If the color is black the term is disabled.  On graphics
        cards with few texture units the map will be ignored.*/
    class Component {
    public:
        /** Color that is constant over the entire surface. */
        Color3              constant;

        /** Color that varies over position.  NULL means white.*/
        Texture::Ref        map;

        inline Component() : constant(0, 0, 0), map(NULL) {}
        inline Component(const Color3& c) : constant(c), map(NULL) {}
        inline Component(float c) : constant((float)c, (float)c, (float)c), map(NULL) {}
        inline Component(const Color3& c, const Texture::Ref& t) : constant(c), map(t) {}
        inline Component(const Texture::Ref& t) : constant(1, 1, 1), map(t) {}

        /** True if this material is definitely (0,0,0) everywhere */
        inline bool isBlack() const {
            // To be black, it doesn't matter what your texture is--it will be multiplied by zero!
            return (constant.r == 0) && (constant.g == 0) && (constant.b == 0);
        }

        /** True if this material is definitely (1,1,1) everywhere */
        inline bool isWhite() const {
            // To be white, you have to have no texture map and all white constants.
            return (constant.r == 1) && (constant.g == 1) && (constant.b == 1) && map.isNull();
        }

        inline bool operator==(const Component& other) const {
            return (constant == other.constant) && (map == other.map);
        }

        /** Returns true if both components will produce similar non-zero terms in a
            lighting equation.  Black and white are only similar to themselves. */
        bool similarTo(const Component& other) const;
    };

    /** Diffuse (Lambertian) reflection of lights. 

        The alpha channel is used as a mask, e.g., to cut out the
        shape of a leaf or a billboard, but does NOT encode
        transparency.  Use the transmit member to specify (optionally
        colored) transparency.*/
    Component               diffuse;

    /** Glow without illuminating other objects. */
    Component               emit;

    /** Specular (glossy) reflection of lights. */
    Component               specular;

    /** Sharpness of light reflections.*/
    Component               specularExponent;

    /** Translucency.  Can be colored.*/
    Component               transmit;

    /** Perfect specular (mirror) reflection of the environment. */
    Component               reflect;

    /** 
        @brief Hint to the renderer for the quality of displacement
        mapping.

        Only relevant if normalBumpMap is non-NULL.

        If 0, normal mapping is used with no displacement.
        If 1, Kaneko-Welsh parallax mapping with offset limiting is used.
        If >1, Brawley-Tatarchuk parallax occlusion mapping is used.
        */
    int                     parallaxSteps;

    /** RGB*2-1 = tangent space normal, A = tangent space bump height.
      If NULL bump mapping is disabled. */
    Texture::Ref            normalBumpMap;

    /** Reserved for experimentation. This allows you to pass one
        additional texture of your choice into the shader; you can
        then modify the shader code directly to recieve and apply this
        map.  If non-NULL, CUSTOMMAP is #defined in the shader. */
    Texture::Ref            customMap;

    /** Reserved for experimentation.  If finite, CUSTOMCONSTANT is
        #defined in the shader.  */
    Vector4                 customConstant;
   
    /** Multiply normal map alpha values (originally on the range 0-1)
        by this constant to obtain the real-world bump height. Should
        already be factored in to the normal map normals.*/
    float                   bumpMapScale;

    /** On the range [0, 1].  Translates the bump map in and out of
        the surface. */
    float                   bumpMapBias;

    /** Default material is white and slightly shiny. */
    Material() : diffuse(1), emit(0), 
        specular(0.25), specularExponent(60), 
        transmit(0), reflect(0), parallaxSteps(1), 
        customConstant((float)inf(),(float)inf(),(float)inf(),(float)inf()), 
        bumpMapScale(0.02), bumpMapBias(0) {
    }

    /** Create a purely diffuse material with this reflectivity. */
    static Material::Ref createDiffuse(const Color3& diffuse);

    /** Returns true if @a this material uses similar terms as @a
        other (used by G3D::SuperShader), although the actual textures
        may differ. */
    bool similarTo(const Material& other) const;
    inline bool similarTo(const Material::Ref& other) const {
        return similarTo(*other);
    }

    /** 
     To be identical, two materials must not only have the same images in their
     textures but must share pointers to the same underlying G3D::Texture objects.
     */
    inline bool operator==(const Material& other) const {
        return (diffuse == other.diffuse) &&
               (specular == other.specular) &&
               (specularExponent == other.specularExponent) &&
               (transmit == other.transmit) &&
               (reflect == other.reflect) &&
               (normalBumpMap == other.normalBumpMap) &&
               (bumpMapScale == other.bumpMapScale) &&
               (bumpMapBias == other.bumpMapBias) &&
               (customMap == other.customMap) &&
               (customConstant == other.customConstant) &&
               (parallaxSteps == other.parallaxSteps);
    }


    inline size_t hashCode() const {
        // Intentionally does not take all terms into account, for performance
        return
            (size_t)((diffuse.constant.b + (diffuse.constant.r + (diffuse.constant.g * 1024)) * 1024) * 1024) ^ 
            (size_t)(diffuse.map.pointer()) ^
            (size_t)(specular.constant.b * 0xFFFFFF) ^ 
            (size_t)(specular.map.pointer()) ^
            (size_t)(reflect.constant.b * 0xFFFFFF) ^
            (size_t)(transmit.constant.b * 0xFFFFFF) ^
            (size_t)(customMap.pointer()) ^
            (size_t)((customConstant.x + (customConstant.y + (customConstant.z * 1024)) * 1024) * 1024);
    }


    inline bool operator!=(const Material& other) const {
        return !(*this == other);
    }


    /** Returns a string of GLSL macros (e.g., #define DIFFUSEMAP) that
        describe the specified components of this Material, as used by 
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
};


} // namespace G3D

template <>
struct HashTrait<G3D::Material> {
    static size_t hashCode(const G3D::Material& m) {
        return m.hashCode();
    }
};
#endif
