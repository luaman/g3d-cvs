/**
 @file ArticulatedModel.cpp
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu

 @created 2003-09-14
 @edited  2009-11-14
 */

#include "GLG3D/ArticulatedModel.h"
#include "GLG3D/IFSModel.h"
#include "Load3DS.h"
#include "G3D/ThreadSet.h"
#include "GLG3D/GLCaps.h"
#include "G3D/Any.h"
#include "G3D/FileSystem.h"

namespace G3D {
        
ArticulatedModel::Specification::Specification() {}

ArticulatedModel::Specification::Specification(const Any& any) {
    *this = Specification();
    any.verifyName("ArticulatedModel::Specification");
    for (Any::AnyTable::Iterator it = any.table().begin(); it.hasMore(); ++it) {
        const std::string& key = toLower(it->key);
        if (key == "filename") {
            filename = System::findDataFile(it->value.string());
        } else if (key == "preprocess") {
            preprocess = it->value;
        } else if (key == "settings") {
            settings = it->value;
        } else {
            any.verify(false, "Illegal key: " + it->key);
        }
    }
}

ArticulatedModel::Specification::operator Any() const {
    Any a(Any::TABLE, "ArticulatedModel::Specification");
    a.set("filename", filename);
    a.set("preprocess", preprocess);
    a.set("settings", settings);
    return a;
}

///////////////////////////////////////////////////////////

ArticulatedModel::Preprocess::Preprocess(const Any& any) {
    *this = Preprocess();
    any.verifyName("ArticulatedModel::Preprocess");
    for (Any::AnyTable::Iterator it = any.table().begin(); it.hasMore(); ++it) {
        const std::string& key = toLower(it->key);
        if (key == "stripmaterials") {
            stripMaterials = it->value.boolean();
        } else if (key == "texturedimension") {
            // TODO
        } else if (key == "addbumpmaps") {
            addBumpMaps = it->value.boolean();
        } else if (key == "xform") {
            xform = it->value;
        } else if (key == "parallaxsteps") {
            parallaxSteps = it->value;
        } else if (key == "bumpmapscale") {
            bumpMapScale = it->value;
        } else if (key == "normalmapwhiteheightinpixels") {
            normalMapWhiteHeightInPixels = it->value;
        } else if (key == "materialsubstitution") {
            // TODO
        } else if (key == "materialoverride") {
            materialOverride = Material::create(it->value);
        } else {
            any.verify(false, "Illegal key: " + it->key);
        }
    }
}

ArticulatedModel::Preprocess::operator Any() const {
    Any a(Any::TABLE, "ArticulatedModel::Preprocess");
    a.set("stripMaterials", stripMaterials);


    // TODO: a.set("materialOverride", stripMaterials);
    // a["textureDimension"] = TODO
    a.set("addBumpMaps", addBumpMaps);
    a.set("xform", xform.operator Any());
    a.set("parallaxSteps", parallaxSteps);
    a.set("bumpMapScale", bumpMapScale);
    a.set("normalMapWhiteHeightInPixels", normalMapWhiteHeightInPixels);
    //a["materialSubstitution"] = materialSubstitution

    return a;
}

///////////////////////////////////////////////////////////

ArticulatedModel::Settings::Settings(const Any& any) {
    *this = Settings();
    any.verifyName("ArticulatedModel::Settings");
    for (Any::AnyTable::Iterator it = any.table().begin(); it.hasMore(); ++it) {
        const std::string& key = toLower(it->key);
        if (key == "weld") {
            weld = it->value;
        } else {
            any.verify(false, "Illegal key: " + it->key);
        }
    }
}

ArticulatedModel::Settings::operator Any() const {
    Any a(Any::TABLE, "ArticulatedModel::Settings");
    a.set("weld", weld);
    return a;
}
//////////////////////////////////////////////////////////

ArticulatedModel::Ref ArticulatedModel::createHeightfield(const Image1::Ref& height, float xzExtent, float yExtent, const Vector2& texScale) {
    ArticulatedModel::Ref model = ArticulatedModel::createEmpty();
    ArticulatedModel::Part& part = model->partArray.next();
    ArticulatedModel::Part::TriList::Ref triList = part.newTriList();

    bool spaceCentered = true;
    bool twoSided = false;

    MeshAlg::generateGrid(part.geometry.vertexArray, part.texCoordArray, triList->indexArray, height->width() - 1, height->height() - 1, texScale, 
        spaceCentered, twoSided, CFrame(Matrix4::scale(xzExtent, yExtent, xzExtent).upper3x3()), height);
    part.name = "Root";

    triList->primitive = PrimitiveType::TRIANGLES;
    triList->twoSided = false;

    model->updateAll();

    return model;
}

void ArticulatedModel::setStorage(ImageStorage s) {
    for (int p = 0; p < partArray.size(); ++p) {
        Part& part = partArray[p];
        for (int t = 0; t < part.triList.size(); ++t) {
            part.triList[t]->material->setStorage(s);
        }
    }
}


ArticulatedModel::Part::TriList::Ref ArticulatedModel::Part::newTriList(const Material::Ref& mat) {
    TriList::Ref t = new TriList();

    if (mat.isNull()) {
        Material::Settings s;
        s.setLambertian(Color3::white() * 0.8f);
        s.setSpecular(Color3::black());
        s.setShininess(0);

        t->material = Material::create(s);
    } else {
        t->material = mat;
    }

    triList.append(t);
    return t;
}


ArticulatedModel::Ref ArticulatedModel::fromFile(const std::string& filename, const Vector3& scale) {
    return fromFile(filename, Preprocess(scale), Settings());
}


ArticulatedModel::Ref ArticulatedModel::fromFile(const std::string& filename, const CoordinateFrame& xform) {
    return fromFile(filename, Preprocess(xform.toMatrix4()), Settings());
}


ArticulatedModel::Ref ArticulatedModel::fromFile(const std::string& filename, const Matrix4& xform) {
    return fromFile(filename, Preprocess(xform), Settings());
}

ArticulatedModel::Ref ArticulatedModel::fromFile(const std::string& filename, float scale) {
    return fromFile(filename, Preprocess(scale), Settings());
}


ArticulatedModel::Ref ArticulatedModel::fromFile(const std::string& filename, const Preprocess& preprocess, const Settings& settings) {
    alwaysAssertM(FileSystem::exists(filename),
        filename + " cannot be loaded by ArticulatedModel because it does not exist.");

    ArticulatedModel* model = new ArticulatedModel();
    model->setSettings(settings);

    std::string ext = filenameExt(toLower(filename));

    if (ext == "3ds") {
        model->init3DS(filename, preprocess);
    } else if ((ext == "ifs") || (ext == "ply2") || (ext == "off")) {
        model->initIFS(filename, preprocess.xform);
    }

    if (preprocess.materialOverride.notNull()) {
        for (int p = 0; p < model->partArray.size(); ++p) {
            Part& part = model->partArray[p];
            for (int t = 0; t < part.triList.size(); ++t) {
                part.triList[t]->material = preprocess.materialOverride;
            }
        }
    }

    model->updateAll();

    return model;
}


ArticulatedModel::Ref ArticulatedModel::createEmpty() {
    return new ArticulatedModel();
}

/** If filename + any image extension exists, returns the name, otherwise returns "". */
static std::string findAnyImage(const std::string& filename) {
    static const char* ext[] = {"png", "jpg", "tga", "bmp", "pcx", ""};
    // TODO: Case sensitivity
    for (int i = 0; ext[i][0] != '\0'; ++i) {
        std::string tmp = filename + "." + ext[i];
        if (FileSystem::exists(tmp)) {
            return tmp;
        }
    }
    return "";
}

void ArticulatedModel::init3DS(const std::string& filename, const Preprocess& preprocess) {

    const Matrix4& xform = preprocess.xform;

    // Note: vertices are actually mutated by scale; it is not carried along as
    // part of the scene graph transformation.

    // Note: moving textures from CPU to GPU is the slow part of this process

    // Returns the index in partArray of the part with this name.
    Table<std::string, int>     partNameToIndex;

    // Cached bump maps; if NULL, there is no bump map for this file
    Table<std::string, Texture::Ref> bumpMap;

    //Table<std::string, TextureRef> texCache;

    std::string path = filenamePath(filename);
    Load3DS load;
    load.load(filename);

    partArray.resize(load.objectArray.size());

    // Rotation/scale component
    Matrix3 R = xform.upper3x3();

    for (int p = 0; p < load.objectArray.size(); ++p) {
        Load3DS::Object& object = load.objectArray[p];

        Part& part = partArray[p];

        // Process geometry
        part.geometry.vertexArray = object.vertexArray;
        std::string name = object.name;
        int count = 0;
        while (partNameToIndex.containsKey(name)) {
            ++count;
            name = object.name + format("_#%d", count);
        }

        part.cframe = object.keyframe.approxCoordinateFrame();
        debugAssert(isFinite(part.cframe.rotation.determinant()));
        debugAssert(part.cframe.rotation.isOrthonormal());

        if (! part.cframe.rotation.isRightHanded()) {
            // TODO: how will this impact other code?  I think we can't just strong-arm it like this
            part.cframe.rotation.setColumn(0, -part.cframe.rotation.column(0));
        }

        debugAssert(part.cframe.rotation.isRightHanded());

        // Scale and rotate the cframe positions, but do not translate them
        part.cframe.translation = R * part.cframe.translation;

        debugAssert(R.column(0).isFinite());

        part.name = name;
        partNameToIndex.set(part.name, p);

        // All 3DS parts are promoted to the root in the current implementation.
        // TODO: this makes good animation impossible
        part.parent = -1;

        //debugPrintf("%s %d %d\n", object.name.c_str(), object.hierarchyIndex, object.nodeID);

        if (part.hasGeometry()) {
            // Convert vertices to object space (there is no surface normal data at this point)
            debugAssert(part.geometry.normalArray.size() == 0);
            Matrix4 netXForm = part.cframe.inverse().toMatrix4() * xform;
            
            debugAssertM(netXForm.row(3) == Vector4(0,0,0,1), 
                        "3DS file loading requires that the last row of the xform matrix be 0, 0, 0, 1");
            
            const Matrix3& S = netXForm.upper3x3();
            const Vector3& T = netXForm.column(3).xyz();
            for (int v = 0; v < part.geometry.vertexArray.size(); ++v) {
#               ifdef G3D_DEBUG
                {
                    const Vector3& vec = part.geometry.vertexArray[v];
                    debugAssert(vec.isFinite());
                }
#               endif

                part.geometry.vertexArray[v] = S * part.geometry.vertexArray[v] + T;

#               ifdef G3D_DEBUG
                {
                    const Vector3& vec = part.geometry.vertexArray[v];
                    debugAssert(vec.isFinite());
                }
#               endif
            }

            part.texCoordArray = object.texCoordArray;

            if (object.faceMatArray.size() == 0) {

                // Lump everything into one part
                Part::TriList::Ref triList = part.newTriList();
                triList->indexArray = object.indexArray;

            } else {
                for (int m = 0; m < object.faceMatArray.size(); ++m) {
                    const Load3DS::FaceMat& faceMat = object.faceMatArray[m];

                    if (faceMat.faceIndexArray.size() > 0) {

                        Material::Ref mat;
                        bool twoSided = false;

                        const std::string& materialName = faceMat.materialName;
                        if (load.materialNameToIndex.containsKey(materialName)) {
                            int i = load.materialNameToIndex[materialName];
                            const Load3DS::Material& material = load.materialArray[i];
                            if (! preprocess.materialSubstitution.get(material.texture1.filename, mat)) {
                                const Material::Settings& spec = compute3DSMaterial(&material, path, preprocess);
                                mat = Material::create(spec);
                            }
                            twoSided = material.twoSided || mat->hasAlphaMask();
                        } else {
                            mat = Material::create();
                            logPrintf("Referenced unknown material '%s'\n", materialName.c_str());
                        }                        

                        Part::TriList::Ref triList = part.newTriList(mat);
                        debugAssert(isValidHeapPointer(triList.pointer()));
                        triList->twoSided = twoSided;

                        // Construct an index array for this part
                        for (int i = 0; i < faceMat.faceIndexArray.size(); ++i) {
                            // 3*f is an index into object.indexArray
                            int f = faceMat.faceIndexArray[i];
                            debugAssert(f >= 0);
                            for (int v = 0; v < 3; ++v) {
                                triList->indexArray.append(object.indexArray[3 * f + v]);
                            }
                        }
                        debugAssert(triList->indexArray.size() > 0);


                    } // if there are indices on this part
                } // for m
            } // if has materials 
        }
    }
}


static std::string find3DSTexture(std::string _filename, const std::string& path) {    
    if (_filename != "") {
        std::string filename = _filename;
        if (endsWith(toUpper(filename), "GIF")) {
            // Load PNG instead of GIF, since we can't load GIF
            filename = filename.substr(0, filename.length() - 3) + "png";
        }

        if (! FileSystem::exists(filename) && FileSystem::exists(pathConcat(path, filename))) {
            filename = pathConcat(path, filename);
        }

        // Load textures
        filename = System::findDataFile(filename, false);
        
        if (filename == "") {
            logPrintf("Could not locate 3DS file texture '%s'\n", _filename.c_str());
        }
        return filename;
    } else {
        return "";
    }
}

Material::Settings ArticulatedModel::compute3DSMaterial
(const void*         ptr,
 const std::string&  path,
 const Preprocess&   preprocess) {

    const Load3DS::Material& material = *reinterpret_cast<const Load3DS::Material*>(ptr);

    Material::Settings spec;

    if (preprocess.stripMaterials || preprocess.materialOverride.notNull()) {
        spec.setLambertian(Color3::one() * 0.7f);
        spec.setSpecular(Color3::one() * 0.2f);
        spec.setGlossyExponentShininess(100);
        return spec;
    }

    const Load3DS::Map& texture1 = material.texture1;

    const Color4& lambertianConstant = 
        Color4((Color3::white() * material.texture1.pct) *
               (1.0f - material.transparency), 1.0f);

    std::string lambertianFilename = find3DSTexture(texture1.filename, path);
    
    spec.setLambertian(lambertianFilename, lambertianConstant);

    // Strength of the shininess (higher is brighter)
    spec.setSpecular(material.shininessStrength * material.specular * (1.0f - material.transparency));

    //extent (area, higher is closely contained, lower is spread out) of shininess
    // Do not exceed 128, which is the OpenGL fixed function maximum
    spec.setShininess(material.shininess * 128);

    spec.setTransmissive(Color3::white() * material.transparency);
    spec.setEmissive(Color3::white() * material.emissive);

    std::string bumpFilename = find3DSTexture(material.bumpMap.filename, path);
    if (bumpFilename != "") {
        // TODO: use percentage specified in material.bumpMap
        spec.setBump(bumpFilename);
    }

    // TODO: load reflection, specular, etc maps.
    // triList->material.reflect.map = 

    if (preprocess.addBumpMaps) {
        // See if a bump map exists:
        std::string filename = 
            pathConcat(pathConcat(path, filenamePath(texture1.filename)),
                       filenameBase(texture1.filename) + "-bump");

        filename = findAnyImage(filename);
        if (filename != "") {
            BumpMap::Settings s;
            s.scale = preprocess.bumpMapScale;
            s.bias = 0;
            s.iterations = preprocess.parallaxSteps;
            spec.setBump(filename, s, preprocess.normalMapWhiteHeightInPixels);
        }
    } // if bump maps

    return spec;
}


void ArticulatedModel::Part::computeNormalsAndTangentSpace
    (const ArticulatedModel::Settings& settings) {

    if (triList.size() == 0) {
        geometry.vertexArray.resize(0);
        geometry.normalArray.resize(0);
        indexArray.resize(0);
        texCoordArray.resize(0);
        return;
    }

    Array<Array<int>*> indexArrayArray;
    indexArrayArray.resize(triList.size());
    for (int t = 0; t < triList.size(); ++t) {
        indexArrayArray[t] = &(triList[t]->indexArray);
    }

    if (geometry.vertexArray.size() > 0) {
        Welder::weld(geometry.vertexArray,
                     texCoordArray,
                     geometry.normalArray,
                     indexArrayArray,
                     settings.weld);
    }

    Array<MeshAlg::Face>    faceArray;
    Array<MeshAlg::Vertex>  vertexArray;
    Array<MeshAlg::Edge>    edgeArray;
    Array<Vector3>          faceNormalArray;

    Stopwatch s;
    computeIndexArray();

    MeshAlg::computeAdjacency(geometry.vertexArray, indexArray, faceArray, edgeArray, vertexArray);

    // Compute a tangent space basis
    if (texCoordArray.size() > 0) {
        // computeTangentSpaceBasis will generate binormals, but
        // we throw them away and recompute
        // them in the vertex shader.
        Array<Vector3> T;
        Array<Vector3> B;

        MeshAlg::computeTangentSpaceBasis(
            geometry.vertexArray,
            texCoordArray,
            geometry.normalArray,
            faceArray,
            T,
            B);

        // Pack the tangents 
        packedTangentArray.resize(T.size());
        for (int i = 0; i < T.size(); ++i) {
            const Vector3& t = T[i];
            const Vector3& b = B[i];
            const Vector3& n = geometry.normalArray[i];
            Vector4& p = packedTangentArray[i];
            p.x = t.x;
            p.y = t.y;
            p.z = t.z;
            p.w = sign(t.cross(b).dot(n));
        }
    } else {
        packedTangentArray.clear();
    }
}


void ArticulatedModel::Part::updateVAR(VertexBuffer::UsageHint hint) {
    if (geometry.vertexArray.size() == 0) {
        // Has no geometry
        return;
    }

    SuperSurface::CPUGeom g(NULL, &geometry, &texCoordArray, &packedTangentArray);
    g.copyVertexDataToGPU(vertexVAR, normalVAR, packedTangentVAR, texCoord0VAR, hint);

    for (int i = 0; i < triList.size(); ++i) {
        triList[i]->updateVAR(hint, vertexVAR, normalVAR, packedTangentVAR, texCoord0VAR);
    }
}


void ArticulatedModel::Part::computeBounds() {
    for (int t = 0; t < triList.size(); ++t) {
        triList[t]->computeBounds(*this);
    }
}

/** Used by ArticulatedModel::updateAll */
class PartUpdater : public GThread {
protected:

    Array<ArticulatedModel::Part*>&     m_partArray;
    int                                 m_startIndex;
    int                                 m_endIndex;
    const ArticulatedModel::Settings    m_settings;

public:
    /** Stores the pointer to partArray.*/
    PartUpdater(
        Array<ArticulatedModel::Part*>& partArray, 
        int startIndex, int endIndex,
        const ArticulatedModel::Settings& settings) :
        GThread("Part Updater"),
        m_partArray(partArray),
        m_startIndex(startIndex),
        m_endIndex(endIndex),
        m_settings(settings) {
    }

    /** Processes from startIndex to endIndex, inclusive. */
    virtual void threadMain() {
        for (int i = m_startIndex; i <= m_endIndex; ++i) {
            ArticulatedModel::Part* part = m_partArray[i];
            part->computeNormalsAndTangentSpace(m_settings);
            part->computeBounds();
            debugAssert(part->geometry.normalArray.size() ==
                        part->geometry.vertexArray.size());
        }
    }
};

void ArticulatedModel::updateAll() {
    // Extract the parts with real geometry
    Array<Part*> geometryPart;
    for (int p = 0; p < partArray.size(); ++p) {
        Part* part = &partArray[p];
        if (part->hasGeometry()) {
            geometryPart.append(part);
        } else {
            // Cheap to update this part right here, since it has nothing in it
            part->computeNormalsAndTangentSpace(m_settings);
            part->computeBounds();
            part->updateVAR();
        }
    }

    // Choose a reasonable number of threads
    int numThreads = 1;
    if ((geometryPart.size() >= 2) && (System::numCores() > 1)) {
        // Use at least two cores, and up to n-1 of them
        numThreads = min(geometryPart.size(), max(System::numCores() - 1, 2));
    }
    
    // Assign threads
    ThreadSet threads;
    int startIndex = 0;
    for (int t = 0; t < numThreads; ++t) {
        int endIndex =
            (numThreads == 1) ? 
            (geometryPart.size() - 1) :
            (geometryPart.size() - 1) * t / (numThreads - 1);
        GThread::Ref thread = new PartUpdater(geometryPart, startIndex, endIndex, m_settings);
        threads.insert(thread);
        startIndex = endIndex + 1;
    }
    debugAssertM(startIndex == geometryPart.size(), 
                 "Did not spawn threads for all parts");
    threads.start(GThread::USE_CURRENT_THREAD);
    threads.waitForCompletion();

    // Upload data to GPU
    for (int p = 0; p < geometryPart.size(); ++p) {
        Part* part = geometryPart[p];
        part->updateVAR();
    }

    m_numTriangles = 0;
    for (int p = 0; p < partArray.size(); ++p) {
        Part& part = partArray[p];
        m_numTriangles += part.indexArray.size() / 3;
    }

#   ifdef G3D_DEBUG
    // Check for correctness
    for (int p = 0; p < partArray.size(); ++p) {
        Part& part = partArray[p];
        debugAssert(part.geometry.normalArray.size() == part.geometry.vertexArray.size());
    }
#   endif
}


void ArticulatedModel::initIFS(const std::string& filename, const Matrix4& xform) {
    Array<int>      index;
    Array<Vector3>  vertex;
    Array<Vector2>  texCoord;
    
    IFSModel::load(filename, name, index, vertex, texCoord);

    // Transform vertices
    for (int v = 0; v < vertex.size(); ++v) {
        vertex[v] = xform.homoMul(vertex[v], 1.0f);
    }

    // Convert to a Part
    Part& part = partArray.next();

    part.cframe = CoordinateFrame();
    part.name = "root";
    part.parent = -1;
    part.geometry.vertexArray = vertex;
    part.texCoordArray = texCoord;

    Part::TriList::Ref triList = part.newTriList();
    triList->indexArray = index;
}


void ArticulatedModel::Part::TriList::updateVAR
    (VertexBuffer::UsageHint hint,
     const VertexRange& vertexVAR,
     const VertexRange& normalVAR,
     const VertexRange& tangentVAR,
     const VertexRange& texCoord0VAR) {

    vertex = vertexVAR;
    normal = normalVAR;
    packedTangent = tangentVAR;
    texCoord0 = texCoord0VAR;

    if (indexArray.size() == 0) {
        // Has no indices
        return;
    }

    int indexSize = sizeof(int);
    if (index.size() != indexArray.size()) {
        // Create new
        VertexBufferRef area = VertexBuffer::create(indexSize * indexArray.size(), hint, VertexBuffer::INDEX);
        index = VertexRange(indexArray, area);
    } else {
        // Update in place
        index.update(indexArray);
    }
}


void ArticulatedModel::Part::TriList::computeBounds(const Part& parentPart) {
    if (indexArray.size() > 0) { 
        MeshAlg::computeBounds(parentPart.geometry.vertexArray, indexArray, boxBounds, sphereBounds);
    } else {
        boxBounds = AABox();
        sphereBounds = Sphere();
    }
}


void ArticulatedModel::Part::computeIndexArray() {
    indexArray.clear();
    for (int t = 0; t < triList.size(); ++t) {
        indexArray.append(triList[t]->indexArray);
    }
}


static void addRect(const Vector3& v0, const Vector3& v1, 
                    const Vector3& v2, const Vector3& v3, 
                    Array<Vector3>& vertexArray, 
                    Array<int>& indexArray) {

    int v = vertexArray.size();
    vertexArray.append(v0, v1, v2, v3);

    indexArray.append(v + 0, v + 1, v + 2);
    indexArray.append(v + 0, v + 2, v + 3);
}


ArticulatedModel::Ref ArticulatedModel::createCornellBox(float scale, const Color3& left, const Color3& right, const Color3& walls) {

    ArticulatedModel::Ref model = ArticulatedModel::createEmpty();
    model->name = "Cornell Box";

    ArticulatedModel::Part& part = model->partArray.next();
    Array<Vector3>& vertex = part.geometry.vertexArray;
    part.name = "root";

    float c = -0.275f * scale;

    // Data used is captured from the photographs and balanced to
    // achieve (perceptual) uniform brightness on all surfaces; this
    // integrates the spectral data.

    // White faces
    {
        ArticulatedModel::Part::TriList::Ref triList = part.newTriList(Material::createDiffuse(walls));
        triList->twoSided = true;

        Array<int>& index = triList->indexArray;

        // Top
        addRect(Vector3(-c,  c,  c), Vector3(-c,  c, -c), Vector3( c,  c, -c), Vector3( c,  c,  c), vertex, index);
    
        // Back
        addRect(Vector3(-c,  c, -c), Vector3(-c, -c, -c), Vector3( c, -c, -c), Vector3( c,  c, -c), vertex, index);

        // Floor
        addRect(Vector3( c, -c,  c), Vector3( c, -c, -c), Vector3(-c, -c, -c), Vector3(-c, -c,  c), vertex, index);
    }

    // Left red face
    {
        ArticulatedModel::Part::TriList::Ref triList = part.newTriList(Material::createDiffuse(left));
        triList->twoSided = true;

        Array<int>& index = triList->indexArray;
        addRect(Vector3(-c,  c,  c), Vector3(-c, -c,  c), Vector3(-c, -c, -c), Vector3(-c,  c, -c), vertex, index);
    }

    // Right green face
    {
        ArticulatedModel::Part::TriList::Ref triList = part.newTriList(Material::createDiffuse(right));
        triList->twoSided = true;

        Array<int>& index = triList->indexArray;
        addRect(Vector3( c,  c, -c), Vector3( c, -c, -c), Vector3( c, -c,  c), Vector3( c,  c,  c), vertex, index);
    }

    model->updateAll();

    return model;
}


void ArticulatedModel::facet() {
    for (int p = 0; p < partArray.size(); ++p) {
        Part& dstPart = partArray[p];

        // Copy the old part
        Part srcPart = dstPart;

        dstPart.geometry.vertexArray.fastClear();
        dstPart.geometry.normalArray.fastClear();
        dstPart.texCoordArray.fastClear();
        dstPart.indexArray.fastClear();

        int n = 0;
        for (int t = 0; t < srcPart.triList.size(); ++t) {
            const Part::TriList::Ref srcTriList = srcPart.triList[t];
            Part::TriList::Ref dstTriList = dstPart.triList[t];
            dstTriList->indexArray.fastClear();

            // Unroll the arrays
            for (int x = 0; x < srcTriList->indexArray.size(); ++x) {
                int i = srcTriList->indexArray[x];

                // Create the vertex
                dstPart.geometry.vertexArray.append(srcPart.geometry.vertexArray[i]);
                if (srcPart.texCoordArray.size() > 0) {
                    dstPart.texCoordArray.append(srcPart.texCoordArray[i]);
                }
                dstPart.indexArray.append(n);
                dstTriList->indexArray.append(n);
                ++n;
            }
        }
    }
    updateAll();
}


const ArticulatedModel::Pose& ArticulatedModel::defaultPose() {
    static const Pose p;
    return p;
}


void ArticulatedModel::pose(
    Array<Surface::Ref>&     posedArray, 
    const CoordinateFrame&      cframe, 
    const Pose&                 posex) {

    for (int p = 0; p < partArray.size(); ++p) {
        const Part& part = partArray[p];
        if (part.parent == -1) {
            // This is a root part, pose it
            part.pose(this, p, posedArray, cframe, posex);
        }
    }
}


void ArticulatedModel::Part::pose
    (const ArticulatedModel::Ref&      model,
     int                               partIndex,
     Array<Surface::Ref>&              posedArray,
     const CoordinateFrame&            parent, 
     const Pose&                       posex) const {

    CoordinateFrame frame;

    if (posex.cframe.containsKey(name)) {
        frame = parent * cframe * posex.cframe[name];
    } else {
        frame = parent * cframe;
    }

    debugAssert(! isNaN(frame.translation.x));
    debugAssert(! isNaN(frame.rotation[0][0]));

    if (hasGeometry()) {

        for (int t = 0; t < triList.size(); ++t) {
            if (triList[t].notNull() && (triList[t]->indexArray.size() > 0)) {
                SuperSurface::CPUGeom cpuGeom(&triList[t]->indexArray, &geometry, 
                                              &texCoordArray, &packedTangentArray);

                posedArray.append(SuperSurface::create(model->name, frame, triList[t],
                                                            cpuGeom, model));
            }
        }
    }

    // Recursively pose subparts and pass along our coordinate frame.
    for (int i = 0; i < subPartArray.size(); ++i) {
        int p = subPartArray[i];
        debugAssertM(model->partArray[p].parent == partIndex,
            "Parent and child pointers do not match.");(void)partIndex;

        model->partArray[p].pose(model, p, posedArray, frame, posex);
    }
}


} // G3D
