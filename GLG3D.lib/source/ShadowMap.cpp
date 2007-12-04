/**
  @file ShadowMap.cpp

  @author Morgan McGuire, morgan@cs.williams.edu
 */
#include "GLG3D/ShadowMap.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/AABox.h"
#include "G3D/Box.h"
#include "GLG3D/Draw.h"

namespace G3D {

ShadowMap::ShadowMap() : m_polygonOffset(0.5f), m_lastRenderDevice(NULL), m_colorTextureIsDirty(true) {
    m_depthModeStack.append(Texture::DEPTH_LEQUAL);
}


void ShadowMap::pushDepthReadMode(Texture::DepthReadMode m) {
    Texture::DepthReadMode old = m_depthModeStack.last();
    m_depthModeStack.append(m);

    // Only make the OpenGL calls if necessary
    if (m != old) {
        setMode(m);
    }
}


void ShadowMap::popDepthReadMode() {
    Texture::DepthReadMode old = m_depthModeStack.pop();

    // Only make the OpenGL calls if necessary
    if (m_depthModeStack.last() != old) {
        setMode(old);
    }
}


void ShadowMap::setMode(Texture::DepthReadMode m) {

    if (m_depthTexture.isNull()) {
        return;
    }

    GLenum target = m_depthTexture->openGLTextureTarget();

    debugAssert(target == GL_TEXTURE_2D);
    glBindTexture(target, m_depthTexture->openGLID());
    GLenum old = glGetInteger(GL_TEXTURE_BINDING_2D);

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
    
    // Unbind the texture
    glBindTexture(target, old);
}


void ShadowMap::setSize(int desiredSize) {
    if (desiredSize == 0) {
        m_depthTexture = NULL;
        m_framebuffer = NULL;
        m_colorTexture = NULL;
        return;
    }

    if (! GLCaps::supports_GL_ARB_shadow()) {
        return;
    }
    
    int SHADOW_MAP_SIZE = desiredSize;

    if (! GLCaps::supports_GL_EXT_framebuffer_object()) {
        // Restrict to screen size
        SHADOW_MAP_SIZE = 512;
    }

    Texture::Settings textureSettings = Texture::Settings::shadow();

    m_depthTexture = Texture::createEmpty(
                                     "Shadow Map",
                                     SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
                                     TextureFormat::DEPTH16(),
                                     Texture::DIM_2D, 
                                     textureSettings);
    m_colorTexture = NULL;

    if (GLCaps::supports_GL_EXT_framebuffer_object()) {
        m_framebuffer = Framebuffer::create("Shadow Frame Buffer");
        m_framebuffer->set(Framebuffer::DEPTH_ATTACHMENT, m_depthTexture);
    }

    setMode(m_depthModeStack.last());
}


bool ShadowMap::enabled() const {
    return m_depthTexture.notNull();
}


void ShadowMap::updateDepth(
    RenderDevice* renderDevice,
    const GLight& light, 
    float lightProjX,
    float lightProjY,
    float lightProjNear,
    float lightProjFar,
    const Array<PosedModel::Ref>& shadowCaster) {

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
    
    GLenum oldDrawBuffer = glGetInteger(GL_DRAW_BUFFER);

    renderDevice->pushState(m_framebuffer);
        if (m_framebuffer.notNull()) {
            glReadBuffer(GL_NONE);
            glDrawBuffer(GL_NONE);
        } else {
            debugAssert(rect.height() <= renderDevice->height());
            debugAssert(rect.width() <= renderDevice->width());
            renderDevice->setViewport(rect);
        }

        // Construct a projection and view matrix for the camera so we can 
        // render the scene from the light's point of view
        //
        // Since we're working with a directional light, 
        // we want to make the center of projection for the shadow map
        // be in the direction of the light but at a finite distance 
        // to preserve z precision.
        Matrix4 lightProjectionMatrix(Matrix4::orthogonalProjection(-lightProjX, lightProjX, -lightProjY, 
                                                                    lightProjY, lightProjNear, lightProjFar));

        // Find the scene bounds
        AABox sceneBounds;
        shadowCaster[0]->worldSpaceBoundingBox().getBounds(sceneBounds);

        for (int i = 1; i < shadowCaster.size(); ++i) {
            AABox bounds;
            shadowCaster[i]->worldSpaceBoundingBox().getBounds(bounds);
            sceneBounds.merge(bounds);
        }

        CoordinateFrame lightCFrame;
        Vector3 center = sceneBounds.center();
        lightCFrame.translation = (light.position.xyz() * 20 + center) * (1 - light.position.w) + 
            light.position.w * light.position.xyz();

        Vector3 perp = Vector3::unitZ();
        // Avoid singularity when looking along own z-axis
        if (abs((lightCFrame.translation - center).direction().dot(perp)) > 0.8) {
            perp = Vector3::unitY();
        }
        lightCFrame.lookAt(center, perp);
        
        //renderDevice->setColorClearValue(Color3::white());
        bool debugShadows = false;
        renderDevice->setColorWrite(debugShadows);
        renderDevice->setDepthWrite(true);
        renderDevice->clear(debugShadows, true, false);

        // Draw from the light's point of view
        renderDevice->setCameraToWorldMatrix(lightCFrame);
        renderDevice->setProjectionMatrix(lightProjectionMatrix);

        // Flip the Y-axis to account for the upside down Y-axis on read back textures
        m_lightMVP = lightProjectionMatrix * lightCFrame.inverse();

        static const Matrix4 bias(
                                  0.5f, 0.0f, 0.0f, 0.5f,
                                  0.0f, 0.5f, 0.0f, 0.5f,
                                  0.0f, 0.0f, 0.5f, 0.5f - 0.003f,
                                  0.0f, 0.0f, 0.0f, 1.0f);

        m_biasedLightMVP = bias * m_lightMVP;

        // Avoid z-fighting
        renderDevice->setPolygonOffset(m_polygonOffset);

        renderDevice->setAlphaTest(RenderDevice::ALPHA_GREATER, 0.5);
        for (int s = 0; s < shadowCaster.size(); ++s) {
            shadowCaster[s]->render(renderDevice);
        }
    renderDevice->popState();

    if (m_framebuffer.isNull()) {
        debugAssert(m_depthTexture.notNull());
        m_depthTexture->copyFromScreen(rect);
    } else {
        glDrawBuffer(oldDrawBuffer);
    }

    m_colorTextureIsDirty = true;
}


TextureRef ShadowMap::colorDepthTexture() const {
    if (m_colorTextureIsDirty) {
        // We pretend that this is a const method, but
        // we might have to update the internal cache.
        const_cast<ShadowMap*>(this)->computeColorTexture();
    }

    return m_colorTexture;
}


void ShadowMap::computeColorTexture() {
    RenderDevice* rd = m_lastRenderDevice;
    debugAssertM(rd, "Must call updateDepth() before colorDepthTexture()");

    if (m_colorTexture.isNull()) {
        Texture::Settings settings = Texture::Settings::video();
        settings.interpolateMode = Texture::NEAREST_NO_MIPMAP;
        settings.wrapMode = WrapMode::CLAMP;

        // Must be RGB16 or RGB16F because OpenGL can't render to
        // luminance textures and we need high bit depth for the
        // depth.
        
        const TextureFormat* fmt = TextureFormat::RGB16F();
        debugAssert(GLCaps::supportsTexture(fmt));

        m_colorTexture = Texture::createEmpty
            ("ShadowMap color texture", m_depthTexture->width(), m_depthTexture->height(), 
             fmt, Texture::DIM_2D, settings);
    }

    // Convert depth to color
    if (m_colorConversionFramebuffer.isNull()) {
        m_colorConversionFramebuffer = Framebuffer::create("Depth to Color Conversion");
    }
    
    m_colorConversionFramebuffer->set(Framebuffer::COLOR_ATTACHMENT0, m_colorTexture);
    
    pushDepthReadMode(Texture::DEPTH_NORMAL);
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
    popDepthReadMode();
    m_colorTextureIsDirty = false;
}

}
