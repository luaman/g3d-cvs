/**
 @file MD3Model.h

 Quake III MD3 model loading and posing

 */

#if 0

#ifndef G3D_MD3MODEL_H
#define G3D_MD3MODEL_H

#include <string>
#include "G3D/Matrix3.h"
#include "G3D/Table.h"
#include "GLG3D/Surface.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/Texture.h"

namespace G3D {

// forward declaration of md3 struct
struct MD3SurfaceHeader;

/**
    Quake III MD3 model loader
 */
class MD3Model : public ReferenceCountedObject {

public:
    typedef ReferenceCountedPointer<MD3Model> Ref;

    virtual ~MD3Model();

    static MD3Model::Ref fromFile(const std::string& filename);

    void pose(float frameNum, const std::string& skinName, Array<Surface::Ref>& posedModelArray, const CoordinateFrame& coordFrame = CoordinateFrame());

    CoordinateFrame     getTag(float frameNum, const std::string& name) const;

    void                getTagNames(Array<std::string>& names) const;

    int                 numFrames() const { return m_numFrames; }
private:

    struct SurfaceData {
        // helper data copied from the surface header
        int                                     m_numFrames;

        int                                     m_numVertices;

        // array of vertices for each frame of animation
        Array< Array<Vector3> >                 m_frames;

        // array of normals for each vertex
        Array< Array<Vector3> >                 m_normals;

        // array of indices used to create triangles from each frame's vertices
        Array<int>                              m_triangles;

        // array of texture coordinates for each vertex
        Array<Vector2>                          m_textureCoords;

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


    class MD3Surface : public Surface {
    public:
        MD3Surface(float frameNum, const CoordinateFrame& coordFrame,const MD3Model& model, int surfaceIndex, const std::string& skinName);

        virtual ~MD3Surface();

        virtual std::string name() const;

        virtual void getCoordinateFrame(CoordinateFrame& c) const;

        virtual const MeshAlg::Geometry& objectSpaceGeometry() const;

        virtual const Array<Vector3>& objectSpaceFaceNormals(bool normalize = true) const;

        virtual const Array<MeshAlg::Face>& faces() const;

        virtual const Array<MeshAlg::Edge>& edges() const;

        virtual const Array<MeshAlg::Vertex>& vertices() const;

        virtual const Array<MeshAlg::Face>& weldedFaces() const;

        virtual const Array<MeshAlg::Edge>& weldedEdges() const;

        virtual const Array<MeshAlg::Vertex>& weldedVertices() const;

        virtual const Array<int>& triangleIndices() const;

        virtual void getObjectSpaceBoundingSphere(Sphere&) const;

        virtual void getObjectSpaceBoundingBox(AABox&) const;

        virtual int numBoundaryEdges() const;

        virtual int numWeldedBoundaryEdges() const;

        virtual const Array<Vector2>& texCoords() const;

        virtual bool hasTexCoords() const;

        virtual void defaultRender(RenderDevice* rd) const;

    private:
        MeshAlg::Geometry       m_geometry;
        Array<Vector3>          m_faceNormals;
        Array<MeshAlg::Face>    m_faces;
        Array<MeshAlg::Edge>    m_edges;
        Array<MeshAlg::Vertex>  m_adjacencies;
        Array<MeshAlg::Face>    m_weldedFaces;
        Array<MeshAlg::Edge>    m_weldedEdges;
        Array<MeshAlg::Vertex>  m_weldedAdjecencies;
        Array<int>              m_triangles;
        Array<Vector2>          m_texCoords;

        CoordinateFrame         m_coordFrame;

        Texture::Ref            m_texture;
    };

    MD3Model();

    bool loadFile(const std::string& filename);

    void loadSurface(BinaryInput* bi, SurfaceData& surfaceData);

    void loadFrame(BinaryInput* bi, FrameData& frameData);

    void loadTag(BinaryInput* bi, FrameData& frameData);

    void loadSkin(const std::string& skinName);

    // number of frames of animation for all surfaces
    int                         m_numFrames;

    std::string                 m_modelDir;

    // surface data
    Array<SurfaceData>          m_surfaces;

    // per-frame bounding box and translatoin information
    Array<FrameData>            m_frames;

    // table of skins for surfaces
    Table< std::string, Table<std::string, Texture::Ref> > m_skins;
};

} // namespace G3D

#endif //G3D_MD3MODEL_H

#endif // 0
