/**
 @file ArticulatedModel.cpp
 @maintainer Morgan McGuire, morgan@cs.williams.edu

 @created 2003-09-14
 @edited  2009-03-14
 */

#include "GLG3D/ArticulatedModel.h"
#include "GLG3D/IFSModel.h"
#include "Load3DS.h"
#include "G3D/ThreadSet.h"
#include "GLG3D/GLCaps.h"

namespace G3D {

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
    return fromFile(filename, PreProcess(scale), Settings());
}


ArticulatedModel::Ref ArticulatedModel::fromFile(const std::string& filename, const CoordinateFrame& xform) {
    return fromFile(filename, PreProcess(xform.toMatrix4()), Settings());
}


ArticulatedModel::Ref ArticulatedModel::fromFile(const std::string& filename, float scale) {
    return fromFile(filename, PreProcess(scale), Settings());
}


ArticulatedModel::Ref ArticulatedModel::fromFile(const std::string& filename, const PreProcess& preprocess, const Settings& settings) {
    debugAssertM(fileExists(filename),
        filename + " cannot be loaded by ArticulatedModel because it does not exist.");

    ArticulatedModel* model = new ArticulatedModel();
    model->setSettings(settings);

    std::string ext = filenameExt(toLower(filename));

    if (ext == "3ds") {
        model->init3DS(filename, preprocess);
    } else if ((ext == "ifs") || (ext == "ply2") || (ext == "off")) {
        model->initIFS(filename, preprocess.xform);
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
        if (fileExists(tmp)) {
            return tmp;
        }
    }
    return "";
}

void ArticulatedModel::init3DS(const std::string& filename, const PreProcess& preprocess) {

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
        const Load3DS::Object& object = load.objectArray[p];

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
        part.parent = -1;

        //debugPrintf("%s %d %d\n", object.name.c_str(), object.hierarchyIndex, object.nodeID);

        if (part.hasGeometry()) {
            // Convert to object space (there is no normal data at this point)
            debugAssert(part.geometry.normalArray.size() == 0);
            Matrix4 netXForm = part.cframe.inverse().toMatrix4() * xform;
            
            debugAssertM(netXForm.row(3) == Vector4(0,0,0,1), 
                        "3DS file loading requires that the last row of the xform matrix be 0, 0, 0, 1");
            
            Matrix3 S = netXForm.upper3x3();
            Vector3 T = netXForm.column(3).xyz();

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

                            const Material::Settings& spec = compute3DSMaterial(&material, path, preprocess);
                            twoSided = material.twoSided;

                            mat = Material::create(spec);

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
                                triList->indexArray.append(object.indexArray[3*f + v]);
                            }
                        }
                        debugAssert(triList->indexArray.size() > 0);


                    } // if there are indices on this part
                } // for m
            } // if has materials 
        }
    }
}


Material::Settings ArticulatedModel::compute3DSMaterial
(const void*         ptr,
 const std::string&  path,
 const PreProcess&   preprocess) {

    const Load3DS::Material& material = *reinterpret_cast<const Load3DS::Material*>(ptr);

    Material::Settings spec;
    spec.setName(material.name);
    spec.setTextureDimension(preprocess.textureDimension);

    const Load3DS::Map& texture1 = material.texture1;

    std::string textureFile = texture1.filename;

    const Color4& lambertianConstant = 
        Color4((Color3::white() * material.texture1.pct) *
               (1.0f - material.transparency), 1.0f);

    std::string lambertianFilename = "";

    if (texture1.filename != "") {
        if (endsWith(toUpper(textureFile), "GIF")) {
            // Load PNG instead of GIF, since we can't load GIF
            textureFile = textureFile.substr(0, textureFile.length() - 3) + "png";
        }

        if (! fileExists(textureFile) && fileExists(path + textureFile)) {
            textureFile = path + textureFile;
        }

        // Load textures
        lambertianFilename = System::findDataFile(textureFile, false);
        
        if (lambertianFilename == "") {
            logPrintf("Could not locate 3DS file texture '%s'\n", textureFile.c_str());
        }
    }
    
    spec.setLambertian(lambertianFilename, lambertianConstant);

    // Strength of the shininess (higher is brighter)
    spec.setSpecular(material.shininessStrength * material.specular * (1.0f - material.transparency));

    //extent (area, higher is closely contained, lower is spread out) of shininess
    // Do not exceed 128, which is the OpenGL fixed function maximum
    spec.setShininess(material.shininess * 128);

    spec.setTransmissive(Color3::white() * material.transparency);
    spec.setEmissive(Color3::white() * material.emissive);

    // TODO: load reflection, bump, etc maps.
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
            s.offset = 0;
            s.iterations = preprocess.parallaxSteps;
            spec.setBump(filename, s, preprocess.normalMapWhiteHeightInPixels);
        }
    } // if bump maps

    return spec;
}


void ArticulatedModel::Part::computeNormalsAndTangentSpace
    (const ArticulatedModel::Settings& settings) {

    Array<Array<int>*> indexArrayArray(triList.size());
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
        Array<Vector3> bitangentArray;

        MeshAlg::computeTangentSpaceBasis(
            geometry.vertexArray,
            texCoordArray,
            geometry.normalArray,
            faceArray,
            tangentArray,
            bitangentArray);
    } else {
        tangentArray.clear();
    }
}


void ArticulatedModel::Part::updateVAR(VertexBuffer::UsageHint hint) {
    if (geometry.vertexArray.size() == 0) {
        // Has no geometry
        return;
    }

    int vtxSize = sizeof(Vector3) * geometry.vertexArray.size();
    int texSize = sizeof(Vector2) * texCoordArray.size();
    int tanSize = sizeof(Vector3) * tangentArray.size();

    if ((vertexVAR.maxSize() >= vtxSize) &&
        (normalVAR.maxSize() >= vtxSize) &&
        ((tanSize == 0) || (tangentVAR.maxSize() >= tanSize)) &&
        ((texSize == 0) || (texCoord0VAR.maxSize() >= texSize))) {
        VertexRange::updateInterleaved
           (geometry.vertexArray, vertexVAR,
            geometry.normalArray, normalVAR,
            tangentArray,         tangentVAR,
            texCoordArray,        texCoord0VAR);

    } else {

        // Maximum round-up size of varArea.
        size_t roundOff = 16;

        // Allocate new VARs
        VertexBufferRef varArea = VertexBuffer::create(vtxSize * 2 + texSize + tanSize + roundOff, hint);
        VertexRange::createInterleaved
            (geometry.vertexArray, vertexVAR,
             geometry.normalArray, normalVAR,
             tangentArray,         tangentVAR,
             texCoordArray,        texCoord0VAR,
             varArea);       
    }

    for (int i = 0; i < triList.size(); ++i) {
        triList[i]->updateVAR(hint, vertexVAR, normalVAR, tangentVAR, texCoord0VAR);
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
    tangent = tangentVAR;
    texCoord0 = texCoord0VAR;

    if (indexArray.size() == 0) {
        // Has no indices
        return;
    }

    size_t indexSize = sizeof(int);
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
    MeshAlg::computeBounds(parentPart.geometry.vertexArray, indexArray, boxBounds, sphereBounds);
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
     Array<Surface::Ref>&           posedArray,
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
                GenericSurface::CPUGeom cpuGeom(& triList[t]->indexArray, &geometry, 
                                                   &texCoordArray, &tangentArray);

                posedArray.append(GenericSurface::create(model->name, frame, triList[t],
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
