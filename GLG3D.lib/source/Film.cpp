/**
 @file Film.cpp
 @author Morgan McGuire, morgan@cs.williams.edu
 */

#include "GLG3D/Film.h"
#include "G3D/debugAssert.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GaussianBlur.h"
#include "GLG3D/Draw.h"

namespace G3D {

static const char* shaderCode = 
STR(
uniform sampler2D sourceTexture;
uniform sampler2D bloomTexture;
uniform float     bloomStrength;
uniform float     exposure;

// 1.0 / monitorGamma.  Usually about invGamma = 0.5
uniform float     invGamma;

void main(void) {
 
    vec3 src   = texture2D(sourceTexture, gl_TexCoord[g3d_Index(sourceTexture)].st).rgb;
    vec3 bloom = texture2D(bloomTexture, gl_TexCoord[g3d_Index(bloomTexture)].st).rgb;
        
    // Parens are to force scalar multiplies over vector ones
    src = src   * ((1.0 - bloomStrength) * exposure) + 
          bloom * (bloomStrength * exposure);

    // Fix out-of-gamut saturation
    src /= max(max(src.r, src.g), max(src.b, 1.0));
    
    // Invert the gamma curve (TODO: use a texture?)
    vec3 dst = pow(src, vec3(invGamma, invGamma, invGamma));
    
    gl_FragColor.rgb = dst;
});


Film::Film(const ImageFormat* f) :
    m_intermediateFormat(f),
    m_gamma(2.0f),
    m_exposure(1.0f),
    m_bloomStrength(0.08f),
    m_bloomRadiusFraction(0.03f) {

    init();
}


Film::Ref Film::create(const ImageFormat* f) {
    return new Film(f);
}


void Film::init() {
    debugAssertM(m_framebuffer.isNull(), "init called twice");

    debugAssertGLOk();
    m_framebuffer = Framebuffer::create("Film");

    // Cache shader
    static WeakReferenceCountedPointer<Shader> commonShader;
    m_shader = commonShader.createStrongPtr();
    if (m_shader.isNull()) {
        commonShader = m_shader = Shader::fromStrings("", shaderCode);
    }
}


void Film::exposeAndRender(RenderDevice* rd, const Texture::Ref& input) {
    if (m_framebuffer.isNull()) {
        init();
    }

    const int w = input->width();
    const int h = input->height();

    int blurDiameter = iRound(m_bloomRadiusFraction * 2.0f * max(w, h));
    if (isEven(blurDiameter)) {
        ++blurDiameter;
    }
    
    // Blur diameter for the vertical blur (at half resolution)
    int halfBlurDiameter = blurDiameter / 2;
    if (isEven(halfBlurDiameter)) {
        ++halfBlurDiameter;
    }

    float bloomStrength = m_bloomStrength;
    if (halfBlurDiameter <= 1) {
        // Turn off bloom; the filter radius is too small
        bloomStrength = 0;
    }

    // Allocate intermediate buffers, perhaps because the input size is different than was previously used.
    if (m_temp.isNull() || (m_temp->width() != w/2) || (m_temp->height() != h/2)) {
        // Make smaller to save fill rate, since it will be blurry anyway
        m_temp    = Texture::createEmpty("Film Temp",    w/2, h/2, m_intermediateFormat, Texture::DIM_2D_NPOT, Texture::Settings::video());
        m_blurry  = Texture::createEmpty("Film Blurry",  w/4, h/4, m_intermediateFormat, Texture::DIM_2D_NPOT, Texture::Settings::video());

        // Clear textures on the first time they are used; this is
        // required by NVIDIA drivers

        // Bind a texture first so that the framebuffer has a size
        //m_framebuffer->set(Framebuffer::COLOR_ATTACHMENT0, m_temp);
        m_framebuffer->set(Framebuffer::COLOR_ATTACHMENT0, m_temp);
        rd->push2D(m_framebuffer);
            rd->setColorClearValue(Color3::black());

            rd->clear();

            m_framebuffer->set(Framebuffer::COLOR_ATTACHMENT0, m_blurry);
            rd->clear();
        rd->pop2D();
    }

    // Bloom
    if (bloomStrength > 0) {
        // Bind a texture first so that the framebuffer has a size
        m_framebuffer->set(Framebuffer::COLOR_ATTACHMENT0, m_temp);
        rd->push2D(m_framebuffer);
        {
            // Blur vertically
            GaussianBlur::apply(rd, input, Vector2(0, 1), blurDiameter, m_temp->vector2Bounds());

            // Blur horizontally
            m_framebuffer->set(Framebuffer::COLOR_ATTACHMENT0, m_blurry);
            GaussianBlur::apply(rd, m_temp, Vector2(1, 0), halfBlurDiameter, m_blurry->vector2Bounds());
        }
        rd->pop2D();
    }

    rd->push2D();
    {
        // Combine, fix saturation, gamma correct and draw
        m_shader->args.set("sourceTexture",  input);
        m_shader->args.set("bloomTexture",   m_blurry);
        m_shader->args.set("bloomStrength",  m_bloomStrength);
        m_shader->args.set("exposure",       m_exposure);
        m_shader->args.set("invGamma",       1.0f / m_gamma);
        rd->setShader(m_shader);

        Draw::rect2D(input->rect2DBounds(), rd);
    }
    rd->pop2D();
}


void Film::makeGui(class GuiPane* pane, float maxExposure) {
    pane->addNumberBox("Gamma",         &m_gamma, "", GuiTheme::LOG_SLIDER, 1.0f, 7.0f, 0.1f);
    pane->addNumberBox("Exposure",      &m_exposure, "", GuiTheme::LOG_SLIDER, 0.01f, maxExposure);
    pane->addNumberBox("Bloom Str.",    &m_bloomStrength, "", GuiTheme::LOG_SLIDER, 0.0f, 1.0f);
    pane->addNumberBox("Bloom Radius",  &m_bloomRadiusFraction, "", GuiTheme::LOG_SLIDER, 0.0f, 0.2f);
    
}

}
