/** 
  @file SuperShader.h

  A one-size-fits-all shader that combines most common illumination effects
  efficiently.

  @created 2005-01-01
  @edited  2007-12-16
  @author Morgan McGuire, morgan@cs.williams.edu
 */

#ifndef SUPERSHADER_H
#define SUPERSHADER_H

#include "G3D/ReferenceCount.h"
#include "G3D/Table.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Material.h"
#include "GLG3D/SkyParameters.h"
#include "GLG3D/Shader.h"
#include "GLG3D/ShadowMap.h"

namespace G3D {

/**
 A one-size-fits-all shader system that combines most common
 illumination effects efficiently.  G3D::PosedModel is designed
 (through the G3D::ArticulatedModel class) to use this system
 internally, so for most applications you will not even need to
 interact with SuperShader directly.

 The most reason to explicitly use the SuperShader system is to add
 your own custom rendering passes after those handled by PosedModel.
 Sample code for doing so (e.g., if creating a rendering pass for
 caustics) looks like:

 <pre>
    PassRef causticPass = Pass::fromFiles("caustic.vrt", "caustic.pix");
    causticPass->args.set("causticMap", causticMap);

    PosedModel::sortAndRender(rd, camera, allModels, lighting, shadowMaps, causticPass);
  </pre>

  or, if you want to abstract your pass more explicitly,

 <pre>
    typedef ReferenceCountedPointer<class CausticPass> CausticPassRef;
    class CausticPass : public SuperShader::Pass {    
    protected:
        CausticPass() : SuperShader::Pass("caustic.vrt", "caustic.pix") {}
    public:
        static CausticPassRef create() { return new CausticPass();    }
        void setCaustics(TextureRef m) { args.set("causticMap",  m);  }
    };

    CausticPassRef causticPass = CausticPass::create();
    causticPass->setCaustics(causticMap);

    PosedModel::sortAndRender(rd, camera, allModels, lighting, shadowMaps, causticPass);
 </pre>

 The primary customisation in this example is within caustic.pix,
 which is just a copy of NonShadowedPass.pix that has been edited to
 render caustics (underwater focused lights) instead of direct
 illumination terms.

 Note that PosedModel assumes that all extra passes are additive; if
 you want a non-additive rendering pass then you have to manually
 invoke the individual render methods on the posed models.

 See the source file ArticulatedModel_pose.cpp for examples of how to use
 the built-in SuperShader Passes

 @sa SuperShader::Pass

 @cite McGuire, The %SuperShader. Chapter 8.1, 485--498, in <i>ShaderX<sup>4</sup></i>, W. Engel ed., 2005.
 */
namespace SuperShader {

/** Configures the material arguments on a SuperShader NonShadowed shader for
    the opaque pass with LIGHTS_PER_PASS lights. */
void configureShaderArgs(
    const LightingRef&              lighting,
    const Material&                 material,
    VertexAndPixelShader::ArgList&  args);

/** Configures a SuperShader NonShadowed shader for an additive light pass
    using lights lightArray[lightIndex] through lightArray[lightIndex+LIGHTS_PER_PASS] (if both are in bounds.)*/
void configureShaderExtraLightArgs(
    const Array<GLight>&           lightArray,
    int                            lightIndex,
    VertexAndPixelShader::ArgList&  args);

void configureShadowShaderArgs(
    const GLight&                   light, 
    const ShadowMapRef&             shadowMap,
    const Material&                 material,
    VertexAndPixelShader::ArgList&  args);


/** Creates the non-shadow casting and shadow casting shaders
    appropriate for this material.  These may come from a
    pre-compiled cache. 

    @deprecated

    @sa NonShadowedSuperShader, ShadowMappedSuperShader
    */
void createShaders(
    const Material& material,
    ShaderRef&      nonShadowedShader,
    ShaderRef&      shadowMappedShader);

typedef ReferenceCountedPointer<class Pass>            PassRef;
typedef ReferenceCountedPointer<class NonShadowedPass> NonShadowedPassRef;
typedef ReferenceCountedPointer<class ExtraLightPass>  ExtraLightPassRef;
typedef ReferenceCountedPointer<class ShadowedPass>    ShadowedPassRef;

class Pass : public ReferenceCountedObject {
private:
    class Cache {
        typedef Table<Material, ShaderRef, Material::SimilarHashCode, Material::SimilarTo> ShaderTable;

        /** Maps concatenated vertex+pixel filename to cached shaders */
        typedef Table<std::string, ShaderTable> MaterialTable;

    private:
        
        MaterialTable       table;

    public:

        /** Adds a shader to the list of cached ones.  Only call when 
            getSimilar returned NULL.
            @param key vertexFilename + pixelFilename*/
        void add(const std::string& key, const Material& mat, const ShaderRef& shader);

        /** Returns the shader for a similar material or 
            NULL if one does not exist. 
            @param key vertexFilename + pixelFilename */
        ShaderRef getSimilar(const std::string& key, const Material& mat) const;

        void clear() {
            table.clear();
        }
    };

    /** Maps filenames and materials to compiled shaders. */
    static Cache cache;

    /** Maps filenames to shader source code. */
    static Table<std::string, std::string> shaderTextCache;

    /** Loads a shader with the specified defines prepended onto
        its body. Called from getConfiguredShader. */
    static ShaderRef loadShader(
        const std::string& vertexFilename,
        const std::string& pixelFilename,
        const std::string& defines);

protected:

    /** Special case cache for its own subclass. */
    static NonShadowedPassRef nonShadowedInstance;

    /** Special case cache for its own subclass. */
    static ShadowedPassRef    shadowedInstance;

    /** Special case cache for its own subclass. */
    static ExtraLightPassRef  extraLightInstance;

    /** Returns a shader (perhaps from the cache) 
        with arguments and #defines configured for 
        this material. */
    static ShaderRef getConfiguredShader(
        const std::string&  vertexFilename,
        const std::string&  pixelFilename,
        const Material&     material);

    std::string m_vertexFilename;
    std::string m_pixelFilename;

    /** For subclasses that don't want to load from a file. */
    Pass();

    Pass(const std::string& vertexFilename, const std::string& pixelFilename);

public:

    /** These will be merged with the material-based args when applied to the shader.
        These arguments will override the material ones if there are duplicates
        (although it is not recommended to override material properties here.)

        Subclasses contain helper methods for automatically filling
        out these arguments, but you can always directly access the
        full argument list.  However, be aware that those arguments
        will remain until you explicitly clear the list with
        args.clear().
      */
    VertexAndPixelShader::ArgList args;

    /** 
     Constructs a new SuperShader pass from the specified vertex and pixel shaders.   
     */
    static PassRef fromFiles
    (const std::string& vertexFilename,
     const std::string& pixelFilename);

    /** Given a material and the current settings on this pass, returns the 
        appropriately configured shader. Note that this shader may be shared 
        cached between calls and is only affected by state set on the Pass 
        <b>before</b> getConfiguredShader is called.

        @param c
        Affects the subsequent calls to getConfiguredShader by setting the
        backside argument.  If CULL_CURRENT, the current state is unmodified.*/
    virtual ShaderRef getConfiguredShader(
        const Material& material,
        RenderDevice::CullFace c = RenderDevice::CULL_CURRENT);

    /**
      Clears the static cache of SuperShader::Pass to clean up memory.
     */
    static void purgeCache();
};


/**
  SuperShader pass that handles
  NonShadowedSuperShader::LIGHTS_PER_PASS non-shadow casting diffuse and specular,
  ambient, emissive, reflective, and refractive terms.

  Used by G3D::ArticulatedModel.
 */
class NonShadowedPass : public Pass {
protected:
    
    Color3   m_emissiveScale;
    Color3   m_environmentMapColor;

    NonShadowedPass();

public:

    /** Number of non-shadowed lights supported in a single pass.  
    
        (<i>Note:</i> Using more lights than this on one object
        with ArticulatedModel) will be slow because it will
        make multiple rendering passes. */
    enum {LIGHTS_PER_PASS = 4};

    /** There is only one instance of this class, which can be obtained
        by this helper method.
     */
    static NonShadowedPassRef instance();

    /** 
        Sets the lighting to be used by the next getConfiguredShader call.
        Uses these values immediately; does not track the specific lighting
        reference.

        Only the first LIGHTS_PER_PASS non-shadowed lights will be used.
        If you need more lights, make an additional pass with ExtraLightPass.
     */
    void setLighting(const LightingRef& lighting);

    virtual ShaderRef getConfiguredShader(
        const Material& material,
        RenderDevice::CullFace c = RenderDevice::CULL_CURRENT);
};


/**
  SuperShader pass that adds NonShadowedSuperShader::LIGHTS_PER_PASS
  non-shadow casting lights (uses the same NonShadowedPass shaders.)
 */
class ExtraLightPass : public Pass {
protected:

    ExtraLightPass();

public:

    /** Number of non-shadowed lights supported in a single pass.  
    
        (<i>Note:</i> Using more lights than this on one object
        with ArticulatedModel) will be slow because it will
        make multiple rendering passes. */
    enum {LIGHTS_PER_PASS = 4};

    static ExtraLightPassRef instance();

    /** Renders using the next LIGHTS_PER_PASS after and including index. */
    void setLighting(const Array<GLight>& lightArray, int index = 0);
};


class ShadowedPass : public Pass {
protected:

    ShadowedPass();

public:

    enum {LIGHTS_PER_PASS = 1};

    static ShadowedPassRef instance();

    void setLight(const GLight& light, const ShadowMapRef& shadowMap);
};

} // namespace SuperShader

} // namespace G3D

#endif

