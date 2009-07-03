/**
 @file   SuperBSDF.cpp
 @author Morgan McGuire, morgan@cs.williams.edu
 @date   2008-08-10
*/
#include "GLG3D/SuperBSDF.h"

namespace G3D {

float SuperBSDF::ignoreFloat;

#define INV_PI  (0.318309886f)
#define INV_8PI (0.0397887358f)

SuperBSDF::Ref SuperBSDF::create
(const Component4& lambertian,
 const Component4& specular,
 const Component3& transmissive,
 float             eta_t,
 const Color3&     extinction_t,
 float             eta_r,
 const Color3&     extinction_r) {

    SuperBSDF::Ref bsdf      = new SuperBSDF();

    bsdf->m_lambertian      = lambertian;
    bsdf->m_specular        = specular;
    bsdf->m_transmissive    = transmissive;
    bsdf->m_eta_t           = eta_t;
    bsdf->m_extinction_t    = extinction_t;
    bsdf->m_eta_r           = eta_r;
    bsdf->m_extinction_r    = extinction_r;

    return bsdf;
}


Color4 SuperBSDF::shadeDirect
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
             (e + 8.0f) * INV_8PI);
    }
    
    return Color4(result * power_i * cos_i, diffuse.a);
}


void SuperBSDF::getImpulses
(const Vector3&  n,
 const Vector2&  texCoord,
 const Vector3&  w_i,
 Array<Impulse>& impulseArray,
 bool            lowFreq) const {

    Color3 F(0,0,0);
    bool Finit = false;

    ////////////////////////////////////////////////////////////////////////////////
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
            
            if (shininess == packedSpecularMirror()) {
                // Mirror
                
                Impulse& imp     = impulseArray.next();
                imp.w            = w_i.reflectAbout(n);
                imp.coefficient  = F;
                imp.eta          = m_eta_r;
                imp.extinction   = m_extinction_r;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
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
       
        debugAssert(w_i.dot(n) > 0);
        Impulse& imp     = impulseArray.next();

        imp.coefficient  = p_transmit;
        imp.w            = (-w_i).refractionDirection(n, m_eta_t, m_eta_r);
        imp.eta          = m_eta_t;
        imp.extinction   = m_extinction_t;
        if (imp.w.isZero()) {
            // Total internal refraction
            impulseArray.popDiscard();
        }
    }
}


void SuperBSDF::glossyScatter
(const Vector3& w_i,
 float          g,
 const Vector3& n,
 G3D::Random&   r,
 Vector3&       w_o) const {

    // Notation translator from the jgt paper:
    //
    //      n_u = n_v = g
    //      k1 = w_i
    //      k2 = w_o
    //      h  = w_h

    float intensity;
    do {
        // Eq. 6 and 10 (eq. 9 cancels for isotropic)
        // Generate a cosine distributed half-vector:
        const Vector3& w_h = Vector3::cosPowHemiRandom(n, g, r);

        // Now transform to the output vector: (eq. 7)
        w_o = w_i.reflectAbout(w_h);

        // The output vector has three problems (with solutions used in
        // this implementation):
        //
        //   1. Distribution is off because measures in w_h and w_o space
        //        don't line up (Rejection sample by discrepancy)
        //
        //   2. May be below the surface of the plane (Loop until a sample
        //        is found above; the scatter function's choice of glossy
        //        scattering means that this method is conditioned on a
        //        bounce occuring).  Since when w_h = n, w_o = mirror
        //        reflection vector, there always exists some probability
        //        distribution above the plane.
        //
        //   3. Does not account for the n.dot(w_o) probability (Rejection
        //        sample by discrepancy)

        const float cos_o = w_o.dot(n);

        // Adjust for the measure difference between w_o and w_h spaces (eq. 8)
        intensity = cos_o / (4.0f * w_i.dot(w_h));
    } while (r.uniform() > intensity);
}

#if 0
    // Let phi be the angle about the normal
    const float phi = r.uniform(0, G3D::twoPi());
    const float cos_phi = cos(phi);
    const float sin_phi = sin(phi);

    // Rejection sampling of angle relative to normal
    while (true) {
        const float cos_theta = pow(r.uniform(0.0f, 1.0f), 1.0f / (g + 1.0f));
        const float sin_theta = sqrtf(1.0f - square(cos_theta));
        
        // In the reference frame of the surface
        const Vector3 h_tangentSpace(cos_phi * sin_theta, sin_phi * sin_theta, cos_theta);
        
        const Vector3& h = h_tangentSpace;//TODO;
        
        // Set the attenuation to ps from the paper, computed based on
        // the monte carlo section of the paper
        const Vector3& k1 = w_i;
        const float hdotk = h.dot(k1);
        
        if (hdotk > 0.0f) {
            // On the front side of the specular lobe; we can continue        
            Vector3 k2 = (-w_i + 2.0f * hdotk * h).direction();
        
            // Ensure that we're above the plane of the surface
            if (k2.dot(n) > 0.0f) {

                // Compute the density of the perturbed ray
                const float hdotn = n.dot(h);
                const float factor1 = (g + 1.0f) / G3D::twoPi();
                const float factor2 = pow(hdotn, g);
                
                const float inv_actual_density = (4.0 * hdotk) / (factor1 * factor2);
                
                // Compute the density of what we actually want (from the BRDF)
                float brdf;
                //AshikminShirleyAnisotropicPhongBRDF::ComputeDiffuseSpecularFactors( diffuseFactor, brdf, k2, ri, NU, NV, Rs );
                
                // Now we need to correct for the fact that we sampled against
                // the wrong distribution by a factor of N dot L.
                float specFactor = inv_actual_density * brdf;
                
                //specular.ray.Set( ri.ptIntersection, k2 );
            }
        }
    }

    return true;
}
#endif

bool SuperBSDF::scatter
(const Vector3& n,
 const Vector2& texCoord,
 const Vector3& w_i,
 const Color3&  power_i,
 Vector3&       w_o,
 Color3&        power_o,
 float&         eta_o,
 Color3&        extinction_o,
 Random&        random,
 bool           lowFreq,
 float&         density) const {

    // Choose a random number on [0, 1], then reduce it by each kind of
    // scattering's probability until it becomes negative (i.e., scatters).
    float r = random.uniform();

    ////////////////////////////////////////////////////////////////////////////////
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
            eta_o = m_eta_r;
            extinction_o = m_extinction_r;

            return true;
        }
    }

    Color3 F(0,0,0);
    bool Finit = false;

    ////////////////////////////////////////////////////////////////////////////////
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

                    glossyScatter(w_i, shininess, n, random, w_o);
                    power_o = p_specular * power_i * (1.0f / p_specularAvg);
                    density = p_specularAvg * 0.1f;

                } else {
                    // Mirror

                    w_o = w_i.reflectAbout(n);
                    power_o = p_specular * power_i * (1.0f / p_specularAvg);
                    density = p_specularAvg;
                }

                eta_o = m_eta_r;
                extinction_o = m_extinction_r;
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
            w_o = (-w_i).refractionDirection(n, m_eta_t, m_eta_r);
            density = p_transmitAvg;
            eta_o = m_eta_t;
            extinction_o = m_extinction_t;

            debugAssert(w_o.isZero() || ((w_o.dot(n) < 0) && w_o.isUnit()));

            // w_o is zero on total internal refraction
            return ! w_o.isZero();
        }
    }

    // Absorbed
    return false;
}


void SuperBSDF::setStorage(ImageStorage s) const {
    m_lambertian.setStorage(s);
    m_transmissive.setStorage(s);
    m_specular.setStorage(s);
}


bool SuperBSDF::similarTo(const SuperBSDF::Ref& other) const {
    return 
        (m_lambertian.factors()   == other->m_lambertian.factors()) &&
        (m_transmissive.factors() == other->m_transmissive.factors()) &&
        (m_specular.factors()     == other->m_specular.factors());
}


bool SuperBSDF::hasMirror() const {
    const Color4& m = m_specular.max();
    return (m.a == 1.0f) && ! m.rgb().isZero();
}


bool SuperBSDF::hasGlossy() const {
    float avg = m_specular.mean().a;
    return (avg > 0) && (avg < 1) && ! m_specular.max().rgb().isZero();
}


bool SuperBSDF::hasLambertian() const {
    return ! m_lambertian.max().rgb().isZero();
}

}
