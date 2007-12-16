/** 
  @file SuperShader.h

  A one-size-fits-all shader that combines most common illumination effects
  efficiently.

  @author Morgan McGuire, morgan@cs.brown.edu
 */

#ifndef SUPERSHADER_H
#define SUPERSHADER_H

#include "G3D/ReferenceCount.h"
#include "GLG3D/SkyParameters.h"
#include "GLG3D/Material.h"
#include "GLG3D/Shader.h"
#include "GLG3D/ShadowMap.h"

namespace G3D {

typedef ReferenceCountedPointer<class SuperShader> SuperShaderRef;

/**
 A one-size-fits-all shader that combines most common illumination effects
 efficiently.

 To use SuperShader you will need the four files in the current directory
 at runtime:
 <ul>
 <li>ShadowMappedLightPass.vrt
 <li>ShadowMappedLightPass.pix
 <li>NonShadowedPass.vrt
 <li>NonShadowedPass.pix
 </ol>

 these are located in the data/SuperShader directory of the G3D distribution.

 Example of using with a PosedModel:

 <pre>
    <i> members:</i>
    SuperShader::Material material;
    ShaderRef nonshadowShader;
    ShaderRef shadowShader;

    <i> in constructor:</i>
    ... set fields of material ...
    SuperShader::createShaders(material, nonshadowShader, shadowShader);

    <i> in onGraphics:</i>

    rd->pushState();
        SuperShader::configureShaderArgs(localLighting, material, shader->args);
        rd->setShader(shader);
        rd->setObjectToWorldMatrix(posed->coordinateFrame());
        rd->setShadeMode(RenderDevice::SHADE_SMOOTH);
        posed->sendGeometry(rd);
    rd->popState();
 </pre>

 @cite McGuire, The %SuperShader. Chapter 8.1, 485--498, in <i>ShaderX<sup>4</sup></i>, W. Engel ed., 2005.
 */
class SuperShader {
public:

    typedef G3D::Material Material;

    /** Number of non-shadowed lights supported in a single pass.  Using more lights than this on one object
        will be slow because it will make multiple rendering passes. */
    enum {LIGHTS_PER_PASS = 4};

    /** Configures the material arguments on a SuperShader NonShadowed shader for
        the opaque pass with LIGHTS_PER_PASS lights. */
    static void configureShaderArgs(
        const LightingRef&              lighting,
        const Material&                 material,
        VertexAndPixelShader::ArgList&  args);

    /** Configures a SuperShader NonShadowed shader for an additive light pass
        using lights lightArray[lightIndex] through lightArray[lightIndex+LIGHTS_PER_PASS] (if both are in bounds.)*/
    static void configureShaderExtraLightArgs(
        const Array<GLight>&           lightArray,
        int                            lightIndex,
        VertexAndPixelShader::ArgList&  args);

    static void configureShadowShaderArgs(
        const GLight&                   light, 
        const ShadowMapRef&             shadowMap,
        const Material&                 material,
        VertexAndPixelShader::ArgList&  args);

private:

    /*
     TODO:
      Put a separate cache in each subclass
       (Make superShader handle fixed function cases as well?)

      // ArticulatedModel knows how to set the lighting fields of these automatically
      class NonShadowedSuperShader : public SuperShader;
      class ShadowMappedSuperShader : public SuperShader;
      
      // Set your own lighting fields explicitly
      class SpotLightCustomSuperShader : public SuperShader;
      SuperShaderRef spotShader = SpotLightCustomSuperShader::create();
      spotShader->setLightPosition(v); ...

      rd->setBlendFunc(additive);
      PosedModel::renderSuperShaderPass(spotShader, rd);

      void PosedModel::renderSuperShaderPass(const Array<PosedModelRef>& posed, SuperShaderRef superShader, RenderDevice* rd) {
          bool corrupted = false;

          rd->pushState();  // Just assume that state will be corrupted
          for (int p = 0; p < posed.size(); ++p) {
              if (posed[p]->hasSuperShader()) {
                 // SuperShader
                 posed[p]->renderSuperShaderPass(rd, superShader);
              }
          }
          rd->popState();
      }

      // Returns false if it left state modified
      bool PosedArticulatedModel::renderSuperShaderPass(RenderDevice* rd, SuperShaderRef superShader) {
           ShaderRef shader = superShader->getShader(material); // Does cache and configure
           rd->setShader(shader);
           renderGeometry();
           return shader->preservesState();
      }

      
        ShaderRef NonShadowedSuperShader::getShader(MaterialRef) {
            shader = get from cache
            configureShaderArgs(shader);
            return shader
        }
      */

    class Cache {
    public:
        struct Pair {
        public:
            ShaderRef       nonShadowedShader;
            ShaderRef       shadowMappedShader;
        };
    private:

        Array<Material>     materialArray;
        Array<Pair>         shaderArray;

    public:

        /** Adds a shader to the list of cached ones.  Only call when 
            getSimilar returned NULL.*/
        void add(const Material& mat, const Pair& pair);

        /** Returns the shader for a similar material or 
            NULL, NULL if one does not exist. */
        Pair getSimilar(const Material& mat) const;

        void clear() {
            materialArray.clear();
            shaderArray.clear();
        }
    };

    static Cache cache;

    static Cache::Pair getShader(const Material& material);

    // Don't call
    SuperShader() {
        debugAssert(false);
    }

public:

    /** Creates the non-shadow casting and shadow casting shaders
        appropriate for this material.  These may come from a
        pre-compiled cache. */
    static void createShaders(
        const Material& material,
        ShaderRef&      nonShadowedShader,
        ShaderRef&      shadowMappedShader);

    /**
      Clears the static cache of shaders to clean up memory.
     */
    static void purgeCache() {
        cache.clear();
    }
    
}; // SuperShader

}

#endif
