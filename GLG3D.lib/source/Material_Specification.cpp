/**
 @file   Material_Specification.h
 @author Morgan McGuire, http://graphics.cs.williams.edu
 @date   2009-03-10
 \edited 2010-03-29
*/
#include "GLG3D/Material.h"
#include "G3D/Any.h"

namespace G3D {

Material::Specification::Specification() : 
  m_depthWriteHintDistance(nan()),
  m_lambertianConstant(Color4(0.85f, 0.85f, 0.85f, 1.0f)),
  m_specularConstant(Color3::zero()),
  m_shininessConstant(SuperBSDF::packedSpecularNone()),
  m_transmissiveConstant(Color3::zero()),
  m_etaTransmit(1.0f),
  m_extinctionTransmit(1.0f),
  m_etaReflect(1.0f),
  m_extinctionReflect(1.0f),
  m_emissiveConstant(Color3::zero()),
  m_refractionHint(RefractionQuality::DYNAMIC_FLAT), 
  m_mirrorHint(MirrorQuality::STATIC_ENV) {
}


Material::Specification::Specification(const Any& any) {
    *this = Specification();

    if (any.type() == Any::STRING) {
        // Single filename; treat as a diffuse-only texture map
        setLambertian(any.resolveStringAsFilename());
        return;
    }

    any.verifyName("Material::Specification");

    for (Any::AnyTable::Iterator it = any.table().begin(); it.hasMore(); ++it) {
        const std::string& key = toLower(it->key);
        if (key == "lambertian") {
            if (it->value.type() == Any::STRING) {
                setLambertian(it->value.resolveStringAsFilename());
            } else if (beginsWith(toLower(it->value.name()), "color4")) {
                setLambertian(Color4(it->value));
            } else if (beginsWith(toLower(it->value.name()), "color3")) {
                setLambertian(Color3(it->value));
            } else {
                // Full specification
                setLambertian(Texture::Specification(it->value));
            }
        } else if (key == "specular") {
            if (it->value.type() == Any::STRING) {
                setSpecular(it->value.resolveStringAsFilename());
            } else if (beginsWith(toLower(it->value.name()), "color3")) {
                setSpecular(Color3(it->value));
            } else {
                // Full specification
                setSpecular(Texture::Specification(it->value));
            }
        } else if (key == "shininess") {
            switch (it->value.type()) {
            case Any::STRING:
                setShininess(it->value.resolveStringAsFilename());
                break;

            case Any::ARRAY:
                if (beginsWith(toLower(it->value.name()), "glossyexponent")) {
                    it->value.verifySize(1);
                    setGlossyExponentShininess(it->value[0]);
                } else if (beginsWith(toLower(it->value.name()), "mirror")) {
                    it->value.verifySize(0);
                    setMirrorShininess();
                } else {
                    // Full specification
                    setShininess(Texture::Specification(it->value));
                }
                break;

            default:
                // Full specification
                setShininess(Texture::Specification(it->value));
                break;
            }    
        } else if (key == "transmissive") {
            if (beginsWith(toLower(it->value.name()), "color3")) {
                setTransmissive(Color3(it->value));
            } else {
                // Full specification
                setTransmissive(Texture::Specification(it->value));
            }
        } else if (key == "emissive") {
            if (beginsWith(toLower(it->value.name()), "color3")) {
                setEmissive(Color3(it->value));
            } else {
                // Full specification
                setEmissive(Texture::Specification(it->value));
            }
        } else if (key == "bump") {
            setBump(BumpMap::Specification(it->value));
        } else if (key == "refractionhint") {
            m_refractionHint = it->value;
        } else if (key == "mirrorhint") {
            m_mirrorHint = it->value;
        } else if (key == "etatransmit") {
            m_etaTransmit = it->value;
        } else if (key == "extinctiontransmit") {
            m_extinctionTransmit = it->value;
        } else if (key == "etareflect") {
            m_etaReflect = it->value;
        } else if (key == "extinctionreflect") {
            m_extinctionReflect = it->value;
        } else if (key == "customshaderprefix") {
            m_customShaderPrefix = it->value.string();
        } else if (key == "depthwritehintdistance") {
            m_depthWriteHintDistance = it->value;
        } else {
            any.verify(false, "Illegal key: " + it->key);
        }
    }
}


void Material::Specification::setLambertian(const std::string& filename, const Color4& constant) {
    m_lambertian = Texture::Specification();
    m_lambertian.filename = filename;
    m_lambertianConstant = constant;
}


void Material::Specification::setLambertian(const Color4& constant) {
    setLambertian("", constant);
}


void Material::Specification::setLambertian(const Texture::Specification& spec) {
    m_lambertianConstant = Color4::one();
    m_lambertian = spec;        
}


void Material::Specification::removeLambertian() {
    setLambertian(Color4(0,0,0,1));
}


void Material::Specification::setEmissive(const std::string& filename, const Color3& constant) {
    m_emissive = Texture::Specification();
    m_emissive.filename = filename;
    m_emissiveConstant = constant;
}


void Material::Specification::setEmissive(const Color3& constant) {
    setEmissive("", constant);
}


void Material::Specification::removeEmissive() {
    setEmissive(Color3::zero());
}


void Material::Specification::setEmissive(const Texture::Specification& spec) {
    m_emissiveConstant = Color3::one();
    m_emissive = spec;        
}


void Material::Specification::setSpecular(const std::string& filename, const Color3& constant) {
    m_specular = Texture::Specification();
    m_specular.filename = filename;
    m_specularConstant = constant;
}


void Material::Specification::setSpecular(const Color3& constant) {
    setSpecular("", constant);
}


void Material::Specification::setSpecular(const Texture::Specification& spec) {
    m_specularConstant = Color3::one();
    m_specular = spec;        
}


void Material::Specification::removeSpecular() {
    setSpecular(Color3::zero());
}


void Material::Specification::setShininess(const std::string& filename, float constant) {
    m_shininess = Texture::Specification();
    m_shininess.filename = filename;
    m_shininessConstant = constant;
    if (constant == SuperBSDF::packedSpecularNone()) {
        removeSpecular();
    }
}


void Material::Specification::setShininess(float constant) {
    setShininess("", constant);
}


void Material::Specification::setShininess(const Texture::Specification& spec) {
    m_shininessConstant = 1.0;
    m_shininess = spec;        
}


void Material::Specification::setTransmissive(const std::string& filename, const Color3& constant) {
    m_transmissive = Texture::Specification();
    m_transmissive.filename = filename;
    m_transmissiveConstant = constant;
}


void Material::Specification::setTransmissive(const Color3& constant) {
    setTransmissive("", constant);
}


void Material::Specification::setTransmissive(const Texture::Specification& spec) {
    m_transmissiveConstant = Color3::one();
    m_transmissive = spec;        
}


void Material::Specification::removeTransmissive() {
    setTransmissive(Color3::zero());
}


void Material::Specification::setEta(float etaTransmit, float etaReflect) {
    m_etaTransmit = etaTransmit;
    m_etaReflect = etaReflect;
    debugAssert(m_etaTransmit > 0);
    debugAssert(m_etaTransmit < 10);
    debugAssert(m_etaReflect > 0);
    debugAssert(m_etaReflect < 10);
}


void Material::Specification::setBump
(const std::string&         filename, 
 const BumpMap::Settings&   settings,
 float 	                    normalMapWhiteHeightInPixels) {
    
     m_bump = BumpMap::Specification();
     m_bump.texture.filename = filename;
     m_bump.texture.preprocess = Texture::Preprocess::normalMap();
     m_bump.texture.preprocess.bumpMapPreprocess.zExtentPixels = 
         normalMapWhiteHeightInPixels;
     m_bump.settings = settings;
}


void Material::Specification::removeBump() {
    m_bump.texture.filename = "";
}


bool Material::Specification::operator==(const Specification& s) const {
    return 
        (m_lambertian == s.m_lambertian) &&
        (m_lambertianConstant == s.m_lambertianConstant) &&

        (m_specular == s.m_specular) &&
        (m_specularConstant == s.m_specularConstant) &&

        (m_shininess == s.m_shininess) &&
        (m_shininessConstant == s.m_shininessConstant) &&

        (m_transmissive == s.m_transmissive) &&
        (m_transmissiveConstant == s.m_transmissiveConstant) &&

        (m_emissive == s.m_emissive) &&
        (m_emissiveConstant == s.m_emissiveConstant) &&

        (m_bump == s.m_bump) &&

        (m_etaTransmit == s.m_etaTransmit) &&
        (m_extinctionTransmit == s.m_extinctionTransmit) &&
        (m_etaReflect == s.m_etaReflect) &&
        (m_extinctionReflect == s.m_extinctionReflect) &&

        (m_refractionHint == s.m_refractionHint) &&
        (m_mirrorHint == s.m_mirrorHint) &&
        
        (m_customShaderPrefix == s.m_customShaderPrefix) &&
        
        (m_depthWriteHintDistance == s.m_depthWriteHintDistance);
}


size_t Material::Specification::hashCode() const {
    return 
        HashTrait<std::string>::hashCode(m_lambertian.filename) ^
        m_lambertianConstant.hashCode() ^

        HashTrait<std::string>::hashCode(m_specular.filename) ^
        m_specularConstant.hashCode() ^

        HashTrait<std::string>::hashCode(m_shininess.filename) ^
        (size_t)m_shininessConstant ^

        HashTrait<std::string>::hashCode(m_transmissive.filename) ^
        m_transmissiveConstant.hashCode() ^

        HashTrait<std::string>::hashCode(m_emissive.filename) ^
        m_emissiveConstant.hashCode() ^

        HashTrait<std::string>::hashCode(m_bump.texture.filename) ^
        
        HashTrait<std::string>::hashCode(m_customShaderPrefix) 
        ^ (int)(m_depthWriteHintDistance);
}


Component4 Material::Specification::loadLambertian() const {
    Texture::Ref texture;

    if (m_lambertian.filename != "") {
        texture = Texture::create(m_lambertian);
    }

    return Component4(m_lambertianConstant, texture);
}


Component3 Material::Specification::loadTransmissive() const {
    Texture::Ref texture;

    if (m_transmissive.filename != "") {
        texture = Texture::create(m_transmissive);
    }

    return Component3(m_transmissiveConstant, texture);
}


Component4 Material::Specification::loadSpecular() const {
    Texture::Ref texture;

    if (m_specular.filename != "") {
        if (m_shininess.filename != "") {
            // Glossy and shiny
            texture = Texture::fromTwoFiles
                (m_specular.filename, 
                 m_shininess.filename,
                 m_specular.desiredFormat,
                 m_specular.dimension,
                 m_specular.settings);
        } else {
            // Only specular
            texture = Texture::create(m_specular);
        }
    } else if (m_shininess.filename != "") {
        
        // Only shininess.  Pack it into the alpha of an all-white texture
        GImage s(m_shininess.filename);

        s.convertToL8();
        GImage pack(s.width(), s.height(), 4);
        int n = s.width() * s.height();
        for (int i = 0; i < n; ++i) {
            pack.pixel4()[i] = Color4uint8(255, 255, 255, s.pixel1()[i].value);
        }

        texture = Texture::fromGImage
            (m_shininess.filename,
             pack,
             ImageFormat::RGBA8(),
             m_shininess.dimension,
             m_shininess.settings);
    }

    return Component4(Color4(m_specularConstant, m_shininessConstant), texture);
}


Component3 Material::Specification::loadEmissive() const {
    Texture::Ref texture;

    if (m_emissive.filename != "") {
        texture = Texture::create(m_emissive);
    }

    return Component3(m_emissiveConstant, texture);

}

} // namespace G3D

