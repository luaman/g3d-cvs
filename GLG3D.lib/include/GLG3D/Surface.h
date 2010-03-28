/**
  @file Surface.h
  
  @maintainer Morgan McGuire, http://graphics.cs.williams.edu

  @created 2003-11-15
  @edited  2010-03-27
 */ 

#ifndef GLG3D_Surface_h
#define GLG3D_Surface_h

#include "G3D/Array.h"
#include "G3D/Color4.h"
#include "G3D/MeshAlg.h"
#include "GLG3D/Texture.h"
#include "GLG3D/SkyParameters.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/ShadowMap.h"

namespace G3D {

class ShadowMap;

namespace SuperShader {
class Pass;
typedef ReferenceCountedPointer<Pass> PassRef;
}

/** 
  Simple material used by IFSModel and MD2Model pose methods.
  This class is provided as a convenience; it is not necessary
  to use it.  If you do not provide a material, whatever
  state is currently on the G3D::RenderDevice is used.  
  
  This is a legacy class for fixed function rendering. You
  probably don't want to use GMaterial at all if you
  are writing vertex and pixel shaders.

  @deprecated
  @sa G3D::Material
 */
class GMaterial {
public:
    float                   specularCoefficient;
    float                   shininess;
    /** Diffuse color */
    Color4                  color;
    Array<Texture::Ref>     texture;

    inline GMaterial(TextureRef t = NULL) : specularCoefficient(0.2f), shininess(10), color(Color3::white()) {
        if (t.notNull()) {
            texture.append(t);
        }
    }

    /** Applies this material to the render device */
    void configure(class RenderDevice* rd) const;
};

typedef ReferenceCountedPointer<class Surface> SurfaceRef;

/**
 Base class for posed models.  A posed model is a snapshot of geometry
 and other rendering information, frozen in time, of a potentially 
 animated or deformable "Model".  G3D has no API (but some conventions) for
 how you get a Surface from a model class.
 
 <B>G3D does not provide a scene graph structure</B> (e.g. there is no Model base class)
 because the visible surface determination, rendering, and caching behavior of a scene
 graph are different for each application.  There is currently no one-size-fits-all
 scene graph design pattern.
 
 G3D <B>does</B> provide some basic infrastructure for making your
 own scene graph, however.  Model classes (e.g. G3D::IFSModel, G3D::MD2Model),
 which have no common base class, should all implement a <CODE>pose</CODE> 
 method.  The arguments to this method are all of the parameters (e.g. 
 animation frame, limb position) needed for reducing the general purpose,
 poseable model to a specific world space mesh for rendering.  This
 instance specific mesh is a G3D::Surface.  Because all Surfaces
 have the same interface, they can be used interchangably.

 Use G3D::SurfaceWrapper to encapsulate an existing posed model
 with your own.

 A common strategy when implementing Surface is to not compute "derived"
 values like the object space face normals until they are needed.  That is,
 the first call to a method might be very expensive because it goes off
 and computes some value that will be cached for future calls.

 <b>Rendering</b>
 The easiest way to render is Surface::render().  More sophisticated rendering, e.g., in the presence of shadows, can be accomplished with 
 the separate renderShadowMappedLightPass etc. routines.

 Surface also allows you to directly extract and operate on its geometry.  This is useful for adding effects like
 outlines in cartoon rendering, physics hit boxes, and shadow volume rendering.  You can directly render from the geometry
 using the following rendering code (it is much faster if you can avoid re-creating the VARs and VertexBuffer every frame!)
 <pre>
        VertexBufferRef area = VertexBuffer::create(sizeof(Vector3) * posed->objectSpaceGeometry().vertexArray.size() * 2 + sizeof(Vector2) * posed->texCoords().size());
        VertexRange vertex(posed->objectSpaceGeometry().vertexArray, area);
        VertexRange normal(posed->objectSpaceGeometry().normalArray, area);
        VertexRange texCoord(posed->texCoords(), area);

        rd->setObjectToWorldMatrix(posed->coordinateFrame());
        rd->setShadeMode(RenderDevice::SHADE_SMOOTH);
        rd->beginIndexedPrimitives();
            rd->setVertexArray(vertex);
            rd->setNormalArray(normal);
            rd->setTexCoordArray(0, texCoord);
            rd->sendIndices(PrimitiveType::TRIANGLES, posed->triangleIndices());
        rd->endIndexedPrimitives();
 </pre>
 */
class Surface : public ReferenceCountedObject {
protected:

    Surface() {}

public:
    typedef ReferenceCountedPointer<class Surface> Ref;

    /** \brief How sortAndRender() configures the RenderDevice to process alpha */
    enum AlphaMode {
        /** Alpha > 0.5 is rendered, alpha <= 0.5 is discarded. */
        ALPHA_BINARY,

        /** Convert alpha to coverage values using <code>glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB)</code>. 
           Requires a MSAA framebuffer to be bound.*/
        // See http://www.dhpoware.com/samples/glMultiSampleAntiAliasing.html for an example.
        ALPHA_TO_COVERAGE,
        
        /** Render surfaces with partial coverage from back to front, using Porter and Duff's OVER operator.
            This leaves the depth buffer inconsistent with the color buffer and requires a sort, but often gives 
            the best appearance.
         */
        ALPHA_BLEND
    };

    virtual ~Surface() {}

    virtual std::string name() const = 0;

    /** If true, this object transmits light and depends on
        back-to-front rendering order and should be rendered in sorted
        order. Default is false.*/
    virtual bool hasTransmission() const {
        return false;
    }

    /** If true, this object's material produces subpixel coverage (i.e. alpha) and may require back-to-front rendering
        depending on Surface::AlphaMode. */
    virtual bool hasPartialCoverage() const {
        return false;
    }

    /** 
      Divides the inModels into a front-to-back sorted array of opaque
      models and a back-to-front sorted array of potentially
      transparent models.  Any data originally in the output arrays is
      cleared.

      @param wsLookVector Sort axis; usually the -Z axis of the camera.
     */
    static void sortFrontToBack
       (Array<Surface::Ref>&       surfaces, 
        const Vector3&             wsLookVector);

    static void sortBackToFront
       (Array<Surface::Ref>&       surfaces, 
        const Vector3&             wsLookVector) {
        sortFrontToBack(surfaces, -wsLookVector);
    }

    static void getBoxBounds(const Array<Surface::Ref>& models, AABox& bounds);
    static void getSphereBounds(const Array<Surface::Ref>& models, Sphere& bounds);

    /** Computes the array of models that can be seen by @a camera*/
    static void cull(const class GCamera& camera, const class Rect2D& viewport, const Array<Surface::Ref>& allModels, Array<Surface::Ref>& outModels);

    /** Object to world space coordinate frame.*/
    virtual void getCoordinateFrame(CoordinateFrame& c) const = 0;

    virtual CoordinateFrame coordinateFrame() const;

    /** Get the <B>object space</B> geometry (faster than getWorldSpaceGeometry). 
        Object and world space geometry only differ by a CoordinateFrame transformation.*/
    virtual const MeshAlg::Geometry& objectSpaceGeometry() const = 0;

    /** Get the <B>world space</B> geometry. */
    virtual void getWorldSpaceGeometry(MeshAlg::Geometry& geometry) const;

    /** @deprecated Use objectSpaceFaceNormals() */
    virtual void getObjectSpaceFaceNormals(Array<Vector3>& faceNormals, bool normalize = true) const;

    virtual void getWorldSpaceFaceNormals(Array<Vector3>& faceNormals, bool normalize = true) const;

    /** Return a pointer to an array of object space face normals. */
    virtual const Array<Vector3>& objectSpaceFaceNormals(bool normalize = true) const = 0;

    // Returns a reference rather than filling out an array because most
    // Surfaces have this information available.
    /**
      Adjacency information respecting the underlying connectivity
      of the mesh-- colocated vertices are treated as distinct.
     */
    virtual const Array<MeshAlg::Face>& faces() const = 0;

    virtual const Array<MeshAlg::Edge>& edges() const = 0;

    virtual const Array<MeshAlg::Vertex>& vertices() const = 0;

    /** If this model has texture coordinates, returns per-vertex texture coordinates
        in an array indexed the same as vertices() and geometry.vertexArray, otherwise
        returns an array of size zero.  Default implementation returns an array of size zero.
    */
    virtual const Array<Vector2>& texCoords() const;

    /** Returns per-vertex tangent vectors, if available. May return an empty array.
     Packs two tangents, T1 and T2 that form a reference frame with the normal such that 
            
            - \f$ \vec{x} = \vec{T}_1 = \vec{t}_{xyz}\f$ 
            - \f$ \vec{y} = \vec{T}_2 = \vec{t}_w * (\vec{n} \times \vec{t}_{xyz})  \f$
            - \f$ \vec{z} = \vec{n} \f$ */
    virtual const Array<Vector4>& objectSpacePackedTangents() const;

    /** Returns true if this model has texture coordinates. */ 
    virtual bool hasTexCoords() const {
        return false;
    }

    /** Contain adjacency information that merges colocated vertices
        (see MeshAlg::weldAdjacency) */
    virtual const Array<MeshAlg::Face>& weldedFaces() const = 0;

    virtual const Array<MeshAlg::Edge>& weldedEdges() const = 0;

    virtual const Array<MeshAlg::Vertex>& weldedVertices() const = 0;

    /**
     Indices into the Geometry that create triangles.  May be welded or not, depending on 
     the model.
     */
    virtual const Array<int>& triangleIndices() const = 0;

    virtual void getObjectSpaceBoundingSphere(Sphere&) const = 0;

    virtual Sphere objectSpaceBoundingSphere() const;

    virtual AABox objectSpaceBoundingBox() const;

    virtual void getWorldSpaceBoundingSphere(Sphere& s) const;

    virtual Sphere worldSpaceBoundingSphere() const;

    virtual void getObjectSpaceBoundingBox(AABox&) const = 0;

    virtual void getWorldSpaceBoundingBox(AABox& box) const;

    virtual AABox worldSpaceBoundingBox() const;

    /** Render using current fixed function lighting environment. Do not 
        change the steBehavior 
        with regard to stencil, shadowing, etc. is intentionally undefinded. 

        Default implementation calls defaultRender.
        */
    virtual void render(class RenderDevice* renderDevice) const;

    /**
     Number of edges that have only one adjacent face in edges().
     These boundary edges are all at the end of the edge list.
     */
    virtual int numBoundaryEdges() const = 0;

    /**
     Number of edges that have only one adjacent face in weldedEdges().
     These boundary edges are all at the end of the edge list.
     */
    virtual int numWeldedBoundaryEdges() const = 0;

    /** @deprecated */
    inline int numBrokenEdges() const {
        return numBoundaryEdges();
    }

    /** 
     Render all terms that are independent of shadowing 
     (e.g., transparency, reflection, ambient illumination, 
     emissive illumination, nonShadowCastingLights). Transparent objects are assumed to render additively 
     (but should set the blend mode themselves). Restore all state to the original form
     on exit.  Default implementation invokes render.

     Implementation must obey the current stencil, depth write, color write, and depth test modes.
     Implementation may freely set the blending, and alpha test modes.

     The caller should invoke this in depth sorted back to front order for transparent objects.

     The default implementation configures the non-shadow casting lights and calls render.

     Implementation advice:
      <UL>
        <LI> If color write is disabled, don't bother performing any shading on this object.
        <LI> It may be convenient to support multiple lights by invoking renderShadowedLightPass multiple times.
      </UL>

    */
    virtual void renderNonShadowed(
        RenderDevice* rd,
        const LightingRef& lighting) const;

    /** Render illumination from this light source additively. Implementation must set the
        alpha blending mode to additive.  Must obey the current stencil, depth write, and depth test modes.
        Default implementation enables a single light and calls render.
    */
    virtual void renderShadowedLightPass(
        RenderDevice* rd, 
        const GLight& light) const;

    /** Render illumination from this source additively, held out by the shadow map (which the caller 
        must have computed, probably using renderNonShadowed).  Default implementation
        configures the shadow map in texture unit 1 and calls render. */
    virtual void renderShadowMappedLightPass(
        RenderDevice* rd, 
        const GLight& light,
        const ReferenceCountedPointer<ShadowMap>& shadowMap) const;

    /**
     Removes elements from \a all and puts them in \a translucent.
     \a translucent is cleared first.
     Always treats hasTransmissive() objects as translucent.
     If \a partialCoverageIsTranslucent is true, also treats hasPartialCoverage as translucent.
     */
    static void extractTranslucent(Array<Surface::Ref>& all, Array<Surface::Ref>& translucent, bool partialCoverageIsTranslucent);

    /** Render geometry only (no shading), and ignore color (but do perform alpha testing).
        Render only back or front faces (two-sided surfaces render no matter what).

        Does not sort or cull based on the view frustum of the camera like other batch rendering routines.

        Used for early-Z and shadow mapping.
     */    
    static void renderDepthOnly(
        RenderDevice* rd, 
        const Array<Surface::Ref>& allModels, 
        RenderDevice::CullFace cull);

    /**
     Configures the SuperShader with the G3D::Material for this object
     and renders it.  If this object does not support G3D::Materials
     (or an equivalent) may render nothing.  These passes will be additively blended
     with previous ones.

     @return True if state was preserved, false if the renderdevice is in a different state than when called.
     @beta
     */
    virtual bool renderSuperShaderPass(
        RenderDevice* rd, 
        const SuperShader::PassRef& pass) const {
        (void) rd;
        (void) pass;
        return true;
    }

    /** @deprecated */
    virtual void renderShadowMappedLightPass(
        RenderDevice* rd, 
        const GLight& light,
        const Matrix4& lightMVP,
        const Texture::Ref& shadowMap) const;

    /**
      Sends all geometry including texture coordinates (uploading it
      first if necessary) but does not set any render device state or
      use any textures.

      This is useful when applying your own G3D::Shader to an existing
      Surface.
    */
    virtual void sendGeometry(RenderDevice* rd) const;


    /**
       Renders an array of models with the full G3D illumination model
       (correct transparency, multiple direct lights, multiple shadow
       mapped lights), optimizing ArticulatedModels separately to
       minimize state changes.  
       
       Calls renderTranslucent() for translucent surface rendering.  If you
       need to perform other rendering before translucents, explicitly remove
       non-opaque objects from \a allModels yourself and then call renderTranslucent
       later.  Note that you can use the shadow maps that were computed by sortAndRender.

       \param shadowMaps As many shadow maps as there are
       shadow casting lights must be provided.  Do not render the shadow maps yourself;
       sortAndRender() does that and puts the results back into the array. 
    */
    static void sortAndRender
    (class RenderDevice*                rd, 
     const class GCamera&               camera,
     const Array<SurfaceRef>&           allModels, 
     const LightingRef&                 lighting, 
     const Array<ReferenceCountedPointer<ShadowMap> >&  shadowMaps,
     const Array<SuperShader::PassRef>& extraAdditivePasses,
     AlphaMode                          alphaMode = ALPHA_BINARY);

    static void sortAndRender
    (class RenderDevice*                rd, 
     const class GCamera&               camera,
     const Array<SurfaceRef>&           allModels, 
     const LightingRef&                 lighting, 
     const Array< ReferenceCountedPointer<ShadowMap> >& shadowMaps);
    
    static void sortAndRender
    (RenderDevice*                      rd, 
     const GCamera&                     camera,
     const Array<SurfaceRef>&           allModels, 
     const LightingRef&                 lighting, 
     const ReferenceCountedPointer<ShadowMap>&  shadowMap = NULL);

    /** Render elements of modelArray, handling transmission reasonably.  Special cased
        code for refracting SuperSurface instances.  Called from sortAndRender().

        Assumes:

         - modelArray is sorted from back to front
         - shadowMapArray has the length of lighting->shadowedLightArray and contains
           already computed shadow maps

        Works correctly, but is inefficient, for non-translucent surfaces.

        \param alphaMode Mode for resolving partial coverage (which is independent of transmission)
      */
    static void renderTranslucent
    (RenderDevice*                  rd,
     const Array<Surface::Ref>&     modelArray,
     const Lighting::Ref&           lighting,
     const Array<SuperShader::PassRef>& extraAdditivePasses,
     const Array< ReferenceCountedPointer<ShadowMap> >&   shadowMapArray = Array<ShadowMap::Ref>(),
     RefractionQuality              maxRefractionQuality = RefractionQuality::BEST,
     AlphaMode                      alphaMode = ALPHA_BINARY);

    /** A hint to the renderer indicating that this surface should write to the depth buffer.  Typically overridden to return false for surfaces with very low
        partial coverage (alpha) or transmission values, or to resolve artifacts for specific scenes.  The default value is ! hasTransmission().*/
    virtual bool depthWriteHint(float distanceToCamera) const {
        (void)distanceToCamera;
        return ! hasTransmission();
    }
protected:

    /**
       Implementation must obey the current stencil, depth write, color write, and depth test modes.
       Implementation may freely set the blending, and alpha test modes.

       Default implementation renders the triangles returned by getIndices
       and getGeometry. 
    */
    virtual void defaultRender(RenderDevice* rd) const;
};


/////////////////////////////////////////////////////////////////

typedef ReferenceCountedPointer<class Surface2D> Surface2DRef;

/** Primarily for use in GUI rendering. */
class Surface2D : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Surface2D> Ref;

    /** Assumes that the RenderDevice is configured in in RenderDevice::push2D mode. */
    virtual void render(RenderDevice* rd) const = 0;

    /** Conservative 2D rendering bounds.
     */
    virtual Rect2D bounds() const = 0;

    /**
     2D objects are drawn from back to front, creating the perception of overlap.
     0 = closest to the front, 1 = closest to the back. 
     */
    virtual float depth() const = 0;

    /** Sorts from farthest to nearest. */
    static void sort(Array<Surface2DRef>& array);

    /** Calls sort, RenderDevice::push2D, and then render on all elements */
    static void sortAndRender(RenderDevice* rd, Array<Surface2DRef>& array);
};


/** @deprecated Use Surface */
typedef Surface PosedModel;

/** @deprecated Use Surface2D */
typedef Surface2D PosedModel2D;

}

#endif
