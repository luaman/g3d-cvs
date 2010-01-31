/**
  @file ShadowMap.h

  @author Morgan McGuire, http://graphics.cs.williams.edu
 */
#ifndef G3D_ShadowMap_h
#define G3D_ShadowMap_h

#include "G3D/platform.h"
#include "G3D/GLight.h"
#include "G3D/Matrix4.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/GCamera.h"
#include "G3D/AABox.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Texture.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Framebuffer.h"

namespace G3D {

class Surface;

class ShadowMap : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<ShadowMap> Ref;

protected:
    std::string         m_name;

    Texture::Ref        m_depthTexture;

    /** If NULL, use the backbuffer */
    Framebuffer::Ref    m_framebuffer;

    Framebuffer::Ref    m_colorConversionFramebuffer;

    Matrix4             m_lightMVP;

    CFrame              m_lightFrame;
    Matrix4             m_biasedLightProjection;
    Matrix4             m_lightProjection;
    
    Matrix4             m_biasedLightMVP;

    float               m_polygonOffset;
    float               m_backfacePolygonOffset;

    class RenderDevice* m_lastRenderDevice;

    /** True when m_colorTexture is out of date */
    bool                m_colorDepthTextureIsDirty;

    Texture::Ref        m_colorDepthTexture;

    void computeColorDepthTexture();

    ShadowMap(const std::string& name);

    void renderDepthOnly(RenderDevice* renderDevice, const Array<Surface::Ref>& shadowCaster, RenderDevice::CullFace cullFace) const;

public:

    /** For debugging purposes. */
    const std::string& name() const {
        return m_name;
    }

    /** Force the texture into this depth comparison mode */
    void setMode(Texture::DepthReadMode m);

    /** 
    \brief Computes a reference frame (as a camera) and projection
    matrix for the light.
    
    \param lightProjX Scene bounds in the light's reference frame for
    a directional light.  Not needed for a spot light

    \param lightProjY Scene bounds in the light's reference frame for
    a directional light.  Not needed for a spot light

    \param lightProjNear Shadow map near plane depth in the light's
    reference frame for a directional light.  For a spot light, a
    larger value will be chosen if the method determines that it can
    safely do so.  For directional and point lights, this value is
    used directly.

    \param lightProjFar Shadow map far plane depth in the light's
    reference frame for a directional light.   For a spot light, a
    smaller value will be chosen if the method determines that it can
    safely do so.  For directional and point lights, this value is
    used directly.    

    \param intensityCutoff Don't bother shadowing objects that cannot be brighter than this value.
    Set to 0 to cast shadows as far as the entire scene.
    */
    static void computeMatrices
    (const GLight& light, const AABox& sceneBounds, GCamera& lightFrame, Matrix4& lightProjectionMatrix,
     float lightProjX = 12, float lightProjY = 12, float lightProjNear = 0.5f, float lightProjFar = 60,
     float intensityCutoff = 1/255.0f);

    /** Call with desiredSize = 0 to turn off shadow maps.
     */
    virtual void setSize(int desiredSize = 1024, const Texture::Settings& settings = Texture::Settings::shadow());

    static ShadowMap::Ref create(const std::string& name = "Shadow Map", int size = 1024, 
                               const Texture::Settings& settings = Texture::Settings::shadow());

    
    /** Increase to hide self-shadowing artifacts, decrease to avoid
        gap between shadow and object.  Default = 0.5.  
        
        \param b if nan(), the backface offset is set to s, otherwise it is set to b */
    inline void setPolygonOffset(float s, float b = nan()) {
        m_polygonOffset = s;
        if (isNaN(b)) {
            m_backfacePolygonOffset = s;
        } else {
            m_backfacePolygonOffset = b;
        }
    }

    inline float polygonOffset() const {
        return m_polygonOffset;
    }
    
    inline float backfacePolygonOffset() const {
        return m_backfacePolygonOffset;
    }

    /** MVP adjusted to map to [0,0],[1,1] texture coordinates and addjusted in z 
        for depth comparisons to avoid self-shadowing artifacts on front faces. 
        
        Equal to biasedLightProjection() * lightFrame().inverse().

        This includes Y inversion, on the assumption that shadow maps are rendered
        to texture.
     */
    const Matrix4& biasedLightMVP() const {
        return m_biasedLightMVP;
    }

    /** The coordinate frame of the light source */
    const CFrame& lightFrame() const {
        return m_lightFrame;
    }

    /** Projection matrix for the light, biased to avoid self-shadowing.*/
    const Matrix4& biasedLightProjection() const {
        return m_biasedLightProjection;
    }

    /** Projection matrix for the light, biased to avoid self-shadowing.*/
    const Matrix4& lightProjection() const {
        return m_lightProjection;
    }

    bool enabled() const;

    /** 
    \param biasDepth amount to bias z values by in the biasedMVP when
    later rendering Usually around 0.0001-0.005
    */
    virtual void updateDepth
    (class RenderDevice*           renderDevice, 
     const CoordinateFrame&        lightFrame,
     const Matrix4&                lightProjectionMatrix,
     const Array< ReferenceCountedPointer<Surface> >& shadowCaster,
     float                         biasDepth = 0.001f,
     RenderDevice::CullFace        cullFace = RenderDevice::CULL_FRONT);

    /** Model-View-Projection matrix that maps world space to the
        shadow map pixels; used for rendering the shadow map itself.  Note that
        this maps XY to [-1,-1],[1,1]. Most applications will use biasedLightMVP
        to avoid self-shadowing problems. */
    const Matrix4& lightMVP() const {
        return m_lightMVP;
    }

    Texture::Ref depthTexture() const {
        return m_depthTexture;
    }

    /** Returns the depthTexture as RGB16F format. Useful for cards
        that do not support reading against depth textures.*/
    Texture::Ref colorDepthTexture() const;

    inline Rect2D rect2DBounds() const {
        return m_depthTexture->rect2DBounds();
    }
};

}

#endif // G3D_SHADOWMAP_H
