/** 
  @file SuperShader.h

  A one-size-fits-all shader that combines most common illumination effects
  efficiently.

  @created 2005-01-01
  @edited  2010-03-27
  @author Morgan McGuire, http://graphics.cs.williams.edu
 */

#ifndef GLG3D_SuperShader_h
#define GLG3D_SuperShader_h

#include "G3D/ReferenceCount.h"
#include "G3D/Table.h"
#include "GLG3D/Material.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/SkyParameters.h"
#include "GLG3D/Shader.h"
#include "GLG3D/ShadowMap.h"

namespace G3D {

/**
 @brief A one-size-fits-all shader system that combines most common
 illumination effects efficiently.  

 G3D::Surface is designed (through the G3D::ArticulatedModel class)
 to use this system internally, so for most applications you will not
 even need to interact with SuperShader directly.

 The most reason to explicitly use the SuperShader system is to add
 your own custom rendering passes after those handled by Surface.
 Sample code for doing so (e.g., if creating a rendering pass for
 caustics) looks like:

 <pre>
    PassRef causticPass = Pass::fromFiles("caustic.vrt", "caustic.pix");
    causticPass->args.set("causticMap", causticMap);

    Surface::sortAndRender(rd, camera, allModels, lighting, shadowMaps, causticPass);
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

    Surface::sortAndRender(rd, camera, allModels, lighting, shadowMaps, causticPass);
 </pre>

 The primary customisation in this example is within caustic.pix,
 which is just a copy of NonShadowedPass.pix that has been edited to
 render caustics (underwater focused lights) instead of direct
 illumination terms.

 Note that Surface assumes that all extra passes are additive; if
 you want a non-additive rendering pass then you have to manually
 invoke the individual render methods on the posed models.

 See the source file ArticulatedModel_pose.cpp for examples of how to use
 the built-in SuperShader Passes

 @sa G3D::SuperShader::Pass, G3D::Material

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
    const ShadowMap::Ref&           shadowMap,
    const Material&                 material,
    VertexAndPixelShader::ArgList&  args);

/**
 \param i The index of the light when bound in the shader */
void configureSingleLightShaderArgs(
   const GLight& light,
   int i,
   VertexAndPixelShader::ArgList&  args, 
   bool shadowMapPass = false);

typedef ReferenceCountedPointer<class Pass>            PassRef;
typedef ReferenceCountedPointer<class NonShadowedPass> NonShadowedPassRef;
typedef ReferenceCountedPointer<class ExtraLightPass>  ExtraLightPassRef;
typedef ReferenceCountedPointer<class ShadowedPass>    ShadowedPassRef;

class Pass : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Pass> Ref;
private:

    /** Read a shader and force all of its #includes to be resolved */
    static void primeCodeCache(const std::string& originalFilename);

    class Cache {
        /** Maps macros (defines) to the shader compiled with them set this way */
        typedef Table<std::string, Shader::Ref> ShaderTable;

        /** Maps concatenated vertex+pixel filename to cached shaders */
        typedef Table<std::string, ShaderTable> MaterialTable;

    private:
        
        MaterialTable       table;

    public:

        /** Adds a shader to the list of cached ones.  Only call when 
            getSimilar returned NULL.
            @param key vertexFilename + pixelFilename
            \param macros: Material.computeDefines() + extraDefines */
        void add(const std::string& key, const std::string& macros, const ShaderRef& shader);

        /** Returns the shader for a similar material or 
            NULL if one does not exist. 
            @param key vertexFilename + pixelFilename 
            \param macros: Material.computeDefines() + extraDefines */
        ShaderRef getSimilar(const std::string& key, const std::string& macros) const;

        void clear() {
            table.clear();
        }
    };

    /** Maps filenames and materials to compiled shaders. */
    static Cache cache;

    /** Maps filenames to shader source code. */
    static Table<std::string, std::string> shaderTextCache;

    /** Loads a shader with the specified defines prepended onto
        its body. Called from getConfiguredShader. 
        
        Caches source code from files to avoid touching disk.*/
    static Shader::Ref loadShader
     (const std::string& vertexFilename,
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
        with arguments and \#defines configured for 
        this material. 
        
        \param extraDefines Code to insert after the material 
         macros; typically compile-time parameters defined by macros.
        */
    static ShaderRef getConfiguredShader
     (const std::string&  vertexFilename,
      const std::string&  pixelFilename,
      const Material&     material,
      const std::string&  extraDefines);

    std::string m_vertexFilename;
    std::string m_pixelFilename;

    /** For subclasses that don't want to load from a file. */
    Pass();

    Pass(const std::string& vertexFilename, const std::string& pixelFilename);

public:

    /** These will be merged with the Material-based args when applied to the shader.
        These arguments will override the Material ones if there are duplicates
        (although it is not recommended to override Material properties here.)

        Subclasses contain helper methods for automatically filling
        out these arguments, but you can always directly access the
        full argument list.  However, be aware that those arguments
        will remain until you explicitly clear the list with
        args.clear().
      */
    VertexAndPixelShader::ArgList args;

    /** This will be added after the Material-based customShaderPrefix when getting the configured the shader.*/
    std::string customShaderPrefix;

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
        backside argument.  If CULL_CURRENT, the current state is unmodified.
        
        \param extraDefines Code to insert after the material 
         macros; typically compile-time parameters defined by macros.*/
    virtual ShaderRef getConfiguredShader(
        const Material&         material,
        RenderDevice::CullFace  c = RenderDevice::CULL_CURRENT);

    /**
      Clears the static cache of SuperShader::Pass to clean up memory or allow reloading.
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

    /** Overrides the default because it requires emissive arguments */
    virtual ShaderRef getConfiguredShader(
        const Material&             material,
        RenderDevice::CullFace      c       = RenderDevice::CULL_CURRENT);
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

    void setLight(const GLight& light, const ShadowMap::Ref& shadowMap);
};

} // namespace SuperShader

} // namespace G3D

#endif

