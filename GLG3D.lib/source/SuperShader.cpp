/**
 @file SuperShader.cpp

 @author Morgan McGuire, morgan@cs.williams.edu
 */

#include "GLG3D/ShadowMap.h"
#include "GLG3D/SuperShader.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/fileutils.h"
#include "G3D/Log.h"

// Defined on some operating systems, but used as a variable
// in this file
#ifdef OPTIONAL
#undef OPTIONAL
#endif

namespace G3D {

namespace SuperShader {

////////////////////////////////////////////////////////////////////////////
// 
// G3D::SuperShader::Pass
//

Table<std::string, std::string> Pass::shaderTextCache;
NonShadowedPassRef Pass::nonShadowedInstance;
ExtraLightPassRef  Pass::extraLightInstance;
ShadowedPassRef    Pass::shadowedInstance;

ShaderRef Pass::getConfiguredShader(
    const std::string&  vertexFilename,
    const std::string&  pixelFilename,
    const Material&     material) {

    const std::string& key = vertexFilename + pixelFilename;

    // First try to load from the cache

    ShaderRef shader = cache.getSimilar(key, material);

    if (shader.isNull()) {
        // Load
        std::string defines;
        material.computeDefines(defines);
        shader = loadShader(vertexFilename, pixelFilename, defines);

        // Put into the cache
        cache.add(key, material, shader);
    }

    material.configure(shader->args);

    return shader;
}


void Pass::primeCodeCache(const std::string& originalFilename) {
    if (shaderTextCache.containsKey(originalFilename)) {
        // Already in cache
        return;
    }

    if (originalFilename == "") {
        shaderTextCache.set("", "");
        return;
    }

    // Locate the file (or error out!)
    std::string filename = System::findDataFile(originalFilename);

    // Read it in
    std::string code = readWholeFile(filename);

    // Process #includes
    std::string path = filenamePath(filename);

    Shader::processIncludes(path, code);

    shaderTextCache.set(originalFilename, code);
}


ShaderRef Pass::loadShader(
    const std::string& vertexFilename,
    const std::string& pixelFilename,
    const std::string& defines) {

    // Fill the cache
    primeCodeCache(vertexFilename);
    primeCodeCache(pixelFilename);

    // Fetch and compile the customized shader
    ShaderRef s = 
        Shader::fromStrings(
            defines + shaderTextCache[vertexFilename],
            defines + shaderTextCache[pixelFilename]);

    // By default, assume backface culling
    s->args.set("backside", 1.0, true);

    s->setPreserveState(false);

    return s;
}


void Pass::purgeCache() {
    cache.clear();
    shaderTextCache.clear();
    nonShadowedInstance = NULL;
    shadowedInstance = NULL;
}


Pass::Pass() {}


Pass::Pass(const std::string& vertexFilename, const std::string& pixelFilename)
: m_vertexFilename(vertexFilename), m_pixelFilename(pixelFilename) {

}


PassRef Pass::fromFiles(const std::string& vertexFilename, const std::string& pixelFilename) {
    return new Pass(vertexFilename, pixelFilename);
}


ShaderRef Pass::getConfiguredShader(const Material& material, RenderDevice::CullFace c) {

    if (c != RenderDevice::CULL_CURRENT) {
        float f = 1.0f;
        if (c == RenderDevice::CULL_FRONT) {
            f = -1.0f;
        }
        args.set("backside", f, true);
    }

    // Get the shader from the cache
    ShaderRef s = getConfiguredShader(m_vertexFilename, m_pixelFilename, material);

    // Merge arguments
    s->args.set(args);

    return s;
}

////////////////////////////////////////////////////////////////////////////
// 
// G3D::SuperShader::Pass::Cache
//

Pass::Cache Pass::cache;


void Pass::Cache::add(const std::string& key, const Material& mat, const ShaderRef& shader) {

    if (! table.containsKey(key)) {
        const ShaderTable newTable;
        table.set(key, newTable);
    }

    ShaderTable& shaderTable = table[key];

    shaderTable.set(mat, shader);
}


ShaderRef Pass::Cache::getSimilar(const std::string& key, const Material& mat) const {
    
    if (! table.containsKey(key)) {
        return NULL;
    }

    const ShaderTable& shaderTable = table[key];
    if (! shaderTable.containsKey(mat)) {
        return NULL;
    } else {
        return shaderTable[mat];
    }
}
    

/////////////////////////////////////////////////////////////////////////////
// 
// G3D::SuperShader::NonShadowedPass
//

NonShadowedPassRef NonShadowedPass::instance() {
    if (nonShadowedInstance.isNull()) {
        nonShadowedInstance = new NonShadowedPass();
    }
    return nonShadowedInstance;
}


NonShadowedPass::NonShadowedPass() : 
    Pass(System::findDataFile("SS_NonShadowedPass.vrt"),
         System::findDataFile("SS_NonShadowedPass.pix")) {
}

// Avoid synthesizing new strings during shader configuration
static const int NUM_LIGHTS = 8;
static std::string lightPositionString[NUM_LIGHTS];
static std::string lightColorString[NUM_LIGHTS];
static std::string lightAttenuationString[NUM_LIGHTS];
static std::string lightDirectionString[NUM_LIGHTS];
static const bool OPTIONAL = true;

static void initializeStringConstants() {
    static bool stringConstantsInitialized = false;

    if (stringConstantsInitialized) {
        return;
    }

    for (int L = 0; L < NUM_LIGHTS; ++L) {
        std::string N = format("%d", L);
        lightPositionString[L]    = "lightPosition" + N;
        lightColorString[L]       = "lightColor" + N;
        lightAttenuationString[L] = "lightAttenuation" + N;
        lightDirectionString[L]   = "lightDirection" + N;
    }

    stringConstantsInitialized = true;
}


static void configureLight(
   const GLight& light,
   int i,
   VertexAndPixelShader::ArgList&  args) {

    args.set(lightPositionString[i],    light.position);
    args.set(lightColorString[i],       light.color);
    const float cosThresh =  cos(toRadians(light.spotCutoff));
    args.set(lightAttenuationString[i], Vector4(light.attenuation[0], light.attenuation[1], light.attenuation[2], 
                               cosThresh));
    args.set(lightDirectionString[i],   light.spotDirection);
}


/** Called by NonShadowedPass::setLighting and ExtraLightPass::setLighting.
 @param lightIndex index of the first light in the array to use.
 @param N number of lights to configure*/
static void configureLights
(int lightIndex, 
 int N, 
 const Array<GLight>& lightArray,
 VertexAndPixelShader::ArgList&  args) {

    initializeStringConstants();

    for (int i = 0; i < N; ++i) {
        if (lightArray.size() > i + lightIndex) {
            const GLight& light = lightArray[i + lightIndex];

            configureLight(light, i, args);
        } else {
            // This light is off. Choose attenuation w = 2 and set its color to black
            args.set(lightPositionString[i],    Vector4(0, 1, 0, 0));
            args.set(lightColorString[i],       Color3::black());
            args.set(lightAttenuationString[i], Vector4(1, 0, 0, 2));
            args.set(lightDirectionString[i],   Vector3(1,0,0));
        }
    }
}


void NonShadowedPass::setLighting(const LightingRef& lighting) {

    configureLights(0, LIGHTS_PER_PASS, lighting->lightArray, args);
    
    args.set("ambientTop",      lighting->ambientTop);
    args.set("ambientBottom",   lighting->ambientBottom);

    if (lighting->environmentMap.notNull()) {
        args.set("environmentMap",  lighting->environmentMap, OPTIONAL);
    }

    // Emissive scale is modified in getConfiguredShader

    m_emissiveScale       = lighting->emissiveScale;
    m_environmentMapColor = lighting->environmentMapColor;

}


ShaderRef NonShadowedPass::getConfiguredShader(
    const Material& material,
    RenderDevice::CullFace c) {

    ShaderRef s = Pass::getConfiguredShader(material, c);

    s->args.set("emissiveConstant",    material.emissive().constant()* m_emissiveScale, OPTIONAL);
    s->args.set("environmentMapScale", m_environmentMapColor, OPTIONAL);

    return s;
}

/////////////////////////////////////////////////////////////////////////////
// 
// G3D::SuperShader::ExtraLightPass
//

ExtraLightPassRef ExtraLightPass::instance() {
    if (extraLightInstance.isNull()) {
        extraLightInstance = new ExtraLightPass();
    }
    return extraLightInstance;
}


ExtraLightPass::ExtraLightPass() : 
    Pass(System::findDataFile("SS_NonShadowedPass.vrt"),
         System::findDataFile("SS_NonShadowedPass.pix")) {

    args.set("ambientTop",      Color3::black());
    args.set("ambientBottom",   Color3::black());

    // Knock out all other terms if they are specified
    args.set("environmentConstant", Color3::black(), OPTIONAL);
    static TextureRef emptyCubeMap = Texture::createEmpty("empty cube map", 16, 16, ImageFormat::RGB8(), Texture::DIM_CUBE_MAP);
    args.set("environmentMap",  emptyCubeMap, OPTIONAL);

    args.set("emitConstant", Color3::black(), OPTIONAL);
    args.set("reflectConstant", Color3::black(), OPTIONAL);
    args.set("transmitConstant", Color3::black(), OPTIONAL);
}


void ExtraLightPass::setLighting(const Array<GLight>& lightArray, int index) {
    configureLights(index, LIGHTS_PER_PASS, lightArray, args);
}


/////////////////////////////////////////////////////////////////////////////
// 
// G3D::SuperShader::ShadowedPass
//

ShadowedPassRef ShadowedPass::instance() {
    if (shadowedInstance.isNull()) {
        shadowedInstance = new ShadowedPass();
    }
    return shadowedInstance;
}


ShadowedPass::ShadowedPass() : 
    Pass(System::findDataFile("SS_ShadowMappedLightPass.vrt"),
         System::findDataFile("SS_ShadowMappedLightPass.pix")) {
}


void ShadowedPass::setLight(
    const GLight&                   light, 
    const ShadowMapRef&             shadowMap) {
    
    configureLight(light, 0, args);

    // Shadow map setup
    if (GLCaps::enumVendor() == GLCaps::ATI) {
        args.set("shadowMap",       shadowMap->colorDepthTexture());
    } else {
        args.set("shadowMap",       shadowMap->depthTexture());
    }

    args.set("lightMVP",        shadowMap->biasedLightMVP());
}

} // SuperShader namespace

} // G3D namespace
