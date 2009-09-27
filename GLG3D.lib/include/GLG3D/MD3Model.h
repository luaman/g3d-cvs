/**
 @file MD3Model.h

 Quake III MD3 model loading and posing

 */

//#if 1

#ifndef G3D_MD3Model_h
#define G3D_MD3Model_h

#include "G3D/platform.h"
#include "G3D/Matrix3.h"
#include "G3D/Table.h"
#include "GLG3D/Surface.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/Texture.h"

namespace G3D {

// forward declaration of md3 struct
struct MD3SurfaceHeader;

/**
    Quake III MD3 model loader.

    Quake 3 uses MD3 models for both characters and non-character objects.  
    Character objects contain three individual "models" inside of them with attachment points.

    TODO: 
    - Fix transform problems with attachment points
    - Rename this to MD3Model::Part, and then make an MD3::Model class that loads all of the part [Corey]
    - Render using SuperSurface [Morgan]
    - Change scale: 60 quake units ~= 3 meters
    - Set up Pose animation for player models ala MD2Model.  See http://www.misfitcode.com/misfitmodel3d/olh_quakemd3.html for the constants for each animation.
 */
class MD3Model : public ReferenceCountedObject {
    // See: http://icculus.org/homepages/phaethon/q3a/formats/md3format.html

    // Terminology:
    //
    // Q3 calls an attachment point a "tag"
    //
    // Player models contain lower.md3, upper.md3, and head.md3.  The lower part is the root.
    // the upper is attached to the lower, and the weapon and head are attached to the upper.
    //
    // Q3 calls the individual parts of the model "surfaces", so in the following,
    // sometimes a "surface" is a part of the model and
    // sometimes it is a G3D::Surface.

public:

    typedef ReferenceCountedPointer<MD3Model> Ref;

private:

    friend class MD3Surface;

    /** TriMesh */
    struct SurfaceData {
        /** helper data copied from the surface header */
        int                                     m_numFrames;

        int                                     m_numVertices;

        /** array of vertices for each frame of animation */
        Array< Array<Vector3> >                 m_frames;

        /** array of normals for each vertex */
        Array< Array<Vector3> >                 m_normals;

        /** array of indices used to create triangles from each frame's vertices */
        Array<int>                              m_triangles;

        /** array of texture coordinates for each vertex */
        Array<Vector2>                          m_textureCoords;

        /** TODO: make this a Material */
        Texture::Ref                            m_texture;

        std::string                             m_name;
    };

    struct FrameData {
        Vector3                                 m_bounds[2];

        Vector3                                 m_localOrigin;

        float                                   m_radius;

        std::string                             m_name;

        // map of tag name to tag data for each frame
        Table< std::string, CoordinateFrame >   m_tags;
    };

    MD3Model();

    bool loadFile(const std::string& filename);

    void loadSurface(BinaryInput& bi, SurfaceData& surfaceData);

    void loadFrame(BinaryInput& bi, FrameData& frameData);

    void loadTag(BinaryInput& bi, FrameData& frameData);

    void loadSkin(const std::string& skinName);

    /** number of frames of animation for all surfaces */
    int                         m_numFrames;

    std::string                 m_modelDir;

    /** surface data */
    Array<SurfaceData>          m_surfaces;

    /** per-frame bounding box and translatoin information */
    Array<FrameData>            m_frames;

    /** table of skins for surfaces.  TODO: Make these materials */
    Table< std::string, Table<std::string, Texture::Ref> > m_skins;

public:

    virtual ~MD3Model();

    static MD3Model::Ref fromFile(const std::string& filename);

    void pose
    (float                      frameNum, 
     const std::string&         skinName, 
     Array<Surface::Ref>&       posedModelArray, 
     const CoordinateFrame&     coordFrame = CoordinateFrame());

    CoordinateFrame getTag(float frameNum, const std::string& name) const;

    void getTagNames(Array<std::string>& names) const;

    inline int numFrames() const { 
        return m_numFrames; 
    }

private:

};

} // namespace G3D

#endif //G3D_MD3Model_h

//#endif // 0
