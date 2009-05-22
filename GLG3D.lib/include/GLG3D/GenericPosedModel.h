/**
  @file GenericPosedModel.h

  @maintainer Morgan McGuire, morgan@cs.williams.edu

  @created 2008-11-12
  @edited  2009-03-30
*/
#ifndef G3D_GenericPosedModel_h
#define G3D_GenericPosedModel_h

#include "G3D/platform.h"
#include "G3D/System.h"
#include "G3D/Array.h"
#include "G3D/Vector3.h"
#include "G3D/Vector2.h"
#include "G3D/MeshAlg.h"
#include "G3D/Sphere.h"
#include "G3D/AABox.h"
#include "G3D/constants.h"
#include "GLG3D/Material.h"
#include "GLG3D/VAR.h"
#include "GLG3D/Lighting.h"
#include "GLG3D/PosedModel.h"

namespace G3D {

/**
   @brief An optimized implementation G3D::PosedModel for
   G3D::SuperShader / G3D::Material classes.

   Used by G3D::ArticulatedModel.
 */
class GenericPosedModel : public PosedModel {
public:

    typedef ReferenceCountedPointer<GenericPosedModel> Ref;

    /** Allocates with System::malloc to avoid the performance
        overhead of creating lots of small heap objects using
        ::malloc. */
    static void* operator new(size_t size) {
        return System::malloc(size);
    }

    static void operator delete(void* p) {
        System::free(p);
    }

    /** @brief A GPU mesh utility class that works with G3D::GenericPosedModel.
        
        A set of lines, points, quads, or triangles that have a
        single Material and can be rendered as a single OpenGL
        primitive using RenderDevice::sendIndices inside a
        RenderDevice::beginIndexedPrimitives() block.
        
        @sa G3D::MeshAlg, G3D::ArticulatedModel, G3D::PosedModel
    */
    class GPUGeom : public ReferenceCountedObject {
    public:
        typedef ReferenceCountedPointer<GPUGeom> Ref;

        MeshAlg::Primitive      primitive;
    
        /** Indices into the VARs. */
        VAR                     index;
        VAR                     vertex;
        VAR                     normal;
        VAR                     tangent;
        VAR                     texCoord0;
        
        /** When true, this primitive should be rendered with
            two-sided lighting and texturing and not cull
            back faces. */
        bool                    twoSided;

        /** Preferred level of refraction quality. The actual level available depends on the renderer.*/
        RefractionQuality       refractionHint;

        /** Preferred level of mirror reflection quality. The actual level available depends on the renderer.*/
        MirrorQuality           mirrorHint;
        
        Material::Ref           material;

        /** Object space bounds */
        AABox                   boxBounds;

        /** Object space bounds */
        Sphere                  sphereBounds;
        
    protected:
        
        inline GPUGeom(PrimitiveType p, bool t, RefractionQuality r, MirrorQuality m) : 
            primitive(p), twoSided(t), refractionHint(r), mirrorHint(m) {}
        
    public:

        inline static Ref create(PrimitiveType p = PrimitiveType::TRIANGLES) {
            return new GPUGeom(p, false, RefractionQuality::DYNAMIC_FLAT, MirrorQuality::STATIC_ENV);
        }
    };


    class CPUGeom {
    public:
        const Array<int>*        index;
        const MeshAlg::Geometry* geometry;
        const Array<Vector3>*    tangent;
        const Array<Vector2>*    texCoord0;
        
        inline CPUGeom(
                const Array<int>* index,
                const MeshAlg::Geometry* geometry,
                const Array<Vector2>* texCoord0,
                const Array<Vector3>* tangent = NULL) : 
            index(index), geometry(geometry), tangent(tangent), 
            texCoord0(texCoord0) {}

        inline CPUGeom() : index(NULL), geometry(NULL), tangent(NULL), texCoord0(NULL) {}
    };

protected:

    std::string             m_name;

    /** Object to world space transformation. */
    CoordinateFrame         m_frame;
    
    GPUGeom::Ref            m_gpuGeom;

    CPUGeom                 m_cpuGeom;

    ReferenceCountedPointer<ReferenceCountedObject> m_source;

    inline GenericPosedModel
    (const std::string&       name,
     const CFrame&            frame, 
     const GPUGeom::Ref&      gpuGeom,
     const CPUGeom&           cpuGeom,
     const ReferenceCountedPointer<ReferenceCountedObject>& source = NULL) :
        m_name(name),
        m_frame(frame), m_gpuGeom(gpuGeom), m_cpuGeom(cpuGeom),
        m_source(source) {}

    /** Set object to world and then draw geometry.  Called from
        render to draw geometry after the material properties are
        set.
    */
    void sendGeometry2(RenderDevice* rd) const;

    /** Renders emission, reflection, and lighting for non-shadowed
        lights.  The first term rendered uses the current
        blending/depth mode and subsequent terms use additive
        blending.  Returns true if anything was rendered, false if
        nothing was rendered (because all terms were black).
    
        Called from renderNonShadowedOpaqueTerms.
    */ 
    bool renderFFNonShadowedOpaqueTerms(
        RenderDevice*                   rd,
        const LightingRef&              lighting) const;
    
    bool renderPS14NonShadowedOpaqueTerms(
        RenderDevice*                   rd,
        const LightingRef&              lighting) const;

    bool renderPS20NonShadowedOpaqueTerms(
        RenderDevice*                   rd,
        const LightingRef&              lighting) const;

    /** Switches between rendering paths.  Called from renderNonShadowed.*/
    bool renderNonShadowedOpaqueTerms(
        RenderDevice*                   rd,
        const LightingRef&              lighting,
        bool                            preserveState) const;

    void renderFFShadowMappedLightPass(
        RenderDevice*                   rd,
        const GLight&                   light,
        const ShadowMapRef&             shadowMap) const;

    void renderPS20ShadowMappedLightPass(
        RenderDevice*                   rd,
        const GLight&                   light,
        const ShadowMapRef&             shadowMap) const;

public:

    inline const GPUGeom::Ref& gpuGeom() const {
        return m_gpuGeom;
    }

    virtual const CPUGeom& cpuGeom() const {
        return m_cpuGeom;
    }

    /** Classification of a graphics card. 
        FIXED_FUNCTION  Use OpenGL fixed function lighting only.
        PS14            Use pixel shader 1.4 (texture crossbar; adds specular maps)
        PS20            Use pixel shader 2.0 (shader objects; full feature)
	 
        @sa profile()
     */
    enum GraphicsProfile {
        UNKNOWN = 0,
        FIXED_FUNCTION,
        PS14,
        PS20};

    /** Called by PosedModel.
	 
        Renders an array of GenericPosedModels in the order that they
        appear in the array, taking advantage of the fact that all
        objects have the same subclass to optimize the rendering
        calls.*/
    static void renderNonShadowed(
        const Array<PosedModel::Ref>& posedArray, 
        RenderDevice* rd, 
        const LightingRef& lighting);

    /** Called by PosedModel.
	 
        Renders an array of GenericPosedModels in the order that they
        appear in the array, taking advantage of the fact that all
        objects have the same subclass to optimize the rendering
        calls.*/
    static void renderShadowMappedLightPass(
        const Array<PosedModel::Ref>& posedArray, 
        RenderDevice* rd, 
        const GLight& light, 
        const ShadowMapRef& shadowMap);

    /** Called by PosedModel.
	 
        Removes the opaque GenericPosedModels from array @a all and appends
        them to the opaqueAmodels array (transparents must be rendered
        inline with other model types).  This produces an array for
        the array versions of renderNonShadowed and
        renderShadowMappedLightPass.
        */
    static void extractOpaque(
        Array<PosedModel::Ref>& all, 
        Array<PosedModel::Ref>& genericModels);

    /** Returns a measure of the capabilities of this machine. This is
        computed during the first rendering and cached. */
    static GraphicsProfile profile();

    /** Force GenericPosedModel to use a different profile.  Only
        works if called before any models are loaded; used mainly for
        debugging. */
    static void setProfile(GraphicsProfile p);

    /** Incremented every time sendGeometry is invoked on any GenericPosedModel. 
        Used for performance profiling. Manually set to zero.
    */
    static int debugNumSendGeometryCalls;

    /** \param source An object to hold a strong pointer to to prevent it from being
        garbage collected.  This is useful because m_cpuGeom often
        contains pointers into an object that may not be held by
        anything else. Note that any ReferenceCountedPointer will automatically
        upcast to this type.*/
    static Ref create
    (const std::string&       name,
     const CFrame&            frame, 
     const GPUGeom::Ref&      gpuGeom,
     const CPUGeom&           cpuGeom = CPUGeom(),
     const ReferenceCountedPointer<ReferenceCountedObject>& source = NULL);

    virtual void sendGeometry(RenderDevice* rd) const;

    virtual std::string name() const;

    virtual bool hasTransparency() const;

    virtual void getCoordinateFrame(CoordinateFrame& c) const;

    virtual const MeshAlg::Geometry& objectSpaceGeometry() const;

    virtual const Array<Vector3>& objectSpaceFaceNormals(bool normalize = true) const;

    virtual const Array<MeshAlg::Face>& faces() const;

    virtual const Array<MeshAlg::Edge>& edges() const;

    virtual const Array<MeshAlg::Vertex>& vertices() const;

    virtual const Array<Vector2>& texCoords() const;

    virtual bool hasTexCoords() const;

    virtual const Array<MeshAlg::Face>& weldedFaces() const;

    virtual const Array<MeshAlg::Edge>& weldedEdges() const;

    virtual const Array<MeshAlg::Vertex>& weldedVertices() const;

    virtual const Array<int>& triangleIndices() const;

    virtual void getObjectSpaceBoundingSphere(Sphere&) const;

    virtual void getObjectSpaceBoundingBox(AABox&) const;

    virtual void render(RenderDevice* renderDevice) const;
    
    virtual void renderNonShadowed(RenderDevice* rd, const LightingRef& lighting) const;
    
    virtual void renderShadowedLightPass(RenderDevice* rd, const GLight& light) const;

    virtual void renderShadowMappedLightPass(RenderDevice* rd, const GLight& light, const Matrix4& lightMVP, const Texture::Ref& shadowMap) const;

    virtual void renderShadowMappedLightPass(RenderDevice* rd, const GLight& light, const ShadowMapRef& shadowMap) const;

    virtual int numBoundaryEdges() const;

    virtual int numWeldedBoundaryEdges() const;

    virtual const Array<Vector3>& objectSpaceTangents() const;
   
    virtual bool renderSuperShaderPass(
        RenderDevice* rd, 
        const SuperShader::PassRef& pass) const;
};

const char* toString(GenericPosedModel::GraphicsProfile p);

} // G3D

#endif
