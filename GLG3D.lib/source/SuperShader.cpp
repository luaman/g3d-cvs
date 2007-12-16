/**
 @file SuperShader.cpp

 @author Morgan McGuire, matrix@graphics3d.com
 */

#include "GLG3D/ShadowMap.h"
#include "GLG3D/SuperShader.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/fileutils.h"

namespace G3D {


void SuperShader::configureShaderExtraLightArgs(
    const Array<GLight>&            lightArray,
    int                             lightIndex,
    VertexAndPixelShader::ArgList&  args) {

    args.set("ambientTop",      Color3::black());
    args.set("ambientBottom",   Color3::black());

    static const std::string num[] = {"0", "1", "2", "3", "4", "5", "6", "7"};
    for (int i = 0; i < LIGHTS_PER_PASS; ++i) {
        const std::string& N = num[i];
        if (lightArray.size() > i + lightIndex) {
            const GLight& light = lightArray[i + lightIndex];

            args.set("lightPosition" + N,   light.position);
            args.set("lightColor" + N,      light.color);
            args.set("lightAttenuation" + N, Vector3(light.attenuation[0], light.attenuation[1], light.attenuation[2]));
        } else {
            args.set("lightPosition" + N,    Vector4(0, 1, 0, 0));
            args.set("lightColor" + N,       Color3::black());
            args.set("lightAttenuation" + N, Vector3(1, 0, 0));
        }
    }
}


void SuperShader::configureShaderArgs(
    const LightingRef&              lighting,
    const Material&                 material,
    VertexAndPixelShader::ArgList&  args) {

    // Material arguments
    if (material.diffuse.constant != Color3::black()) {
        args.set("diffuseConstant",         material.diffuse.constant);
        if (material.diffuse.map.notNull()) {
            args.set("diffuseMap",              material.diffuse.map);
        }
    }

    if (material.customConstant.isFinite()) {
        args.set("customConstant",         material.customConstant);
    }

    if (material.customMap.notNull()) {
        args.set("customMap",              material.customMap);
    }

    if (material.specular.constant != Color3::black()) {
        args.set("specularConstant",        material.specular.constant);
        if (material.specular.map.notNull()) {
            args.set("specularMap",             material.specular.map);
        }

        // If specular exponent is black we get into trouble-- pow(x, 0)
        // doesn't work right in shaders for some reason
        args.set("specularExponentConstant", Color3::white().max(material.specularExponent.constant));

        if (material.specularExponent.map.notNull()) {
            args.set("specularExponentMap",     material.specularExponent.map);
        }
    }

    if (material.reflect.constant != Color3::black()) {
        args.set("reflectConstant",         material.reflect.constant);

        if (material.reflect.map.notNull()) {
            args.set("reflectMap",              material.reflect.map);
        }
    }

    if (material.emit.constant != Color3::black()) {
        args.set("emitConstant",            material.emit.constant * lighting->emissiveScale);

        if (material.emit.map.notNull()) {
            args.set("emitMap",             material.emit.map);
        }
    }

    if (material.normalBumpMap.notNull() && (material.bumpMapScale != 0)) {
        args.set("normalBumpMap",       material.normalBumpMap);
        args.set("bumpMapScale",        material.bumpMapScale);
        args.set("bumpMapBias",         material.bumpMapBias);
    }

    ///////////////////////////////////////////////////
    // Lighting Args
    configureShaderExtraLightArgs(lighting->lightArray, 0, args);

    args.set("ambientTop",      lighting->ambientTop);
    args.set("ambientBottom",   lighting->ambientBottom);

    // Only set the evt map if we need it
    if (! material.reflect.isBlack()) {
        args.set("environmentConstant", lighting->environmentMapColor);
        debugAssert(lighting->environmentMap.notNull());
        args.set("environmentMap",  lighting->environmentMap);
    }
}


void SuperShader::configureShadowShaderArgs(
    const GLight&                   light, 
    const ShadowMapRef&             shadowMap,
    const Material&                 material,
    VertexAndPixelShader::ArgList&  args) {
    
    // TODO: don't even set fields that have no corresponding map
    if (material.diffuse.map.notNull()) {
        args.set("diffuseMap",          material.diffuse.map);
    }

    // TODO: bind only the constants that are used
    args.set("diffuseConstant",         material.diffuse.constant);

    if (material.customConstant.isFinite()) {
        args.set("customConstant",         material.customConstant);
    }

    if (material.customMap.notNull()) {
        args.set("customMap",         material.customMap);
    }

    if (material.specular.map.notNull()) {
        args.set("specularMap",         material.specular.map);
    }
    args.set("specularConstant",        material.specular.constant);

    if (material.specularExponent.map.notNull()) {
        args.set("specularExponentMap", material.specularExponent.map);
    }
    args.set("specularExponentConstant",material.specularExponent.constant);

    if (material.normalBumpMap.notNull() && (material.bumpMapScale != 0)) {
        args.set("normalBumpMap",       material.normalBumpMap);
        args.set("bumpMapScale",        material.bumpMapScale);
        args.set("bumpMapBias",         material.bumpMapBias);
    }

    ///////////////////////////////////////////////////
    // Lighting Args

    args.set("lightPosition",   light.position);
    args.set("lightColor",      light.color);
    args.set("lightAttenuation" , Vector3(light.attenuation[0], 
                                         light.attenuation[1], light.attenuation[2]));

    // Shadow map setup
    if (GLCaps::enumVendor() == GLCaps::ATI) {
        args.set("shadowMap",       shadowMap->colorDepthTexture());
    } else {
        args.set("shadowMap",       shadowMap->depthTexture());
    }

    args.set("lightMVP",        shadowMap->biasedLightMVP());
}

/** Loads the specified text file, using an internal cache to avoid 
    extraneous disk access. */
static const std::string& loadShaderCode(const std::string& filename) {
    static Table<std::string, std::string> shaderTextCache;

    if (! shaderTextCache.containsKey(filename)) {
        shaderTextCache.set(filename, readWholeFile(System::findDataFile(filename)));
    }

    return shaderTextCache[filename];
}


/**
 Loads a shader, where <I>basename</I> contains the path and filename up to the 
 ".vrt"/".pix" extensions, and <I>defines</I> is a string to prepend to the 
 beginning of both vertex and pixel shaders.
 */
static ShaderRef loadShader(const std::string& baseName, const std::string& defines) {

    const std::string& vertexShader = loadShaderCode(baseName + ".vrt");
    const std::string& pixelShader  = loadShaderCode(baseName + ".pix");

    ShaderRef s = Shader::fromStrings
        (baseName + ".vrt", defines + vertexShader, 
         baseName + ".pix", defines + pixelShader);

    return s;
}


SuperShader::Cache::Pair SuperShader::getShader(const Material& material) {
 
    // First check the cache
    Cache::Pair p = cache.getSimilar(material);

    if (p.shadowMappedShader.isNull()) {

        // Not found in cache; load from disk

        static const std::string shadowName    = "ShadowMappedLightPass";
        static const std::string nonShadowName = "NonShadowedPass";

        std::string defines;

        if (material.diffuse.constant != Color3::black()) {
            if (material.diffuse.map.notNull()) {
                defines += "#define DIFFUSEMAP\n";

                // If the color is white, don't multiply by it
                if (material.diffuse.constant != Color3::white()) {
                    defines += "#define DIFFUSECONSTANT\n";
                }
            } else {
                defines += "#define DIFFUSECONSTANT\n";
            }
        }

        if (material.customConstant.isFinite()) {
            defines += "#define CUSTOMCONSTANT\n";
        }

        if (material.customMap.notNull()) {
            defines += "#define CUSTOMMAP\n";
        }

        if (material.specular.constant != Color3::black()) {
            if (material.specular.map.notNull()) {
                defines += "#define SPECULARMAP\n";

                // If the color is white, don't multiply by it
                if (material.specular.constant != Color3::white()) {
                    defines += "#define SPECULARCONSTANT\n";
                }
            } else  {
                defines += "#define SPECULARCONSTANT\n";
            }


            if (material.specularExponent.constant != Color3::black()) {
                if (material.specularExponent.map.notNull()) {
                    defines += "#define SPECULAREXPONENTMAP\n";
                    
                    // If the color is white, don't multiply by it
                    if (material.specularExponent.constant != Color3::white()) {
                        defines += "#define SPECULAREXPONENTCONSTANT\n";
                    }
                } else  {
                    defines += "#define SPECULAREXPONENTCONSTANT\n";
                }
            }
        }

        if (material.emit.constant != Color3::black()) {
            if (material.emit.map.notNull()) {
                defines += "#define EMITMAP\n";

                // If the color is white, don't multiply by it
                if (material.emit.constant != Color3::white()) {
                    defines += "#define EMITCONSTANT\n";
                }
            } else  {
                defines += "#define EMITCONSTANT\n";
            }
        }

        if (material.reflect.constant != Color3::black()) {
            if (material.reflect.map.notNull()) {
                defines += "#define REFLECTMAP\n";

                // If the color is white, don't multiply by it
                if (material.reflect.constant != Color3::white()) {
                    defines += "#define REFLECTCONSTANT\n";
                }
            } else  {
                defines += "#define REFLECTCONSTANT\n";

                if (material.reflect.constant == Color3::white()) {
                    defines += "#define REFLECTWHITE\n";
                }
            }
        }

        if ((material.bumpMapScale != 0) && material.normalBumpMap.notNull()) {
            defines += "#define NORMALBUMPMAP\n";
        }

        p.nonShadowedShader  = loadShader(nonShadowName, defines);
        p.shadowMappedShader = loadShader(shadowName,    defines);

        p.nonShadowedShader->args.set("backside", 1.0);
        p.shadowMappedShader->args.set("backside", 1.0);

        cache.add(material, p);
    }

    return p;
}


void SuperShader::createShaders(
    const Material& material, 
    ShaderRef& nonShadowedShader, 
    ShaderRef& shadowMappedShader) {

    Cache::Pair p       = getShader(material);

    nonShadowedShader   = p.nonShadowedShader;
    shadowMappedShader  = p.shadowMappedShader;
}


////////////////////////////////////////////////////////////////////////////
SuperShader::Cache SuperShader::cache;

void SuperShader::Cache::add(const Material& mat, const Cache::Pair& p) {
    materialArray.append(mat);
    shaderArray.append(p);
}


SuperShader::Cache::Pair SuperShader::Cache::getSimilar(const Material& mat) const {
    for (int m = 0; m < materialArray.size(); ++m) {
        if (materialArray[m].similarTo(mat)) {
            return shaderArray[m];
        }
    }

    return Pair();
}

}
