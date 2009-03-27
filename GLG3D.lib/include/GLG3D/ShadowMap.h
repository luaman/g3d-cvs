/**
  @file ShadowMap.h

  @author Morgan McGuire, morgan@cs.williams.edu
 */
#ifndef G3D_SHADOWMAP_H
#define G3D_SHADOWMAP_H

#include "G3D/Matrix4.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/PosedModel.h"

namespace G3D {

class ShadowMap : public ReferenceCountedObject {
private:
    std::string         m_name;

    TextureRef          m_depthTexture;

    /** If NULL, use the backbuffer */
    FramebufferRef      m_framebuffer;

    FramebufferRef      m_colorConversionFramebuffer;

    Matrix4             m_lightMVP;

    CFrame              m_lightFrame;
    Matrix4             m_biasedLightProjection;
    Matrix4             m_lightProjection;
    
    Matrix4             m_biasedLightMVP;

    float               m_polygonOffset;

    /** Force the texture into this depth comparison mode */
    void setMode(Texture::DepthReadMode m);

    Array<Texture::DepthReadMode> m_depthModeStack;

    class RenderDevice* m_lastRenderDevice;

    /** True when m_colorTexture is out of date */
    bool                m_colorTextureIsDirty;

    TextureRef          m_colorTexture;

    void computeColorTexture();

    ShadowMap(const std::string& name);

public:

    typedef ShadowMapRef Ref;

    /** For debugging purposes. */
    const std::string& name() const {
        return m_name;
    }

    /** 
    \brief Computes a reference frame and projection matrix for the light. 
    
    \param lightProjX Scene bounds in the light's reference frame for a directional light.  Not needed for a spot light
    \param lightProjY Scene bounds in the light's reference frame for a directional light.  Not needed for a spot light
    \param lightProjNear Shadow map near plane depth in the light's reference frame for a directional light.  Not needed for a spot light
    \param lightProjFar Shadow map far plane depth in the light's reference frame for a directional light.  Not needed for a spot light
    */
    static void computeMatrices(const GLight& light, const AABox& sceneBounds, CFrame& lightFrame, Matrix4& lightProjectionMatrix,
            float lightProjX = 12, float lightProjY = 12, float lightProjNear = 0.5f, float lightProjFar = 60);

    /** Call with desiredSize = 0 to turn off shadow maps.
     */
    void setSize(int desiredSize = 1024, const Texture::Settings& settings = Texture::Settings::shadow());

    static ShadowMapRef create(const std::string& name = "Shadow Map", int size = 1024, const Texture::Settings& settings = Texture::Settings::shadow()) {
        ShadowMap* s = new ShadowMap(name);
        s->setSize(size, settings);
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

    /** MVP adjusted to map to [0,0],[1,1] texture coordinates and addjusted in z 
        for depth comparisons to avoid self-shadowing artifacts on front faces. 
        
        Equal to biasedLightProjection() * lightFrame().inverse().
        */
    const Matrix4& biasedLightMVP() const {
        return m_biasedLightMVP;
    }

    /** The coordinate frame of the light source */
    const CFrame& lightFrame() const {
        return m_lightFrame;
    }

    /** Projection matrix for the light, biased to avoid self-shadowing */
    const Matrix4& biasedLightProjection() const {
        return m_biasedLightProjection;
    }

    const Matrix4& lightProjection() const {
        return m_lightProjection;
    }

    bool enabled() const;

    /** 
      @param biasDepth amount to bias z values by in the biasedMVP when later rendering
      */
    void updateDepth(
        class RenderDevice* renderDevice, 
        const CoordinateFrame& lightFrame,
        const Matrix4& lightProjectionMatrix,
        const Array<PosedModel::Ref>& shadowCaster,
        float biasDepth = 0.003f);

    /** Model-View-Projection matrix that maps world space to the
        shadow map pixels; used for rendering the shadow map itself.  Note that
        this maps XY to [-1,-1],[1,1]. Most applications will use biasedLightMVP
        to avoid self-shadowing problems. */
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
