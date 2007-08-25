/**
  @file ShadowMap.h

  @author Morgan McGuire, morgan@cs.williams.edu
 */
#ifndef G3D_SHADOWMAP_H
#define G3D_SHADOWMAP_H

#include "G3D/Matrix4.h"
#include "G3D/ReferenceCount.h"
#include "G3D/GLight.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/PosedModel.h"

namespace G3D {

class ShadowMap : public ReferenceCountedObject {
private:

    TextureRef          m_depthTexture;

    /** If NULL, use the backbuffer */
    FramebufferRef      m_framebuffer;

    FramebufferRef      m_colorConversionFramebuffer;

    Matrix4             m_lightMVP;

    float               m_polygonOffset;

    /** Force the texture into this depth comparison mode */
    void setMode(Texture::DepthReadMode m);

    Array<Texture::DepthReadMode> m_depthModeStack;

    class RenderDevice* m_lastRenderDevice;

    /** True when m_colorTexture is out of date */
    bool                m_colorTextureIsDirty;

    TextureRef          m_colorTexture;

    void computeColorTexture();

    ShadowMap();

public:

    typedef ShadowMapRef Ref;

    static ShadowMapRef create() {
        return new ShadowMap();
    }

    /** Call with desiredSize = 0 to turn off shadow maps.
     */
    void setSize(int desiredSize = 1024);

    static ShadowMapRef create(int size) {
        ShadowMap* s = new ShadowMap();
        s->setSize(size);
        return s;
    }

    /** By default, the texture is configured for fixed function depth comparison using Texture::DEPTH_LEQUAL.  
        Some G3D::Shaders will want a different depth mode; you can use this to temporarily override the 
        depth comparison mode of the current depth buffer. */
    void pushDepthReadMode(Texture::DepthReadMode m);
    void popDepthReadMode();
    
    /** Increase to hide self-shadowing artifacts, decrease to avoid
        gap between shadow and object.  Default = 0.5 */
    void setPolygonOffset(float s) {
        m_polygonOffset = s;
    }

    float polygonOffset() const {
        return m_polygonOffset;
    }


    bool enabled() const;

    void updateDepth(
        class RenderDevice* renderDevice, 
        const GLight& light, 
        float lightProjX,
        float lightProjY,
        float lightProjNear,
        float lightProjFar,
        const Array<PosedModel::Ref>& shadowCaster);

    const Matrix4& lightMVP() const {
        return m_lightMVP;
    }

    TextureRef depthTexture() const {
        return m_depthTexture;
    }

    /** Returns the depthTexture as RGB16F format. Useful for cards
        that do not support reading against depth textures.*/
    TextureRef colorDepthTexture() const;
};

}

#endif // G3D_SHADOWMAP_H
