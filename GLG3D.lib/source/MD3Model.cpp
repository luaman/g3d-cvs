/**
 @file MD3Model.cpp

 Quake III MD3 model loading and posing

 */

//#if 1

#include "G3D/BinaryInput.h"
#include "G3D/fileutils.h"
#include "GLG3D/MD3Model.h"

namespace G3D {

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

// Definition of MD3 file surface header structure
struct MD3SurfaceHeader {
public:
    std::string ident;
    std::string name;
    int         flags;

    int         numFrames;
    int         numShaders;
    int         numVertices;
    int         numTriangles;

    int         offsetTriangles;
    int         offsetShaders;
    int         offsetUVs;
    int         offsetVertices;
    int         offsetEnd;
};

// Definition of MD3 file header structure
struct MD3FileHeader {
public:
    std::string ident;
    int         version;
    std::string name;
    int         flags;

    int         numFrames;
    int         numTags;
    int         numSurfaces;
    int         numSkins;

    int         offsetFrames;
    int         offsetTags;
    int         offsetSurfaces;
    int         offsetEnd;
};

// Helper to convert from left-hand to right-hand system
static void leftToRightHand(Vector3& v) {
    float tempY = v.y;
    v.y = v.z * -1.0f;
    v.z = tempY;
}


MD3Model::MD3Model() {
}


MD3Model::~MD3Model() {
}


MD3Model::Ref MD3Model::fromFile(const std::string& filename) {
    MD3Model* model = new MD3Model;
    if (! model->loadFile(filename)) {
        delete model;
        model = NULL;
    }

    return model;
}


bool MD3Model::loadFile(const std::string& filename) {
    // invalid filename will throw an exception, only need to validate header
    BinaryInput bi(filename, G3D_LITTLE_ENDIAN);

    m_modelDir = filenamePath(filename);

    // read file header
    MD3FileHeader md3File;
    md3File.ident = bi.readString(4);
    md3File.version = bi.readInt32();

    // validate header before continuing
    if ((md3File.ident != "IDP3") || (md3File.version != 15)) {
        return false;
    }

    md3File.name = bi.readString(64);
    md3File.flags = bi.readInt32();

    md3File.numFrames = bi.readInt32();
    md3File.numTags = bi.readInt32();
    md3File.numSurfaces = bi.readInt32();
    md3File.numSkins = bi.readInt32();

    md3File.offsetFrames = bi.readInt32();
    md3File.offsetTags = bi.readInt32();
    md3File.offsetSurfaces = bi.readInt32();
    md3File.offsetEnd = bi.readInt32();

    m_numFrames = md3File.numFrames;

    // allocate frames
    m_frames.resize(md3File.numFrames, false);

    // read in frame data
    bi.setPosition(md3File.offsetFrames);
    for (int frameIndex = 0; frameIndex < md3File.numFrames; ++frameIndex) {
        loadFrame(bi, m_frames[frameIndex]);
    }

    // read in tag data
    bi.setPosition(md3File.offsetTags);
    for (int frameIndex = 0; frameIndex < md3File.numFrames; ++frameIndex) {
        for (int tagIndex = 0; tagIndex < md3File.numTags; ++tagIndex) {
            loadTag(bi, m_frames[frameIndex]);
        }
    }

    // allocate surfaces
    m_surfaces.resize(md3File.numSurfaces, false);

    // read in surface data
    bi.setPosition(md3File.offsetSurfaces);
    for (int surfaceIndex = 0; surfaceIndex < md3File.numSurfaces; ++surfaceIndex) {
        loadSurface(bi, m_surfaces[surfaceIndex]);
    }

    return true;
}


void MD3Model::loadSurface(BinaryInput& bi, SurfaceData& surfaceData) {
    // save start of surface
    int surfaceStart = static_cast<int>(bi.getPosition());

    // read surface header
    MD3SurfaceHeader md3Surface;
    md3Surface.ident = bi.readInt32();
    md3Surface.name = bi.readString(64);
    md3Surface.flags = bi.readInt32();

    md3Surface.numFrames = bi.readInt32();
    md3Surface.numShaders = bi.readInt32();
    md3Surface.numVertices = bi.readInt32();
    md3Surface.numTriangles = bi.readInt32();

    md3Surface.offsetTriangles = bi.readInt32();
    md3Surface.offsetShaders = bi.readInt32();
    md3Surface.offsetUVs = bi.readInt32();
    md3Surface.offsetVertices = bi.readInt32();
    md3Surface.offsetEnd = bi.readInt32();

    // read surface data
    surfaceData.m_name = md3Surface.name;

    // store off some helper data
    surfaceData.m_numFrames = md3Surface.numFrames;
    surfaceData.m_numVertices = md3Surface.numVertices;

    // read triangles
    bi.setPosition(surfaceStart + md3Surface.offsetTriangles);

    surfaceData.m_triangles.resize(md3Surface.numTriangles * 3);

    for (int index = 0; index < md3Surface.numTriangles; ++index) {
        int t1 = bi.readInt32();
        int t2 = bi.readInt32();
        int t3 = bi.readInt32();
        surfaceData.m_triangles[index * 3] = t3;
        surfaceData.m_triangles[index * 3 + 1] = t2;
        surfaceData.m_triangles[index * 3 + 2] = t1;
    }

    // read shaders
    bi.setPosition(surfaceStart + md3Surface.offsetShaders);

    for (int shaderIndex = 0; shaderIndex < md3Surface.numShaders; ++shaderIndex) {
        // read shader name and index
        std::string shaderPath = bi.readString(64);
        int unusedIndex = bi.readInt32();
        
        // find base filename for shader
        std::string shaderName = filenameBaseExt(shaderPath);

        // ignore empty shader names for now (filled in with .skin file)
        if (shaderName.length() > 0) {
            surfaceData.m_texture = Texture::fromFile(m_modelDir + shaderName, ImageFormat::AUTO(), Texture::DIM_2D_NPOT);
        }
    }

    // read texture coordinates
    bi.setPosition(surfaceStart + md3Surface.offsetUVs);

    surfaceData.m_textureCoords.resize(md3Surface.numVertices);

    for (int coordIndex = 0; coordIndex < md3Surface.numVertices; ++coordIndex) {
        float u = bi.readFloat32();
        float v = bi.readFloat32();

        clamp(u, 0.0f, 1.0f);
        clamp(v, 0.0f, 1.0f);

        surfaceData.m_textureCoords[coordIndex] = Vector2(u, v);
    }

    // read vertices
    bi.setPosition(surfaceStart + md3Surface.offsetVertices);

    surfaceData.m_frames.resize(md3Surface.numFrames);
    surfaceData.m_normals.resize(md3Surface.numFrames);

    for (int frameIndex = 0; frameIndex < md3Surface.numFrames; ++frameIndex) {
        surfaceData.m_frames[frameIndex].resize(md3Surface.numVertices);
        surfaceData.m_normals[frameIndex].resize(md3Surface.numVertices);

        for (int vertexIndex = 0; vertexIndex < md3Surface.numVertices; ++vertexIndex) {
            Vector3 vertex(bi.readInt16(), bi.readInt16(), bi.readInt16());

            // MD3 scales vertices by 64
            vertex *= (1.0f / 64.0f);

            leftToRightHand(vertex);

            // convert from left-handed system
            surfaceData.m_frames[frameIndex][vertexIndex] = vertex;

            int16 normal = bi.readInt16();

            surfaceData.m_normals[frameIndex][vertexIndex] = Vector3::unitY();
        }
    }

    // ensure at the end of surface
    bi.setPosition(surfaceStart + md3Surface.offsetEnd);
}


void MD3Model::loadFrame(BinaryInput& bi, FrameData& frameData) {
    frameData.m_bounds[0] = bi.readVector3();
    leftToRightHand(frameData.m_bounds[0]);

    frameData.m_bounds[1] = bi.readVector3();
    leftToRightHand(frameData.m_bounds[1]);

    frameData.m_localOrigin = bi.readVector3();
    leftToRightHand(frameData.m_localOrigin);

    frameData.m_radius = bi.readFloat32();

    std::string name = bi.readString(16);
}


void MD3Model::loadTag(BinaryInput& bi, FrameData& frameData) {
    std::string name = bi.readString(64);

    CoordinateFrame tag;
    tag.translation = bi.readVector3();
    leftToRightHand(tag.translation);

    Vector3 colX = bi.readVector3();
    leftToRightHand(colX);

    Vector3 colY = bi.readVector3();
    leftToRightHand(colY);

    Vector3 colZ = bi.readVector3();
    leftToRightHand(colZ);

/*  if different major order
    Vector3 colX;
    Vector3 colY;
    Vector3 colZ;

    colX.x = bi.readFloat32();
    colY.x = bi.readFloat32();
    colZ.x = bi.readFloat32();

    colX.y = bi.readFloat32();
    colY.y = bi.readFloat32();
    colZ.y = bi.readFloat32();

    colX.z = bi.readFloat32();
    colY.z = bi.readFloat32();
    colZ.z = bi.readFloat32();

    leftToRightHand(colX);
    leftToRightHand(colY);
    leftToRightHand(colZ);
*/

    tag.rotation.setColumn(0, colX);
    tag.rotation.setColumn(1, colY);
    tag.rotation.setColumn(2, colZ);

    frameData.m_tags.set(name, tag);
}


void MD3Model::loadSkin(const std::string& skinName) {
    // only load if not previously loaded
    if (skinName.length() && !m_skins.containsKey(skinName)) {

        // create empty skin table initially
        m_skins.set(skinName, Table<std::string, Texture::Ref>());

        // read file as string to parse easily
        std::string skinFile = readWholeFile(m_modelDir + skinName);

        // split the file into lines
        Array<std::string> lines = stringSplit(skinFile, '\n');

        // parse each line for surface name + texture
        for (int lineIndex = 0; lineIndex < lines.length(); ++lineIndex) {
            std::string line = trimWhitespace(lines[lineIndex]);

            std::string::size_type commaPos = line.find(',');

            // quit parsing if invalid name,texture as this is probably the end of file
            if (commaPos == (line.length() - 1)) {
                continue;
            }

            std::string surfaceName = line.substr(0, commaPos);
            std::string textureName = filenameBaseExt(line.substr(commaPos + 1));

            // only try to load an existing file as the .skin contains invalid names for empty/invalid surfaces
            if (fileExists(m_modelDir + textureName)) {
                Texture::Ref t = Texture::fromFile(m_modelDir + textureName, ImageFormat::AUTO(), Texture::DIM_2D_NPOT);

                m_skins[skinName].set(surfaceName, t);
            }
        }
    }
}

void MD3Model::pose
(float                          frameNum, 
 const std::string&             skinName, 
 Array<Surface::Ref>&           posedModelArray, 
 const CoordinateFrame&         coordFrame) {

    // load skin if not already cached
    loadSkin(skinName);

    for (int surfaceIndex = 0; surfaceIndex < m_surfaces.length(); ++surfaceIndex) {
        posedModelArray.append(new MD3Surface(frameNum, coordFrame, *this, surfaceIndex, skinName));
    }
}


CoordinateFrame MD3Model::getTag(float frameNum, const std::string& name) const {
    int frame1 = iFloor(frameNum);
    int frame2 = iClamp(iCeil(frameNum), 0, m_numFrames - 1);
    float interp = fmod(frameNum, 1.0f);

    CoordinateFrame blendedFrame = m_frames[frame1].m_tags[name].lerp(m_frames[frame2].m_tags[name], interp);

    blendedFrame.translation += m_frames[frame1].m_localOrigin.lerp(m_frames[frame2].m_localOrigin, interp);

    return blendedFrame;
}


void MD3Model::getTagNames(Array<std::string>& names) const {
    m_frames[0].m_tags.getKeys(names);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

MD3Surface::MD3Surface(float frameNum, const CoordinateFrame& coordFrame, const MD3Model& model, int surfaceIndex, const std::string& skinName) {
    const MD3Model::SurfaceData& surfaceData = model.m_surfaces[surfaceIndex];

    debugAssert(frameNum >= 0.0f && frameNum < surfaceData.m_numFrames);

    // assign texture which will be used to determine if surface is ignored in model
    if (model.m_skins.containsKey(skinName) && model.m_skins[skinName].containsKey(surfaceData.m_name)) {
        m_texture = model.m_skins[skinName][surfaceData.m_name];
    } else {
        m_texture = surfaceData.m_texture;
    }

    // don't set geometry to render if no texture
    if (m_texture.isNull()) {
        return;
    }

    // calculate frames for blending
    int frame1 = iFloor(frameNum);
    int frame2 = iClamp(iCeil(frameNum), 0, surfaceData.m_numFrames - 1);
    float interp = fmod(frameNum, 1.0f);

    // copy blended vertex data for frame
    for (int vertexIndex = 0; vertexIndex < surfaceData.m_numVertices; ++vertexIndex) {
        m_geometry.vertexArray.append(surfaceData.m_frames[frame1][vertexIndex].lerp(
            surfaceData.m_frames[frame2][vertexIndex], interp));

        m_geometry.normalArray.append(surfaceData.m_normals[frame1][vertexIndex].lerp(
            surfaceData.m_normals[frame2][vertexIndex], interp));

        m_texCoords.append(surfaceData.m_textureCoords[vertexIndex]);
    }

    // copy static triangle data
    m_triangles.append(surfaceData.m_triangles);

    // add blended frame-specific translation
    m_coordFrame = coordFrame;
    m_coordFrame.translation += model.m_frames[frame1].m_localOrigin.lerp(model.m_frames[frame2].m_localOrigin, interp);
}


MD3Surface::~MD3Surface() {
}


std::string MD3Surface::name() const {
    return "MD3Model";
}


void MD3Surface::getCoordinateFrame(CoordinateFrame& c) const {
    c = m_coordFrame;
}

const MeshAlg::Geometry& MD3Surface::objectSpaceGeometry() const {
    return m_geometry;
}

const Array<Vector3>& MD3Surface::objectSpaceFaceNormals(bool normalize) const {
    return m_faceNormals;
}

const Array<MeshAlg::Face>& MD3Surface::faces() const {
    return m_faces;
}

const Array<MeshAlg::Edge>& MD3Surface::edges() const {
    return m_edges;
}

const Array<MeshAlg::Vertex>& MD3Surface::vertices() const {
    return m_adjacencies;
}

const Array<MeshAlg::Face>& MD3Surface::weldedFaces() const {
    return m_weldedFaces;
}

const Array<MeshAlg::Edge>& MD3Surface::weldedEdges() const {
    return m_weldedEdges;
}

const Array<MeshAlg::Vertex>& MD3Surface::weldedVertices() const {
    return m_weldedAdjecencies;
}

const Array<int>& MD3Surface::triangleIndices() const {
    return m_triangles;
}

void MD3Surface::getObjectSpaceBoundingSphere(Sphere&) const {
}

void MD3Surface::getObjectSpaceBoundingBox(AABox&) const {
}

int MD3Surface::numBoundaryEdges() const {
    return 0;
}

int MD3Surface::numWeldedBoundaryEdges() const {
    return 0;
}

bool MD3Surface::hasTexCoords() const {
    return m_texture.notNull();
}

const Array<Vector2>& MD3Surface::texCoords() const {
    return m_texCoords;
}

void MD3Surface::defaultRender(RenderDevice* rd) const {
    rd->setObjectToWorldMatrix(m_coordFrame);
    rd->setTexture(0, m_texture);

    glFrontFace(GL_CW);
    sendGeometry(rd);
    glFrontFace(GL_CCW);
}

} // namespace G3D

//#endif // 0
