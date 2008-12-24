/**
 @file ArticulatedModel.cpp
 @author Morgan McGuire
 */

#include "GLG3D/ArticulatedModel.h"
#include "GLG3D/IFSModel.h"
#include "Load3DS.h"

namespace G3D {

int ArticulatedModel::debugNumSendGeometryCalls = 0;

// TODO: Texture caching
const ArticulatedModel::Pose ArticulatedModel::DEFAULT_POSE;

static ArticulatedModel::GraphicsProfile graphicsProfile = ArticulatedModel::UNKNOWN;

void ArticulatedModel::setProfile(GraphicsProfile p) {
    graphicsProfile = p;
}


ArticulatedModel::GraphicsProfile ArticulatedModel::profile() {

    if (graphicsProfile == UNKNOWN) {
        if (GLCaps::supports_GL_ARB_shader_objects()) {
            graphicsProfile = PS20;

            
            if (System::findDataFile("NonShadowedPass.vrt") == "") {
                graphicsProfile = UNKNOWN;
                Log::common()->printf("\n\nWARNING: ArticulatedModel could not enter PS20 mode because"
                    "NonShadowedPass.vrt was not found.\n\n");
            }
        }

        
        if (graphicsProfile == UNKNOWN) {
            if (GLCaps::supports("GL_ARB_texture_env_crossbar") &&
                GLCaps::supports("GL_ARB_texture_env_combine") &&
                GLCaps::supports("GL_EXT_texture_env_add") &&
                (GLCaps::numTextureUnits() >= 4)) {
                graphicsProfile = PS14;
            } else {
                graphicsProfile = FIXED_FUNCTION;
            }
        }
    }

    return graphicsProfile;
}


ArticulatedModelRef ArticulatedModel::fromFile(const std::string& filename, const Vector3& scale) {
    return fromFile(filename, Matrix4(scale.x, 0, 0, 0,
                                      0, scale.y, 0, 0,
                                      0, 0, scale.z, 0,
                                      0, 0, 0, 1));
}


ArticulatedModelRef ArticulatedModel::fromFile(const std::string& filename, const CoordinateFrame& xform) {
    return fromFile(filename, xform.toMatrix4());
}


ArticulatedModelRef ArticulatedModel::fromFile(const std::string& filename, float scale) {
    return fromFile(filename, Vector3(scale, scale, scale));
}


ArticulatedModelRef ArticulatedModel::fromFile(const std::string& filename, const Matrix4& xform) {
    debugAssertM(fileExists(filename),
        filename + " cannot be loaded by ArticulatedModel because it does not exist.");

    ArticulatedModel* model = new ArticulatedModel();

    if (endsWith(toLower(filename), ".3ds")) {
        model->init3DS(filename, xform);
    } else if (endsWith(toLower(filename), ".ifs") || endsWith(toLower(filename), ".ply2") || endsWith(toLower(filename), ".off")) {
        model->initIFS(filename, xform);
    }

    model->updateAll();

    return model;
}


ArticulatedModelRef ArticulatedModel::createEmpty() {
    return new ArticulatedModel();
}


void ArticulatedModel::init3DS(const std::string& filename, const Matrix4& xform) {
    // Note: vertices are actually mutated by scale; it is not carried along as
    // part of the scene graph transformation.

    // Returns the index in partArray of the part with this name.
    Table<std::string, int>     partNameToIndex;

    Load3DS load;
    Table<std::string, TextureRef> texCache;

    std::string path = filenamePath(filename);
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
        
        // Scale and rotate the cframe positions, but do not translate them
        part.cframe.translation = R * part.cframe.translation;

        debugAssert(R.getColumn(0).isFinite());

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
                Part::TriList& triList = part.triListArray.next();
                triList.indexArray = object.indexArray;

            } else {
                for (int m = 0; m < object.faceMatArray.size(); ++m) {
                    const Load3DS::FaceMat& faceMat = object.faceMatArray[m];

                    if (faceMat.faceIndexArray.size() > 0) {
                        Part::TriList& triList = part.triListArray.next();
                
                        // Construct an index array for this part
                        for (int i = 0; i < faceMat.faceIndexArray.size(); ++i) {
                            // 3*f is an index into object.indexArray
                            int f = faceMat.faceIndexArray[i];
                            for (int v = 0; v < 3; ++v) {
                                triList.indexArray.append(object.indexArray[3*f + v]);
                            }
                        }
                        debugAssert(triList.indexArray.size() > 0);

                        const std::string& materialName = faceMat.materialName;

                        if (load.materialNameToIndex.containsKey(materialName)) {
                            int i = load.materialNameToIndex[materialName];
                            const Load3DS::Material& material = load.materialArray[i];

                            const Load3DS::Map& texture1 = material.texture1;

                            std::string textureFile = texture1.filename;

                            if (texture1.filename != "") {
                                if (endsWith(toUpper(textureFile), "GIF")) {
                                    // Load PNG instead of GIF, since we can't load GIF
                                    textureFile = textureFile.substr(0, textureFile.length() - 3) + "png";
                                }

                                if (! fileExists(textureFile) && fileExists(path + textureFile)) {
                                    textureFile = path + textureFile;
                                }

                                std::string f = System::findDataFile(textureFile, false);
                                if (f != "") {
                                    if (! texCache.containsKey(f)) {
                                        texCache.set(f, Texture::fromFile(f));
                                    }
                                    triList.material.diffuse.map = texCache[f];
                                } else {
                                    Log::common()->printf("Could not load texture '%s'\n", textureFile.c_str());
                                }

                                triList.material.diffuse.constant = (Color3::white() * material.texture1.pct) *
                                    (1.0f - material.transparency);
                            } else {
                                triList.material.diffuse.constant = material.diffuse * 
                                    (1.0f - material.transparency);
                            }


                            //strength of the shininess (higher is brighter)
                            triList.material.specular.constant = material.shininessStrength * 
                                material.specular * (1.0f - material.transparency);

                            //extent (area, higher is closely contained, lower is spread out) of shininess
                            // Do not exceed 128, which is the OpenGL fixed function maximum
                            triList.material.specularExponent.constant = Color3::white() * material.shininess 
                                * 128.0f;

                            triList.material.transmit.constant = Color3::white() * material.transparency;
                            triList.material.emit.constant = material.diffuse * material.emissive;

                            // TODO: load reflection, bump, etc maps.
                            // triList.material.reflect.map = 

                            triList.twoSided = material.twoSided;

                        } else {
                            logPrintf("Referenced unknown material '%s'\n", materialName.c_str());
                        }
                    } // if there are indices on this part
                } // for m
            } // if has materials 
        }
    }
}


void ArticulatedModel::Part::computeNormalsAndTangentSpace() {

    float normalSmoothingAngle = toRadians(60);
    float vertexWeldRadius     = 0.0001f;
    float texCoordWeldRadius   = 0.00001f;
    float normalWeldRadius     = 0.00001f;

    Array<Array<int>*> indexArrayArray(triListArray.size());
    for (int t = 0; t < triListArray.size(); ++t) {
        indexArrayArray[t] = &(triListArray[t].indexArray);
    }

    MeshAlg::weld(geometry.vertexArray,
                  texCoordArray,
                  geometry.normalArray,
                  indexArrayArray,
                  normalSmoothingAngle,
                  vertexWeldRadius,
                  texCoordWeldRadius,
                  normalWeldRadius);

    Array<MeshAlg::Face>    faceArray;
    Array<MeshAlg::Vertex>  vertexArray;
    Array<MeshAlg::Edge>    edgeArray;
    Array<Vector3>          faceNormalArray;

    computeIndexArray();

    MeshAlg::computeAdjacency(geometry.vertexArray, indexArray, faceArray, edgeArray, vertexArray);

    // Compute a tangent space basis
    if (texCoordArray.size() > 0) {
        // computeTangentSpaceBasic will generate binormals, but
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


void ArticulatedModel::Part::updateVAR(VARArea::UsageHint hint /* = VARArea::WRITE_ONCE */) {
    if (geometry.vertexArray.size() == 0) {
        // Has no geometry
        return;
    }

    size_t vtxSize = sizeof(Vector3) * geometry.vertexArray.size();
    size_t texSize = sizeof(Vector2) * texCoordArray.size();
    size_t tanSize = sizeof(Vector3) * tangentArray.size();

    if ((vertexVAR.maxSize() >= vtxSize) &&
        (normalVAR.maxSize() >= vtxSize) &&
        ((tanSize == 0) || (tangentVAR.maxSize() >= tanSize)) &&
        ((texSize == 0) || (texCoord0VAR.maxSize() >= texSize))) {
        VAR::updateInterleaved
           (geometry.vertexArray, vertexVAR,
            geometry.normalArray, normalVAR,
            tangentArray,         tangentVAR,
            texCoordArray,        texCoord0VAR);

    } else {

        // Maximum round-up size of varArea.
        size_t roundOff = 16;

        // Allocate new VARs
        VARAreaRef varArea = VARArea::create(vtxSize * 2 + texSize + tanSize + roundOff, hint);
        VAR::createInterleaved
            (geometry.vertexArray, vertexVAR,
             geometry.normalArray, normalVAR,
             tangentArray,         tangentVAR,
             texCoordArray,        texCoord0VAR,
             varArea);       
    }
}


void ArticulatedModel::Part::computeBounds() {
    for (int t = 0; t < triListArray.size(); ++t) {
        triListArray[t].computeBounds(*this);
    }
}


void ArticulatedModel::updateAll() {
    for (int p = 0; p < partArray.size(); ++p) {
        Part& part = partArray[p];
        part.computeNormalsAndTangentSpace();
        part.updateVAR();
        part.computeBounds();
    }
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

    Part::TriList& triList = part.triListArray.next();
    triList.indexArray = index;
}


void ArticulatedModel::Part::TriList::computeBounds(const Part& parentPart) {
    MeshAlg::computeBounds(parentPart.geometry.vertexArray, indexArray, boxBounds, sphereBounds);
}


void ArticulatedModel::Part::computeIndexArray() {
    indexArray.clear();
    for (int t = 0; t < triListArray.size(); ++t) {
        indexArray.append(triListArray[t].indexArray);
    }
}


const char* toString(ArticulatedModel::GraphicsProfile p) {
    switch (p) {
    case ArticulatedModel::UNKNOWN:
        return "Unknown";

    case ArticulatedModel::FIXED_FUNCTION:
        return "Fixed Function";

    case ArticulatedModel::PS14:
        return "PS 1.4";

    case ArticulatedModel::PS20:
        return "PS 2.0";

    default:
        return "Error!";
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


ArticulatedModelRef ArticulatedModel::createCornellBox() {

    ArticulatedModelRef model = ArticulatedModel::createEmpty();
    model->name = "Cornell Box";

    ArticulatedModel::Part& part = model->partArray.next();
    Array<Vector3>& vertex = part.geometry.vertexArray;
    part.name = "root";

    float c = -0.275f;

    // Data used is captured from the photographs and balanced to
    // achieve (perceptual) uniform brightness on all surfaces; this
    // integrates the spectral data.

    // White faces
    {
        ArticulatedModel::Part::TriList& triList = part.triListArray.next();
        triList.material = Material::createDiffuse(Color3::white() * 0.72f);
        triList.twoSided = true;

        Array<int>& index = triList.indexArray;

        // Top
        addRect(Vector3(-c,  c,  c), Vector3(-c,  c, -c), Vector3( c,  c, -c), Vector3( c,  c,  c), vertex, index);
    
        // Back
        addRect(Vector3(-c,  c, -c), Vector3(-c, -c, -c), Vector3( c, -c, -c), Vector3( c,  c, -c), vertex, index);

        // Floor
        addRect(Vector3( c, -c,  c), Vector3( c, -c, -c), Vector3(-c, -c, -c), Vector3(-c, -c,  c), vertex, index);
    }

    // Left red face
    {
        ArticulatedModel::Part::TriList& triList = part.triListArray.next();
        triList.material = Material::createDiffuse(Color3::fromARGB(0xB82C1F));
        triList.twoSided = true;

        Array<int>& index = triList.indexArray;
        addRect(Vector3(-c,  c,  c), Vector3(-c, -c,  c), Vector3(-c, -c, -c), Vector3(-c,  c, -c), vertex, index);
    }

    // Right green face
    {
        ArticulatedModel::Part::TriList& triList = part.triListArray.next();
        triList.material = Material::createDiffuse(Color3::fromARGB(0x6AB8B8));
        triList.twoSided = true;

        Array<int>& index = triList.indexArray;
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
        for (int t = 0; t < srcPart.triListArray.size(); ++t) {
            const Part::TriList& srcTriList = srcPart.triListArray[t];
            Part::TriList& dstTriList = dstPart.triListArray[t];
            dstTriList.indexArray.fastClear();

            // Unroll the arrays
            for (int x = 0; x < srcTriList.indexArray.size(); ++x) {
                int i = srcTriList.indexArray[x];

                // Create the vertex
                dstPart.geometry.vertexArray.append(srcPart.geometry.vertexArray[i]);
                if (srcPart.texCoordArray.size() > 0) {
                    dstPart.texCoordArray.append(srcPart.texCoordArray[i]);
                }
                dstPart.indexArray.append(n);
                dstTriList.indexArray.append(n);
                ++n;
            }
        }
    }
    updateAll();
}

} // G3D
