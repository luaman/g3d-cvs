/**
 @file   UberBSDF.cpp
 @author Morgan McGuire, morgan@cs.williams.edu
 @date   2008-08-10
*/
#include "GLG3D/UberBSDF.h"

namespace G3D {

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

    if ((shininess > 0.0f) && (shininess < 1.0f)) {
        // Glossy
        // Half-vector
        const Vector3& w_h = (w_i + w_o).direction();
        const float cos_h = max(0.0f, w_h.dot(n));

        shininess *= SHININESS_MIRROR;
        result += computeF(specular.rgb(), cos_i) * 
            (pow(cos_h, shininess) *
             (shininess + 8.0f) * INV_8PI);
    }
    
    return Color4(result * power_i * cos_i, diffuse.a);
}


bool UberBSDF::scatter
(const Vector3& n,
 const Vector2& texCoord,
 const Vector3& w_i,
 const Color3&  power_i,
 float          eta_other,
 Vector3&       w_o,
 Color3&        power_o,
 Random&        random) const {

     // Choose a random number on [0, 1], then reduce it by each kind of
     // scattering's probablity until it becomes negative (i.e., scatters).
    float r = random.uniform();

    ///////////////////////////////////////////////////////////////////////////////////
    if (m_lambertian.notZero()) {
        // Sample the diffuse coefficients
        const Color4& diffuse = m_lambertian.sample(texCoord);
        
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
            
            return true;
        }
    }

    Color3 F(Color3::black());

    ///////////////////////////////////////////////////////////////////////////////////
    if (m_specular.notZero()) {

        // Sample the specular coefficients
        const Color4& specular = m_specular.sample(texCoord);

        // On the range [0, 1]
        float shininess = specular.a;

        if (shininess > 0.0f) {
            // There is some specularity

            // Cosine of the angle of incidence, for computing F
            const float cos_i = max(0.0f, w_i.dot(n));
            F = computeF(specular.rgb(), cos_i);

            const Color3& p_specular = F;
            const float p_specularAvg = p_specular.average();

            r -= p_specularAvg;
            if (r < 0.0f) {
                if (shininess < 1.0f) {
                    // Glossy                    
                    shininess = unpackGlossyExponent(shininess);

                    const Vector3& w_h = (w_i + w_o).direction();

                    // TODO: glossy scatter
                    w_o = w_i;  // TODO: random distribution about half-vector
                    power_o = p_specular * power_i * (1.0f / p_specularAvg);
                    
                } else {
                    // Mirror

                    w_o = w_i.reflectAbout(n);
                    power_o = p_specular * power_i * (1.0f / p_specularAvg);
                }
                return true;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////
    if (m_transmissive.notZero()) {
        // Sample transmissive
        const Color4& transmit    = m_transmissive.sample(texCoord);
        const Color3& T0          = transmit.rgb();
        
        const Color3& F_t         = (Color3::one() - F);
        const Color3& p_transmit  = F_t * T0;
        const float p_transmitAvg = p_transmit.average();
        
        r -= p_transmitAvg;
        if (r < 0.0f) {

            const float eta = transmit.a;

            // TODO: eta and eta_other might need to be swapped, depending on the direction
            // of the incident photon relative to the normal.

            power_o = p_transmit * power_i * (1.0f / p_transmitAvg);
            w_o = (-w_i).refractionDirection(n, eta, eta_other);

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


bool UberBSDF::isZero() const {
    return 
        m_lambertian.isBlack() &&
        m_transmissive.isZero() &&
        m_specular.isZero();
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
