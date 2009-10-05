/**
 @file MD3Model.cpp

 Quake III MD3 model loading and posing

 */

#include "G3D/BinaryInput.h"
#include "G3D/fileutils.h"
#include "G3D/TextInput.h"
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

//    MD3Part Model loader helper for MD3Model.  Loads an individual .md3 model.
class MD3Part : public ReferenceCountedObject {
    // See: http://icculus.org/homepages/phaethon/q3a/formats/md3format.html

    // Terminology:
    //
    // Q3 calls an attachment point a "tag"
    //
    // Player models contain lower.md3, upper.md3, and head.md3.  The lower part is the root.
    // the upper is attached to the lower, and the weapon and head are attached to the upper.

private:
    friend class MD3Model;

    /** TriMesh */
    struct SurfaceData {
        /** helper data copied from the surface header */
        int                                     m_numFrames;

        int                                     m_numVertices;

        /** Geometry for each frame of animation */
        Array<MeshAlg::Geometry>                m_geometry;

        /** Indexed triangle list. */
        Array<int>                              m_indexArray;

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

        /** map of tag name to tag data for each frame*/
        Table<std::string, CoordinateFrame>     m_tags;
    };

    /** surface data */
    Array<SurfaceData>          m_surfaces;

    /** per-frame bounding box and translation information */
    Array<FrameData>            m_frames;

    int                         m_numFrames;

    std::string                 m_modelDir;
    std::string                 m_modelName;


    MD3Part();

    virtual ~MD3Part() {}

    CoordinateFrame tag(float frameNum, const std::string& name) const;

    bool loadFile(const std::string& filename);

    void loadSurface(BinaryInput& bi, SurfaceData& surfaceData);

    void loadFrame(BinaryInput& bi, FrameData& frameData);

    void loadTag(BinaryInput& bi, FrameData& frameData);
};

// Surface implementation for MD3 models 
class MD3Surface : public Surface {
public:
    MD3Surface() {}

    virtual ~MD3Surface() {}

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
    friend class MD3Model;

    std::string             m_name;
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

///////////////////////////////////////////////////////////////////////////////////////////////////////

MD3Part::MD3Part() : m_numFrames(0) {
}


bool MD3Part::loadFile(const std::string& filename) {
    BinaryInput bi(filename, G3D_LITTLE_ENDIAN);

    // read file header
    MD3FileHeader md3File(bi);
    if (! md3File.ok) {
        return false;
    }

    m_modelDir = filenamePath(filename);
    m_modelName = filenameBase(filename);

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


void MD3Part::loadSurface(BinaryInput& bi, SurfaceData& surfaceData) {
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
        surfaceData.m_indexArray[index * 3] = bi.readInt32();
        surfaceData.m_indexArray[index * 3 + 1] = bi.readInt32();
        surfaceData.m_indexArray[index * 3 + 2] = bi.readInt32();
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
        surfaceData.m_textureCoords[coordIndex].x = bi.readFloat32();
        surfaceData.m_textureCoords[coordIndex].y = bi.readFloat32();
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

            // Decoding is at the bottom of this page:
            // http://icculus.org/homepages/phaethon/q3a/formats/md3format.html
            int16 encNormal = bi.readInt16();

            float nlat = static_cast<float>((encNormal >> 8) & 0xFF);
            float nlng = static_cast<float>(encNormal & 0xFF);

            nlat *= static_cast<float>(pi() / 128);
            nlng *= static_cast<float>(pi() / 128);

            Vector3 normal;
            normal.x = cosf(nlat) * sinf(nlng);
            normal.y = sinf(nlat) * sinf(nlng);
            normal.z = cosf(nlng);

            geom.normalArray[vertexIndex] = normal;
        }
    }

    // Ensure at the end of surface
    bi.setPosition(surfaceStart + md3Surface.offsetEnd);
}


void MD3Part::loadFrame(BinaryInput& bi, FrameData& frameData) {
    frameData.m_bounds[0] = pointToG3D(bi.readVector3());

    frameData.m_bounds[1] = pointToG3D(bi.readVector3());

    frameData.m_localOrigin = pointToG3D(bi.readVector3());

    frameData.m_radius = bi.readFloat32() * Q3_LOAD_SCALE;

    // TODO: why is name ignored?  Should we at least assert it or something?
    const std::string& name = bi.readString(16);
    (void)name;
}


void MD3Part::loadTag(BinaryInput& bi, FrameData& frameData) {
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


CoordinateFrame MD3Part::tag(float frameNum, const std::string& name) const {
    int frame1 = iFloor(frameNum);
    int frame2 = iClamp(iCeil(frameNum), 0, m_numFrames - 1);
    float interp = fmod(frameNum, 1.0f);

    CoordinateFrame blendedFrame = m_frames[frame1].m_tags[name].lerp(m_frames[frame2].m_tags[name], interp);

    blendedFrame.translation += m_frames[frame1].m_localOrigin.lerp(m_frames[frame2].m_localOrigin, interp);

    return blendedFrame;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

MD3Model::MD3Model() {
    memset(m_parts, 0, sizeof(m_parts));
}


MD3Model::~MD3Model() {
    for (int partIndex = 0; partIndex < NUM_PARTS; ++partIndex) {
        if (m_parts[partIndex]) {
            delete m_parts[partIndex];
            m_parts[partIndex] = NULL;
        }
    }
}


MD3Model::Ref MD3Model::fromDirectory(const std::string& modelDir) {
    MD3Model* model = new MD3Model;

    model->loadDirectory(modelDir);

    return model;
}


void MD3Model::loadDirectory(const std::string& modelDir) {
    // load animation.cfg file
    loadAnimationCfg(pathConcat(modelDir, "animation.cfg"));

    // load legs
    std::string filename = pathConcat(modelDir, "lower.md3");

    m_parts[PART_LEGS] = new MD3Part;
    
    if (!m_parts[PART_LEGS]->loadFile(filename)) {
        debugAssertM(false, format("Unable to load %s.", filename.c_str()));
        return;
    }

    // load torso
    filename = pathConcat(modelDir, "upper.md3");

    m_parts[PART_TORSO] = new MD3Part;
    
    if (!m_parts[PART_TORSO]->loadFile(filename)) {
        debugAssertM(false, format ("Unable to load %s.", filename.c_str()));
        return;
    }

    // load head
    filename = pathConcat(modelDir, "head.md3");

    m_parts[PART_HEAD] = new MD3Part;
    
    if (!m_parts[PART_HEAD]->loadFile(filename)) {
        debugAssertM(false, format("Unable to load %s.", filename.c_str()));
        return;
    }

    // load weapon (if it exists, optional)
    filename = pathConcat(modelDir, "weapon.md3");
    if (!fileExists(filename)) {
        return;
    }

    m_parts[PART_WEAPON] = new MD3Part;
    
    if (!m_parts[PART_WEAPON]->loadFile(filename)) {
        debugAssertM(false, format("Unable to load %s.", filename.c_str()));
        return;
    }
}


void MD3Model::loadAnimationCfg(const std::string filename) {
    TextInput::Settings settings;
    settings.generateNewlineTokens = true;

    TextInput ti(filename, settings);

    for (int animIndex = 0; animIndex < NUM_ANIMATIONS; ++animIndex) {
        // check if animations have started in file
        while (ti.hasMore() && (ti.peek().extendedType() != Token::INTEGER_TYPE)) {
            // eat until next line starting with an integer token
            while (ti.hasMore() && (ti.peek().type() != Token::NEWLINE)) {
                ti.read();
            }

            ti.read();
        }

        // return early if this is an invalid file
        if (ti.peek().type() == Token::END) {
            debugAssertM(ti.peek().type() != Token::END, ("Invalid animation.cfg file!"));
            return;
        }

        m_animations[animIndex].start = static_cast<float>(ti.readNumber());
        m_animations[animIndex].num = static_cast<float>(ti.readNumber());
        m_animations[animIndex].loop = static_cast<float>(ti.readNumber());
        m_animations[animIndex].fps = static_cast<float>(ti.readNumber());

        if (ti.peek().type() == Token::NEWLINE) {
            ti.readNewlineToken();
        } else {
            debugAssert(animIndex == (NUM_ANIMATIONS - 1));
        }
    }

    // loop through all legs animations and adjust starting frame number to be relative to model
    float numTorsoAnimations = m_animations[START_LEGS].start - m_animations[START_TORSO].start;
    for (int animIndex = START_LEGS; animIndex <= END_LEGS; ++animIndex) {
        m_animations[animIndex].start -= numTorsoAnimations;
    }
}


void MD3Model::setSkin(const std::string& skinName) {

    // loop through all parts and load part-specific skin based on skinName
    for (int partIndex = 0; partIndex < NUM_PARTS; ++partIndex) {
        if (!m_parts[partIndex]) {
            continue;
        }

        const std::string& filename = pathConcat(m_parts[partIndex]->m_modelDir, m_parts[partIndex]->m_modelName + "_" + skinName + ".skin");

        // create empty skin table initially
        m_skins.set(m_parts[partIndex]->m_modelName, Table<std::string, Texture::Ref>());

        // read file as string to parse easily
        const std::string& skinFile = readWholeFile(filename);

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
            std::string shaderName = filenameBaseExt(line.substr(commaPos + 1));

            std::string textureFilename = pathConcat(m_parts[partIndex]->m_modelDir, shaderName);

            // only try to load an existing file as the .skin contains invalid names for empty/invalid surfaces
            if (fileExists(textureFilename)) {
                const Texture::Ref& t = Texture::fromFile(textureFilename, ImageFormat::AUTO(), Texture::DIM_2D_NPOT);

                // assign the surface skin to the model
                m_skins[m_parts[partIndex]->m_modelName].set(surfaceName, t);
            }
        }
    }
}


void MD3Model::pose(const Pose& pose, Array<Surface::Ref>& posedModelArray, const CoordinateFrame& cframe) {

    // coordinate frame built up from legs
    CoordinateFrame baseFrame = cframe;

    // pose legs part
    if (!m_parts[PART_LEGS]) {
        return;
    }

    posePart(PART_LEGS, pose, posedModelArray, baseFrame);

    float legsFrameNum = findFrameNum(pose.legsAnim, pose.legsTime);
    baseFrame = baseFrame * m_parts[PART_LEGS]->tag(legsFrameNum, "tag_torso");

    // pose torso part
    if (!m_parts[PART_TORSO]) {
        return;
    }

    posePart(PART_TORSO, pose, posedModelArray, baseFrame);

    float torsoFrameNum = findFrameNum(pose.torsoAnim, pose.torsoTime);

    // pose weapon part (since it uses a torso tag)
    if (m_parts[PART_WEAPON]) {
        CoordinateFrame weaponFrame = baseFrame * m_parts[PART_TORSO]->tag(torsoFrameNum, "tag_weapon");
        posePart(PART_WEAPON, pose, posedModelArray, baseFrame);
    }

    baseFrame = baseFrame * m_parts[PART_TORSO]->tag(torsoFrameNum, "tag_head");

    // pose head part
    if (!m_parts[PART_HEAD]) {
        return;
    }

    posePart(PART_HEAD, pose, posedModelArray, baseFrame);
}


void MD3Model::posePart(PartType partType, const Pose& pose, Array<Surface::Ref>& posedModelArray, const CoordinateFrame& cframe) {
    const MD3Part* part = m_parts[partType];

    for (int surfaceIndex = 0; surfaceIndex < part->m_surfaces.length(); ++surfaceIndex) {

        const MD3Part::SurfaceData& surfaceData = part->m_surfaces[surfaceIndex];

        Texture::Ref surfaceTexture;

        // assign texture which will be used to determine if surface is ignored in model
        if (surfaceData.m_texture.notNull()) {
            surfaceTexture = surfaceData.m_texture;
        } else if (m_skins.containsKey(part->m_modelName)) {
            const Table<std::string, Texture::Ref>& surfaceSkins = m_skins[part->m_modelName];
            if (surfaceSkins.containsKey(surfaceData.m_name)) {
                surfaceTexture = surfaceSkins[surfaceData.m_name];
            }
        }

        // Don't set geometry to render if no texture
        if (surfaceTexture.isNull()) {
            continue;
        }

        MD3Surface* md3Surface = new MD3Surface;
        md3Surface->m_texture = surfaceTexture;

        float frameNum = 0.0f;

        if (partType == PART_LEGS) {
            frameNum = findFrameNum(pose.legsAnim, pose.legsTime);
        } else if (partType == PART_TORSO) {
            frameNum = findFrameNum(pose.torsoAnim, pose.torsoTime);
        }

        // Calculate frames for blending
        int frame1 = iFloor(frameNum);
        int frame2 = iClamp(iCeil(frameNum), 0, surfaceData.m_numFrames - 1);
        float interp = fmod(frameNum, 1.0f);

        // copy blended vertex data for frame (TODO: eventually SSE this, but wait until everything is converted to use SuperSurface::CPUData)
        const MeshAlg::Geometry& geom1 = surfaceData.m_geometry[frame1];
        const MeshAlg::Geometry& geom2 = surfaceData.m_geometry[frame2];
        for (int vertexIndex = 0; vertexIndex < surfaceData.m_numVertices; ++vertexIndex) {
            md3Surface->m_geometry.vertexArray.append(geom1.vertexArray[vertexIndex].lerp(geom2.vertexArray[vertexIndex], interp));

            md3Surface->m_geometry.normalArray.append(geom1.normalArray[vertexIndex].lerp(geom2.normalArray[vertexIndex], interp));

            md3Surface->m_texCoords.append(surfaceData.m_textureCoords[vertexIndex]);
        }

        // Copy static triangle data
        md3Surface->m_indexArray.append(surfaceData.m_indexArray);

        // Add blended frame-specific translation
        md3Surface->m_coordFrame = cframe;
        md3Surface->m_coordFrame.translation += part->m_frames[frame1].m_localOrigin.lerp(part->m_frames[frame2].m_localOrigin, interp);

        // set name
        md3Surface->m_name = part->m_modelName + "::" + surfaceData.m_name;

        posedModelArray.append(md3Surface);
    }
}


float MD3Model::findFrameNum(AnimType animType, GameTime animTime) {
    debugAssert(animType < NUM_ANIMATIONS);

    float frameNum = m_animations[animType].start;

    float initialLoopTime = (m_animations[animType].num / m_animations[animType].fps);

    if (animTime < initialLoopTime) {
        // less than 1 loop complete, no need to account for "loop" value
        frameNum += static_cast<float>(animTime * m_animations[animType].fps);
    } else {
        // otherwise find actual frame number after number of loops
        animTime -= initialLoopTime;

        // find time for all subsequent loops
        float otherLoopTime = m_animations[animType].loop / m_animations[animType].fps;

        // how far into the last loop
        float timeIntoLastLoop = fmod(static_cast<float>(animTime), otherLoopTime);

        // "loop" works by specifying the last number of frames to loop over
        // so a loop of 1 with num frames 5 means looping starts at frame 4 with frames {1, 2, 3, 4, 5} originally

        frameNum += (m_animations[animType].num - m_animations[animType].loop);

        frameNum += (timeIntoLastLoop * m_animations[animType].fps);
    }

    return frameNum;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

std::string MD3Surface::name() const {
    return m_name;
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

    sendGeometry(rd);
}

} // namespace G3D
