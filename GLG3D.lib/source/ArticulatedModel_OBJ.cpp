/**
 @file ArticulatedModel_OBJ.cpp
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu

 @created 2010-07-03
 @edited  2010-07-03
 */

#include "GLG3D/ArticulatedModel.h"
#include "G3D/FileSystem.h"
#include "G3D/Log.h"

namespace G3D {


/** Subset of full OBJ parameters */
class MatSpec {
public:
    std::string     name;

    Color4          diffuseConstant;
    std::string     diffuseMap;

    std::string     bumpMap;

    Color3          specularConstant;
    float           shininess;

    float           eta;

    MatSpec() : 
        diffuseConstant(0.8f, 0.8f, 0.8f, 1.0f),
        // The default specular constant of one doesn't work well for G3D
        specularConstant(Color3::zero()),
        shininess(0.0f),
        eta(1.0f) {}

    Material::Ref createMaterial(const ArticulatedModel::Preprocess& preprocess) const {
        debugPrintf("Creating material %s...", name.c_str());
        Material::Specification spec;

        if ((diffuseMap != "") && ! preprocess.stripMaterials) {
            Texture::Specification s;
            s.dimension = Texture::DIM_2D_NPOT;

            // If we turn this off, the BSDF just does it anyway
            s.preprocess.computeMinMaxMean = true;
            s.settings.maxAnisotropy = 2.0f;
            s.filename = diffuseMap;
            spec.setLambertian(s);
            
        } else {
            spec.setLambertian(diffuseConstant);
        }

        // Assume in air
        spec.setEta(max(1.0f, eta), 1.0f);

        // OBJ models are way too specular and not shiny enough
        spec.setSpecular(specularConstant.pow(9.0f) * 0.4f);
        spec.setGlossyExponentShininess(shininess * 100.0f);

        // spec.setBump(bumpMap);

        Material::Ref m = Material::create(spec);
        debugPrintf("Done\n");
        return m;
    }
};

static std::string removeLeadingSlash(const std::string& s) {
    if (s.length() > 0 && isSlash(s[0])) {
        return s.substr(1);
    } else {
        return s;
    }
}

static void loadMTL(const std::string& filename, Table<std::string, Material::Ref>& mtlTable, const ArticulatedModel::Preprocess& preprocess) {
    // http://people.sc.fsu.edu/~burkardt/data/mtl/mtl.html

    const std::string& basePath = FilePath::parent(FileSystem::resolve(filename));

    TextInput::Settings set;
    set.cppBlockComments = false;
    set.cppLineComments = false;
    set.otherCommentCharacter = '#';
    set.generateNewlineTokens = true;

    if (! FileSystem::exists(filename)) {
        logPrintf("WARNING: \"%s\" not found while loading OBJ file.\n", filename.c_str());
        debugPrintf("WARNING: \"%s\" not found while loading OBJ file.\n", filename.c_str());
        return;
    }

    TextInput ti(filename, set);

    // Hack: merge anything sharing diffuse textures
    Table<std::string, Material::Ref> diffuseCache;

    MatSpec matSpec;
    while (ti.hasMore()) {
        // Consume comments/newlines
        while (ti.hasMore() && (ti.peek().type() == Token::NEWLINE)) {
            // Consume the newline
            ti.read();
        }

        // Process one line
        const std::string& cmd = ti.readSymbol();

        if (cmd == "newmtl") {
            // Create the previous material (TODO: note that this code is duplicated below)
            if (matSpec.name != "") {
                if (matSpec.diffuseMap != "" && diffuseCache.containsKey(matSpec.diffuseMap)) {
                    mtlTable.set(matSpec.name, diffuseCache[matSpec.diffuseMap]);
                } else {
                    mtlTable.set(matSpec.name, matSpec.createMaterial(preprocess));
                    if (matSpec.diffuseMap != "") {
                        diffuseCache.set(matSpec.diffuseMap, mtlTable[matSpec.name]);
                    }
                }
            }

            // Reset to defaults
            matSpec = MatSpec();
            matSpec.name = ti.readUntilNewlineAsString();

        } else if ((cmd == "d") || (cmd == "Tr")) {
            // alpha on range [0,1]
            matSpec.diffuseConstant.a = ti.readNumber();
        } else if (cmd == "Ns") {
            // spec exponent on range [0, 1000]
            matSpec.shininess = ti.readNumber();
        } else if (cmd == "Ni") {
            // index of refraction (scalar, generally > 1.0)
            matSpec.eta = ti.readNumber();
        } else if (cmd == "Ka") {
            // rgb ambient on range [0,1]
            // We ignore this
        } else if (cmd == "Kd") {
            // rgb diffuse on range [0,1]
            matSpec.diffuseConstant.r = ti.readNumber();
            matSpec.diffuseConstant.g = ti.readNumber();
            matSpec.diffuseConstant.b = ti.readNumber();
        } else if (cmd == "Ks") {
            // rgb specular on range [0,1]
            matSpec.specularConstant.r = ti.readNumber();
            matSpec.specularConstant.g = ti.readNumber();
            matSpec.specularConstant.b = ti.readNumber();
        } else if (cmd == "Km") {
            // Scalar---mirror?
        } else if (cmd == "map_Kd") {
            matSpec.diffuseMap = FilePath::concat(basePath, removeLeadingSlash(ti.readUntilNewlineAsString()));
        } else if (cmd == "map_Bump") {
            matSpec.bumpMap = FilePath::concat(basePath, removeLeadingSlash(ti.readUntilNewlineAsString()));
        }

        // Read until the end of the line
        while (ti.hasMore() && (ti.read().type() != Token::NEWLINE));
    }

    // Create the previous material
    if (matSpec.name != "") {
        if (matSpec.diffuseMap != "" && diffuseCache.containsKey(matSpec.diffuseMap)) {
            mtlTable.set(matSpec.name, diffuseCache[matSpec.diffuseMap]);
        } else {
            mtlTable.set(matSpec.name, matSpec.createMaterial(preprocess));
            if (matSpec.diffuseMap != "") {
                diffuseCache.set(matSpec.diffuseMap, mtlTable[matSpec.name]);
            }
        }
    }

}


class TriListSpec {
public:
    std::string     name;
    std::string     materialName;
    Array<int>      cpuIndex;
};


static Vector3 readVertex(TextInput& ti, const Matrix4& xform) {
    // Vertex
    Vector4 v;
    v.x = ti.readNumber();
    v.y = ti.readNumber();
    v.z = ti.readNumber();
    v.w = 1.0f;
    return (xform * v).xyz();
}


static Vector3 readNormal(TextInput& ti, const Matrix3& normalXform) {
    Vector3 n;
    n.x = ti.readNumber();
    n.y = ti.readNumber();
    n.z = ti.readNumber();

    return (normalXform * n).direction();
}




void ArticulatedModel::initOBJ(const std::string& filename, const Preprocess& preprocess) {
    Stopwatch loadTimer;

    TextInput::Settings set;
    set.cppBlockComments = false;
    set.cppLineComments = false;
    set.otherCommentCharacter = '#';
    set.generateNewlineTokens = true;

    // Notes on OBJ file format.  See also:
    //
    // -  http://www.martinreddy.net/gfx/3d/OBJ.spec
    // -  http://en.wikipedia.org/wiki/Obj
    // -  http://www.royriggs.com/obj.html
    //
    // OBJ indexing is 1-based.
    // Line breaks are significant.
    // The first token on a line indicates the contents of the line.
    //
    // Faces contain separate indices for normals and texcoords.
    // We load the raw vertices and then form our own optimized
    // gl indices from them.
    //
    // Negative indices are relative to the last coordinate seen.

    // Raw arrays with independent indexing, as imported from the file
    Array<Vector3> rawVertex;
    Array<Vector3> rawNormal;
    Array<Vector2> rawTexCoord;

    // part.geometry.vertexArray[i] = rawVertex[cookVertex[i]];
    Array<int>      cookVertex;
    Array<int>      cookNormal;
    Array<int>      cookTexCoord;

    // Put everything into a single part
    // Convert to a Part
    Part& part = partArray.next();

    part.cframe = CoordinateFrame();
    part.name = "root";
    part.parent = -1;

    // v,t,n repeated for each vertex
    Array<int>     faceTempIndex;

    // Count of errors from mismatched texcoord and vertex
    int texCoordChanged = 0;
    int normalChanged = 0;

    Table<std::string, Material::Ref> materialLibrary;
    Table<std::string, TriListSpec*> groupTable;

    TriListSpec* currentTriList = NULL;
    int numTris = 0;

    const Matrix3 normalXform = preprocess.xform.upper3x3().transpose().inverse();

    const std::string& basePath = FilePath::parent(FileSystem::resolve(filename));

    {
        TextInput ti(filename, set);
        while (ti.hasMore()) {
            // Consume comments/newlines
            while (ti.hasMore() && (ti.peek().type() == Token::NEWLINE)) {
                // Consume the newline
                ti.read();
            }

            if (! ti.hasMore()) {
                break;
            }

            // Process one line
            const std::string& cmd = ti.readSymbol();

            if (cmd == "mtllib") {

                // Specify material library 
                const std::string& mtlFilename = ti.readUntilNewlineAsString();
                loadMTL(FilePath::concat(basePath, mtlFilename), materialLibrary, preprocess);

            } else if (cmd == "g") {

                // New trilist
                const std::string& name = ti.readUntilNewlineAsString();
                if (! groupTable.containsKey(name)) {
                    currentTriList = new TriListSpec();
                    currentTriList->name = name;            
                    groupTable.set(name, currentTriList);
                } else {
                    currentTriList = groupTable[name];
                }


            } else if (cmd == "usemtl") {
                if (currentTriList) {
                    currentTriList->materialName = ti.readUntilNewlineAsString();
                }
            } else if (cmd == "v") {
                rawVertex.append(readVertex(ti, preprocess.xform));
            } else if (cmd == "vt") {
                // Texcoord
                Vector2& t = rawTexCoord.next();
                t.x = ti.readNumber();
                t.y = 1.0f - ti.readNumber();
            } else if (cmd == "vn") {
                // Normal
                rawNormal.append(readNormal(ti, normalXform));
            } else if ((cmd == "f") && currentTriList) {
                // Face

                // Read each vertex
                while (ti.hasMore() && (ti.peek().type() != Token::NEWLINE)) {

                    // Read one 3-part index
                    int v = ti.readNumber();
                    if (v < 0) {
                        v = rawVertex.size() + 1 + v;
                    }

                    int n = 0;
                    int t = 0;

                    if (ti.peek().type() == Token::SYMBOL) {
                        ti.readSymbol("/");
                        if (ti.peek().type() == Token::NUMBER) {
                            t = ti.readNumber();
                            if (t < 0) {
                                t = rawTexCoord.size() + 1 + t;
                            }
                        }
                        if (ti.peek().type() == Token::SYMBOL) {
                            ti.readSymbol("/");
                            if (ti.peek().type() == Token::NUMBER) {
                                n = ti.readNumber();
                                if (n < 0) {
                                    n = rawNormal.size() + 1 + n;
                                }
                            }
                        }
                    }

                    // Switch to zero-based indexing 
                    --v; --n; --t;

                    faceTempIndex.append(v, t, n);
                }

                alwaysAssertM(faceTempIndex.size() >= 3*3, "Face with fewer than three vertices in model.");
                numTris += (faceTempIndex.size()/3) - 2;
                // The faceTempIndex is now a triangle fan.  Convert it to a triangle list and use unique vertices
                for (int i = 2; i < faceTempIndex.size()/3; ++i) {
                    // Always start with vertex 0
                    cookVertex.append(faceTempIndex[0]);
                    cookTexCoord.append(faceTempIndex[1]);
                    cookNormal.append(faceTempIndex[2]);

                    // The vertex just before the one we're adding
                    int j = (i - 1) * 3;
                    cookVertex.append(faceTempIndex[j]);
                    cookTexCoord.append(faceTempIndex[j+1]);
                    cookNormal.append(faceTempIndex[j+2]);

                    // The vertex we're adding
                    j = i * 3;
                    cookVertex.append(faceTempIndex[j]);
                    cookTexCoord.append(faceTempIndex[j+1]);
                    cookNormal.append(faceTempIndex[j+2]);

                    // Update the index array to contain the three vertices we just added
                    currentTriList->cpuIndex.append(cookVertex.size() - 3, cookVertex.size() - 2, cookVertex.size() - 1);
                } 

                faceTempIndex.fastClear();

            }

            // Read until the end of the line
            while (ti.hasMore() && (ti.read().type() != Token::NEWLINE));
        }
    }

    debugPrintf("Creating TriLists\n");

    // Copy geometry
    const int N = cookVertex.size();
    part.geometry.vertexArray.resize(N);
    for (int i = 0; i < N; ++i) {
        part.geometry.vertexArray[i] = rawVertex[cookVertex[i]];
    }

    // Optional normals
    if (rawNormal.size() > 0) {
        part.geometry.normalArray.resize(N);
        for (int i = 0; i < N; ++i) {
            part.geometry.normalArray[i] = rawNormal[cookNormal[i]];
        }
    }

    // Optional texcoords
    if (rawTexCoord.size() > 0) {
        part.texCoordArray.resize(N);
        for (int i = 0; i < N; ++i) {
            part.texCoordArray[i] = rawTexCoord[cookTexCoord[i]];
        }
    }

    // Create trilists
    for (Table<std::string, TriListSpec*>::Iterator it = groupTable.begin(); it.hasMore(); ++it) {
        TriListSpec* s = it->value;

        Material::Ref material;
        if (materialLibrary.containsKey(s->materialName)) {
            material = materialLibrary[s->materialName];
        } else {
            material = Material::createDiffuse(Color3::white() * 0.8f);
            debugPrintf("Warning: unrecognized material: %s\n", s->materialName.c_str());
        }

        Part::TriList::Ref triList = part.newTriList(material);
        triList->twoSided = false;
        triList->indexArray = s->cpuIndex;
    }
    groupTable.deleteValues();
    groupTable.clear();

    debugPrintf("Done loading.  %d vertices, %d faces, %d frames\n\n", cookVertex.size(), numTris, N);
    loadTimer.after("Loading");
}

}
