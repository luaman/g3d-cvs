/**
 @file   Material_Settings.h
 @author Morgan McGuire, morgan@cs.williams.edu
 @date   2009-03-10
*/
#include "GLG3D/Material.h"
#include "G3D/Any.h"

namespace G3D {

Material::Settings::Settings() : 
  m_lambertianFilename(""), 
  m_lambertianConstant(Color4(0.85f, 0.85f, 0.85f, 1.0f)),
  m_specularFilename(""),
  m_specularConstant(Color3::zero()),
  m_shininessFilename(""),
  m_shininessConstant(SuperBSDF::packedSpecularNone()),
  m_transmissiveFilename(""),
  m_transmissiveConstant(Color3::zero()),
  m_etaTransmit(1.0f),
  m_extinctionTransmit(1.0f),
  m_etaReflect(1.0f),
  m_extinctionReflect(1.0f),
  m_emissiveFilename(""),
  m_emissiveConstant(Color3::zero()),
  m_bumpFilename(""),
  m_normalMapWhiteHeightInPixels(0),
  m_textureDimension(Texture::DIM_2D_NPOT) {
}


/*
Material::Settings::Settings(const Any& a) {
    alwaysAssertM(a.type() == AnyVal::TABLE, "Must be a table of values");

    m_name = a.get("name", "Untitled").string();

    m_lambertianFilename = a.get("lambertianFilename", "").string();
    m_lambertianConstant = a.get("lambertianConstant", Color4::one());

    m_specularFilename = a.get("specularFilename", "").string();
    m_specularConstant = a.get("specularConstant", Color4::one());

    m_shininessFilename = a.get("shininessFilename", "").string();
    m_shininessConstant = iClamp(a.get("shininessConstant", 0), 0, 1);

    m_transmissiveFilename = a.get("transmissiveFilename", "").string();
    m_transmissiveConstant = a.get("transmissiveConstant", Color3::black());

    m_etaTransmit = a.get("etaTransmit", 1);
    m_extinctionTransmit = a.get("extinctionTransmit", 0);

    m_etaReflect = a.get("etaReflect", 1);
    m_extinctionReflect = a.get("extinctionReflect", 0);

    m_emissiveFilename = a.get("emissiveFilename", "").string();
    m_emissiveConstant = a.get("emissiveConstant", Color3::black());

    AnyVal b = a.get("bump", AnyVal());
    if (! b.isNil()) {
        m_bumpSettings = BumpMap::Settings::fromAnyVal(b);
        m_bumpFilename = b.get("filename", "").string();
        m_normalMapWhiteHeightInPixels = b.get("normalMapWhiteHeightInPixels", 0);
    } else {
        m_bumpFilename = "";
    }

    m_textureSettings  = Texture::Settings::fromAnyVal(a.get("textureSettings"));
    m_textureDimension = 
        (a.get("textureDimension", "DIM_2D_NPOT").string() == "DIM_2D") ? 
        Texture::DIM_2D : Texture::DIM_2D_NPOT;
}
*/

Material::Settings::operator Any() const {
    Any a(Any::TABLE);
    a.set("name", m_name);
    a.set("lambertianFilename", m_lambertianFilename);
    a.set("lambertianConstant", m_lambertianConstant);

    a.set("specularFilename", m_specularFilename);
    a.set("specularConstant", m_specularConstant);

    a.set("shininessFilename", m_shininessFilename);
    a.set("shininessConstant", m_shininessConstant);

    a.set("transmissiveFilename", m_transmissiveFilename);
    a.set("transmissiveConstant", m_transmissiveConstant);

    a.set("etaTransmit", m_etaTransmit);
    a.set("extinctionTransmit", m_extinctionTransmit); 
    a.set("etaReflect", m_etaReflect);
    a.set("extinctionReflect", m_extinctionReflect); 

    a.set("emissiveFilename", m_emissiveFilename);
    a.set("emissiveConstant", m_emissiveConstant);

    Any b = m_bumpSettings;
    b.set("filename", m_bumpFilename);
    b.set("normalMapWhiteHeightInPixels", m_normalMapWhiteHeightInPixels);
    a.set("bump", b);

//    a.set("textureSettings", m_textureSettings);  TODO
    a.set("textureDimension", (m_textureDimension == Texture::DIM_2D) ? 
        "DIM_2D" : "DIM_2D_NPOT");

    return a;
}


void Material::Settings::setLambertian(const std::string& filename, const Color4& constant) {
    m_lambertianFilename = filename;
    m_lambertianConstant = constant;
}


void Material::Settings::setLambertian(const Color4& constant) {
    setLambertian("", constant);
}


void Material::Settings::removeLambertian() {
    setLambertian(Color4(0,0,0,1));
}


void Material::Settings::setEmissive(const std::string& filename, const Color3& constant) {
    m_emissiveFilename = filename;
    m_emissiveConstant = constant;
}


void Material::Settings::setEmissive(const Color3& constant) {
    setEmissive("", constant);
}


void Material::Settings::removeEmissive() {
    setEmissive(Color3::zero());
}


void Material::Settings::setSpecular(const std::string& filename, const Color3& constant) {
    m_specularFilename = filename;
    m_specularConstant = constant;
}


void Material::Settings::setSpecular(const Color3& constant) {
    setSpecular("", constant);
}


void Material::Settings::removeSpecular() {
    setSpecular(Color3::zero());
}


void Material::Settings::setShininess(const std::string& filename, float constant) {
    m_shininessFilename = filename;
    m_shininessConstant = constant;
    if (constant == SuperBSDF::packedSpecularNone()) {
        removeSpecular();
    }
}


void Material::Settings::setShininess(float constant) {
    setShininess("", constant);
}


void Material::Settings::setTransmissive(const std::string& filename, const Color3& constant) {
    m_transmissiveFilename = filename;
    m_transmissiveConstant = constant;
}


void Material::Settings::setTransmissive(const Color3& constant) {
    setTransmissive("", constant);
}


void Material::Settings::removeTransmissive() {
    setTransmissive(Color3::zero());
}


void Material::Settings::setEta(float etaTransmit, float etaReflect) {
    m_etaTransmit = etaTransmit;
    m_etaReflect = etaReflect;
    debugAssert(m_etaTransmit > 0);
    debugAssert(m_etaTransmit < 10);
    debugAssert(m_etaReflect > 0);
    debugAssert(m_etaReflect < 10);
}


void Material::Settings::setBump
(const std::string&         filename, 
 const BumpMap::Settings&   settings,
 float 	                    normalMapWhiteHeightInPixels) {
    
     m_bumpFilename                 = filename;
     m_normalMapWhiteHeightInPixels = normalMapWhiteHeightInPixels;
     m_bumpSettings                 = settings;
}


void Material::Settings::removeBump() {
    setBump("");
}


bool Material::Settings::operator==(const Settings& s) const {
    return 
        (m_lambertianFilename == s.m_lambertianFilename) &&
        (m_lambertianConstant == s.m_lambertianConstant) &&

        (m_specularFilename == s.m_specularFilename) &&
        (m_specularConstant == s.m_specularConstant) &&

        (m_shininessFilename == s.m_shininessFilename) &&
        (m_shininessConstant == s.m_shininessConstant) &&

        (m_transmissiveFilename == s.m_transmissiveFilename) &&
        (m_transmissiveConstant == s.m_transmissiveConstant) &&

        (m_emissiveFilename == s.m_emissiveFilename) &&
        (m_emissiveConstant == s.m_emissiveConstant) &&

        (m_bumpFilename == s.m_bumpFilename) &&
        (m_bumpSettings == s.m_bumpSettings) &&
        (m_normalMapWhiteHeightInPixels == s.m_normalMapWhiteHeightInPixels) &&

        (m_etaTransmit == s.m_etaTransmit) &&
        (m_extinctionTransmit == s.m_extinctionTransmit) &&
        (m_etaReflect == s.m_etaReflect) &&
        (m_extinctionReflect == s.m_extinctionReflect);
}


size_t Material::Settings::hashCode() const {
    return 
        HashTrait<std::string>::hashCode(m_lambertianFilename) ^
        m_lambertianConstant.hashCode() ^

        HashTrait<std::string>::hashCode(m_specularFilename) ^
        m_specularConstant.hashCode() ^

        HashTrait<std::string>::hashCode(m_shininessFilename) ^
        (size_t)m_shininessConstant ^

        HashTrait<std::string>::hashCode(m_transmissiveFilename) ^
        m_transmissiveConstant.hashCode() ^

        HashTrait<std::string>::hashCode(m_emissiveFilename) ^
        m_emissiveConstant.hashCode() ^

        HashTrait<std::string>::hashCode(m_bumpFilename);
}


Component4 Material::Settings::loadLambertian() const {
    Texture::Ref texture;

    if (m_lambertianFilename != "") {
        texture = Texture::fromFile
            (m_lambertianFilename, 
             TextureFormat::AUTO(),
             m_textureDimension,
             m_textureSettings);
    }

    return Component4(m_lambertianConstant, texture);
}


Component3 Material::Settings::loadTransmissive() const {
    Texture::Ref texture;

    if (m_transmissiveFilename != "") {
        texture = Texture::fromFile
            (m_transmissiveFilename, 
             TextureFormat::RGB8(),
             m_textureDimension,
             m_textureSettings);
    }

    return Component3(m_transmissiveConstant, texture);
}


Component4 Material::Settings::loadSpecular() const {
    Texture::Ref texture;

    if (m_specularFilename != "") {
        if (m_shininessFilename != "") {
            // Glossy and shiny
            texture = Texture::fromTwoFiles
                (m_specularFilename, 
                 m_shininessFilename,
                 ImageFormat::RGBA8(),
                 m_textureDimension,
                 m_textureSettings);
        } else {
            // Only specular
            texture = Texture::fromFile
                (m_specularFilename, 
                 ImageFormat::RGB8(),
                 m_textureDimension,
                 m_textureSettings);
        }
    } else if (m_shininessFilename != "") {
        
        // Only shininess.  Pack it into the alpha of an all-white texture
        GImage s(m_shininessFilename);

        s.convertToL8();
        GImage pack(s.width(), s.height(), 4);
        int n = s.width() * s.height();
        for (int i = 0; i < n; ++i) {
            pack.pixel4()[i] = Color4uint8(255, 255, 255, s.pixel1()[i].value);
        }

        texture = Texture::fromGImage
            (m_shininessFilename,
             pack,
             ImageFormat::RGBA8(),
             m_textureDimension,
             m_textureSettings);
    }

    return Component4(Color4(m_specularConstant, m_shininessConstant), texture);
}


Component3 Material::Settings::loadEmissive() const {
    Texture::Ref texture;

    if (m_emissiveFilename != "") {
        texture = Texture::fromFile
            (m_emissiveFilename, 
             TextureFormat::RGB8(),
             m_textureDimension,
             m_textureSettings);
    }

    return Component3(m_emissiveConstant, texture);

}

} // namespace G3D

