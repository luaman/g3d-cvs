/**
 @file Film.cpp
 @author Morgan McGuire, http://graphics.cs.williams.edu
 */

#include "GLG3D/Film.h"
#include "G3D/debugAssert.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GaussianBlur.h"
#include "GLG3D/Draw.h"

namespace G3D {

// TODO: on newer GPUs when not in bloom mode, use texelFetch when 

static const char* shaderCode = 
STR(
uniform sampler2D sourceTexture;
uniform sampler2D bloomTexture;
uniform float     bloomStrengthScaled;
uniform float     exposure;

// 1.0 / monitorGamma.  Usually about invGamma = 0.5
uniform float     invGamma;

void main(void) {
 
    vec3 src   = texture2D(sourceTexture, gl_TexCoord[0].st).rgb;
    vec3 bloom = texture2D(bloomTexture, gl_TexCoord[0].st).rgb;

    // Parens are to force scalar multiplies over vector ones
    // We multiply the bloomStrength by 5 to make a vector mul into a scalar mul.
/*    src = src   * ((1.0 - bloomStrength) * exposure) + 
          bloom * (5.0 * bloomStrength * exposure);
          */
    src = (src * exposure + bloom * bloomStrengthScaled);

    // Fix out-of-gamut saturation
    // Maximumum channel:
    float m = max(max(src.r, src.g), max(src.b, 1.0));
    // Normalized color
    src /= m;
    // Fade towards white when the max is brighter than 1.0 (like a light saber core)
    src = mix(src, vec3(1.0), clamp((m - 1.0) * 0.2, 0.0, 1.0));
    
    // Invert the gamma curve
    vec3 dst = pow(src, vec3(invGamma, invGamma, invGamma));
    
    gl_FragColor.rgb = dst;
});

static const char* preBloomShaderCode = 
STR(
uniform sampler2D sourceTexture;
uniform float     exposure;

void main(void) { 
    vec3 src = texture2D(sourceTexture, gl_TexCoord[g3d_Index(sourceTexture)].st).rgb * exposure;
    float p  = max(max(src.r, src.g), src.b);
    gl_FragColor.rgb = src * smoothstep(1.0, 2.0, p);
}
);

Film::Film(const ImageFormat* f) :
    m_intermediateFormat(f),
    m_gamma(2.0f),
    m_exposure(1.0f),
    m_bloomStrength(0),// 0.08f), // Commented out because the blur is currently slow
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
    m_blurryFramebuffer = Framebuffer::create("Film blurry");
    m_tempFramebuffer = Framebuffer::create("Film temp");

    // Cache shader
    static WeakReferenceCountedPointer<Shader> commonShader, commonPreBloomShader;
    m_shader = commonShader.createStrongPtr();
    m_preBloomShader = commonPreBloomShader.createStrongPtr();
    if (m_shader.isNull()) {
        commonShader = m_shader = Shader::fromStrings("", shaderCode);
        m_shader->setPreserveState(false);
        commonPreBloomShader = m_preBloomShader = Shader::fromStrings("", preBloomShaderCode);
        m_preBloomShader->setPreserveState(false);
    }
}


void Film::exposeAndRender(RenderDevice* rd, const Texture::Ref& input, int downsample) {
    debugAssertM(downsample == 1, "Downsampling not implemented in this release");
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
        m_preBloom = Texture::createEmpty("Film PreBloom", w, h, m_intermediateFormat, Texture::defaultDimension(), Texture::Settings::video());
        m_temp     = Texture::createEmpty("Film Temp",    w/2, h/2, m_intermediateFormat, Texture::defaultDimension(), Texture::Settings::video());
        m_blurry   = Texture::createEmpty("Film Blurry",  w/4, h/4, m_intermediateFormat, Texture::defaultDimension(), Texture::Settings::video());

		// Clear the newly created textures
        m_preBloom->clear(Texture::CUBE_POS_X, 0, rd);
        m_temp->clear(Texture::CUBE_POS_X, 0, rd);
        m_blurry->clear(Texture::CUBE_POS_X, 0, rd);

        m_framebuffer->set(Framebuffer::COLOR_ATTACHMENT0, m_preBloom);
        m_tempFramebuffer->set(Framebuffer::COLOR_ATTACHMENT0, m_temp);
        m_blurryFramebuffer->set(Framebuffer::COLOR_ATTACHMENT0, m_blurry);
    }

    rd->push2D();

    // Bloom
    if (bloomStrength > 0) {
        Framebuffer::Ref oldFB = rd->framebuffer();
        rd->setFramebuffer(m_framebuffer);
        rd->clear();
        m_preBloomShader->args.set("sourceTexture",  input);
        m_preBloomShader->args.set("exposure", m_exposure);
        rd->setShader(m_preBloomShader);
        Draw::fastRect2D(m_preBloom->rect2DBounds(), rd);

        rd->setFramebuffer(m_tempFramebuffer);
        rd->clear();
        // Blur vertically
        GaussianBlur::apply(rd, m_preBloom, Vector2(0, 1), blurDiameter, m_temp->vector2Bounds());

        // Blur horizontally
        rd->setFramebuffer(m_blurryFramebuffer);
        rd->clear();
        GaussianBlur::apply(rd, m_temp, Vector2(1, 0), halfBlurDiameter, m_blurry->vector2Bounds());

        rd->setFramebuffer(oldFB);
    }

    {
        // Combine, fix saturation, gamma correct and draw
        m_shader->args.set("sourceTexture",  input);
        m_shader->args.set("bloomTexture",   (bloomStrength > 0) ? m_blurry : Texture::zero());
        m_shader->args.set("bloomStrengthScaled",  bloomStrength * 10.0);
        m_shader->args.set("exposure",       m_exposure);
        m_shader->args.set("invGamma",       1.0f / m_gamma);
        rd->setShader(m_shader);

        Draw::fastRect2D(input->rect2DBounds(), rd);
    }
    rd->pop2D();
}


void Film::makeGui(class GuiPane* pane, float maxExposure, float sliderWidth, float indent) {
    GuiNumberBox<float>* n = NULL;

    n = pane->addNumberBox("Gamma",         &m_gamma, "", GuiTheme::LOG_SLIDER, 1.0f, 7.0f, 0.1f);
    n->setWidth(sliderWidth);  n->moveBy(indent, 0);

    n = pane->addNumberBox("Exposure",      &m_exposure, "", GuiTheme::LOG_SLIDER, 0.001f, maxExposure);
    n->setWidth(sliderWidth);  n->moveBy(indent, 0);

    n = pane->addNumberBox("Bloom Str.",    &m_bloomStrength, "", GuiTheme::LOG_SLIDER, 0.0f, 1.0f);
    n->setWidth(sliderWidth);  n->moveBy(indent, 0);

    n = pane->addNumberBox("Bloom Radius",  &m_bloomRadiusFraction, "", GuiTheme::LOG_SLIDER, 0.0f, 0.2f);
    n->setWidth(sliderWidth);  n->moveBy(indent, 0);    
}

}
