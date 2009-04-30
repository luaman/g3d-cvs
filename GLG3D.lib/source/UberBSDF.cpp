/**
 @file   UberBSDF.cpp
 @author Morgan McGuire, morgan@cs.williams.edu
 @date   2008-08-10
*/
#include "GLG3D/UberBSDF.h"

namespace G3D {

float UberBSDF::ignoreFloat;

#define INV_PI  (0.318309886f)
#define INV_8PI (0.0397887358f)

UberBSDF::Ref UberBSDF::create
(const Component4& lambertian,
 const Component4& specular,
 const Component3& transmissive,
 float             eta,
 const Color3&     extinction) {

    UberBSDF::Ref bsdf        = new UberBSDF();

    bsdf->m_lambertian      = lambertian;
    bsdf->m_specular        = specular;
    bsdf->m_transmissive    = transmissive;
    bsdf->m_eta             = eta;
    bsdf->m_extinction      = extinction;

    return bsdf;
}


Color4 UberBSDF::shadeDirect
(const Vector3& n,
 const Vector2& texCoord,
 const Vector3& w_i,
 const Color3&  power_i,
 const Vector3& w_o) const {
    
    // Lambertian coefficients
    const Color4& diffuse = m_lambertian.sample(texCoord);

    if (diffuse.a == 0.0f) {
        // Alpha masked
        return Color4::zero();
    }
    
    const float cos_i = max(0.0f, w_i.dot(n));

    Color3 result = diffuse.rgb() * INV_PI;
    
    const Color4& specular = m_specular.sample(texCoord);
    float shininess = specular.a;

    if (shininess != packedSpecularNone()) {
        // Glossy
        // Half-vector
        const Vector3& w_h = (w_i + w_o).direction();
        const float cos_h = max(0.0f, w_h.dot(n));

        float e = (float)unpackSpecularExponent(shininess);
        
        result += computeF(specular.rgb(), cos_i) * 
            (powf(cos_h, e) *
             (shininess + 8.0f) * INV_8PI);
    }
    
    return Color4(result * power_i * cos_i, diffuse.a);
}


bool UberBSDF::scatter
(const Vector3& n,
 const Vector2& texCoord,
 const Vector3& w_i,
 const Color3&  power_i,
 float          eta_i,
 Vector3&       w_o,
 Color3&        power_o,
 float&         eta_o,
 Random&        random,
 bool           lowFreq,
 float&         density) const {

     // Choose a random number on [0, 1], then reduce it by each kind of
     // scattering's probability until it becomes negative (i.e., scatters).
    float r = random.uniform();

    ///////////////////////////////////////////////////////////////////////////////////
    if (m_lambertian.notBlack()) {
        // Sample the diffuse coefficients
        const Color4& diffuse = 
            lowFreq ?
                m_lambertian.mean() :
                m_lambertian.sample(texCoord);
        
        alwaysAssertM(diffuse.a > 0.0f, "Scattered from an alpha masked location");
        const Color3& p_Lambertian = diffuse.rgb();
        float p_LambertianAvg = p_Lambertian.average();
        
        r -= p_LambertianAvg;
        
        if (r < 0.0f) {
            // Lambertian scatter
            
            // (Cannot hit division by zero because the if prevents this
            // case when p_LambertianAvg = 0)
            power_o = power_i * p_Lambertian / p_LambertianAvg;
            w_o = Vector3::cosHemiRandom(n, random);
            density = p_LambertianAvg * 0.01f;
            eta_o = eta_i;

            return true;
        }
    }

    Color3 F;
    bool Finit = false;

    ///////////////////////////////////////////////////////////////////////////////////
    if (m_specular.notBlack()) {

        // Sample the specular coefficients
        const Color4& specular = 
            lowFreq ?
                m_specular.mean() :
                m_specular.sample(texCoord);

        // On the range [0, 1]
        float shininess = specular.a;

        if (shininess != packedSpecularNone()) {
            // There is some specularity
            
            // Cosine of the angle of incidence, for computing F
            const float cos_i = max(0.0f, w_i.dot(n));
            F = computeF(specular.rgb(), cos_i);
            Finit = true;

            const Color3& p_specular = F;
            const float p_specularAvg = p_specular.average();

            r -= p_specularAvg;
            if (r < 0.0f) {
                if (shininess != packedSpecularMirror()) {
                    // Glossy                    
                    shininess = (float)unpackSpecularExponent(shininess);

                    const Vector3& w_h = (w_i + w_o).direction();
                    (void)w_h;
                    // TODO: glossy scatter
                    w_o = w_i;
                    power_o = p_specular * power_i * (1.0f / p_specularAvg);
                    density = p_specularAvg * 0.1f;

                } else {
                    // Mirror

                    w_o = w_i.reflectAbout(n);
                    power_o = p_specular * power_i * (1.0f / p_specularAvg);
                    density = p_specularAvg;
                }

                eta_o = eta_i;
                return true;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////
    if (m_transmissive.notBlack()) {
        // Fresnel transmissive coefficient
        Color3 F_t;

        if (Finit) {
            F_t = (Color3::one() - F);
        } else {
            // Cosine of the angle of incidence, for computing F
            const float cos_i = max(0.0f, w_i.dot(n));
            // F = lerp(0, 1, pow5(1.0f - cos_i)) = pow5(1.0f - cos_i)
            // F_t = 1 - F
            F_t.r = F_t.g = F_t.b = 1.0f - pow5(1.0f - cos_i);
        }

        // Sample transmissive
        const Color3& transmit = 
            lowFreq ?
                m_transmissive.mean() :
                m_transmissive.sample(texCoord);
        const Color3& T0          = transmit.rgb();
        
        const Color3& p_transmit  = F_t * T0;
        const float p_transmitAvg = p_transmit.average();
        
        r -= p_transmitAvg;
        if (r < 0.0f) {
            debugAssert(w_i.dot(n) > 0);
            power_o = p_transmit * power_i * (1.0f / p_transmitAvg);
            w_o = (-w_i).refractionDirection(n, m_eta, eta_i);
            density = p_transmitAvg;
            eta_o = m_eta;

            debugAssert(w_o.isZero() || ((w_o.dot(n) < 0) && w_o.isUnit()));

            // w_o is zero on total internal refraction
            return ! w_o.isZero();
        }
    }

    // Absorbed
    return false;
}


void UberBSDF::setStorage(ImageStorage s) const {
    m_lambertian.setStorage(s);
    m_transmissive.setStorage(s);
    m_specular.setStorage(s);
}


bool UberBSDF::similarTo(const UberBSDF::Ref& other) const {
    return 
        (m_lambertian.factors()   == other->m_lambertian.factors()) &&
        (m_transmissive.factors() == other->m_transmissive.factors()) &&
        (m_specular.factors()     == other->m_specular.factors());
}


bool UberBSDF::hasMirror() const {
    const Color4& m = m_specular.max();
    return (m.a == 1.0f) && ! m.rgb().isZero();
}


bool UberBSDF::hasGlossy() const {
    float avg = m_specular.mean().a;
    return (avg > 0) && (avg < 1) && ! m_specular.max().rgb().isZero();
}


bool UberBSDF::hasLambertian() const {
    return ! m_lambertian.max().rgb().isZero();
}

}
