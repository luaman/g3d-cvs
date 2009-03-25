/**
 @file   Material.cpp
 @author Morgan McGuire, morgan@cs.williams.edu
 @date   2009-03-19
*/
#include "GLG3D/Material.h"
#include "G3D/Table.h"
#include "G3D/WeakCache.h"

namespace G3D {

Material::Material() : m_customConstant(Color4::inf()) {
}


Material::Ref Material::create() {
    return new Material();
}


Material::Ref Material::createDiffuse(const Color3& p_Lambertian) {
    Settings s;
    s.setLambertian(p_Lambertian);
    return create(s);
}

typedef WeakCache<Material::Settings, Material::Ref> MaterialCache;

/** Provides access to the cache.  This is not a global because the order of initialization
    needs to be carefully defined */
static MaterialCache& globalCache() {
    static MaterialCache c;
    return c;
}


Material::Ref Material::create(const Settings& settings) {
    MaterialCache& cache = globalCache();
    Material::Ref value = cache[settings];

    if (value.isNull()) {
        // Construct the appropriate material
        value = new Material();

        value->m_bsdf =
            UberBSDF::create(settings.loadLambertian(),
                         settings.loadSpecular(),
                         settings.loadTransmissive(),
                         settings.m_eta,
                         settings.m_extinction);

        // load emission map
        value->m_emissive = settings.loadEmissive();

        // load bump map
        if (settings.m_bumpFilename != "") {
            Texture::PreProcess npp = Texture::PreProcess::normalMap();
            npp.normalMapWhiteHeightInPixels = settings.m_normalMapWhiteHeightInPixels;
            
            Texture::Ref normalBump = Texture::fromFile(settings.m_bumpFilename, TextureFormat::RGBA8(),
                settings.m_textureDimension, settings.m_textureSettings, npp); 

            value->m_bump = BumpMap::create(MapComponent<Image4>::create(NULL, normalBump), settings.m_bumpSettings);
        }

        // Update the cache
        cache.set(settings, value);
    }

    return value;
}



void Material::setStorage(ImageStorage s) const {
    m_bsdf->setStorage(s);
    
    m_emissive.setStorage(s);
    
    if (m_bump.notNull()) {
        m_bump->setStorage(s);
    }
}


void Material::computeDefines(std::string& defines) const {
    // Set diffuse if not-black or if there is an alpha mask
    if (m_bsdf->lambertian().notZero()) {
        if (m_bsdf->lambertian().texture().notNull()) {
            defines += "#define LAMBERTIANMAP\n";

            // If the color is white, don't multiply by it
            if (m_bsdf->lambertian().constant() != Color4::one()) {
                defines += "#define LAMBERTIANCONSTANT\n";
            }
        } else {
            defines += "#define LAMBERTIANCONSTANT\n";
        }
    }

    if (m_customConstant.isFinite()) {
        defines += "#define CUSTOMCONSTANT\n";
    }

    if (m_customMap.notNull()) {
        defines += "#define CUSTOMMAP\n";
    }

    if (m_bsdf->specular().notZero()) {
        if (m_bsdf->specular().texture().notNull()) {
            defines += "#define SPECULARMAP\n";

            // If the color is white, don't multiply by it
            if (m_bsdf->specular().constant() != Color4::one()) {
                defines += "#define SPECULARCONSTANT\n";
            }
        } else  {
            defines += "#define SPECULARCONSTANT\n";
        }
    }

    if (m_bsdf->hasMirror()) {
        defines += "#define MIRROR\n";
    }

    if (m_emissive.notZero()) {
        // Must always set the emission constant if there is any emission
        // because it is modified to contain tone mapping information by SuperShader.
        defines += "#define EMISSIVECONSTANT\n";
        if (m_emissive.texture().notNull()) {
            defines += "#define EMISSIVEMAP\n";
        }
    }

    if (m_bump.notNull() && (m_bump->settings().scale != 0)) {
        defines += "#define NORMALBUMPMAP\n";
        defines += format("#define PARALLAXSTEPS (%d)\n", m_bump->settings().iterations);
    }
}


bool Material::similarTo(const Material& other) const {
    return
        m_bsdf->similarTo(other.m_bsdf) &&
        (m_emissive.factors() == other.m_emissive.factors()) &&
        (m_customMap.notNull() == other.m_customMap.notNull()) &&
        (m_customConstant.isFinite() == other.m_customConstant.isFinite()) &&
        (m_bump.isNull() == other.m_bump.isNull()) &&
        (m_bump.isNull() || m_bump->similarTo(other.m_bump));
}


size_t Material::SimilarHashCode::hashCode(const G3D::Material& mat) {
    return 
        (mat.m_bsdf->lambertian().factors() << 10) ^
        (mat.m_bsdf->specular().factors() << 4) ^
        (mat.m_bsdf->transmissive().factors() << 3) ^
        (int)(mat.m_bump.isNull()) ^
        (mat.m_emissive.factors() << 20);
}


void Material::configure(VertexAndPixelShader::ArgList& args) const {

    // Set diffuse if not-black, or if there is an alpha mask
    if (! m_bsdf->lambertian().isBlack() || 
        (m_bsdf->lambertian().texture().notNull() &&
        ! m_bsdf->lambertian().texture()->opaque())) {
        args.set("lambertianConstant",          m_bsdf->lambertian().constant().rgb());
        if (m_bsdf->lambertian().texture().notNull()) {
            args.set("lambertianMap",           m_bsdf->lambertian().texture());
        }
    }

    if (m_customConstant.isFinite()) {
        args.set("customConstant",              m_customConstant);
    }

    if (m_customMap.notNull()) {
        args.set("customMap",                   m_customMap->texture());
    }

    if (m_bsdf->specular().notZero()) {
        args.set("specularConstant",            m_bsdf->specular().constant());
        if (m_bsdf->specular().texture().notNull()) {
            args.set("specularMap",             m_bsdf->specular().texture());
        }
    }

    if (m_emissive.notZero()) {
        args.set("emissiveConstant",             m_emissive.constant());

        if (m_emissive.texture().notNull()) {
            args.set("emissiveMap",              m_emissive.texture());
        }
    }

    if (m_bump.notNull() && (m_bump->settings().scale != 0)) {
        args.set("normalBumpMap",       m_bump->normalBumpMap()->texture());
        args.set("bumpMapScale",        m_bump->settings().scale);
        args.set("bumpMapBias",         m_bump->settings().offset);
    }

    debugAssert(m_bump.isNull() || m_bump->settings().iterations >= 0);
}

} // namespace G3D
