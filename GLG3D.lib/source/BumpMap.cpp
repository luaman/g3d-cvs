/**
 @file   BumpMap.cpp
 @author Morgan McGuire, http://graphics.cs.williams.edu
 @edited 2009-03-25
 @date   2010-01-30
*/
#include "GLG3D/BumpMap.h"
#include "G3D/Any.h"

namespace G3D {

bool BumpMap::Specification::operator==(const Specification& other) const {
    return (texture == other.texture) && (settings == other.settings);
}


BumpMap::Specification::Specification(const Any& any) {
    if (any.type() == Any::STRING) {
        // Treat as a filename
        texture.filename = System::findDataFile(any.string());
        texture.preprocess = Texture::Preprocess::normalMap();
    } else {
        for (Any::AnyTable::Iterator it = any.table().begin(); it.hasMore(); ++it) {
            const std::string& key = toLower(it->key);
            if (key == "texture") {
                texture = it->value;
            } else if (key == "settings") {
                settings = it->value;
            } else {
                any.verify(false, "Illegal key: " + it->key);        
            }
        }
    }
}


BumpMap::BumpMap(const MapComponent<Image4>::Ref& normalBump, const Settings& settings) : 
    m_normalBump(normalBump), m_settings(settings) {
}


BumpMap::Ref BumpMap::create(const MapComponent<Image4>::Ref& normalBump, const Settings& settings) {
    return new BumpMap(normalBump, settings);
}


BumpMap::Ref BumpMap::create(const Specification& spec) {
    return BumpMap::create(MapComponent<Image4>::create(NULL, Texture::create(spec.texture)), spec.settings);    
}


static bool hasTexture(const Component4& c) {
    return 
        (c.factors() == Component4::MAP) ||
        (c.factors() == Component4::MAP_TIMES_CONSTANT);
}


bool BumpMap::similarTo(const BumpMap::Ref& other) const {
    return
//        ((m_numIterations == other->m_numIterations) || 
//         ((m_numIterations > 1) && (other->m_numIterations > 1))) &&
        (m_settings.iterations == other->m_settings.iterations) &&
        (hasTexture(m_normalBump) == hasTexture(other->m_normalBump));
}

///////////////////////////////////////////////////////////

BumpMap::Settings::Settings(const Any& any) {
    *this = Settings();
    any.verifyName("BumpMap::Settings");
    for (Any::AnyTable::Iterator it = any.table().begin(); it.hasMore(); ++it) {
        const std::string& key = toLower(it->key);
        if (key == "iterations") {
            iterations = iMax(0, iRound(it->value.number()));
        } else if (key == "scale") {
            scale = it->value;
        } else if (key == "bias") {
            bias = it->value;
        } else {
            any.verify(false, "Illegal key: " + it->key);
        }
    }
}


BumpMap::Settings::operator Any() const {
    Any any(Any::TABLE, "BumpMap::Settings");
    any.set("scale", scale);
    any.set("bias", bias);
    any.set("iterations", iterations);
    return any;
}


bool BumpMap::Settings::operator==(const Settings& other) const {
    return 
        (scale == other.scale) &&
        (bias == other.bias) &&
        (iterations == other.iterations);

}

} // G3D
