/**
 @file   SuperBSDF.h
 @author Morgan McGuire, http://graphics.cs.williams.edu
 @date   2008-08-10
*/
#ifndef G3D_SuperBSDF_h
#define G3D_SuperBSDF_h

#include "GLG3D/Component.h"
#include "G3D/SmallArray.h"

namespace G3D {

/**
   Description of how a surface reflects light (photons).

   \beta

   This is an analytic energy-conserving Bidirectional Scattering
   Distribution Function (BSDF) with phenomenonlogically meaningful
   parameters. It comprises Lambertian reflection, Schlick's Fresnel
   approximation for glossy and mirror reflection, Sloan, Hoffman, and Lafortune's
   normalization of the Blinn-Phong specular lobe, and transmission
   (without exponential extinction) terms.  It is a extension of the isotropic
   version of Ashikhmin and Shirley's empirical BRDF http://www.cs.utah.edu/~shirley/papers/jgtbrdf.pdf .

   The methods of this class are primarily used for photon mapping,
   ray tracing, and software rasterization.  The G3D::Material class
   manages BSDFs for GPU rasterization.

   A surface is the 2D boundary between two 3D volumes.  BSDF works with single-sided surfaces,
   so it is assumed that for transparent materials there are <i>two</i> oppositely-oriented 
   surfaces, typically with different BSDFs, at every such boundary.  Thus there 
   are two indices of refraction at a surface: one for the inside (side opposite the normal)
   and one for the outside.

   The major routines are:

   <table border=0>
   <tr><td width=20></td><td width=100><b>scatter()</b></td><td>\copybrief scatter()</td></tr>
   <tr><td></td><td><b>shadeDirect()</b></td><td>\copybrief shadeDirect()</td></tr>
   <tr><td></td><td ><b>evaluate()</b></td><td>\copybrief evaluate()</td></tr>
   </table>

   The material is parameterized by:

   <table border=0>
   <tr valign=top><td width=20></td><td width=100>\f$\rho_{L0}\f$</td><td>Peak Lambertian ("diffuse surface color") reflection, on [0, 1]. The actual
        reflectance applied at normal incidence is \f$(1 - F_0) * \rho_{L0}\f$</td></tr>
   </td></tr>
   <tr valign=top><td></td><td>\f$T_0\f$</td><td>transmission modulation factor ("transparent color") on [0, 1]; 
     0 for opaque surfaces. The actual transmission at normal incidence will be 
     \f$(1 - F_0) * T_0\f$</td></tr>
   <tr valign=top><td></td><td>\f$F_0\f$</td><td>Fresnel reflection at normal incidence ("specular/reflection color") on [0, 1]</td></tr>
   <tr valign=top><td></td><td>\f$\sigma\f$</td><td>0 for purely lambertian surfaces, 
     packedSpecularMirror() for perfect reflection, and values between packSpecularExponent(1) and packSpecularExponent(128)
     for glossy reflection.  This is the exponent on the normalized Blinn-Phong lobe.</td></tr>
    <tr valign=top><td></td><td>\f$\eta_i\f$</td><td>Index of refraction outside the material, i.e., on the same side as the normal (only used for surfaces with \f$\rho_t > 0\f$; 
      for computing refraction angle, not used for Fresnel factor).</td></tr>
    <tr valign=top><td></td><td>\f$\eta_o\f$</td><td>Index of refraction inside the material, i.e., opposite the normal (only used for surfaces with \f$\rho_t > 0\f$; 
      for computing refraction angle, not used for Fresnel factor).</td></tr>
    </table>
 
   For energy conservation, choose \f$F_0 + (1 - F_0)T_0\leq 1\f$ and \f$\rho_{L0} + T_0 \leq 1\f$.

   The following terminology for photon scattering is used in the 
   G3D::Material::Settings and G3D::SuperBSDF classes and 
   their documentation:
   \image html scatter-terms.png

   (Departures from theory for artistic control: The direct shader always applies a glossy highlight with an exponent of 128 to mirror surfaces so that
   light sources produce highlights.  Setting the Glossy/Mirror coefficient to zero for a transmissive surface
   guarantees no reflection, although real transmissive surfaces should always be reflective at glancing angles.)

   The BSDF consists of four terms (at most three of which are
   non-zero): Lambertian, Glossy, Mirror, and Transmissive,

   \f[
   f(\vec{\omega}_i, \vec{\omega}_o) = f_L + f_g + f_m + f_t
   \f]

   where

   \f{eqnarray}
     \nonumber f_L &=& \frac{F_L(\vec{\omega}_i)}{\pi} \rho_{L0}\\
     \nonumber f_g &=& \left\{\begin{array}{ccc}
\frac{s + 8}{8 \pi} F_r(\vec{\omega}_i) \max(0, \vec{n} \cdot \vec{\omega}_h)^{s} && \mbox{\texttt{packSpecularExponent}}(0) < \sigma <  \mbox{\texttt{packedSpecularMirror()}}\\
 \\
 0~\mbox{sr}^{-1} & &  \mbox{otherwise}  \\
\end{array}\right.\\
    \nonumber f_m &=& \left\{\begin{array}{ccc}
F_r(\vec{\omega}_i) ~ \delta(\vec{\omega}_o, \vec{\omega}_m) ~/ ~(\vec{\omega}_i \cdot \vec{n}) &&  \sigma =  \mbox{\texttt{packedSpecularMirror()}}\\
 \\
 0~\mbox{sr}^{-1} & &  \mbox{otherwise}  \\
\end{array}\right.\\
     \nonumber f_t &=& F_t(\vec{\omega}_i) ~ T_0 ~ \delta(\vec{\omega}_o, \vec{\omega}_t) ~ / ~(\vec{\omega}_i \cdot \vec{n})
   \f}

   All vectors point outward from the surface. Let

   \f{eqnarray}
       \nonumber \vec{\omega}_h &=& \langle \vec{\omega}_i + \vec{\omega}_o \rangle\\
       \nonumber s &=& \mbox{\texttt{unpackSpecularExponent}}(\sigma)\\
       \nonumber \vec{\omega}_m &=& 2 (\vec{\omega}_i \cdot \vec{n}) \vec{n} - \vec{\omega}_i\\
       \nonumber \vec{\omega}_t &=& -\frac{\eta_i}{\eta_t}(\vec{\omega}_i - (\vec{\omega}_i \cdot \vec{n}) \vec{n}) - \vec{n} \sqrt{1-\left( \frac{\eta_i}{\eta_t} \right)^2(1 - \vec{\omega}_i \cdot \vec{n})^2}\\
       \nonumber F_t(\vec{\omega}_i) &=& 1 - F_r(\vec{\omega}_i)\\
       \nonumber F_L(\vec{\omega}_i) &=& 1 - F_r(\vec{\omega}_i)\\
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

  @sa G3D::Material, G3D::Component, G3D::BumpMap, G3D::GMaterial, G3D::Texture
*/
class SuperBSDF : public ReferenceCountedObject {
public:

    /** Reference counted pointer alias.*/
    typedef ReferenceCountedPointer<SuperBSDF> Ref;

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

    /** \f$\eta_t\f$ For the material on the inside.*/
    float               m_eta_t;

    /** \f$\kappa_t\f$ Extinction coefficient for the material on the inside;
        complex part of the index of refraction.
        http://en.wikipedia.org/wiki/Complex_index_of_refraction#Dispersion_and_absorption*/
    Color3              m_extinction_t;

    /** \f$\eta_r\f$ For the material on the outside.*/
    float               m_eta_r;

    Color3              m_extinction_r;

    inline SuperBSDF() : 
        m_lambertian(Color4(Color3::white() * 0.85f, 1.0f)), 
        m_eta_t(1.0f), 
        m_extinction_t(Color3::zero()),
        m_eta_r(1.0f), 
        m_extinction_r(Color3::zero()){}

    /**
       \param g Glossy exponent
       \param n Surface normal (world space)

       \return the intensity scale (nominally 1, but may be adjusted to take into account non-ideal importance sampling)
    */
    float glossyScatter
    (const Vector3& w_i,
     float          g,
     const Vector3& n,
     G3D::Random&   r,
     Vector3&       w_o) const;

public: 
    static float ignoreFloat;

    static Ref create
    (const Component4& lambertian,
     const Component4& glossy,
     const Component3& transmissive,
     float             eta_transmit = 1.0f,
     const Color3&     extinction_transmit = Color3::zero(),
     float             eta_reflect = 1.0f,
     const Color3&     extinction_reflect = Color3::zero());

    /** Computes F_r, given the cosine of the angle of incidence and 
       the reflectance at normal incidence. */
    inline Color3 computeF(const Color3& F0, float cos_i) const {
        return F0 + (Color3::white() - F0) * pow5(1.0f - cos_i);
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

    /** \f$\eta_t\f$ for the material on the inside of this object (i.e. side opposite the normal).*/
    inline float etaTransmit() const {
        return m_eta_t;
    }


    /** \f$\kappa_t\f$ Extinction coefficient for the material on the inside;
        complex part of the index of refraction.
        http://en.wikipedia.org/wiki/Complex_index_of_refraction#Dispersion_and_absorption*/
    inline const Color3& extinctionTransmit() const {
        return m_extinction_t;
    }

    /** \f$\eta_r\f$ for the material on the outside of this object (i.e. side of the normal).*/
    inline float etaReflect() const {
        return m_eta_r;
    }

    /** \f$\kappa_r\f$ Extinction coefficient for the material on the outside;
        complex part of the index of refraction.
        http://en.wikipedia.org/wiki/Complex_index_of_refraction#Dispersion_and_absorption*/
    inline const Color3& extinctionReflect() const {
        return m_extinction_r;
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
        \f$(f_L + f_g)\f$. 

        Used for direct illumination.  Ignores delta functions because
        for a random pair of directions, there is zero probability of
        sampling the delta function at a non-zero location; the infinite
        result would not be useful anyway.

        @param n \f$\vec{n}\f$ surface normal. 

        @param w_L \f$\vec{\omega}_L = \vec{\omega}_i\f$ unit vector pointing back
        towards where the photon came from (typically, the light)

        @param w_eye \f$\vec{\omega}_{eye} = \vec{\omega}_o\f$ unit vector pointing forward
        towards where the photon is going (typically, the viewer)

        @param texCoord Texture coordinate on the surface at which to
        sample from.

        @param radiance_L Incident power ("light color") along @a w_L

        @return Resulting radiance, with the alpha channel copied from
        the coverage mask.  Factors the geometric w_L dot n term in.
        Unmultipled alpha.

      @beta
    */
    virtual Color4 evaluate
    (const Vector3& n,
     const Vector2& texCoord,
     const Vector3& w_L,
     const Color3&  radiance_i,
     const Vector3& w_eye) const;

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
\int_\cap \frac{1}{\pi} \rho_{L0} F_{L}(\vec{\omega}_i) (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i =
\rho_{L0} F_{L}(\vec{\omega}_i) \\
       \nonumber\rho_g &=& \int_\cap f_g (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i = 
\int_\cap \frac{s + 8}{8 \pi} F_r(\vec{\omega}_i)\max(0, \vec{n} \cdot \vec{\omega}_h)^{s} (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i = F_r(\vec{\omega}_i)\\
       \nonumber\rho_m &=& \int_\cap f_m (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i = 
\int_\cap F_r(\vec{\omega}_i) \delta(\vec{\omega}_o, \vec{\omega}_m) / (\vec{\omega}_i \cdot \vec{n}) (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i = F_r(\vec{\omega}_i)\\
       \nonumber\rho_L &=& \int_\cup f_t (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i = 
\int_\cup F_t(\vec{\omega}_i) T_0 \delta(\vec{\omega}_o, \vec{\omega}_t) / (\vec{\omega}_i \cdot \vec{n}) (\vec{\omega}_i \cdot \vec{n}) d\vec{\omega}_i = F_t(\vec{\omega}_i) T_0
       \f}

       Note that at most one of the glossy and mirror probabilities may be non-zero.

       Not threadsafe unless \link setStorage() setStorage\endlink(<code>COPY_TO_CPU</code>) has been called first.

       \param lowFreq If true, sample from the average texture color instead of at each texel.  This can
       improve performance by increasing memory coherence.

       \param eta_other Index of refraction on the side of the normal (i.e., material that is being exited to enter the 
         object whose surface this BSDF describes)

       @beta

       @return false if the photon was absorbed, true if it scatters. */
    virtual bool scatter
    (const Vector3& n,
     const Vector2& texCoord,
     const Vector3& w_i,
     const Color3&  power_i,
     Vector3&       w_o,
     Color3&        power_o,
     float&         eta_o,
     Color3&        extinction_o,
     Random&        r = Random::common(),
     bool           lowFreq = false,
     float&         density = ignoreFloat) const;


    /** Infinite peak in the BSDF.  For use with getImpulses.*/
    class Impulse {
    public:
        Vector3   w;

        /** \f$ f(\vec{\omega}_i, \vec{\omega}_o)
            \mbox{max}(\vec{\omega}_i \cdot \vec{n}, 0) /
            \delta(\vec{\omega}_o, \vec{\omega}_o) \f$ for the
            impulse; the integral of the BSDF over a small area.  This
            is the factor to multiply scattered illumination by.  

            For backwards recursive ray tracing, this is the
            coefficient on the recursive path's radiance. Do not
            multiply this by a cosine factor; that has already been
            factored in.*/
        Color3    coefficient;

        float     eta;

        Color3    extinction;
    };

    /** 
        \brief Get the infinite peaks of the BSDF (usually refraction
        and mirror reflection).

        Used for Whitted backwards ray tracing with a small number of
        samples, where w_i = w_eye.  Distribution (stochastic) ray
        tracers should use the scatter() method instead.

        \param lowFreq If true, sample from the average texture color
        instead of at each texel.  This can improve performance by
        increasing memory coherence.

        \param impulseArray Impulses are appended to this (it is not
        cleared first)
        

     */
    virtual void getImpulses
    (const Vector3&  n,
     const Vector2&  texCoord,
     const Vector3&  w_i,
     SmallArray<Impulse, 3>& impulseArray,
     bool            lowFreq = false) const;

    /** Convenience overload */
    virtual void getImpulses
    (const Vector3&  n,
     const Vector2&  texCoord,
     const Vector3&  w_i,
     Array<Impulse>& impulseArray,
     bool            lowFreq = false) const;


    /** True if this absorbs all light */
    inline bool isZero() const {
        return m_lambertian.isBlack() && 
               m_specular.isBlack() &&
               m_transmissive.isBlack();
    }

    /** Returns true if both have the same Component::Factors for each component. */
    bool similarTo(const SuperBSDF::Ref& other) const;

    /** The glossy exponent is packed so that 0 = no specular, 
        1 = mirror (infinity), and on the open interval \f$e \in (0, 1), ~ e \rightarrow 1024 e^2 + 1\f$.
        This function abstracts the unpacking, since it may change in future versions.
        
        Because direct shading is specified for SuperBSDF to apply a
        glossy reflection to mirror surfaces, e = 1 produces 1025 as
        well.
        */
    static inline float unpackSpecularExponent(float e) {
        return square((clamp(e, 0.0f, 1.0f) * 255.0f - 1.0f) * (1.0f /253.0f)) * 1024.0f + 1.0f;
    }

    /** The value that a specular mirror is packed as */
    inline static float packedSpecularMirror() {
        return 1.0f;
    }

    /** The value that a non-specular surface is packed as */
    inline static float packedSpecularNone() {
        return 0.0f;
    }

    /** Packing is \f$\frac{ \sqrt{ \frac{x - 1}{1024} } * 253 + 1}{255} \f$ */
    inline static float packSpecularExponent(float x) {
        debugAssert(x > 0);
        // Never let the exponent go above the max representable non-mirror value in a uint8
        return (clamp((float)(sqrt((x - 1.0f) * (1.0f / 1024.0f))), 0.0f, 1.0f) * 253.0f + 1.0f) * (1.0f / 255.0f);
    }
};

}

#endif

