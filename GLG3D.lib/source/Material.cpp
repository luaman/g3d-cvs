/**
 @file   Material.cpp
 @author Morgan McGuire, http://graphics.cs.williams.edu
 @created  2009-03-19
 @edited   2010-03-19
*/
#include "GLG3D/Material.h"
#include "G3D/Table.h"
#include "G3D/WeakCache.h"

#ifdef OPTIONAL
#   undef OPTIONAL
#endif

namespace G3D {

Material::Material() : m_customConstant(Color4::inf()), m_depthWriteHintDistance(nan()) {
}


Material::Ref Material::createEmpty() {
    return new Material();
}


Material::Ref Material::create
(const SuperBSDF::Ref&               bsdf,
 const Component3&                   emissive,
 const BumpMap::Ref&                 bump,
 const MapComponent<Image4>::Ref&    customMap,
 const Color4&                       customConstant,
 const std::string&                  customShaderPrefix) {

    Material::Ref m = Material::createEmpty();

    m->m_bsdf = bsdf;
    m->m_emissive = emissive;
    m->m_bump = bump;
    m->m_customMap = customMap;
    m->m_customConstant = customConstant;
    m->m_customShaderPrefix = customShaderPrefix;

    return m;
}


Material::Ref Material::createDiffuse(const Color3& p_Lambertian) {
    Specification s;
    s.setLambertian(p_Lambertian);
    return create(s);
}


Material::Ref Material::createDiffuse(const std::string& lambertianFilename) {
    Specification s;
    s.setLambertian(lambertianFilename);
    return create(s);
}

typedef WeakCache<Material::Settings, Material::Ref> MaterialCache;

/** Provides access to the cache.  This is not a global because the order of initialization
    needs to be carefully defined */
static MaterialCache& globalCache() {
    static MaterialCache c;
    return c;
}


Material::Ref Material::create(const Specification& specification) {
    MaterialCache& cache = globalCache();
    Material::Ref value = cache[specification];

    if (value.isNull()) {
        // Construct the appropriate material
        value = new Material();

        value->m_bsdf =
            SuperBSDF::create(
                specification.loadLambertian(),
                specification.loadSpecular(),
                specification.loadTransmissive(),
                specification.m_etaTransmit,
                specification.m_extinctionTransmit,
                specification.m_etaReflect,
                specification.m_extinctionReflect);

        value->m_depthWriteHintDistance = specification.m_depthWriteHintDistance;
        value->m_customShaderPrefix = specification.m_customShaderPrefix;
        value->m_refractionHint = specification.m_refractionHint;
        value->m_mirrorHint = specification.m_mirrorHint;

        // load emission map
        value->m_emissive = specification.loadEmissive();

        // load bump map
        if (! specification.m_bump.texture.filename.empty()) {
            value->m_bump = BumpMap::create(specification.m_bump);
        }

        // Update the cache
        cache.set(specification, value);
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

void Material::configure(VertexAndPixelShader::ArgList& args) const {
    static const bool OPTIONAL = true;

    if (m_bsdf->lambertian().notBlack() || m_bsdf->lambertian().nonUnitAlpha()) {
        if (m_bsdf->lambertian().texture().notNull()) {
            args.set("lambertianMap",            m_bsdf->lambertian().texture(), OPTIONAL);
            if (m_bsdf->lambertian().constant() != Color4::one()) {
                args.set("lambertianConstant",   m_bsdf->lambertian().constant(), OPTIONAL);
            }
        } else {
            args.set("lambertianConstant",       m_bsdf->lambertian().constant(), OPTIONAL);
        }
    }

    if (m_bsdf->specular().notBlack()) {
        if (m_bsdf->specular().texture().notNull()) {
            args.set("specularMap",              m_bsdf->specular().texture(), OPTIONAL);
            if (m_bsdf->specular().constant() != Color4::one()) {
                args.set("specularConstant",     m_bsdf->specular().constant(), OPTIONAL);
            }
        } else {
            args.set("specularConstant",         m_bsdf->specular().constant(), OPTIONAL);
        }
    }

    if (m_customConstant.isFinite()) {
        args.set("customConstant",               m_customConstant, OPTIONAL);
    }

    if (m_customMap.notNull()) {
        args.set("customMap",                    m_customMap->texture(), OPTIONAL);
    }

    if (m_emissive.notBlack()) {
        args.set("emissiveConstant",             m_emissive.constant(), OPTIONAL);

        if (m_emissive.texture().notNull()) {
            args.set("emissiveMap",              m_emissive.texture(), OPTIONAL);
        }
    }

    if (m_bump.notNull() && (m_bump->settings().scale != 0)) {
        args.set("normalBumpMap",       m_bump->normalBumpMap()->texture(), OPTIONAL);
        if (m_bump->settings().iterations > 0) {
            args.set("bumpMapScale",        m_bump->settings().scale, OPTIONAL);
            args.set("bumpMapBias",         m_bump->settings().bias, OPTIONAL);
        }
    }

    debugAssert(m_bump.isNull() || m_bump->settings().iterations >= 0);
}


void Material::computeDefines(std::string& defines) const {
    // Set diffuse if not-black or if there is an alpha mask

    if (m_bsdf->lambertian().notBlack() || m_bsdf->lambertian().nonUnitAlpha()) {
        if (m_bsdf->lambertian().texture().notNull()) {
            defines += "#define LAMBERTIANMAP\n";
            if (m_bsdf->lambertian().constant() != Color4::one()) {
                defines += "#define LAMBERTIANCONSTANT\n";
            }
        } else {
            defines += "#define LAMBERTIANCONSTANT\n";
        }
    }

    if (m_bsdf->specular().notBlack()) {
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

    if (m_emissive.notBlack()) {
        // Must always set the emissive constant if there is any emission
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

    if (m_customConstant.isFinite()) {
        defines += "#define CUSTOMCONSTANT\n";
    }

    if (m_customMap.notNull()) {
        defines += "#define CUSTOMMAP\n";
    }

    defines += m_customShaderPrefix;
}


bool Material::similarTo(const Material& other) const {
    return
        m_bsdf->similarTo(other.m_bsdf) &&
        (m_emissive.factors() == other.m_emissive.factors()) &&
        (m_customMap.notNull() == other.m_customMap.notNull()) &&
        (m_customConstant.isFinite() == other.m_customConstant.isFinite()) &&
        (m_bump.isNull() == other.m_bump.isNull()) &&
        (m_bump.isNull() || m_bump->similarTo(other.m_bump)) &&
        (m_customShaderPrefix == other.m_customShaderPrefix);
}


size_t Material::SimilarHashCode::hashCode(const G3D::Material& mat) {
    return 
        (mat.m_bsdf->lambertian().factors() << 10) ^
        (mat.m_bsdf->specular().factors() << 4) ^
        (mat.m_bsdf->transmissive().factors() << 3) ^
        (int)(mat.m_bump.isNull()) ^
        (mat.m_emissive.factors() << 20) ^
        HashTrait<std::string>::hashCode(mat.m_customShaderPrefix);
}


bool Material::hasAlphaMask() const {
    return m_bsdf->lambertian().min().a < 1.0f;
}



} // namespace G3D
