/**
 @file MD3Model.cpp

 Quake III MD3 model loading and posing

 */

//#if 1

#include "G3D/BinaryInput.h"
#include "G3D/fileutils.h"
#include "GLG3D/MD3Model.h"


namespace G3D {


// 60 quake units ~= 2 meters
#define Q3_LOAD_SCALE (2.0f / 60.0f)

inline static Vector3 vectorToG3D(const Vector3& v) {
    return Vector3(v.y, v.z, -v.x);
}

/** Takes a point in the Q3 coordinate system to one in the G3D coordinate system */
inline static Vector3 pointToG3D(const Vector3& v) {
    return vectorToG3D(v) * Q3_LOAD_SCALE;
}


class MD3Surface : public Surface {
public:
    MD3Surface
    (float                      frameNum, 
     const CoordinateFrame&     coordFrame, 
     const MD3Model&            model, 
     int                        surfaceIndex, 
     const std::string&         skinName);

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
    Array<int>              m_indexArray;
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

    MD3SurfaceHeader(BinaryInput& bi) {
        ident           = bi.readInt32();
        name            = bi.readString(64);
        flags           = bi.readInt32();

        numFrames       = bi.readInt32();
        numShaders      = bi.readInt32();
        numVertices     = bi.readInt32();
        numTriangles    = bi.readInt32();

        offsetTriangles = bi.readInt32();
        offsetShaders   = bi.readInt32();
        offsetUVs       = bi.readInt32();
        offsetVertices  = bi.readInt32();
        offsetEnd       = bi.readInt32();
    }
};

// Definition of MD3 file header structure
class MD3FileHeader {
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

    /** True if this file had the right version number */
    bool        ok;

    MD3FileHeader(BinaryInput& bi) : ok(true) {
        ident   = bi.readString(4);
        version = bi.readInt32();

        // validate header before continuing
        if ((ident != "IDP3") || (version != 15)) {
            ok = false;
            return;
        }

        name            = bi.readString(64);
        flags           = bi.readInt32();

        numFrames       = bi.readInt32();
        numTags         = bi.readInt32();
        numSurfaces     = bi.readInt32();
        numSkins        = bi.readInt32();

        offsetFrames    = bi.readInt32();
        offsetTags      = bi.readInt32();
        offsetSurfaces  = bi.readInt32();
        offsetEnd       = bi.readInt32();
    }
};


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
    MD3FileHeader md3File(bi);
    if (! md3File.ok) {
        return false;
    }

    m_numFrames = md3File.numFrames;

    // allocate frames
    m_frames.resize(md3File.numFrames, false);

    // Read in frame data
    bi.setPosition(md3File.offsetFrames);
    for (int frameIndex = 0; frameIndex < m_frames.size(); ++frameIndex) {
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
    MD3SurfaceHeader md3Surface(bi);


    // read surface data
    surfaceData.m_name = md3Surface.name;

    // store off some helper data
    surfaceData.m_numFrames = md3Surface.numFrames;
    surfaceData.m_numVertices = md3Surface.numVertices;

    // read triangles
    bi.setPosition(surfaceStart + md3Surface.offsetTriangles);

    surfaceData.m_indexArray.resize(md3Surface.numTriangles * 3);

    for (int index = 0; index < md3Surface.numTriangles; ++index) {
        // Winding direction is backwards in Q3 because their
        // coordinate system is flipped
        const int t1 = bi.readInt32();
        const int t2 = bi.readInt32();
        const int t3 = bi.readInt32();
        surfaceData.m_indexArray[index * 3] = t3;
        surfaceData.m_indexArray[index * 3 + 1] = t2;
        surfaceData.m_indexArray[index * 3 + 2] = t1;
    }

    // read shaders
    bi.setPosition(surfaceStart + md3Surface.offsetShaders);

    for (int shaderIndex = 0; shaderIndex < md3Surface.numShaders; ++shaderIndex) {
        // read shader name and index
        const std::string& shaderPath = bi.readString(64);
        const int unusedIndex = bi.readInt32();
        
        // find base filename for shader
        const std::string& shaderName = filenameBaseExt(shaderPath);

        // ignore empty shader names for now (filled in with .skin file)
        if (! shaderName.empty()) {
            surfaceData.m_texture = Texture::fromFile(m_modelDir + shaderName, ImageFormat::AUTO(), Texture::DIM_2D_NPOT);
        }
    }

    // read texture coordinates
    bi.setPosition(surfaceStart + md3Surface.offsetUVs);

    surfaceData.m_textureCoords.resize(md3Surface.numVertices);

    for (int coordIndex = 0; coordIndex < md3Surface.numVertices; ++coordIndex) {
        // TODO: are you sure these should be clamped?  There's nothing wrong with
        // wrapping texcoords
        const float u = clamp(bi.readFloat32(), 0.0f, 1.0f);
        const float v = clamp(bi.readFloat32(), 0.0f, 1.0f);

        surfaceData.m_textureCoords[coordIndex] = Vector2(u, v);
    }

    // read vertices
    bi.setPosition(surfaceStart + md3Surface.offsetVertices);

    surfaceData.m_geometry.resize(md3Surface.numFrames);

    const int N = md3Surface.numVertices;
    for (int frameIndex = 0; frameIndex < md3Surface.numFrames; ++frameIndex) {
        MeshAlg::Geometry& geom = surfaceData.m_geometry[frameIndex];
        geom.vertexArray.resize(N);
        geom.normalArray.resize(N);

        for (int vertexIndex = 0; vertexIndex < N; ++vertexIndex) {
            Vector3 vertex;
            for (int j = 0; j < 3; ++j) {
                vertex[j] = bi.readInt16();
            }

            // MD3 scales vertices by 64 when packing them into integers
            vertex *= (1.0f / 64.0f);

            geom.vertexArray[vertexIndex] = pointToG3D(vertex);

            // TODO: Unpack normal.  Encoding is at the bottom of this page:
            // http://icculus.org/homepages/phaethon/q3a/formats/md3format.html
            int16 normal = bi.readInt16();
            geom.normalArray[vertexIndex] = pointToG3D(Vector3::unitX());
        }
    }

    // Ensure at the end of surface
    bi.setPosition(surfaceStart + md3Surface.offsetEnd);
}


void MD3Model::loadFrame(BinaryInput& bi, FrameData& frameData) {
    frameData.m_bounds[0] = pointToG3D(bi.readVector3());

    frameData.m_bounds[1] = pointToG3D(bi.readVector3());

    frameData.m_localOrigin = pointToG3D(bi.readVector3());

    frameData.m_radius = bi.readFloat32() * Q3_LOAD_SCALE;

    // TODO: why is name ignored?  Should we at least assert it or something?
    const std::string& name = bi.readString(16);
    (void)name;
}


void MD3Model::loadTag(BinaryInput& bi, FrameData& frameData) {
    std::string name = bi.readString(64);

    CoordinateFrame tag;
    tag.translation = pointToG3D(bi.readVector3());

    Matrix3 raw;
    for (int a = 0; a < 3; ++a) {
        raw.setColumn(a, vectorToG3D(bi.readVector3()));
    }

    // Perform the vectorToG3D transform on the columns
    tag.rotation.setColumn(Vector3::X_AXIS,  raw.column(Vector3::Y_AXIS));
    tag.rotation.setColumn(Vector3::Y_AXIS,  raw.column(Vector3::Z_AXIS));
    tag.rotation.setColumn(Vector3::Z_AXIS, -raw.column(Vector3::X_AXIS));

    frameData.m_tags.set(name, tag);
}


void MD3Model::loadSkin(const std::string& skinName) {
    // only load if not previously loaded
    if (skinName.length() && !m_skins.containsKey(skinName)) {

        // create empty skin table initially
        m_skins.set(skinName, Table<std::string, Texture::Ref>());

        // read file as string to parse easily
        const std::string& skinFile = readWholeFile(m_modelDir + skinName);

        // split the file into lines
        const Array<std::string>& lines = stringSplit(skinFile, '\n');

        // parse each line for surface name + texture
        for (int lineIndex = 0; lineIndex < lines.length(); ++lineIndex) {
            const std::string& line = trimWhitespace(lines[lineIndex]);

            const std::string::size_type commaPos = line.find(',');

            // quit parsing if invalid name,texture as this is probably the end of file
            if (commaPos == (line.length() - 1)) {
                continue;
            }

            const std::string& surfaceName = line.substr(0, commaPos);
            const std::string& textureName = filenameBaseExt(line.substr(commaPos + 1));

            // only try to load an existing file as the .skin contains invalid names for empty/invalid surfaces
            if (fileExists(m_modelDir + textureName)) {
                const Texture::Ref& t = Texture::fromFile(m_modelDir + textureName, ImageFormat::AUTO(), Texture::DIM_2D_NPOT);

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


CoordinateFrame MD3Model::tag(float frameNum, const std::string& name) const {
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

    // Don't set geometry to render if no texture
    if (m_texture.isNull()) {
        return;
    }

    // Calculate frames for blending
    int frame1 = iFloor(frameNum);
    int frame2 = iClamp(iCeil(frameNum), 0, surfaceData.m_numFrames - 1);
    float interp = fmod(frameNum, 1.0f);

    // copy blended vertex data for frame (TODO: eventually SSE this, but wait until everything is converted to use SuperSurface::CPUData)
    const MeshAlg::Geometry& geom1 = surfaceData.m_geometry[frame1];
    const MeshAlg::Geometry& geom2 = surfaceData.m_geometry[frame2];
    for (int vertexIndex = 0; vertexIndex < surfaceData.m_numVertices; ++vertexIndex) {
        m_geometry.vertexArray.append(geom1.vertexArray[vertexIndex].lerp(geom2.vertexArray[vertexIndex], interp));

        m_geometry.normalArray.append(geom1.normalArray[vertexIndex].lerp(geom2.normalArray[vertexIndex], interp));

        m_texCoords.append(surfaceData.m_textureCoords[vertexIndex]);
    }

    // Copy static triangle data
    m_indexArray.append(surfaceData.m_indexArray);

    // Add blended frame-specific translation
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
    return m_indexArray;
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
