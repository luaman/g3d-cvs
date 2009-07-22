/** @file ArticulatedModel.h
    @author Morgan McGuire, morgan@cs.williams.edu
*/
#ifndef G3D_ArticulatedModel_h
#define G3D_ArticulatedModel_h

#include "G3D/Sphere.h"
#include "G3D/Box.h"
#include "G3D/Matrix4.h"
#include "G3D/Welder.h"
#include "GLG3D/VertexRange.h"
#include "GLG3D/Surface.h"
#include "GLG3D/Material.h"
#include "GLG3D/SuperSurface.h"
#include "GLG3D/Component.h"

namespace G3D {

/**
 @brief A model composed of a hierarchy of rigid parts (i.e., a scene graph).

 The hierarchy may have multiple roots.  Renders efficiently using the
 static methods on Surface.  Surface recognizes
 ArticulatedModels explicitly and optimizes across them.  Rendering
 provides full effects including shadows, parallax mapping, and
 correct transparency. Use a custom SuperShader::Pass to add new
 effects.
 
 Loads 3DS, PLY2, OFF, and IFS files (Articulatedmodel::fromFile), or
 you can create models (ArticulatedModel::createEmpty) from code at
 run time.  You can also load a model and then adjust the materials
 explicitly.  See ArticulatedModel::PreProcess and
 ArticulatedModel::Setings for options.
 
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
 
 <b>Known Bug:</b> rotations are loaded incorrectly for a small number
 of older 3DS files.  These files will have parts located in the wrong
 position.  Re-exporting those files from 3DS Max tends to fix the
 problem.
 */
class ArticulatedModel : public ReferenceCountedObject {
public:
	
    typedef ReferenceCountedPointer<class ArticulatedModel> Ref;

    /** @brief Options to apply while loading models. 
    
      You can use the @a xform parameter to scale, translate, and rotate 
      (or even invert!) the model as it is loaded. */
    class PreProcess {
    public:

        /** Default is <b>Texture::DIM_2D</b>.  Use Texture::DIM_2D_NPOT to load non-power 
            of 2 textures without rescaling them.*/
        Texture::Dimension            textureDimension;

        /** If a material's diffuse texture is name X.Y and X-bump.* file exists,
            add that to the material as a bump map. Default is <b>false</b>.
            
            @beta May be generalized to support maps for all Material parameters 
            later. */
        bool                          addBumpMaps;

        /** Transformation to apply to geometry after it is loaded. 
           Default is <b>Matrix4::identity()</b>*/
        Matrix4                       xform;

        /** For files that have normal/bump maps but no specification of the bump-map algorithm, 
            use this as the number of Material::parallaxSteps. Default is <b>0</b>
            (Blinn Normal Mapping) */
        int                           parallaxSteps;

        /** For files that have normal/bump maps but no specification of the elevation of the bump
            map, this is used. See Material::bumpMapScale. Default = 0.05.*/
        float                         bumpMapScale;

        /** When loading normal maps, argument used for G3D::GImage::computeNormalMap() whiteHeightInPixels.  Default is -0.02f */
        float                         normalMapWhiteHeightInPixels;

        inline PreProcess() : textureDimension(Texture::DIM_2D), addBumpMaps(false), xform(Matrix4::identity()), parallaxSteps(0), bumpMapScale(0.05f), normalMapWhiteHeightInPixels(-0.02f) {}

        explicit inline PreProcess(const Matrix4& m) : textureDimension(Texture::DIM_2D), addBumpMaps(false), xform(m), parallaxSteps(0), bumpMapScale(0.05f), normalMapWhiteHeightInPixels(-0.02f) {}

        /** Initializes with a scale matrix */
        explicit inline PreProcess(const Vector3& scale) : textureDimension(Texture::DIM_2D), addBumpMaps(false), xform(Matrix4::scale(scale)), parallaxSteps(0), bumpMapScale(0.05f), normalMapWhiteHeightInPixels(-0.02f) {}
 
        /** Initializes with a scale matrix */
        explicit inline PreProcess(const float scale) : textureDimension(Texture::DIM_2D), addBumpMaps(false), xform(Matrix4::scale(scale)), parallaxSteps(0), bumpMapScale(0.05f), normalMapWhiteHeightInPixels(-0.02f) {}
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
    };

private:

    friend class Part;

public:

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
    static const Pose& defaultPose();

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

            inline TriList() : GPUGeom(PrimitiveType::TRIANGLES, false, RefractionQuality::DYNAMIC_FLAT, MirrorQuality::STATIC_ENV) {}

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
        VertexRange                         vertexVAR;
		
        /** Copy of geometry.normalArray stored on the GPU. Written by updateVAR. */
        VertexRange                         normalVAR;
        
        /** Copy of tangentArray stored on the GPU.  Written by updateVAR.*/
        VertexRange                         tangentVAR;
        
        /** Copy of texCoordArray stored on the GPU.  Written by updateVAR.*/
        VertexRange                         texCoord0VAR;
        
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
        Array<TriList::Ref>         triList;
        
        /** Indices into part array of sub-parts (scene graph children) in the containing model.*/
        Array<int>                  subPartArray;
        
        /** Index into the part array of the parent.  If -1, this is a root node. */
        int                         parent;

        /** Union of the index arrays for all triLists. This is not used for normal rendering. */
        Array<int>                  indexArray;

        inline Part() : parent(-1) {}

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
            return geometry.vertexArray.size() > 0;
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
    void init3DS(const std::string& filename, const PreProcess& preprocess);

    /** Called from the constructor */
    void initIFS(const std::string& filename, const Matrix4& xform);

    /** Called from init3DS. 1st argument is a Load3DS::Material pointer; it is void* to work around
        a dependency problem of having Load3DS.h included here.

        @param path Current file load path*/
    static Material::Settings compute3DSMaterial
        (const void* material, 
         const std::string& path, 
         const PreProcess& preprocess);

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
        Array<Surface::Ref>&  posedModelArray,
        const CoordinateFrame&   cframe = CoordinateFrame(),
        const Pose&              pose = defaultPose());
  

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

       Supports 3DS, IFS, OFF, and PLY2 file formats.  The format of a file is 
       detected by the extension.      
     */
    static ArticulatedModel::Ref fromFile
        (const std::string&  filename, 
         const PreProcess&   preprocess = PreProcess(),
         const Settings&     settings   = Settings());

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
};

}

#endif
