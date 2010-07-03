/** @file ArticulatedModel.h
    @author Morgan McGuire, http://graphics.cs.williams.edu
*/
#ifndef G3D_ArticulatedModel_h
#define G3D_ArticulatedModel_h

#include "G3D/Sphere.h"
#include "G3D/Box.h"
#include "G3D/Matrix4.h"
#include "G3D/Welder.h"
#include "G3D/Image1.h"
#include "GLG3D/VertexRange.h"
#include "GLG3D/Surface.h"
#include "GLG3D/Material.h"
#include "GLG3D/SuperSurface.h"
#include "GLG3D/Component.h"
#include "G3D/PhysicsFrameSpline.h"

namespace G3D {

class Any;

/**
 @brief A model composed of a hierarchy of rigid parts (i.e., a scene graph).

 The hierarchy may have multiple roots.  Renders efficiently using the
 static methods on Surface.  Surface recognizes
 ArticulatedModels explicitly and optimizes across them.  Rendering
 provides full effects including shadows, parallax mapping, and
 correct transparency. Use a custom SuperShader::Pass to add new
 effects.
 
 Loads <a href="http://www.the-labs.com/Blender/3DS-details.html">3DS</a>, 
 <a href="http://people.sc.fsu.edu/~jburkardt/data/obj/obj.html">OBJ</a>, <a href="http://www.riken.jp/brict/Yoshizawa/Research/PLYformat/PLYformat.html">PLY2</a>,
 <a href="http://local.wasp.uwa.edu.au/~pbourke/dataformats/ply/">PLY</a>, <a href="http://people.sc.fsu.edu/~jburkardt/data/off/off.html">OFF</a>, 
 <a href="http://www.mralligator.com/q3/">BSP</a>, and <a href="http://graphics.cs.brown.edu/games/brown-mesh-set/">IFS</a> files (with Articulatedmodel::fromFile), or
 you can create models (with ArticulatedModel::createEmpty) from code at
 run time.  You can also load a model and then adjust the materials
 explicitly.  See ArticulatedModel::Preprocess and
 ArticulatedModel::Setings for options.  See <a href="http://web.axelero.hu/karpo/">3D Object Converter</a> for 
 a shareware program that converts between many other file formats.

 Note that merging parts by material can dramatically improve the performance 
 of rendering imported models.  The easiest way to import models is to create a
 .am.any file for them and then specify merging and other transformations 
 in the data file instead of in code.

 BSP models loaded by ArticulatedModel will produce G3D::SuperSurface when posed,
 which is useful for advanced rendering algorithms.
 However, they will render less efficiently than with G3D::BSPMap because they do not
 perform viewer-dependent scene culling.
 
 Use the ArticulatedModel::Pose class to explicitly adjust the
 relationships between parts in the heirarchy.
 
 <b>Data Files</b>
 <br>
 To use on a pixel shader 2.0 or higher GPU, you will need the "SS_" shaders
 from the G3D/data/SuperShader directory.

 Since G3D doesn't load GIF files, any material in a 3DS file with a GIF filename is converted to
 the equivalent PNG filename.
 
 <b>Creating a Hierarchy</b>
 <br>
 <ol>
   <li> Call ArticulatedModel::createEmpty

   <li> Resize the ArticulatedModel::partArray to the desired number
		of parts (a part is a set of meshes that do not move
		relative to one another).
   <li> For each part in partArray,
     <ol>
       <li> Fill out Part::geometry.vertexArray and Part::texCoordArray data
       <li> Use Part::newTriList to extend the Part::triList
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

 When following this process, it is <i>not</i> necessary to call the
 other computeXXX methods on the Parts or to fill out the
 Part::geometry.normalArray, Part::indexArray, Part::tangentArray or
 Part::xxxVAR fields, or call TriList::computeBounds.
 
 <b>Known Bug:</b> 3DS Hierarchy is not properly parsed.  Re-exporting those files from 
 3DS Max tends to avoid the problem.  OBJ materials are not always loaded properly.
 */
class ArticulatedModel : public ReferenceCountedObject {
public:
    friend class Part;
	
    typedef ReferenceCountedPointer<class ArticulatedModel> Ref;

    enum {ALL = -5, USE_NAME = -6};


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


    class PoseSpline {
    public:
        typedef Table<std::string, PhysicsFrameSpline> SplineTable;
        SplineTable partSpline;

        PoseSpline();

        /**
         The Any must be a table mapping part names to PhysicsFrameSplines.
         Note that a single PhysicsFrame (or any equivalent of it) can serve as
         to create a PhysicsFrameSpline.  

         <pre>
            PoseSpline {
                "part1" = PhysicsFrameSpline {
                   control = ( Vector3(0,0,0),
                               CFrame::fromXYZYPRDegrees(0,1,0,35)),
                   cyclic = true
                },

                "part2" = Vector3(0,1,0)
            }
         </pre>
        */
        PoseSpline(const Any& any);
     
        /** Get the pose at time t, overriding values in \a pose that are specified by the spline. */
        void get(float t, ArticulatedModel::Pose& pose);
    };


    /**
     Parameters applied when G3D::ArticulatedModel::Part::computeNormalsAndTangentSpace 
     is caled by G3D::ArticulatedModel::updateAll.
     */
    class Settings {
    public:
        Welder::Settings                     weld;

        inline Settings() {}

        /** This forces "flat shading" on the model and causes it to render significantly
            slower than a smooth shaded object. However, it can be very useful for debugging in certain
            conditions and for rendering polyhedra.*/
        inline static Settings facet() {
            Settings s;
            s.weld.normalSmoothingAngle = 0;
            return s;
        }

        Settings(const Any& any);
        operator Any() const;
    };


    /**
      A named sub-set of the model that has a single reference frame.
      A Part's reference is relative to its parent's.

      Transparent rendering may produce artifacts if Parts are large
      or non-convex.
     */
    class Part {
    public:
        
        /** A set of triangles that have a single material and can be
            rendered as a single OpenGL primitive. */
        class TriList : public SuperSurface::GPUGeom {
        private:

            friend class Part;

            inline TriList() : GPUGeom(PrimitiveType::TRIANGLES, false) {}

        public:
            typedef ReferenceCountedPointer<TriList> Ref;

            /** CPU indices into the containing Part's VertexRange arrays for
                a triangle list. */
            Array<int>              indexArray;            
            
            /** Recomputes the GPUGeom bounds.  Called automatically by
                initIFS and init3DS.  Must be invoked manually if the
                geometry is later changed. */
            void computeBounds(const Part& parentPart);

            /** Called from Part::updateVAR(). Writes the GPUGeom fields */
            void updateVAR(VertexBuffer::UsageHint hint,
                           const VertexRange& vertexVAR,
                           const VertexRange& normalVAR,
                           const VertexRange& tangentVAR,
                           const VertexRange& texCoord0VAR);

        };

        /** Each part must have a unique name */
        std::string                 name;

        /** Position of this part's reference frame <B>relative to
            parent</B>.  During posing, any dynamically applied
            transformation at this part occurs after the cframe is
            applied.
        */
        CoordinateFrame             cframe;

        /** Copy of geometry.vertexArray stored on the GPU. Written by updateVAR.*/
        VertexRange                 vertexVAR;
		
        /** Copy of geometry.normalArray stored on the GPU. Written by updateVAR. */
        VertexRange                 normalVAR;
        
        /** Copy of packedTangentArray stored on the GPU.  Written by updateVAR.*/
        VertexRange                 packedTangentVAR;
        
        /** Copy of texCoordArray stored on the GPU.  Written by updateVAR.*/
        VertexRange                 texCoord0VAR;
        
        /** CPU geometry; per-vertex positions and normals.
            
            After changing, call updateVAR.  
            Note that you may call computeNormalsAndTangentSpace if you update the 
            vertices and texture coordinates but need updated tangents and normals
            computed for you.
        */
        MeshAlg::Geometry           geometry;
	
        /** CPU texture coordinates. */
        Array<Vector2>              texCoordArray;
        
        /** CPU per-vertex tangent vectors, typically computed by computeNormalsAndTangentSpace.
            Packs two tangents, T1 and T2 that form a reference frame with the normal such that 
            
            - \f$ \vec{x} = \vec{T}_1 = \vec{t}_{xyz}\f$ 
            - \f$ \vec{y} = \vec{T}_2 = \vec{t}_w * (\vec{n} \times \vec{t}_{xyz})  \f$
            - \f$ \vec{z} = \vec{n} \f$ 
        */
        Array<Vector4>              packedTangentArray;
		
        /** A collection of meshes that describe this part.*/
        Array<TriList::Ref>         triList;
        
        /** Indices into part array of sub-parts (scene graph children) in the containing model.*/
        Array<int>                  subPartArray;
        
        /** Index into the part array of the parent.  If -1, this is a root node. */
        int                         parent;

        /** Union of the index arrays for all triLists. This is not used for normal rendering. */
        Array<int>                  indexArray;

        Part() : parent(-1) {}

        /** Removes CPU geometry but retains GPU geometry (until next update()), cframe, and hierarchy.*/
        void removeGeometry() {
            triList.clear();
            packedTangentArray.clear();
            texCoordArray.clear();
            geometry.clear();
            indexArray.clear();
        }


        /** Creates a new tri list, adds it to the Part, and returns it.
            If \a mat is NULL, creates a default white material. */
        TriList::Ref newTriList(const Material::Ref& mat = NULL);

        /**
           Called by the various ArticulatedModel::render calls.
		 
         Does not restore rendering state when done.
         @param parent Object-to-World reference frame of parent.
         */
        void render(RenderDevice* rd, const CoordinateFrame& parent, const Pose& pose) const;

        /** Called by ArticulatedModel::pose */
        void pose(
            const ArticulatedModel::Ref& model,
            int                     partIndex,
            Array<Surface::Ref>&    posedArray,
            const CoordinateFrame&  parent, 
            const Pose&             posex) const;

        /** Some parts have no geometry because they are interior nodes in the hierarchy. */
        inline bool hasGeometry() const {
            return (geometry.vertexArray.size() > 0) || (vertexVAR.size() > 0);
        }

        /** Recomputes geometry.normalArray and tangentArray.

            Invoke when geometry.vertexArray has been changed. 
            Called from ArticulatedModel::updateAll().
			
			The Part::indexArray must be set before calling this
            (e.g., by calling computeIndexArray())*/
        void computeNormalsAndTangentSpace(const ArticulatedModel::Settings& settings);

        /** Called automatically by updateAll */
        void computeIndexArray();

        /** When geometry or texCoordArray is changed, invoke to
            update (or allocate for the first time) the VertexRange data.  You
            should either call updateNormals first, or write your own
            normals into the array in geometry before calling this.*/
        void updateVAR(VertexBuffer::UsageHint hint = VertexBuffer::WRITE_ONCE);

        /** Called automatically by updateAll
            Calls computeBounds on each triList in this Part's triListArray.*/
        void computeBounds();
    };

    /** Identifies an individual part, for use with G3D::ArticulatedModel::Operation. */
    class PartID {
    private:
        friend class ArticulatedModel;

        /** Note special values ALL and USE_NAME */
        int             m_index;
        std::string     m_name;

    public:

        PartID(const std::string& name);
        PartID(int index = ALL);
        PartID(const Any& any);

        /** True if this part is the ALL part.*/
        bool isAll() const {
            return m_index == ALL;
        }
    };

    /** For use with G3D::ArticulatedModel::Preprocess.  Defines
        a mini processing language.  A sample use is:
      
        <pre>
        ArticulatedModel::Specification {
           preprocess = ArticulatedModel::Preprocess {
              program = (
                  rename("toe", "root");
                  setMaterial("hand", #include("skin.mat"));
                  setTwoSided("eye", 1, true);
              )
           }
        }
        </pre>               

        This language contains zero or more statements.
        The legal statements are:

        <pre>
        partid   := string | number
        parts    := partid |  '(' partid [',' partid]* ')'
        trilist  := number
        trilists := trilist  |  '(' trilist [',' trilist]* ')'
        target   := partid  | parts ',' trilists

        Statements:

          <b>rename( partid, string );</b>

          <b>setMaterial( [ target ,] materialSpec );</b>

          <b>transform( [target ,] matrix4);</b>
            Transforms all of the vertices and normals of the targets, in their own object
            spaces.  Note that this is applied after the global preprocess transform

          <b>setTwoSided( [ target ,] boolean );</b>
          
          <b>setCFrame(parts, cframe);</b>

          <b>remove( target );</b>
            When triList is removed, it is replaced with a NULL triList.  The triList array
            is shrunk if there are no subsequent triLists, and the part geometry is removed
            if there are no triLists.  Thus indexing of valid triLists is not affected.

            When a part is removed, its geometry is wiped but the part and transform remain.

          <b>merge( partid [, partid]* );</b>
            Merge all trilists from all parts into the first trilist of the first part,
            obtaining its material and two-sided flag.  Then executes a remove on all but the first
            part and trilist.

          <b>merge()</b>
            Execute merge on all parts.

          <b>mergeByMaterial();</b>
            For each triList, merge all other triLists (of all parts) that have 
            the same material into it.
        </pre>
    */
    class Operation : public ReferenceCountedObject {
    public:
        typedef ReferenceCountedPointer<Operation> Ref;
    
        virtual void apply(ArticulatedModel::Ref model) = 0;

        static Ref create(const Any& any);
    };

    class MergeByMaterialOperation : public Operation {
    public:
        typedef ReferenceCountedPointer<Operation> Ref;
    
        virtual void apply(ArticulatedModel::Ref model);

        static Ref create(const Any& any);
    };

    class RenameOperation : public Operation {
    public:
        typedef ReferenceCountedPointer<RenameOperation> Ref;
        
        PartID              sourcePart;
        std::string         name;

        static Ref create(const Any& any);
        virtual void apply(ArticulatedModel::Ref model);
    };

    class SetCFrameOperation : public Operation {
    public:
        typedef ReferenceCountedPointer<SetCFrameOperation> Ref;
        
        Array<PartID>       sourcePart;
        CFrame              cframe;

        static Ref create(const Any& any);
        virtual void apply(ArticulatedModel::Ref model);
    };

    class TriListOperation : public Operation {
    protected:

        void parseTarget(const Any& any, int numContentArgs);
        virtual void process(ArticulatedModel::Ref model, int partIndex, Part& part) = 0;

    public:
        Array<PartID>       sourcePart;

        /** May be ALL */
        Array<int>          sourceTriList;
        virtual void apply(ArticulatedModel::Ref model);
    };

    /** Replaces the part with an empty part, so that numbering is unchanged */
    class RemoveOperation : public TriListOperation {
    protected:
        virtual void process(ArticulatedModel::Ref model, int partIndex, Part& part);
    public:
        typedef ReferenceCountedPointer<RemoveOperation> Ref;
        static Ref create(const Any& any);
    };

    class SetTwoSidedOperation : public TriListOperation {
    protected:
        virtual void process(ArticulatedModel::Ref model, int partIndex, Part& part);
    public:
        bool                twoSided;
        typedef ReferenceCountedPointer<SetTwoSidedOperation> Ref;
        static Ref create(const Any& any);
    };

    class SetMaterialOperation : public TriListOperation {
    protected:
        virtual void process(ArticulatedModel::Ref model, int partIndex, Part& part);
    public:
        Material::Ref       material;
        typedef ReferenceCountedPointer<SetMaterialOperation> Ref;
        static Ref create(const Any& any);
    };

    /** Transforms the geometry, but not the cframe or the sub-parts. */
    class TransformOperation : public Operation {
    protected:
        void transform(Part& p) const;
    public:
        Array<PartID>       sourcePart;
        Matrix4             xform;
        typedef ReferenceCountedPointer<TransformOperation> Ref;
        static Ref create(const Any& any);
        virtual void apply(ArticulatedModel::Ref model);
    };

    /**
        Merge all trilists from all parts into the first trilist of the first part,
        obtaining its material and two-sided flag.
    */
    class MergeOperation : public Operation {
    public:
        Array<PartID>       part;
        typedef ReferenceCountedPointer<MergeOperation> Ref;

        /** For use when parsing Anys */
        static Ref create(const Any& any);

        /** For explicit creation in C++ code */
        static Ref create() {
            return new MergeOperation();
        }

        virtual void apply(ArticulatedModel::Ref model);
    };

    /** @brief Options to apply while loading models. 
    
      You can use the @a xform parameter to scale, translate, and rotate 
      (or even invert!) the model as it is loaded. */
    class Preprocess {
    public:
        /** Removes all material properties while loading for cases where only geometry is desired. */
        bool                          stripMaterials;

        /** Transformation to apply to geometry after it is loaded. 
           Default is <b>Matrix4::identity()</b>*/
        Matrix4                       xform;

        /** If a material's diffuse texture is name X.Y and X-bump.* file exists,
            add that to the material as a bump map. Default is <b>false</b>.
            
            @beta May be generalized to support maps for all Material parameters 
            later. */
        bool                          addBumpMaps;

        /** For files that have normal/bump maps but no specification of the bump-map algorithm, 
            use this as the number of Material::parallaxSteps. Default is <b>0</b>
            (Blinn Normal Mapping) */
        int                           parallaxSteps;

        /** For files that have normal/bump maps but no specification of the elevation of the bump
            map, this is used. See Material::bumpMapScale. Default = 0.05.*/
        float                         bumpMapScale;

        /** When loading normal maps, argument used for G3D::GImage::computeNormalMap() whiteHeightInPixels.  Default is -0.02f */
        float                         normalMapWhiteHeightInPixels;

        /** During loading, whenever a material whose diffuse texture is named X is specified, it is automatically replaced
            with materialSubstitution[X].*/
        Table<std::string, Material::Ref> materialSubstitution;

        /** Override all materials with this one, if non-NULL.  This
            forces stripMaterials to be true.*/
        Material::Ref                 materialOverride;

        /** \sa ArticulatedModel::replaceTwoSidedWithGeometry */
        bool                          replaceTwoSidedWithGeometry;

        /** Operations are performed after all other transformations during loading, except for replaceTwoSidedWithGeometry. */
        Array<Operation::Ref>         program;
        
        Preprocess(const Any& any);
        operator Any() const;

        inline Preprocess() : stripMaterials(false), xform(Matrix4::identity()), addBumpMaps(false), parallaxSteps(0), bumpMapScale(0.05f), normalMapWhiteHeightInPixels(-0.02f), replaceTwoSidedWithGeometry(false) {}

        explicit inline Preprocess(const Matrix4& m) : stripMaterials(false), xform(m), addBumpMaps(false), parallaxSteps(0), bumpMapScale(0.05f), normalMapWhiteHeightInPixels(-0.02f), replaceTwoSidedWithGeometry(false) {}

        /** Initializes with a scale matrix */
        explicit inline Preprocess(const Vector3& scale) : stripMaterials(false), xform(Matrix4::scale(scale)), addBumpMaps(false), parallaxSteps(0), bumpMapScale(0.05f), normalMapWhiteHeightInPixels(-0.02f), replaceTwoSidedWithGeometry(false) {}
 
        /** Initializes with a scale matrix */
        explicit inline Preprocess(const float scale) : stripMaterials(false), xform(Matrix4::scale(scale)), addBumpMaps(false), parallaxSteps(0), bumpMapScale(0.05f), normalMapWhiteHeightInPixels(-0.02f), replaceTwoSidedWithGeometry(false) {}
    };


    /** \beta */
    class Specification {
    public:
        std::string     filename;
        Preprocess      preprocess;
        Settings        settings;

        Specification();

        Specification(const Any& any);
        operator Any() const;
    };
        

public:

    /** Identity transformation.*/
    static const Pose& defaultPose();


    /** All parts. Root parts are identified by (parent == -1).  
        It is assumed that each part has exactly
        one parent.
     */
    Array<Part>                 partArray;

    /** Total triangle count, updated by updateAll. */
    int                         m_numTriangles;

    /** 
 	 \brief Compute all mesh properties from a triangle soup of vertices with optional texture coordinates.

     Weld vertices, (re)compute vertex normals and tangent space basis, upload data to the GPU data, 
	 and update shaders on all Parts.  If you modify Part vertices explicitly, invoke this afterward 
	 to update dependent state. 
	
     This process is fairly slow and is usually only invoked once, either internally by fromFile() when
     the model is loaded, or explicitly by the programmer when a model is created procedurally.

     <b>You do not need to call this if you only change the materials and not the geometry.</b>
	*/
    void updateAll();

private:

    Settings                    m_settings;

    /** Called from the constructor */
    void init3DS(const std::string& filename, const Preprocess& preprocess);

    /** Called from the constructor */
    void initOBJ(const std::string& filename, const Preprocess& preprocess);

    /** Called from the constructor */
    void initIFS(const std::string& filename, const Matrix4& xform);

    /** Called from the constructor */
    void initBSP(const std::string& filename, const Preprocess& preprocess);

    /** Called from init3DS. 1st argument is a Load3DS::Material pointer; it is void* to work around
        a dependency problem of having Load3DS.h included here.

        @param path Current file load path*/
    static Material::Settings compute3DSMaterial
        (const void* material, 
         const std::string& path, 
         const Preprocess& preprocess);

public:

    /** Name of this model, for debugging purposes. */
    std::string                 name;

    /** Appends one posed model per sub-part with geometry.

        If the lighting environment is NULL the system will
        default using to whatever fixed function state is enabled
        (e.g., with renderDevice->setLight).  If the lighting
        environment is specified, the SuperShader will be used,
        providing detailed illuminaton.
    */
    void pose(
        Array<Surface::Ref>&  posedModelArray,
        const CoordinateFrame&   cframe = CoordinateFrame(),
        const Pose&              pose = defaultPose());
  

    /** Converts a part name to an index.  Returns -1 if the part name is not found.*/
    int partIndex(const PartID& id) const;

    inline const Settings& settings() const {
        return m_settings;
    }

    /** Sets the storage of all materials on this model. */
    void setStorage(ImageStorage s);

    inline void setSettings(const Settings& s) {
        m_settings = s;
    }

    /** Triangle count for the whole model. */
    inline int numTriangles() const {
        return m_numTriangles;
    }

    /** @deprecated */
    static ArticulatedModel::Ref fromFile(const std::string& filename, const Matrix4& xform);

    /** @deprecated */
    static ArticulatedModel::Ref fromFile(const std::string& filename, const CFrame& xform);

    /** @deprecated */
    static ArticulatedModel::Ref fromFile(const std::string& filename, const Vector3& scale);

    /** @deprecated */
    static ArticulatedModel::Ref fromFile(const std::string& filename, float scale);

    /**
       @brief Load a 3D model from disk, optionally applying some processing.

       Supports 3DS, OBJ, IFS, OFF, BSP, PLY, and PLY2 file formats.  The format of a file is 
       detected by the extension.   
     */
    static ArticulatedModel::Ref fromFile
        (const std::string&  filename, 
         const Preprocess&   preprocess = Preprocess(),
         const Settings&     settings   = Settings());

    static ArticulatedModel::Ref create(const Specification& s) {
        return fromFile(s.filename, s.preprocess, s.settings);
    }

    static ArticulatedModel::Ref createHeightfield(const Image1::Ref& height, float xzExtent = 10.0f, float yExtent = 1.0f, const Vector2& texScale = Vector2(1.0, 1.0));

    static ArticulatedModel::Ref createHeightfield(const std::string& filename, float xzExtent = 10.0f, float yExtent = 1.0f, const Vector2& texScale = Vector2(1.0, 1.0)) {
        return createHeightfield(Image1::fromFile(filename), xzExtent, yExtent, texScale);
    }
    
    /**
     Creates a new model, on which you can manually build geometry by editing the 
     partArray directly. 
     */
    static ArticulatedModel::Ref createEmpty();

    /** Create a 0.5^3 meter cube with colored sides, approximating the data from
        http://www.graphics.cornell.edu/online/box/data.html
        
        @param scale Enlarge the box by this factor.
        */
    static ArticulatedModel::Ref createCornellBox(
        float         scale      = 1.0f, 
        const Color3& leftColor  = Color3::fromARGB(0xB82C1F), 
        const Color3& rightColor = Color3::fromARGB(0x6AB8B8),
        const Color3& backColor  = Color3::white() * 0.72f);

    /**
     Iterate through the entire model and force all triangles to use vertex 
     normals instead of face normals.

     This forces "flat shading" on the model and causes it to render significantly
     slower than a smooth shaded object. However, it can be very useful for debugging in certain
     conditions.

     @deprecated
     */
    void facet();

    /** Doubles the geometry for any twoSided triList and then removes the flag. 
      You need to call updateAll() after invoking this. */
    void replaceTwoSidedWithGeometry();
};

}

#endif
