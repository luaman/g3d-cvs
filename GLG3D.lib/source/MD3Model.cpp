/**
 @file MD3Model.cpp

 Quake III MD3 model loading and posing

 */

#include "G3D/BinaryInput.h"
#include "G3D/fileutils.h"
#include "G3D/HashTrait.h"
#include "G3D/TextInput.h"
#include "GLG3D/MD3Model.h"
#include "G3D/FileSystem.h"
#include "G3D/Any.h"
#include "GLG3D/SuperSurface.h"

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

const std::string& MD3Model::toString(PartType t) {
    const std::string name[] = {"lower", "upper", "head", "weapon"};
    debugAssert(t >= 0 && t <= 3);
    return name[t];
}

static Material::Ref defaultMaterial() {
    static Material::Ref m;
    if (m.isNull()) {
        m = Material::createDiffuse(Color3::white() * 0.99f);
    }
    return m;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void MD3Model::Skin::loadSkinFile(const std::string& filename, PartSkin& partSkin) {
    // Read file as string to parse easily
    const std::string& skinFile = readWholeFile(filename);

    // Split the file into lines
    Array<std::string> lines = stringSplit(skinFile, '\n');

    // Parse each line for surface name + texture
    for (int lineIndex = 0; lineIndex < lines.length(); ++lineIndex) {
        std::string line = trimWhitespace(lines[lineIndex]);

        std::string::size_type commaPos = line.find(',');

        // Quit parsing this line if no comma is found or it is at the end of the line
        if ((commaPos == (line.length() - 1)) || (commaPos == std::string::npos)) {
            continue;
        }

        // Figure out actual texture filename
        const std::string& triListName = line.substr(0, commaPos);
        const std::string& textureName = filenameBaseExt(line.substr(commaPos + 1));

        if (textureName == "nodraw") {
            // Intentionally NULL material
            partSkin.set(triListName, NULL);
        } else {
            // Assume textures are relative to the current .skin file
            const std::string& textureFilename = FilePath::concat(FilePath::parent(filename), textureName);

            if (FileSystem::exists(textureFilename)) {
                partSkin.set(triListName, Material::createDiffuse(textureFilename));
            } else {
                partSkin.set(triListName, defaultMaterial());
            }
        }
    }
}

MD3Model::Skin::Ref MD3Model::Skin::create
 (const std::string& path,
  const std::string& lowerSkin, 
  const std::string& upperSkin, 
  const std::string& headSkin) {
    
    Skin::Ref s = new Skin();
    if (! headSkin.empty()) {
        s->partSkin.resize(3);
    } else if (! upperSkin.empty()) {
        s->partSkin.resize(2);
    } else if (! lowerSkin.empty()) {
        s->partSkin.resize(1);
    } else {
        alwaysAssertM(false, "No skins specified!");
    }

    // Load actual .skin files
    const Array<std::string> filename
        (FilePath::concat(path, lowerSkin), 
         FilePath::concat(path, upperSkin),
         FilePath::concat(path, headSkin));

    for (int i = 0; i < s->partSkin.size(); ++i) {
        loadSkinFile(filename[i], s->partSkin[i]);
    }
    
    return s;
}


MD3Model::Skin::Ref MD3Model::Skin::create
(const std::string& commonPath,
 const std::string& commonSuffix) {
    return MD3Model::Skin::create(
        commonPath,
        "lower_" + commonSuffix + ".skin",
        "upper_" + commonSuffix + ".skin",
        "head_" + commonSuffix + ".skin");
}


MD3Model::Skin::Ref MD3Model::Skin::create(const Any& any) {
    Skin::Ref s = new Skin();
    any.verifyType(Any::ARRAY);
    any.verifyName("MD3Model::Skin");
    s->partSkin.resize(any.size());
    for (int i = 0; i < s->partSkin.size(); ++i) {
        const Any& src = any[i];
        PartSkin& dst = s->partSkin[i];
        if (src.type() == Any::STRING) {
            loadSkinFile(src.resolveStringAsFilename(), dst);
        } else {
            for (Table<std::string, Any>::Iterator it = src.table().begin(); it.hasMore(); ++it) {
                if (it->value.type() == Any::NONE) {
                    dst.set(it->key, NULL);
                } else {
                    dst.set(it->key, Material::create(it->value));
                }
            }
        }
    }

    return s;
}
//////////////////////////////////////////////////////////////////////////////////////////////


MD3Model::Specification::Part::Part(const Any& any) {
    any.verifyName("Part");

    load = true;

    if (any.containsKey("skin")) {
        skinName = any["skin"].string();
    }

    if (any.containsKey("material")) {
        material = Material::create(any["material"]);
    }
}

MD3Model::Specification::Specification(const Any& any) {
    any.verifyName("MD3Model::Specification");
    
    directory = any["directory"].resolveStringAsFilename();

    // Expect the base part (legs/lower)
    parts[PART_LOWER] = any.containsKey("legs") ? any["legs"] : any["lower"];

    // Rest of parts are optional
    if (any.containsKey("torso")) {
        parts[PART_UPPER] = any["torso"];
    }
    if (any.containsKey("upper")) {
        parts[PART_UPPER] = any["upper"];
    }

    if (parts[PART_UPPER].load) {
        if (any.containsKey("head")) {
            parts[PART_HEAD] = any["head"];
        }

        if (any.containsKey("weapon")) {
            parts[PART_HEAD] = any["weapon"];
        }
    }
}

MD3Model::Ref MD3Model::create(const MD3Model::Specification& spec) {
    MD3Model* model = new MD3Model;

    model->loadSpecification(spec);

    return model;
}



/** Definition of MD3 file surface header structure.  These correspond to triLists */
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

/** MD3Part Model loader helper for MD3Model.  Loads an individual .md3 model. */
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
    struct TriList {
        /** helper data copied from the surface header */
        int                                     m_numFrames;

        int                                     m_numVertices;

        /** Geometry for each frame of animation */
        Array<MeshAlg::Geometry>                m_geometry;

        /** Indexed triangle list. */
        Array<int>                              m_indexArray;
        VertexRange                             m_gpuIndex;

        /** array of texture coordinates for each vertex */
        Array<Vector2>                          m_textureCoords;

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
    Array<TriList>              m_triListArray;

    /** per-frame bounding box and translation information */
    Array<FrameData>            m_frames;

    int                         m_numFrames;

    std::string                 m_modelDir;
    std::string                 m_modelName;

    MD3Part();

    virtual ~MD3Part() {}

    CoordinateFrame tag(float frameNum, const std::string& name) const;

    bool loadFile(const std::string& filename);

    void loadSurface(BinaryInput& bi, TriList& triList);

    void loadFrame(BinaryInput& bi, FrameData& frameData);

    void loadTag(BinaryInput& bi, FrameData& frameData);
};


///////////////////////////////////////////////////////////////////////////////////////////////////////

MD3Part::MD3Part() : m_numFrames(0) {
}


bool MD3Part::loadFile(const std::string& filename) {
    BinaryInput bi(filename, G3D_LITTLE_ENDIAN);

    // Read file header
    MD3FileHeader md3File(bi);
    if (! md3File.ok) {
        return false;
    }

    m_modelDir = filenamePath(filename);
    m_modelName = filenameBase(filename);

    m_numFrames = md3File.numFrames;

    // Allocate frames
    m_frames.resize(md3File.numFrames, false);

    // Read in frame data
    bi.setPosition(md3File.offsetFrames);
    for (int frameIndex = 0; frameIndex < m_frames.size(); ++frameIndex) {
        loadFrame(bi, m_frames[frameIndex]);
    }

    // Read in tag data
    bi.setPosition(md3File.offsetTags);
    for (int frameIndex = 0; frameIndex < md3File.numFrames; ++frameIndex) {
        for (int tagIndex = 0; tagIndex < md3File.numTags; ++tagIndex) {
            loadTag(bi, m_frames[frameIndex]);
        }
    }

    // Allocate surfaces
    m_triListArray.resize(md3File.numSurfaces, false);

    // Read in surface data
    bi.setPosition(md3File.offsetSurfaces);
    for (int surfaceIndex = 0; surfaceIndex < md3File.numSurfaces; ++surfaceIndex) {
        loadSurface(bi, m_triListArray[surfaceIndex]);
    }

    return true;
}


void MD3Part::loadSurface(BinaryInput& bi, TriList& triList) {
    // Save start of surface
    int surfaceStart = static_cast<int>(bi.getPosition());

    // Read surface header
    MD3SurfaceHeader md3Surface(bi);


    // Read surface data
    triList.m_name = md3Surface.name;

    // Store off some helper data
    triList.m_numFrames = md3Surface.numFrames;
    triList.m_numVertices = md3Surface.numVertices;

    // Read triangles
    bi.setPosition(surfaceStart + md3Surface.offsetTriangles);

    triList.m_indexArray.resize(md3Surface.numTriangles * 3);

    for (int index = 0; index < md3Surface.numTriangles; ++index) {
        triList.m_indexArray[index * 3] = bi.readInt32();
        triList.m_indexArray[index * 3 + 1] = bi.readInt32();
        triList.m_indexArray[index * 3 + 2] = bi.readInt32();
    }

    VertexBuffer::Ref vb = VertexBuffer::create(triList.m_indexArray.size() * sizeof(int), VertexBuffer::WRITE_ONCE, VertexBuffer::INDEX);
    triList.m_gpuIndex = VertexRange(triList.m_indexArray, vb);

    // Read shaders
    bi.setPosition(surfaceStart + md3Surface.offsetShaders);

    for (int shaderIndex = 0; shaderIndex < md3Surface.numShaders; ++shaderIndex) {
        // Read shader name and index
        const std::string& shaderPath = bi.readString(64);
        const int unusedIndex = bi.readInt32();
        
        /*
        // Find base filename for shader
        const std::string& shaderName = filenameBaseExt(shaderPath);

        Shader/textures loaded elsewhere in the current implementation
        // Ignore empty shader names for now (filled in with .skin file)
        if (! shaderName.empty() && FileSystem::exists(pathConcat(m_modelDir, shaderName))) {
            triList.m_texture = Texture::fromFile(m_modelDir + shaderName, ImageFormat::AUTO(), Texture::DIM_2D_NPOT);
        }
        */
    }

    // Read texture coordinates
    bi.setPosition(surfaceStart + md3Surface.offsetUVs);

    triList.m_textureCoords.resize(md3Surface.numVertices);

    for (int coordIndex = 0; coordIndex < md3Surface.numVertices; ++coordIndex) {
        triList.m_textureCoords[coordIndex].x = bi.readFloat32();
        triList.m_textureCoords[coordIndex].y = bi.readFloat32();
    }

    // Read vertices
    bi.setPosition(surfaceStart + md3Surface.offsetVertices);

    triList.m_geometry.resize(md3Surface.numFrames);

    const int N = md3Surface.numVertices;
    for (int frameIndex = 0; frameIndex < md3Surface.numFrames; ++frameIndex) {
        MeshAlg::Geometry& geom = triList.m_geometry[frameIndex];
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

            nlat *= static_cast<float>(pi() / 128.0f);
            nlng *= static_cast<float>(pi() / 128.0f);

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
    System::memset(m_parts, 0, sizeof(m_parts));
}


MD3Model::~MD3Model() {
    for (int partIndex = 0; partIndex < NUM_PARTS; ++partIndex) {
        if (m_parts[partIndex]) {
            delete m_parts[partIndex];
            m_parts[partIndex] = NULL;
        }
    }
}


MD3Model::Ref MD3Model::fromDirectory(const std::string& modelDir, const Skin::Ref& defaultSkin) {
    MD3Model* model = new MD3Model();

    // Create default spec to load all parts with default skin and materials
    Specification spec;
    spec.directory = modelDir;
    spec.parts[PART_LOWER].load = true;
    spec.parts[PART_UPPER].load = true;
    spec.parts[PART_HEAD].load = true;
    spec.parts[PART_WEAPON].load = true;

    model->loadSpecification(spec);
    model->m_defaultSkin = defaultSkin;

    return model;
}


void MD3Model::loadSpecification(const Specification& spec) {
    // Load animation.cfg file
    loadAnimationCfg(pathConcat(spec.directory, "animation.cfg"));

    // Load legs
    std::string filename = pathConcat(spec.directory, "lower.md3");

    m_parts[PART_LOWER] = new MD3Part;
    
    if (!m_parts[PART_LOWER]->loadFile(filename)) {
        debugAssertM(false, format("Unable to load %s.", filename.c_str()));
        return;
    }

    // Load torso
    if (spec.parts[PART_UPPER].load) {
        filename = pathConcat(spec.directory, "upper.md3");

        m_parts[PART_UPPER] = new MD3Part;
        
        if (!m_parts[PART_UPPER]->loadFile(filename)) {
            debugAssertM(false, format ("Unable to load %s.", filename.c_str()));
            return;
        }
    }

    // Load head
    if (spec.parts[PART_HEAD].load) {
        filename = pathConcat(spec.directory, "head.md3");

        m_parts[PART_HEAD] = new MD3Part;
        
        if (!m_parts[PART_HEAD]->loadFile(filename)) {
            debugAssertM(false, format("Unable to load %s.", filename.c_str()));
            return;
        }
    }

    // Load weapon (if it exists, optional)
    if (spec.parts[PART_WEAPON].load) {
        filename = pathConcat(spec.directory, "weapon.md3");
        if (! FileSystem::exists(filename)) {
            return;
        }

        m_parts[PART_WEAPON] = new MD3Part;
        
        if (!m_parts[PART_WEAPON]->loadFile(filename)) {
            debugAssertM(false, format("Unable to load %s.", filename.c_str()));
            return;
        }
    }
}


void MD3Model::loadAnimationCfg(const std::string& filename) {
    TextInput::Settings settings;
    settings.generateNewlineTokens = true;

    TextInput ti(filename, settings);

    for (int animIndex = 0; animIndex < NUM_ANIMATIONS; ++animIndex) {
        // Check if animations have started in file
        while (ti.hasMore() && (ti.peek().extendedType() != Token::INTEGER_TYPE)) {
            // Eat until next line starting with an integer token
            while (ti.hasMore() && (ti.peek().type() != Token::NEWLINE)) {
                ti.read();
            }

            ti.read();
        }

        // Return early if this is an invalid file
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

    // Loop through all legs animations and adjust starting frame number to be relative to part
    float numTorsoAnimations = m_animations[START_LOWER].start - m_animations[START_UPPER].start;
    for (int animIndex = START_LOWER; animIndex <= END_LOWER; ++animIndex) {
        m_animations[animIndex].start -= numTorsoAnimations;
    }
}


void MD3Model::pose(Array<Surface::Ref>& posedModelArray, const CoordinateFrame& cframe, const Pose& pose) {

    // Coordinate frame built up from lower part
    CoordinateFrame baseFrame = cframe;

    // Pose lower part
    if (! m_parts[PART_LOWER]) {
        return;
    }

    posePart(PART_LOWER, pose, posedModelArray, baseFrame);

    float legsFrameNum = findFrameNum(pose.legsAnim, pose.legsTime);
    baseFrame = baseFrame * m_parts[PART_LOWER]->tag(legsFrameNum, "tag_torso");

    // Pose upper part
    if (! m_parts[PART_UPPER]) {
        return;
    }

    posePart(PART_UPPER, pose, posedModelArray, baseFrame);

    float torsoFrameNum = findFrameNum(pose.torsoAnim, pose.torsoTime);

    // Pose weapon part (since it uses a torso tag)
    if (m_parts[PART_WEAPON]) {
        CoordinateFrame weaponFrame = baseFrame * m_parts[PART_UPPER]->tag(torsoFrameNum, "tag_weapon");
        posePart(PART_WEAPON, pose, posedModelArray, baseFrame);
    }

    baseFrame = baseFrame * m_parts[PART_UPPER]->tag(torsoFrameNum, "tag_head");

    // Pose head part
    if (! m_parts[PART_HEAD]) {
        return;
    }

    posePart(PART_HEAD, pose, posedModelArray, baseFrame);
}


void MD3Model::posePart(PartType partType, const Pose& pose, Array<Surface::Ref>& posedModelArray, const CoordinateFrame& cframe) {
    const MD3Part* part = m_parts[partType];

    Skin::Ref skin;
    if (pose.skin.isNull()) {
        skin = m_defaultSkin;
    } else {
        skin = pose.skin;
    }

    for (int surfaceIndex = 0; surfaceIndex < part->m_triListArray.length(); ++surfaceIndex) {

        const MD3Part::TriList& triList = part->m_triListArray[surfaceIndex];
        Material::Ref material;

        // Make sure there is a skin for this part
        if (skin.isNull()) {
            material = defaultMaterial();
        } else if (skin->partSkin.size() > partType) {
            const Skin::PartSkin& partSkin = skin->partSkin[partType];

            // See if there is a skin for this trilist
            Material::Ref* ptr = partSkin.getPointer(triList.m_name);
            if (ptr) {
                material = *ptr;
            } else {
                // Use default material
                material = defaultMaterial();
            }
        }

        if (material.isNull()) {
            // triLists with an intentionally NULL material do not render
            continue;
        }

        // Set up blending
        /////////////////////////////////////////////////////////////////
        // TODO: abstract this blending logic into a method
        // TODO: handle blends between different animations the way that MD2Model does)

        float frameNum = 0.0f;

        if (partType == PART_LOWER) {
            frameNum = findFrameNum(pose.legsAnim, pose.legsTime);
        } else if (partType == PART_UPPER) {
            frameNum = findFrameNum(pose.torsoAnim, pose.torsoTime);
        }

        // Calculate frames for blending
        int frame1 = iFloor(frameNum);
        int frame2 = iClamp(iCeil(frameNum), 0, triList.m_numFrames - 1);
        float interp = fmod(frameNum, 1.0f);

        /////////////////////////////////////////////////////////////////

        // Keep a back pointer so that the index array can't be deleted
        CFrame partFrame = cframe;

        // TODO: shouldn't this translation be in the reference frame of CFrame?  That is,
        // should this be: trans += cframe.rotation * (...)
        partFrame.translation += part->m_frames[frame1].m_localOrigin.lerp(part->m_frames[frame2].m_localOrigin, interp);

        SuperSurface::Ref surface = 
            SuperSurface::create
               (part->m_modelName + "::" + triList.m_name, 
                partFrame, 
                SuperSurface::GPUGeom::create(), 
                SuperSurface::CPUGeom(), 
                this);

        // Use the internal storage of the surface
        SuperSurface::CPUGeom& cpuGeom = surface->cpuGeom();

        // Need an empty array for the tangents; safe to make static since this is never used.
        const static Array<Vector4> packedTangentArray;
        cpuGeom.index         = &triList.m_indexArray;
        cpuGeom.geometry      = &surface->internalGeometry();
        cpuGeom.packedTangent = &packedTangentArray;
        cpuGeom.texCoord0     = &triList.m_textureCoords;

        surface->gpuGeom()->material = material;

        // Copy blended vertex data for frame
        const MeshAlg::Geometry& geom1 = triList.m_geometry[frame1];
        const MeshAlg::Geometry& geom2 = triList.m_geometry[frame2];

        const int N = geom1.vertexArray.size();
        Array<Vector3>& vertexArray = const_cast< Array<Vector3>& >(cpuGeom.geometry->vertexArray);
        Array<Vector3>& normalArray = const_cast< Array<Vector3>& >(cpuGeom.geometry->normalArray);
        vertexArray.resize(N);
        normalArray.resize(N);
        for (int v = 0; v < N; ++v) {
            vertexArray[v] = geom1.vertexArray[v].lerp(geom2.vertexArray[v], interp);
            normalArray[v] = geom1.normalArray[v].lerp(geom2.normalArray[v], interp);
        }

        // Upload data to the GPU
        SuperSurface::GPUGeom::Ref gpuGeom = surface->gpuGeom();
        cpuGeom.copyVertexDataToGPU(gpuGeom->vertex, gpuGeom->normal, gpuGeom->packedTangent, 
                                    gpuGeom->texCoord0, VertexBuffer::WRITE_EVERY_FRAME);

        gpuGeom->index = triList.m_gpuIndex;

        // TODO: replace with accurate bounds
        gpuGeom->boxBounds = AABox(-Vector3::inf(), Vector3::inf());
        gpuGeom->sphereBounds = Sphere(Vector3::zero(), finf());

        posedModelArray.append(surface);
    }
}


float MD3Model::findFrameNum(AnimType animType, GameTime animTime) {
    debugAssert(animType < NUM_ANIMATIONS);

    float frameNum = m_animations[animType].start;

    float initialLoopTime = (m_animations[animType].num / m_animations[animType].fps);

    if (animTime < initialLoopTime) {
        // Less than 1 loop complete, no need to account for "loop" value
        frameNum += static_cast<float>(animTime / initialLoopTime) * m_animations[animType].num;
    } else {
        if (m_animations[animType].loop > 0.0f) {
            // otherwise find actual frame number after number of loops
            animTime -= initialLoopTime;

            // Find time for all subsequent loops
            float otherLoopTime = m_animations[animType].loop / m_animations[animType].fps;

            // How far into the last loop
            float timeIntoLastLoop = fmod(static_cast<float>(animTime), otherLoopTime);

            // "loop" works by specifying the last number of frames to loop over
            // so a loop of 1 with num frames 5 means looping starts at frame 4 with frames {1, 2, 3, 4, 5} originally

            frameNum += (m_animations[animType].num - m_animations[animType].loop);

            frameNum += (timeIntoLastLoop / otherLoopTime) * m_animations[animType].loop;
        } else {
            frameNum += m_animations[animType].num - 1;
        }
    }

    return frameNum;
}


} // namespace G3D
