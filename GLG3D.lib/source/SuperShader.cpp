/**
 @file SuperShader.cpp

 @author Morgan McGuire, matrix@graphics3d.com
 */

#include "GLG3D/SuperShader.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/fileutils.h"

namespace G3D {

void SuperShader::Material::enforceDiffuseMask() {
    if (! changed) {
        return;
    }

    if (diffuse.map.notNull() && ! diffuse.map->opaque()) {
        // There is a mask.  Extract it.

        TextureRef mask = diffuse.map->alphaOnlyVersion();

        static const int numComponents = 5;
        Component* component[numComponents] = {&emit, &specular, &specularExponent, &transmit, &reflect};

        // Spread the mask to other channels that are not black
        for (int i = 0; i < numComponents; ++i) {
            if (! component[i]->isBlack()) {
                if (component[i]->map.isNull()) {
                    // Add a new map that is the mask
                    component[i]->map = mask;
                } else {
                    // TODO: merge instead of replacing!
                    component[i]->map = mask;
                }
            }
        }
    }

    changed = false;
}

void SuperShader::configureShader(
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


    if (material.specular.constant != Color3::black()) {
        args.set("specularConstant",        material.specular.constant);

        if (material.specular.map.notNull()) {
            args.set("specularMap",             material.specular.map);
        }
    }

    // If specular exponent is black we get into trouble-- pow(x, 0) doesn't work right in shaders for some reason
    args.set("specularExponentConstant",Color3::white().max(material.specularExponent.constant));

    if (material.specularExponent.map.notNull()) {
        args.set("specularExponentMap",     material.specularExponent.map);
    }

    if (material.reflect.constant != Color3::black()) {
        args.set("reflectConstant",         material.reflect.constant);

        if (material.reflect.map.notNull()) {
            args.set("reflectMap",              material.reflect.map);
        }
    }

    if (material.emit.constant != Color3::black()) {
        args.set("emitConstant",            material.emit.constant);

        if (material.emit.map.notNull()) {
            args.set("emitMap",             material.emit.map);
        }
    }

    if (material.normalBumpMap.notNull() && (material.bumpMapScale != 0)) {
        args.set("normalBumpMap",       material.normalBumpMap);
        args.set("bumpMapScale",        material.bumpMapScale);
    }

    ///////////////////////////////////////////////////
    // Lighting Args

    args.set("ambientTop",      lighting->ambientTop);
    args.set("ambientBottom",   lighting->ambientBottom);

    if (lighting->lightArray.size() > 0) {
        args.set("lightPosition",   lighting->lightArray[0].position);
        args.set("lightColor",      lighting->lightArray[0].color);
    } else {
        args.set("lightPosition",   Vector4(0,1,0,0));
        args.set("lightColor",      Color3::black());
    }

    // Only set the evt map if we need it
    if (! material.reflect.isBlack()) {
        args.set("environmentConstant", lighting->environmentMapColor);
        debugAssert(lighting->environmentMap.notNull());
        args.set("environmentMap",  lighting->environmentMap);
    }
}


void SuperShader::configureShadowShader(
    const GLight&                   light, 
    const Matrix4&                  lightMVP, 
    const TextureRef&               shadowMap,
    const Material&                 material,
    VertexAndPixelShader::ArgList&  args) {
    
    // TODO: don't even set fields that have no corresponding map
    if (material.diffuse.map.notNull()) {
        args.set("diffuseMap",          material.diffuse.map);
    }

    // TODO: bind only the constants that are used
    args.set("diffuseConstant",         material.diffuse.constant);

    if (material.specular.map.notNull()) {
        args.set("specularMap",         material.specular.map);
    }
    args.set("specularConstant",        material.specular.constant);

    if (material.specularExponent.map.notNull()) {
        args.set("specularExponentMap",     material.specularExponent.map);
    }
    args.set("specularExponentConstant",material.specularExponent.constant);

    if (material.normalBumpMap.notNull() && (material.bumpMapScale != 0)) {
        args.set("normalBumpMap",       material.normalBumpMap);
        args.set("bumpMapScale",        material.bumpMapScale);
    }

    ///////////////////////////////////////////////////
    // Lighting Args

    args.set("lightPosition",   Vector4(light.position.xyz().direction(),0));
    args.set("lightColor",      light.color);

    // Shadow map setup
    args.set("shadowMap",       shadowMap);

    // Bias the shadow map so that we don't get acne
    static const Matrix4 bias(
        0.5f, 0.0f, 0.0f, 0.5f,
        0.0f, 0.5f, 0.0f, 0.5f,
        0.0f, 0.0f, 0.5f, 0.5f - 0.003f,
        0.0f, 0.0f, 0.0f, 1.0f);

    args.set("lightMVP",        bias * lightMVP);
}

/** Loads the specified text file, using an internal cache to avoid 
    extraneous disk access. */
static const std::string& loadShaderCode(const std::string& filename) {
    static Table<std::string, std::string> shaderTextCache;

    if (! shaderTextCache.containsKey(filename)) {
        shaderTextCache.set(filename, readWholeFile(filename));
    }

    return shaderTextCache[filename];
}


/**
 Loads a shader, where <I>basename</I> contains the path and filename up to the 
 ".vrt"/".pix" extensions, and <I>defines</I> is a string to prepend to the 
 beginning of both vertex and pixel shaders.
 */
static ShaderRef loadShader(const std::string& baseName, const std::string& defines) {
    debugAssert(fileExists(baseName + ".vrt"));

    const std::string& vertexShader = loadShaderCode(baseName + ".vrt");
    const std::string& pixelShader  = loadShaderCode(baseName + ".pix");

    ShaderRef s = Shader::fromStrings(
        baseName + ".glsl.vrt", 
        defines + vertexShader, 
        baseName + ".glsl.frg",
        defines + pixelShader);

    return s;
}


SuperShader::Cache::Pair SuperShader::getShader(const Material& material) {
 
    // First check the cache
    Cache::Pair p = cache.getSimilar(material);

    if (p.shadowMappedShader.isNull()) {

        // Not found in cache; load from disk
        std::string path = "";

        static const std::string shadowName    = "ShadowMappedLightPass";
        static const std::string nonShadowName = "NonShadowedPass";

        if (! fileExists(path + shadowName + ".vrt")) {
            if (fileExists("data/" + shadowName + ".vrt")) {
                path = "data/";
            } else if (fileExists("../" + shadowName + ".vrt")) {
                path = "../";
            } else {
                debugAssertM(false, "Could not find the SuperShader *.pix and *.vrt files.");
            }
        }

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

        // TODO... other terms

        p.nonShadowedShader  = loadShader(path + nonShadowName, defines);
        p.shadowMappedShader = loadShader(path + shadowName,    defines);

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

///////////////////////////////////////////////////////////////////////////////////

bool SuperShader::Material::similarTo(const Material& other) const {
    return
        diffuse.similarTo(other.diffuse) &&
        emit.similarTo(other.emit) &&
        specular.similarTo(other.specular) &&
        specularExponent.similarTo(other.specularExponent) &&
        transmit.similarTo(other.transmit) &&
        reflect.similarTo(other.reflect) &&
        (normalBumpMap.isNull() == other.normalBumpMap.isNull());
}

}
