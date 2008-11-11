/** @file ArticulatedModel.h
    @author Morgan McGuire
*/
#ifndef G3D_ARTICULATEDMODEL_H
#define G3D_ARTICULATEDMODEL_H

#include "G3D/Sphere.h"
#include "G3D/Box.h"
#include "GLG3D/VAR.h"
#include "GLG3D/PosedModel.h"
#include "GLG3D/SuperShader.h"

namespace G3D {

typedef ReferenceCountedPointer<class ArticulatedModel> ArticulatedModelRef;

/**
 A model composed of a hierarchy of rigid parts (i.e., a scene graph).  The hierarchy may have multiple roots.  
 Renders efficiently using the static methods on PosedModel.  PosedModel recognizes ArticulatedModels explicitly and
 optimizes across them.
 Rendering provides full effects including shadows, parallax mapping, and correct transparency. Use a custom SuperShader::Pass 
 to add new effects.
 
 Loads 3DS and IFS files (Articulatedmodel::fromFile), or you can create 
 models (ArticulatedModel::createEmpty) from code at run time.  You can also load a model and then adjust
 the materials explicitly. 
 
 Use the ArticulatedModel::Pose class to explicitly adjust the relationships between parts in the
 heirarchy.
 
 <b>Data Files</b>
 <br>
 To use on a pixel shader 2.0 or higher GPU, you will need the four files in the current directory
 at runtime:

 <ul>
 <li>ShadowMappedLightPass.vrt
 <li>ShadowMappedLightPass.pix
 <li>NonShadowedPass.vrt
 <li>NonShadowedPass.pix
 </ul>

 These are located in the data/SuperShader directory of the G3D distribution.

 Since G3D doesn't load GIF files, any material in a 3DS file with a GIF filename is converted to
 the equivalent PNG filename.
 
 <b>Creating a Heirarchy</b>
 <br>
 <ol>
   <li> Call ArticulatedModel::createEmpty
   <li> Resize the ArticulatedModel::partArray to the desired number of parts (a part is a set of meshes 
		that do not move relative to one another).
   <li> For each part in partArray,
     <ol>
       <li> Fill out Part::geometry.vertexArray, Part::triListArray, and Part::texCoordArray data
       <li> Set the Part::name, which must be unique
       <li> For each tri list in the part (a tri list is a mesh with a G3D::Material),
          <ol>
             <li> Complete the Part::TriList::indexArray and other fields except for the bounds.
          </ol>
       <li> Set the index of the Part::parent Part
	   <li> Set the indices of the child Parts in the Part::subPartArray
     </ol> 
   <li> Call ArticulatedModel::updateall
 </ol>

 When following this process, it is <i>not</i> necessary to call the other computeXXX methods on the Parts or to 
 fill out the Part::geometry.normalArray, Part::indexArray, Part::tangentArray or Part::xxxVAR fields, or
call TriList::computeBounds.
 
 <b>Known Bug:</b> rotations are loaded incorrectly for a small number of older 3DS files.  These files
 will have parts located in the wrong position.  Re-exporting those files from 3DS Max tends to fix the
 problem.
 */
class ArticulatedModel : public ReferenceCountedObject {
public:
	
    typedef ReferenceCountedPointer<class ArticulatedModel> Ref;

private:

    friend class PosedArticulatedModel;
    friend class Part;

public:

    /** Incremented every time sendGeometry is invoked on any posed piece of an ArticulatedModel. 
	    Used for performance profiling.
	   */
    static int debugNumSendGeometryCalls;

    /** Called by PosedModel.
	 
	    Renders an array of PosedAModels in the order that they appear in the array, taking advantage of 
        the fact that all objects have the same subclass to optimize the rendering calls.*/
    static void renderNonShadowed(
        const Array<PosedModel::Ref>& posedArray, 
        RenderDevice* rd, 
        const LightingRef& lighting);

    /** Called by PosedModel.
	 
	    Renders an array of PosedAModels in the order that they appear in the array, taking advantage of 
        the fact that all objects have the same subclass to optimize the rendering calls.*/
    static void renderShadowMappedLightPass(
        const Array<PosedModel::Ref>& posedAModelArray, 
        RenderDevice* rd, 
        const GLight& light, 
        const ShadowMapRef& shadowMap);

    /** Called by PosedModel.
	 
	    Removes the opaque PosedAModels from array @a all and appends them to the opaqueAmodels array (transparents
        must be rendered inline with other model types).
        This produces an array for the array versions of renderNonShadowed and renderShadowMappedLightPass. 
        */
    static void extractOpaquePosedAModels(
        Array<PosedModel::Ref>& all, 
        Array<PosedModel::Ref>& opaqueAmodels);

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

    /** Returns a measure of the capabilities of this machine. This is computed during the
	    first rendering and cached. */
    static GraphicsProfile profile();

    /** Force articulated model to use a different profile.  Only works if called
        before any models are loaded; used mainly for debugging. */
    static void setProfile(GraphicsProfile p);

	/** Specifies the transformation that occurs at each node in the heirarchy. 
	  */
    class Pose {
    public:
        /** Mapping from names to coordinate frames (relative to parent).
            If a name is not present, its coordinate frame is assumed to
            be the identity.
         */
        Table<std::string, CoordinateFrame>     cframe;

        Pose() {}
    };

	/** Identity transformation.*/
    static const Pose DEFAULT_POSE;

    /**
      A named sub-set of the model that has a single reference frame.  A Part's reference
      is relative to its parent's.

      Transparent rendering may produce artifacts if Parts are large or non-convex. 
     */
    class Part {
    public:
        
        /** A set of triangles that have a single material and can be rendered as a 
            single OpenGL primitive. */
        class TriList {
        public:
            Array<int>              indexArray;
            
            /** When true, this trilist enables two-sided lighting and texturing and
                does not cull back faces.*/
            bool                    twoSided;
            
            Material                material;
            
            /** In the same space as the vertices. Computed by computeBounds() */
            Sphere                  sphereBounds;
            
            /** In the same space as the vertices. Computed by computeBounds() */
            Box                     boxBounds;
            
            TriList() : twoSided(false) {}
            
            /** Recomputes the bounds.  Called automatically by initIFS and init3DS.
                Must be invoked manually if the geometry is later changed. */
            void computeBounds(const Part& parentPart);
        };

        /** Each part must have a unique name */
        std::string                 name;

        /** Position of this part's reference frame <B>relative to parent</B>.
            During posing, any dynamically applied transformation at this part
            occurs after the cframe is applied.
        */
        CoordinateFrame             cframe;

        /** Copy of geometry.vertexArray stored on the GPU. Written by updateVAR.*/
        VAR                         vertexVAR;
		
        /** Copy of geometry.normalArray stored on the GPU. Written by updateVAR. */
        VAR                         normalVAR;
        
        /** Copy of tangentArray stored on the GPU.  Written by updateVAR.*/
        VAR                         tangentVAR;
        
        /** Copy of texCoordArray stored on the GPU.  Written by updateVAR.*/
        VAR                         texCoord0VAR;
        
        /** CPU geometry; per-vertex positions and normals.
            
            After changing, call updateVAR.  
            Note that you may call computeNormalsAndTangentSpace if you update the 
            vertices and texture coordinates but need updated tangents and normals
            computed for you.
        */
        MeshAlg::Geometry           geometry;
	
        /** CPU texture coordinates. */
        Array<Vector2>              texCoordArray;
        
        /** CPU per-vertex tangent vectors, typically computed by computeNormalsAndTangentSpace. */
        Array<Vector3>              tangentArray;
		
        /** A collection of meshes that describe this part.*/
        Array<TriList>              triListArray;
        
        /** Indices into part array of sub-parts (scene graph children) in the containing model.*/
        Array<int>                  subPartArray;
        
        /** Index into the part array of the parent.  If -1, this is a root node. */
        int                         parent;

        /** Union of the index arrays for all triLists. */
        Array<int>                  indexArray;

        inline Part() : parent(-1) {}

        /**
		 Called by the various ArticulatedModel::render calls.
		 
         Does not restore rendering state when done.
         @param parent Object-to-World reference frame of parent.
         */
        void render(RenderDevice* rd, const CoordinateFrame& parent, const Pose& pose) const;

        /** Called by ArticulatedModel::pose */
        void pose(
            ArticulatedModelRef     model,
            int                     partIndex,
            Array<PosedModel::Ref>& posedArray,
            const CoordinateFrame&  parent, 
            const Pose&             posex) const;

        /** Some parts have no geometry because they are interior nodes in the hierarchy. */
        inline bool hasGeometry() const {
            return geometry.vertexArray.size() > 0;
        }

        /** Recomputes geometry.normalArray and tangentArray.

            Invoke when geometry.vertexArray has been changed. 
            Called from ArticulatedModel::updateAll().
			
			The Part::indexArray must be set before calling this
            (e.g., by calling computeIndexArray())*/
        void computeNormalsAndTangentSpace();

        /** Called automatically by updateAll */
        void computeIndexArray();

        /** When geometry or texCoordArray is changed, invoke to
            update (or allocate for the first time) the VAR data.  You
            should either call updateNormals first, or write your own
            normals into the array in geometry before calling this.*/
        void updateVAR(VARArea::UsageHint hint = VARArea::WRITE_ONCE);

		/** Called automatically by updateAll
			Calls computeBounds on each triList in this Part's triListArray.*/
		void computeBounds();
    };

    /** All parts. Root parts are identified by (parent == -1).
     */
    Array<Part>                 partArray;

    /** 
 	 @brief Compute all mesh properties from a triangle soup of vertices with optional texture coordinates.

     Weld vertices, (re)compute vertex normals and tangent space basis, upload data to the GPU data, 
	 and update shaders on all Parts.  If you modify Part vertices explicitly, invoke this afterward 
	 to update dependent state. 
	
     This process is fairly slow and is usually only invoked once, either internally by fromFile() when
     the model is loaded, or explicitly by the programmer when a model is created procedurally.
	*/
    void updateAll();

private:

    /** Called from the constructor */
    void init3DS(const std::string& filename, const Matrix4& xform);

    /** Called from the constructor */
    void initIFS(const std::string& filename, const Matrix4& xform);

public:

	/** Name of this model for debugging purposes. */
    std::string                 name;

    /** Appends one posed model per sub-part with geometry.

        If the lighting environment is NULL the system will
        default using to whatever fixed function state is enabled
        (e.g., with renderDevice->setLight).  If the lighting
        environment is specified, the SuperShader will be used,
        providing detailed illuminaton.
    */
    void pose(
        Array<PosedModel::Ref>&  posedModelArray,
        const CoordinateFrame&   cframe = CoordinateFrame(),
        const Pose&              pose = DEFAULT_POSE);

    /** 
      Supports 3DS, IFS, and PLY2 file formats.  The format of a file is detected by the extension. 

      You can use the @a xform parameter to scale, translate, and rotate (or even invert!) the model
      as it is loaded.  Example:
      <pre>
      CoordinateFrame xform(Matrix3::fromAxisAngle(axis, angle) * scale, translation);
      model = ArticulatedModel::fromFile(filename, xform);
      </pre>

      @param xform Transform all vertices by this RST matrix during loading loading
      @deprecated Use the Matrix4 version
      */
    static ArticulatedModelRef fromFile(const std::string& filename, const CoordinateFrame& xform);

    /**
      Supports 3DS, IFS, and PLY2 file formats.  The format of a file is detected by the extension. 

      You can use the @a xform parameter to scale, translate, and rotate (or even invert!) the model
      as it is loaded.  Example:
      <pre>
      Matrix4 xform(Matrix3::fromAxisAngle(axis, angle) * scale, translation);
      model = ArticulatedModel::fromFile(filename, xform);
      </pre>

      @param xform Transform all vertices by this matrix during loading 
     */
    static ArticulatedModelRef fromFile(const std::string& filename, const Matrix4& xform);

    static ArticulatedModelRef fromFile(const std::string& filename, const Vector3& scale);

    /**
     Creates a new model, on which you can manually build geometry by editing the partArray directly. 
     */
    static ArticulatedModelRef createEmpty();

    /** Load a model from disk */
    static ArticulatedModelRef fromFile(const std::string& filename, float scale = 1.0);

    /** Create a 0.5^3 meter cube with colored sides, approximating the data from
        http://www.graphics.cornell.edu/online/box/data.html */
    static ArticulatedModelRef createCornellBox();

    /**
     Iterate through the entire model and force all triangles to use vertex 
     normals instead of face normals.

     This forces "flat shading" on the model and causes it to render significantly
     slower than normal. However, it can be very useful for debugging in certain
     conditions.

     @beta
     */
    void facet();
};

const char* toString(ArticulatedModel::GraphicsProfile p);

}

#endif //G3D_ARTICULATEDMODEL
