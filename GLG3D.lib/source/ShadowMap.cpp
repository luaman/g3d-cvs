/**
  @file ShadowMap.cpp

  @author Morgan McGuire, morgan@cs.williams.edu
  @edited 2009-03-27
 */
#include "GLG3D/ShadowMap.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/AABox.h"
#include "G3D/Box.h"
#include "G3D/Sphere.h"
#include "GLG3D/Draw.h"

namespace G3D {

ShadowMap::ShadowMap(const std::string& name) : m_name(name), m_polygonOffset(0.5f), 
    m_lastRenderDevice(NULL), m_colorTextureIsDirty(true) {
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


void ShadowMap::setSize(int desiredSize, const Texture::Settings& textureSettings) {
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

    m_depthTexture = Texture::createEmpty(
                                     m_name,
                                     SHADOW_MAP_SIZE, SHADOW_MAP_SIZE,
                                     ImageFormat::DEPTH16(),
                                     Texture::DIM_2D, 
                                     textureSettings);
    m_colorTexture = NULL;

    if (GLCaps::supports_GL_EXT_framebuffer_object()) {
        m_framebuffer = Framebuffer::create(m_name + " Frame Buffer");
        m_framebuffer->set(Framebuffer::DEPTH_ATTACHMENT, m_depthTexture);
    }

    setMode(m_depthModeStack.last());
}


bool ShadowMap::enabled() const {
    return m_depthTexture.notNull();
}

void ShadowMap::updateDepth(
    RenderDevice*                   renderDevice,
    const CoordinateFrame&          lightCFrame, 
    const Matrix4&                  lightProjectionMatrix,
    const Array<PosedModel::Ref>&   shadowCaster,
    float                           biasDepth) {

    m_lightProjection = lightProjectionMatrix;
    m_lightFrame = lightCFrame;
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

        //renderDevice->setColorClearValue(Color3::white());
        bool debugShadows = false;
        renderDevice->setColorWrite(debugShadows);
        renderDevice->setDepthWrite(true);
        renderDevice->clear(debugShadows, true, false);

        // Draw from the light's point of view
        renderDevice->setCameraToWorldMatrix(m_lightFrame);
        renderDevice->setProjectionMatrix(m_lightProjection);

        m_lightMVP = m_lightProjection * m_lightFrame.inverse();

        static const Matrix4 bias(
                                  0.5f, 0.0f, 0.0f, 0.5f,
                                  0.0f, 0.5f, 0.0f, 0.5f,
                                  0.0f, 0.0f, 0.5f, 0.5f - biasDepth,
                                  0.0f, 0.0f, 0.0f, 1.0f);

        m_biasedLightProjection = bias * m_lightProjection;
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
        
        const ImageFormat* fmt = ImageFormat::RGB16F();
        debugAssert(GLCaps::supportsTexture(fmt));

        m_colorTexture = Texture::createEmpty
            (m_name, m_depthTexture->width(), m_depthTexture->height(), 
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


void ShadowMap::computeMatrices
(const GLight&  light, 
 const AABox&   sceneBounds,
 GCamera&       lightFrame,
 Matrix4&       lightProjectionMatrix,
 float          lightProjX,
 float          lightProjY,
 float          lightProjNear,
 float          lightProjFar) {

    lightFrame.setCoordinateFrame(light.frame());

    if (light.position.w == 0) {
        // Move directional light away from the scene.  It must be far enough to see all objects
        lightFrame.setPosition(lightFrame.coordinateFrame().translation * 
            max(sceneBounds.extent().length() / 2.0f, lightProjNear, 30.0f) + sceneBounds.center());
    }

    const CFrame& f = lightFrame.coordinateFrame();

    if (light.spotCutoff <= 90) {
        // Spot light; we can set the lightProj bounds intelligently

        debugAssert(light.position.w == 1.0f);

        // Find nearest and farthest corners of the scene bounding box
        lightProjNear = (float)inf();
        lightProjFar  = 0;
        for (int c = 0; c < 8; ++c) {
            Vector3 v = sceneBounds.corner(c);
            v = f.pointToObjectSpace(v);
            lightProjNear = min(lightProjNear, -v.z);
            lightProjFar = max(lightProjFar, -v.z);
        }
        
        // Don't let the near get too close to the source
        lightProjNear = max(0.2f, lightProjNear);

        // Don't bother tracking shadows past the effective radius
        lightProjFar = min(light.effectSphere().radius, lightProjFar);
        lightProjFar = max(lightProjNear + 0.1f, lightProjFar);

        // The cutoff is half the angle of extent (See the Red Book, page 193)
        const float angle = toRadians(light.spotCutoff);
        lightProjX = lightProjNear * sin(angle);

        if (! light.spotSquare) {
            // Fit a square around the circle
            lightProjX *= 2.0f / sqrt(2.0f);
        }

        // Symmetric in x and y
        lightProjY = lightProjX;

        lightProjectionMatrix = 
            Matrix4::perspectiveProjection(-lightProjX, lightProjX, -lightProjY, 
                                            lightProjY, lightProjNear, lightProjFar);

    } else if (light.position.w == 0) {
        // Directional light

        /*
        // Find the bounds on the projected character
        AABox2D bounds;
        for (int m = 0; m < allModels.size(); ++m) {
            const PosedModelRef& model = allModels[m];
            const Sphere& b = model->worldSpaceBoundingSphere();
            
        }*/

        // Construct a projection and view matrix for the camera so we can 
        // render the scene from the light's point of view
        //
        // Since we're working with a directional light, 
        // we want to make the center of projection for the shadow map
        // be in the direction of the light but at a finite distance 
        // to preserve z precision.
        
        lightProjectionMatrix = Matrix4::orthogonalProjection(-lightProjX, lightProjX, -lightProjY, 
                                                         lightProjY, lightProjNear, lightProjFar);

    } else {
        // Point light.  Nothing good can happen here, but generate something

        lightProjectionMatrix = Matrix4::perspectiveProjection(-lightProjX, lightProjX, -lightProjY, 
                                                              lightProjY, lightProjNear, lightProjFar);
    }

    float fov = atan2(lightProjX, lightProjNear) * 2.0f;
    lightFrame.setFieldOfView(fov, GCamera::HORIZONTAL);
    lightFrame.setNearPlaneZ(-lightProjNear);
    lightFrame.setFarPlaneZ(-lightProjFar);
}

}
