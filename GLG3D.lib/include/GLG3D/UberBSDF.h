/**
 @file   UberBSDF.h
 @author Morgan McGuire, morgan@cs.williams.edu
 @date   2008-08-10
*/
#ifndef G3D_BSDF_h
#define G3D_BSDF_h

#include "GLG3D/Component.h"

namespace G3D {

/**
   Description of how a surface reflects light (photons).

   This is an analytic energy-conserving Bidirectional Scattering
   Distribution Function (BSDF) with phenomenonlogically meaningful
   parameters. It comprises Lambertian reflection, Schlick's Fresnel
   approximation for glossy and mirror reflection, Sloan, Hoffman, and Lafortune's
   normalization of the Blinn-Phong specular lobe, and transmission
   (without extinction) terms.

   The methods of this class are primarily used for photon mapping,
   ray tracing, and software rasterization.  The G3D::Material class
   manages BSDFs for GPU rasterization.

   The major routines are:

   <table border=0>
   <tr><td width=20></td><td width=100><b>scatter()</b></td><td>\copybrief scatter()</td></tr>
   <tr><td></td><td><b>shadeDirect()</b></td><td>\copybrief shadeDirect()</td></tr>
   <tr><td></td><td ><b>evaluate()</b></td><td>\copybrief evaluate()</td></tr>
   </table>

   The material is parameterized by:

   <table border=0>
   <tr valign=top><td width=20></td><td width=100>\f$\rho_L\f$</td><td>Lambertian ("surface color") reflection on [0, 1]</td></tr>
   <tr valign=top><td></td><td>\f$T_0\f$</td><td>transmission modulation factor ("transparent color") on [0, 1]; 
     0 for opaque surfaces. The actual transmission at normal incidence will be 
     \f$(1 - F_0) * T_0\f$</td></tr>
   <tr valign=top><td></td><td>\f$F_0\f$</td><td>Fresnel reflection at normal incidence ("specular/reflection color") on [0, 1]</td></tr>
   <tr valign=top><td></td><td>\f$s\f$</td><td><code>SHININESS_NONE</code> for purely lambertian surfaces, 
     <code>SHININESS_MIRROR</code> for perfect reflection, and values between 1 and 128
     for glossy reflection.  This is the exponent on the normalized Blinn-Phong lobe.</td></tr>
    <tr valign=top><td></td><td>\f$\eta\f$</td><td>Index of refraction (only used for surfaces with \f$\rho_t > 0\f$; 
      for computing refraction angle, not used for Fresnel factor).</td></tr>
    </table>
 
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
       - a = \f$s\f$ : shininess ("specular exponent"), packed.
    */
    inline const Component4& specular() const {
        return m_specular;
    }

    /** @brief Evaluate the diffuse (finite) portion of the BSDF: 
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
    virtual Color4 shadeDirect
    (const Vector3& n,
     const Vector2& texCoord,
     const Vector3& w_i,
     const Color3&  power_i,
     const Vector3& w_o) const;

    /** \brief Move or copy data to CPU or GPU.  
        Called from G3DMaterial::setStorage(). */
    virtual void setStorage(ImageStorage s) const;

    /** \brief Return true if there is any glossy (non-Lambertian, non-mirror) 
        reflection from this BSDF. */
    bool hasGlossy() const;

    /** \brief Return true if there is any mirror reflection from this BSDF. */
    bool hasMirror() const;

    /** \brief Return true if there is any Lambertian reflection from this BSDF. */
    bool hasLambertian() const;

    /** \brief Return true if there is any Lambertian, mirror, or glossy reflection from this BSDF (not just mirror!)*/
    inline bool hasReflection() const {
        return ! m_lambertian.isBlack() ||
               ! m_specular.isBlack();
    }

    /**
       \brief Sample outgoing photon direction \f$\vec{\omega}_o\f$ from the 
       distribution \f$f(\vec{\omega}_i, \vec{\omega}_o)\cos \theta_i\f$.

       Used in forward photon tracing.  The extra cosine term handles the 
       projected area effect.
       
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

       Not threadsafe unless \link setStorage() setStorage\endlink(<code>COPY_TO_CPU</code>) has been called first.

       \param lowFreq If true, sample from the average texture color instead of at each texel.  This can
       improve performance by increasing memory coherence.

       @beta

       @return false if the photon was absorbed, true if it scatters. */
    virtual bool scatter
    (const Vector3& n,
     const Vector2& texCoord,
     const Vector3& w_i,
     const Color3&  power_i,
     float          eta_other,
     Vector3&       w_o,
     Color3&        power_o,
     Random&        r = Random::common(),
     bool           lowFreq = false) const;

    /** True if this absorbs all light */
    inline bool isZero() const {
        return m_lambertian.isBlack() && 
               m_specular.isBlack() &&
               m_transmissive.isBlack();
    }

    /** Returns true if both have the same Component::Factors for each component. */
    bool similarTo(const UberBSDF::Ref& other) const;


    /** The glossy exponent is packed so that 0 = no specular, 
        1 = mirror (infinity), and on the open interval \f$e \in (0, 1), ~ e \rightarrow 127e + 1\f$. 
        This function abstracts the unpacking, since it may change in future versions.*/
    static inline int unpackSpecularExponent(float e) {
        return iRound(e * 127.0f) + 1;
    }

    /** The value that a specular mirror is packed as */
    inline static float packedSpecularMirror() {
        return 1.0f;
    }

    /** The value that a non-specular surface is packed as */
    inline static float packedSpecularNone() {
        return 0.0f;
    }

    inline static float packSpecularExponent(int x) {
        debugAssert(x > 0 && x < 129);
        return (x - 1) / 127.0f;
    }
};

}

#endif

