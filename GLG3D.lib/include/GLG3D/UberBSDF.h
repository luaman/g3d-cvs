/**
 @file   UberBSDF.h
 @author Morgan McGuire, morgan@cs.williams.edu
 @date   2008-08-10
*/
#ifndef G3D_BSDF_h
#define G3D_BSDF_h

#include "GLG3D/Component.h"

namespace G3D {

/** Constants for use with shininess for G3D::UberBSDF and G3D::Material. */
enum {
    /** No specular term; purely diffuse (may be transmissive as well) */
    SHININESS_NONE = 0, 

    /** Mirror reflection. */
    SHININESS_MIRROR = 129 
};

/**
   Description of how a surface reflects light (photons).

   This is an analytic energy-conserving Bidirectional Scattering
   Distribution Function (BSDF) with phenomenonlogically meaningful
   parameters. It comprises Lambertian reflection, Schlick's Fresnel
   approximation for glossy and mirror reflection, Sloan and Hoffman's
   normalization of the Blinn-Phong specular lobe, and transmission
   (without extinction coefficients) terms.

   The material is parameterized by:

  - \f$\rho_L\f$ : lambertian ("diffuse color") reflection on [0, 1]
  - \f$T_0\f$ : transmission modulation factor ("transparent color") on [0, 1]; 
     0 for opaque surfaces. The actual transmission at normal incidence will be 
     \f$(1 - F_0) * T_0\f$.
  - \f$F_0\f$ : Fresnel reflection at normal incidence ("specular/reflection color") on [0, 1]
  - \f$s\f$ : <code>SHININESS_NONE</code> for purely lambertian surfaces, 
     <code>SHININESS_MIRROR</code> for perfect reflection, and values between 0 and 128
     for glossy reflection.  This is the exponent on the normalized Blinn-Phong lobe.
  - \f$\eta\f$ : Index of refraction (only used for surfaces with \f$\rho_t > 0\f$; 
      for computing refraction angle, not used for Fresnel factor).
 
   For energy conservation, \f$\rho_L + F_0 + (1 - F_0)T_0\leq 1\f$.

   The BSDF consists of four terms (at most three of which are
   non-zero): Lambertian, Glossy, Mirror, and Transmissive,

   \f[
   f(\vec{\omega}_i, \vec{\omega}_o) = f_L + f_g + f_m + f_t
   \f]

   where

   \f{eqnarray}
     \nonumber f_L &=& \frac{1}{\pi} \rho_L\\
     \nonumber f_g &=& \left\{\begin{array}{ccc}
\frac{s + 8}{8 \pi} F_r(\vec{\omega}_i) \max(0, \vec{n} \cdot \vec{\omega}_h)^{s} && \mbox{\texttt{SHININESS\_NONE}} < s <  \mbox{\texttt{SHININESS\_MIRROR}}\\
 \\
 0~\mbox{sr}^{-1} & &  \mbox{otherwise}  \\
\end{array}\right.\\
    \nonumber f_m &=& \left\{\begin{array}{ccc}
F_r(\vec{\omega}_i) ~ \delta(\vec{\omega}_o, \vec{\omega}_m) ~/ ~(\vec{\omega}_i \cdot \vec{n}) &&  s =  \mbox{\texttt{SHININESS\_MIRROR}}\\
 \\
 0~\mbox{sr}^{-1} & &  \mbox{otherwise}  \\
\end{array}\right.\\
     \nonumber f_t &=& F_t(\vec{\omega}_i) ~ T_0 ~ \delta(\vec{\omega}_o, \vec{\omega}_t) ~ / ~(\vec{\omega}_i \cdot \vec{n})
   \f}

   All vectors point outward from the surface. Let

   \f{eqnarray}
       \nonumber \vec{\omega}_h &=& \langle \vec{\omega}_i + \vec{\omega}_o \rangle\\
       \nonumber \vec{\omega}_m &=& 2 (\vec{\omega}_i \cdot \vec{n}) \vec{n} - \vec{\omega}_i\\
       \nonumber \vec{\omega}_t &=& -\frac{\eta_i}{\eta_t}(\vec{\omega}_i - (\vec{\omega}_i \cdot \vec{n}) \vec{n}) - \vec{n} \sqrt{1-\left( \frac{\eta_i}{\eta_t} \right)^2(1 - \vec{\omega}_i \cdot \vec{n})^2}\\
       \nonumber F_t(\vec{\omega}_i) &=& 1 - F_r(\vec{\omega}_i)\\
       \nonumber F_r(\vec{\omega}_i) &=& F_0 + (1 - F_0) (1 - \max(0, \vec{\omega}_i \cdot \vec{n}))^5
   \f}

  \f$F_r(\vec{\omega}_i)\f$ is the Fresnel mirror reflection coefficient, which is 
  approximated by Schlick's method as shown above.

  The \f$T_0\f$ factor is the only significant source of error in the
  BSDF. An accurate scatting function would transmit with probabilty
  \f$F_t\f$ and then attenuate the scattered photon based on the
  distance traveled through the translucent medium.  The concession to
  applying a constant attenuation is a typical one in rendering,
  however.  

  The following terminology for photon scattering is used in the 
  G3D::Material::Settings and G3D::UberBSDF classes and 
  their documentation:
  \image html scatter-terms.png

  @sa G3D::Material, G3D::Component, G3D::BumpMap, G3D::GMaterial, G3D::Texture
*/
class UberBSDF : public ReferenceCountedObject {
public:

    /** Reference counted pointer alias.*/
    typedef ReferenceCountedPointer<UberBSDF> Ref;

protected:

    /** @brief Packed factors affecting the lambertian term.

        - rgb = \f$\rho_L\f$ : lambertian scattering probability
        - a = coverage mask (mainly useful only for maps, not constants).
    */
    Component4          m_lambertian;

    /**
       @brief Packed factors affecting mirror and glossy reflection.

       - rgb = \f$F_0\f$ : glossy scattering probability/Fresnel
         reflectance at normal incidence. This is dependent on eta,
         although the interface allows them to be set independently.
       - a = \f$s/129\f$ : shininess (specular exponent) divided by 129.
    */
    Component4          m_specular;

    /** \f$T_0\f$ : transmissivity */
    Component3          m_transmissive;

    /** \f$\eta\f$ */
    float               m_eta;

    /** Reserved for future use */
    Color3              m_extinction;

    inline UberBSDF() : 
        m_lambertian(Color4(Color3::white() * 0.85f, 1.0f)), 
        m_eta(1.0f), 
        m_extinction(Color3::zero()) {}

public: 

    static Ref create
    (const Component4& lambertian,
     const Component4& glossy,
     const Component3& transmissive,
     float             eta,
     const Color3&     extinction = Color3::zero());


    /** Returns x<sup>5</sup> */
    static inline float pow5(float x) {
        const float y = x * x;
        return y * y * x;
    }


    /** Computes F_r given the cosine of the angle of incidence */
    inline Color3 computeF(const Color3& F0, float cos_i) const {
        return Color3::white().lerp(F0, pow5(cos_i));
    }

    /** @brief Packed factors affecting the lambertian term.

        - rgb = \f$\rho_L\f$ : lambertian scattering probability
        - a = coverage mask (mainly useful only for maps, not constants).
    */
    inline const Component4& lambertian() const {
        return m_lambertian;
    }

    /** \f$T_0\f$ : transmissivity */
    inline const Component3& transmissive() const {
        return m_transmissive;
    }

    /** \f$\eta\f$ */
    inline float eta() const {
        return m_eta;
    }

    /**
       @brief Packed factors affecting mirror and glossy reflection.

       - rgb = \f$F_0\f$ : specular scattering probability/Fresnel
         reflectance at normal incidence. This is dependent on eta,
         although the interface allows them to be set independently.
       - a = \f$s\f$ : shininess ("specular exponent")
    */
    inline const Component4& specular() const {
        return m_specular;
    }

    /** @brief Evaluate the finite portion of the BSDF: 
        \f$(f_L + f_g) \max(0, \vec{\omega}_i \cdot \vec{n})\f$. 

        Used for direct illumination.  Ignores delta functions because
        for a random pair of directions, there is zero probability of
        sampling the delta function at a non-zero location; the infinite
        result would not be useful anyway.

        @param n \f$\vec{n}\f$ surface normal. 

        @param w_i \f$\vec{\omega}_i\f$ unit vector pointing back
        towards where the photon came from (typically, the light)

        @param w_o \f$\vec{\omega}_o\f$ unit vector pointing forward
        towards where the photon is going (typically, the viewer)

        @param texCoord Texture coordinate on the surface at which to
        sample from.

        @param power_i Incident power ("light color") along @a w_i

        @return Resulting power, with the alpha channel copied from
        the coverage mask.  Unmultipled alpha.

      @beta
    */
    Color4 shadeDirect
    (const Vector3& n,
     const Vector2& texCoord,
     const Vector3& w_i,
     const Color3&  power_i,
     const Vector3& w_o) const;

    /** Move or copy data to CPU or GPU.  Called from setStorage(). */
    void setStorage(ImageStorage s) const;

    /** Return true if there is any glossy (non-Lambertian, non-mirror) 
        reflection from this BSDF. */
    bool hasGlossy() const;

    /** Return true if there is any mirror reflection from this BSDF. */
    bool hasMirror() const;

    /** Return true if there is any Lambertian reflection from this BSDF. */
    bool hasLambertian() const;

    /**
       Used in forward photon tracing.
       
       The probability of different kinds of scattering are given by:

       \f{eqnarray}
       \nonumber\rho_L &=& \int_\cap f_L (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i = 
\int_\cap \frac{1}{\pi} \rho_L (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i =
\rho_L \\
       \nonumber\rho_g &=& \int_\cap f_g (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i = 
\int_\cap \frac{s + 8}{8 \pi} F_r \max(0, \vec{n} \cdot \vec{\omega}_h)^{s} (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i = F_r(\vec{\omega}_i)\\
       \nonumber\rho_m &=& \int_\cap f_m (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i = 
\int_\cap F_r(\vec{\omega}_i) \delta(\vec{\omega}_o, \vec{\omega}_m) / (\vec{\omega}_i \cdot \vec{n}) (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i = F_r(\vec{\omega}_i)\\
       \nonumber\rho_L &=& \int_\cup f_t (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i = 
\int_\cup F_t(\vec{\omega}_i) T_0 \delta(\vec{\omega}_o, \vec{\omega}_t) / (\vec{\omega}_i \cdot \vec{n}) (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i = F_t(\vec{\omega}_i) T_0
       \f}

       Note that at most one of the glossy and mirror probabilities may be non-zero.

       Not threadsafe unless setStorage(COPY_TO_CPU) has been called first.

       <b>(Not currently implemented)</b>
       @beta

       @return false if the photon was absorbed, true if it scatters. */
    bool scatter
    (const Vector3& n,
     const Vector2& texCoord,
     const Vector3& w_i,
     const Color3&  power_i,
     float          eta_other,
     Vector3&       w_o,
     Color3&        power_o,
     Random&        r = Random::common()) const;

    /** True if this absorbs all light */
    bool isZero() const;

    /** The glossy exponent is packed so that 0 = no specular, 
        1 = mirror (infinity), and on the open interval \f$e \in (0, 1), ~ e \rightarrow 127e + 1\f$. 
        This function abstractsthe unpacking, since it may change in future versions.*/
    static inline int unpackGlossyExponent(float e) {
        return iRound(e * 127.0f) + 1;
    }

    /** Returns true if both have the same Component::Factors for each component. */
    bool similarTo(const UberBSDF::Ref& other) const;
};

}

#endif

