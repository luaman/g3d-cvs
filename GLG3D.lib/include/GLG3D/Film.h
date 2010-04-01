/**
 @file Film.h
 @author Morgan McGuire, http://graphics.cs.williams.edu
 @created 2008-07-01
 @edited  2010-02-01
 */

#ifndef G3D_Film_h
#define G3D_Film_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Shader.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/Texture.h"
#include "GLG3D/GuiContainer.h"

namespace G3D {

/** \brief Tone mapping controls for simulating bloom and gamma correction.

   Computer displays are not capable of representing the range of values
   that are rendered by a physically based system.  For example, the brightest
   point on a monitor rarely has the intensity of a light bulb.  Furthermore,
   for historical (and 2D GUI rendering) reasons, monitors apply a power
   ("gamma") curve to values.  So rendering code that directly displays
   radiance values on a monitor will neither capture the desired tonal range
   nor even present the values scaled linearly.

   The Film class corrects for this using the simple tone mapping algorithm
   presented in Pharr and Humphreys 2004.

   To use, render to a G3D::Texture using G3D::Framebuffer, then pass that
   texture to exposeAndDraw() to produce the final image for print or display
   on screen. For example, on initialization, perform:

   <pre>
    film = Film::create();
    fb = Framebuffer::create("Offscreen");
    colorBuffer = Texture::createEmpty("Color", renderDevice->width(), renderDevice->height(), ImageFormat::RGB16F(), Texture::DIM_2D_NPOT, Texture::Settings::video());
    fb->set(Framebuffer::COLOR_ATTACHMENT0, colorBuffer);
    fb->set(Framebuffer::DEPTH_ATTACHMENT, 
        Texture::createEmpty("Depth", renderDevice->width(), renderDevice->height(), ImageFormat::DEPTH24(), Texture::DIM_2D_NPOT, Texture::Settings::video()));
   </pre>

   and then per frame,
   <pre>
    rd->pushState(fb);
        ...Rendering code here...
    rd->popState(); 
    film->exposeAndRender(rd, colorBuffer);
   </pre>

   The bloom effects are most pronounced when rendering values that are 
   actually proportional to radiance.  That is, if all of the values in the
   input are on a narrow range, there will be little bloom.  But if the 
   sky, highlights, emissive surfaces, and light sources are 10x brighter 
   than most scene objects, they will produce attractive glows and halos.

   When rendering multiple viewports or off-screen images, use a separate 
   Film instance for each size of input for maximum performance.

   Requires shaders.
   @beta To be merged with ToneMap
*/
class Film : public ReferenceCountedObject {
public:

    typedef ReferenceCountedPointer<class Film> Ref;

private:

    const ImageFormat* m_intermediateFormat;

    /** Working framebuffer */
    Framebuffer::Ref        m_framebuffer;
    Framebuffer::Ref        m_tempFramebuffer;
    Framebuffer::Ref        m_blurryFramebuffer;

    /** Expose, invert gamma and correct out-of-gamut colors */
    Shader::Ref             m_shader;

    /** Expose before bloom */
    Shader::Ref             m_preBloomShader;

    /** Output of blend shader/input to the vertical blur. 16-bit float.*/
    Texture::Ref            m_blended;

    /** float pre-bloom curve applied */
    Texture::Ref            m_preBloom;

    /** float blurred vertical */
    Texture::Ref            m_temp;

    /** float blurred vertical + horizontal */
    Texture::Ref            m_blurry;

    /** \brief Monitor gamma used in tone-mapping. Default is 2.0. */
    float                   m_gamma;

    /** \brief Exposure time.  If the input images to exposeAndDraw() measure radiance,
      this is in units of seconds.  Most rendering intensities are scaled by an arbitrary 
      constant, however, so the units here aren't important; larger is brighter. */
    float                   m_exposure;

    /** \brief 0 = no bloom, 1 = blurred out image. */
    float                   m_bloomStrength;

    /** \brief Bloom filter kernel radius as a fraction 
     of the larger of image width/height.*/
    float                   m_bloomRadiusFraction;

    /** Loads the shaders. Called from expose. */
    void init();

    Film(const ImageFormat* f);

public:
    
    /** \brief Create a new Film instance.
    
        @param intermediateFormat Intermediate precision to use when processing images.  Defaults to 
         16F to conserve space (and bandwidth); a float texture is used in case values are not on the range (0, 1).
         If you know that your data is on a smaller range, try ImageFormat::RGB8() or 
         ImageFormat::RGB10A2() for increased
         space savings or performance.
      */
    static Ref create(const ImageFormat* intermediateFormat = ImageFormat::RGB16F());

    /** \brief Monitor gamma used in tone-mapping. Default is 2.0. */
    inline float gamma() const {
        return m_gamma;
    }

    /** \brief Exposure time.  If the input images to exposeAndRender() measure radiance,
      this is in units of seconds.  Most rendering intensities are scaled by an arbitrary 
      constant, however, so the units here aren't importan; larger is brighter. */
    inline float exposure() const {
        return m_exposure;
    }

    /** \brief 0 = no bloom, 1 = blurred out image. */
    inline float bloomStrength() const {
        return m_bloomStrength;
    }

    /** \brief Bloom filter kernel radius as a fraction 
     of the larger of image width/height.*/
    inline float bloomRadiusFraction() const {
        return m_bloomRadiusFraction;
    }

    void setGamma(float g) {
        m_gamma = g;
    }

    void setExposure(float e) {
        m_exposure = e;
    }

    void setBloomStrength(float s) {
        m_bloomStrength = s;
    }

    void setBloomRadiusFraction(float f) {
        m_bloomRadiusFraction = f;
    }

    /** Adds controls for this Film to the specified GuiPane. */
    void makeGui(class GuiPane*, float maxExposure = 10.0f, float sliderWidth = GuiContainer::CONTROL_WIDTH, float controlIndent = 0);

    /** \brief Renders the input as filtered by the film settings to the currently bound framebuffer.
        \param downsample One side of the downsampling filter in pixels. 1 = no downsampling. 2 = 2x2 downsampling (antialiasing). Not implemented.
    */
    void exposeAndRender
        (RenderDevice* rd, 
         const Texture::Ref& input0,
         int downsample = 1);
};

} // namespace
#endif
