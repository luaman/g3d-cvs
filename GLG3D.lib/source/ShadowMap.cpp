/**
  @file ShadowMap.cpp

  @author Morgan McGuire, http://graphics.cs.williams.edu
  @edited 2010-01-30
 */
#include "GLG3D/ShadowMap.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/AABox.h"
#include "G3D/Box.h"
#include "G3D/Sphere.h"
#include "GLG3D/Draw.h"
#include "GLG3D/Surface.h"

namespace G3D {

ShadowMap::ShadowMap(const std::string& name) :
    m_name(name), 
    m_polygonOffset(1.0f), 
    m_backfacePolygonOffset(1.0f),
    m_lastRenderDevice(NULL), 
    m_bias(0.002f),
    m_colorDepthTextureIsDirty(true) {
}


ShadowMap::Ref ShadowMap::create(const std::string& name, int size, const Texture::Settings& settings) {
    ShadowMap* s = new ShadowMap(name);
    s->setSize(size, settings);
    return s;
}


void ShadowMap::setMode(Texture::DepthReadMode m) {

    if (m_depthTexture.isNull()) {
        return;
    }

    GLenum target = m_depthTexture->openGLTextureTarget();

    debugAssert(target == GL_TEXTURE_2D);

    // Save old texture
    GLenum oldID = glGetInteger(GL_TEXTURE_BINDING_2D);

    // Bind this texture
    glBindTexture(target, m_depthTexture->openGLID());

    // Configure
    switch (m) {
    case Texture::DEPTH_NORMAL:
        glTexParameteri(target, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
        glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
        break;

    default:
        glTexParameteri(target, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
        glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, 
                        GL_COMPARE_R_TO_TEXTURE_ARB);

        glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC_ARB, 
                        (m == Texture::DEPTH_LEQUAL) ? 
                        GL_LEQUAL : GL_GEQUAL);
        break;
    }
    
    // Unbind the texture, restoring whatever was there
    glBindTexture(target, oldID);
}


void ShadowMap::setSize(int desiredSize, const Texture::Settings& textureSettings) {
    if (desiredSize == 0) {
        m_depthTexture = NULL;
        m_framebuffer = NULL;
        m_colorDepthTexture = NULL;
        return;
    }

    if (! GLCaps::supports_GL_ARB_shadow()) {
        return;
    }
    
    if (! GLCaps::supports_GL_EXT_framebuffer_object()) {
        // Restrict to screen size
        desiredSize = 512;
    }

    Texture::Dimension dim = Texture::DIM_2D;
    if (! isPow2(desiredSize) && GLCaps::supports_GL_ARB_texture_non_power_of_two()) {
        dim = Texture::DIM_2D_NPOT;
    }

    m_depthTexture = Texture::createEmpty
        (m_name,
         desiredSize, desiredSize,
         ImageFormat::DEPTH16(),
         dim, 
         textureSettings);
    m_colorDepthTexture = NULL;

    if (GLCaps::supports_GL_EXT_framebuffer_object()) {
        m_framebuffer = Framebuffer::create(m_name + " Frame Buffer");
        m_framebuffer->set(Framebuffer::DEPTH_ATTACHMENT, m_depthTexture);
    }
}


bool ShadowMap::enabled() const {
    return m_depthTexture.notNull();
}


void ShadowMap::updateDepth
(RenderDevice*                   renderDevice,
 const CoordinateFrame&          lightCFrame, 
 const Matrix4&                  lightProjectionMatrix,
 const Array<Surface::Ref>&      shadowCaster,
 float                           biasDepth,
 RenderDevice::CullFace          cullFace) {

    if (biasDepth < 0) {
        biasDepth = m_bias;
    }

    m_lightProjection  = lightProjectionMatrix;
    m_lightFrame       = lightCFrame;
    m_lastRenderDevice = renderDevice;

    if (shadowCaster.size() == 0) {
        return;
    }

    debugAssert(GLCaps::supports_GL_ARB_shadow()); 
    debugAssertGLOk();

    if (m_framebuffer.isNull()) {
        // Ensure that the buffer fits on screen
        if ((m_depthTexture->width() > renderDevice->width()) ||
            (m_depthTexture->height() > renderDevice->height())) {

            int size = iMin(renderDevice->width(), renderDevice->height());
            if (! G3D::isPow2(size)) {
                // Round *down* to the nearest power of 2 (can't round up or
                // we might exceed the renderdevice size.
                size = G3D::ceilPow2(size) / 2;
            }
            setSize(size);
        }
    }

    Rect2D rect = m_depthTexture->rect2DBounds();

    debugAssertGLOk();
    renderDevice->pushState(m_framebuffer);
    {
        if (m_framebuffer.notNull()) {
            renderDevice->setDrawBuffer(RenderDevice::DRAW_NONE);
            renderDevice->setReadBuffer(RenderDevice::READ_NONE);
        } else {
            debugAssert(rect.height() <= renderDevice->height());
            debugAssert(rect.width() <= renderDevice->width());
            renderDevice->setViewport(rect);
        }

        debugAssertGLOk();
        //renderDevice->setColorClearValue(Color3::white());
        bool debugShadows = false;
        renderDevice->setColorWrite(debugShadows);
        renderDevice->setDepthWrite(true);
        renderDevice->clear(debugShadows, true, false);

        // Draw from the light's point of view
        renderDevice->setCameraToWorldMatrix(m_lightFrame);
        renderDevice->setProjectionMatrix(m_lightProjection);

        m_lightMVP = m_lightProjection * m_lightFrame.inverse();

        // Map [-1, 1] to [0, 1] (divide by 2 and add 0.5),
        // applying a bias term to offset the z value
        const Matrix4 bias(0.5f, 0.0f, 0.0f, 0.5f,
                           0.0f, 0.5f, 0.0f, 0.5f,
                           0.0f, 0.0f, 0.5f, 0.5f - biasDepth,
                           0.0f, 0.0f, 0.0f, 1.0f);
        
        m_biasedLightProjection = bias * m_lightProjection;
        m_biasedLightMVP = bias * m_lightMVP;

        renderDevice->setAlphaTest(RenderDevice::ALPHA_GREATER, 0.5);

        renderDepthOnly(renderDevice, shadowCaster, cullFace);
    }
    renderDevice->popState();

    if (m_framebuffer.isNull()) {
        debugAssert(m_depthTexture.notNull());
        RenderDevice::ReadBuffer old = renderDevice->readBuffer();
        renderDevice->setReadBuffer(RenderDevice::READ_BACK);
        m_depthTexture->copyFromScreen(rect);
        renderDevice->setReadBuffer(old);
    }

    m_colorDepthTextureIsDirty = true;
}


void ShadowMap::renderDepthOnly(RenderDevice* renderDevice, const Array<Surface::Ref>& shadowCaster, RenderDevice::CullFace cullFace, float polygonOffset) const {
    renderDevice->setPolygonOffset(polygonOffset);
    Surface::renderDepthOnly(renderDevice, shadowCaster, cullFace);
}


void ShadowMap::renderDepthOnly(RenderDevice* renderDevice, const Array<Surface::Ref>& shadowCaster, RenderDevice::CullFace cullFace) const {
    debugAssertGLOk();
    if (cullFace == RenderDevice::CULL_BACK) {
        renderDepthOnly(renderDevice, shadowCaster, cullFace, m_polygonOffset);
    } else if (cullFace == RenderDevice::CULL_FRONT) {
        renderDepthOnly(renderDevice, shadowCaster, cullFace, m_backfacePolygonOffset);
    } else if (m_backfacePolygonOffset == m_polygonOffset) {
        renderDepthOnly(renderDevice, shadowCaster, cullFace, m_polygonOffset);
    } else {
        // Different culling values
        renderDepthOnly(renderDevice, shadowCaster, RenderDevice::CULL_BACK, m_polygonOffset);
        renderDepthOnly(renderDevice, shadowCaster, RenderDevice::CULL_FRONT, m_backfacePolygonOffset);
    }

    debugAssertGLOk();
}


Texture::Ref ShadowMap::colorDepthTexture() const {
    if (m_colorDepthTextureIsDirty) {
        // We pretend that this is a const method, but
        // we might have to update the internal cache.
        const_cast<ShadowMap*>(this)->computeColorDepthTexture();
    }

    return m_colorDepthTexture;
}


void ShadowMap::computeColorDepthTexture() {
    RenderDevice* rd = m_lastRenderDevice;
    debugAssertM(rd, "Must call updateDepth() before colorDepthTexture()");

    if (m_colorDepthTexture.isNull()) {
        Texture::Settings settings = Texture::Settings::video();
        settings.interpolateMode = Texture::NEAREST_NO_MIPMAP;
        settings.wrapMode = m_depthTexture->settings().wrapMode;

        // Must be RGB16 or RGB16F because OpenGL can't render to
        // luminance textures and we need high bit depth for the
        // depth.
        
        const ImageFormat* fmt = ImageFormat::RGB16F();
        debugAssert(GLCaps::supportsTexture(fmt));

        m_colorDepthTexture = Texture::createEmpty
            (m_name, m_depthTexture->width(), m_depthTexture->height(), 
             fmt, Texture::defaultDimension(), settings);
    }

    // Convert depth to color
    if (m_colorConversionFramebuffer.isNull()) {
        m_colorConversionFramebuffer = Framebuffer::create("Depth to Color Conversion");
    }
    
    m_colorConversionFramebuffer->set(Framebuffer::COLOR_ATTACHMENT0, m_colorDepthTexture);

    setMode(Texture::DEPTH_NORMAL);
    rd->push2D(m_colorConversionFramebuffer);
    {
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
        rd->setAlphaTest(RenderDevice::ALPHA_ALWAYS_PASS, 0);
        rd->setColorWrite(true);
        rd->setDepthWrite(false);
        rd->setTexture(0, m_depthTexture);
        Draw::fastRect2D(m_depthTexture->rect2DBounds(), rd);
    }
    rd->pop2D();
    setMode(m_depthTexture->settings().depthReadMode);
    m_colorDepthTextureIsDirty = false;
}


void ShadowMap::computeMatrices
(const GLight&  light, 
 const AABox&   sceneBounds,
 GCamera&       lightFrame,
 Matrix4&       lightProjectionMatrix,
 float          lightProjX,
 float          lightProjY,
 float          lightProjNearMin,
 float          lightProjFarMax,
 float          intensityCutoff) {

    lightFrame.setCoordinateFrame(light.frame());

    if (light.position.w == 0) {
        // Move directional light away from the scene.  It must be far enough to see all objects
        lightFrame.setPosition(lightFrame.coordinateFrame().translation * 
            max(sceneBounds.extent().length() / 2.0f, lightProjNearMin, 30.0f) + sceneBounds.center());
    }

    const CFrame& f = lightFrame.coordinateFrame();

    float lightProjNear = lightProjNearMin;
    float lightProjFar  = lightProjFarMax;

    // TODO: for a spot light, only consider objects this light can see

    // Find nearest and farthest corners of the scene bounding box
    lightProjNear = finf();
    lightProjFar  = 0;
    for (int c = 0; c < 8; ++c) {
        Vector3 v = sceneBounds.corner(c);
        v = f.pointToObjectSpace(v);
        lightProjNear = min(lightProjNear, -v.z);
        lightProjFar = max(lightProjFar, -v.z);
    }
    
    // Don't let the near get too close to the source, and obey
    // the specified hint.
    lightProjNear = max(lightProjNearMin, lightProjNear);
    
    // Don't bother tracking shadows past the effective radius
    lightProjFar = min(light.effectSphere(intensityCutoff).radius, lightProjFar);
    lightProjFar = max(lightProjNear + 0.1f, min(lightProjFarMax, 
                                                 lightProjFar));

    if (light.spotCutoff <= 90) {
        // Spot light; we can set the lightProj bounds intelligently

        debugAssert(light.position.w == 1.0f);

        // The cutoff is half the angle of extent (See the Red Book, page 193)
        const float angle = toRadians(light.spotCutoff);

        lightProjX = tan(angle) * lightProjNear;
      
        // Symmetric in x and y
        lightProjY = lightProjX;

        lightProjectionMatrix = 
            Matrix4::perspectiveProjection
            (-lightProjX, lightProjX, -lightProjY, 
             lightProjY, lightProjNear, lightProjFar);

    } else if (light.position.w == 0) {
        // Directional light

        // Construct a projection and view matrix for the camera so we can 
        // render the scene from the light's point of view
        //
        // Since we're working with a directional light, 
        // we want to make the center of projection for the shadow map
        // be in the direction of the light but at a finite distance 
        // to preserve z precision.
        
        lightProjectionMatrix = 
            Matrix4::orthogonalProjection
            (-lightProjX, lightProjX, -lightProjY, 
             lightProjY, lightProjNear, lightProjFar);

    } else {
        // Point light.  Nothing good can happen here, but at least we
        // generate something

        lightProjectionMatrix =
            Matrix4::perspectiveProjection
            (-lightProjX, lightProjX, -lightProjY, 
             lightProjY, lightProjNear, lightProjFar);
    }

    float fov = atan2(lightProjX, lightProjNear) * 2.0f;
    lightFrame.setFieldOfView(fov, GCamera::HORIZONTAL);
    lightFrame.setNearPlaneZ(-lightProjNear);
    lightFrame.setFarPlaneZ(-lightProjFar);
}

}
