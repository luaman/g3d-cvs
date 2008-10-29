/**
 @file MeshAlgWeld2.cpp


 @author Kyle Whitson

 @created 2008-07-30
 @edited  2008-07-30
 */

#include "G3D/platform.h"
#include "G3D/Vector2.h"
#include "G3D/Vector3.h"
#include "G3D/PointAABSPTree.h"
#include "G3D/MeshAlg.h"

namespace G3D {
namespace _internal {

typedef Vector2 TexCoord;
class NodeArrays;
class Node;

/**
	Class containing arrays which specify vertices, normals, and texture 
    coordinates of mesh vertices.
    vertexArray, normalArray, and texCoordArray are parallel arrays, so 
    vertexArray[i], normalArray[i], and texCoordArray[i] together describe 
    one vertex.
    Pointers are used to the arrays so that NodeArrays can be easily created
    to use pre-existing mesh data without copying arrays. All pointers are
    const except for normalArray, which is changed when vertex normals are
    merged.
*/
class NodeArrays {
private:
	bool						deleteArrays;
public:
	Array<int>*	const			indexArray;
	Array<Vector3>*	const		vertexArray;
	Array<Vector3>*     		normalArray;
	Array<TexCoord>* const		texCoordArray;

	/** Constructor that allocates Arrays.*/
	NodeArrays();

	/** Constructor that uses pre-existing Arrays.*/
	NodeArrays
        (Array<int>* const     _indexArray, 
         Array<Vector3>* const _vertexArray, 
         Array<Vector3>* const _normalArray, 
		Array<TexCoord>* const _texCoordArray);

    /** Returns the size of the vertex array.
        Both the normal and texCoord arrays have either this size or zero (the 
        texCoord array has size 0 if the mesh is untextured, and the normal
        array has size 0 if normals have not yet been computed by the welding 
        algorithm.*/
    int size();

    /** Appends the vertex, normal, and texCoord values referenced by the
        passed Node to the arrays referenced by this NodeArrays object.
        The index array is left unchanged.*/
    void append(const Node& n);

    /** Clears each array without de-allocating memory.*/
    void fastClearAll();

	/** If the constructor allocated arrays, deletes them here.*/
	~NodeArrays();
};

/** A pointer to one index in parallel vertex, normal, and texCoord arrays. 
    This describes one vertex of a mesh face.*/
class Node {

private:
	int					        m_index;
	NodeArrays*                 m_arrays;

public:

    class Position;
    class Equals;
    class HashFunc;

	bool operator==(const Node& other) const;
	
	Node(const int index, NodeArrays* arrays);

    Node();

	const Vector3& vertex() const;

	const Vector3& normal() const;

	const TexCoord& texCoord() const;

	const int index() const;
};

NodeArrays::NodeArrays() : 
    deleteArrays(true),
    indexArray(new Array<int>()),
    vertexArray(new Array<Vector3>()), 
    normalArray(new Array<Vector3>()), 
	texCoordArray(new Array<TexCoord>()) {}

NodeArrays::NodeArrays(Array<int>* const _indexArray, 
    Array<Vector3>* const _vertexArray, 
    Array<Vector3>* const _normalArray, 
    Array<TexCoord>* const _texCoordArray) : 
    deleteArrays(false),
    indexArray(_indexArray),
    vertexArray(_vertexArray), 
    normalArray(_normalArray), 
    texCoordArray(_texCoordArray) {}


int NodeArrays::size() {
    return vertexArray->size();
}


void NodeArrays::append(const Node& n) {
    vertexArray->append(n.vertex());
    normalArray->append(n.normal());
    texCoordArray->append(n.texCoord());
}


void NodeArrays::fastClearAll() {
    indexArray->fastClear();
    vertexArray->fastClear();
    normalArray->fastClear();
    texCoordArray->fastClear();
}


NodeArrays::~NodeArrays() {
	if (deleteArrays) {
		delete indexArray;
		delete vertexArray;
		delete normalArray;
		delete texCoordArray;
	}
}


class Node::Position {
public:
	static void getPosition(const Node& obj, G3D::Vector3& p) {
		p = obj.vertex();
	}
};

class Node::Equals {
public:
	static bool equals(const Node& a, const Node& b) {
		return a == b;
	}
};

class Node::HashFunc {
public:
    static size_t hashCode(const Node& key) {
        return HashTrait<Vector3>::hashCode(key.vertex());
    }
};

bool Node::operator==(const Node& other) const {
	return m_index == other.m_index && m_arrays == other.m_arrays;
}

Node::Node(const int index, NodeArrays* arrays) : 
    m_index(index), m_arrays(arrays) {}

Node::Node() : m_index(0), m_arrays(NULL) {}

const Vector3& Node::vertex() const {
	return (*(m_arrays->vertexArray))[m_index];
}
const Vector3& Node::normal() const {
	return (*(m_arrays->normalArray))[m_index];
}
const TexCoord& Node::texCoord() const {
    if(m_arrays->texCoordArray->size() > 0) {
	    return (*m_arrays->texCoordArray)[m_index];
    } else {
        return TexCoord::zero();
    }
}
const int Node::index() const {
	return m_index;
}

class MeshHelper {

public:

private:
	

    typedef PointAABSPTree<Node, Node::Position, Node::HashFunc, Node::Equals> 
        Grid;

	/** Constructed from the arrays passed to the constructor.*/
	NodeArrays		inputArrays;

	/** The input arrays are "unrolled" (so that each face has its own vertex, 
        normal, and texCoords) here.*/
	NodeArrays		unrolledArrays;

	/** Contains the arrays after similar nodes have been merged.*/
	NodeArrays		outputArrays;

	/** Used for finding nearby nodes. 
        After unrolling, the nodes from the unrolled arrays are placed here.
        When merging, the merged nodes are placed here so that no two similar 
        nodes appear in the output.*/
	Grid			nodes;

    /** Tracks which index in the unrolled arrays has been assigned to each 
        index in the input arrays. */
    Array<int> unrolledIndex;

    /** Tracks which index in the output arrays has been assigned to each index 
        in the unrolled arrays. */
    Array<int> outputIndex;

	float			r;

	/** 1/(r^2), for use in the distance formula.*/
	float			rSquaredInverse;

	/** 1/(s^2), for use in the distance formula.*/
	float			sSquaredInverse;

	/** 1-cos(theta), for use in the distance formula.*/
	float			oneMinusCosineTheta;

    /** Called by mergeNormals.
        Computes the sum of an Array of Vector3s.*/
    Vector3 vec3Sum(const Array<Vector3>& v) {
        if(v.size() == 0) return Vector3::zero();

        Vector3 sum = v[0];
        for(int i = 1; i < v.size(); ++i) {
            sum += v[i];
        }
        return sum;
    }

	/** Called by the constructor.
        "Unrolls" the arrays in inputArrays according to the index array. The 
        unrolled arrays are stored in unrolledArrays, and any old data in the 
        vertex and texCoord arrays of unrolledArrays is lost.*/
	void unroll() {
        alwaysAssertM((inputArrays.vertexArray->size() == 
            inputArrays.texCoordArray->size()) || 
            (inputArrays.texCoordArray->size() == 0), 
            "Mesh is textured, but vertex and texCoord arrays are different lengths");
       
        unrolledIndex.resize(inputArrays.size());

		unrolledArrays.vertexArray->fastClear();
		unrolledArrays.texCoordArray->fastClear();

        if(inputArrays.texCoordArray->size() > 0) {

            // Textured mesh
		    for(int i = 0; i < inputArrays.indexArray->size(); ++i) {
			    unrolledArrays.vertexArray->append(
                    (*inputArrays.vertexArray)[(*inputArrays.indexArray)[i]]);
                unrolledArrays.texCoordArray->append(
                    (*inputArrays.texCoordArray)[(*inputArrays.indexArray)[i]]);
                
                int oldIndex = (*inputArrays.indexArray)[i];
                unrolledIndex[oldIndex] = i;
		    }
        } else {

            // Untextured mesh
            for(int i = 0; i < inputArrays.indexArray->size(); ++i) {
                unrolledArrays.vertexArray->append(
                    (*inputArrays.vertexArray)[(*inputArrays.indexArray)[i]]);

                int oldIndex = (*inputArrays.indexArray)[i];
                unrolledIndex[oldIndex] = i;
            }
        }
	}

	/** Called by the constructor.
        Uses the data in unrolledArrays.vertexArray to fill 
        unrolledArrays.normalArray with the vector normals before smoothing.
		Assumes unroll has been called, and overwrites any old data in the 
        normal array of unrolledArrays.*/
	void computeFlatNormals() {
		debugAssert(unrolledArrays.size() > 0);
		unrolledArrays.normalArray->fastClear();
		for(int i = 0; i < unrolledArrays.size(); i += 3) {
			Vector3 f = ((*unrolledArrays.vertexArray)[i+1] - 
                (*unrolledArrays.vertexArray)[i]).cross(
				(*unrolledArrays.vertexArray)[i+2] - 
                (*unrolledArrays.vertexArray)[i]);
			Vector3 normal = f;
			if(normal != Vector3::zero()) {
				normal = f.direction();
			}
			for(int j = 0; j < 3; ++j) {
				unrolledArrays.normalArray->append(normal);
			}
		}
	}

	/** Called by mergeNormals.
        Finds all neighboring nodes (as dictated by vertex position) which also 
        have similar normals. 
        Any old data in similarNormals will be lost. 
        The normals of neighboring nodes with similar normals will be put into 
        similarNormals. */
	void getSimilarNormals(const Node& n, Array<Vector3>& similarNormals, 
        const float cosThreshold) {
		similarNormals.fastClear();
		Sphere s(n.vertex(), r);
        Array<Node> neighbors;
        nodes.getIntersectingMembers(s, neighbors);

		for(int i = 0; i < neighbors.size(); ++i) {
            float cosAngle = n.normal().dot(neighbors[i].normal());

			if (cosAngle > cosThreshold) {
				similarNormals.append(neighbors[i].normal());
            }
		}
	}

    /** Called by the constructor.
        Uses the data in unrolledArrays.normalArray to merge the normals of 
        nearby vertices with similar normals. 
        Assumes that computeFlatNormals has been called, and changes the data 
        in unrolledArrays->normalArray.*/
    void mergeNormals(const float cosThreshold) {
        Array<Vector3>* newNormals = new Array<Vector3>();

        for(int i = 0; i < unrolledArrays.size(); ++i) {
            Node n(i, &unrolledArrays);
            Array<Vector3> neighbors;

            // Get the neighbors of this node
            getSimilarNormals(n, neighbors, cosThreshold);

            // The new normal of this node is the average of the vectors in 
            // neighbors
            Vector3 newNormal = vec3Sum(neighbors).direction();
            newNormals->append(newNormal);
        }

        delete unrolledArrays.normalArray;
        unrolledArrays.normalArray = newNormals;
        newNormals = NULL;
    }

	/** Called by getMatchingNodeIndex
        If there is at least one node in outputGrid within a distance of 1 (per 
        the distanceSquared function), returns true and and stores the nearest 
        node in newNode.
		If there is no such node, returns false and leaves newNode unchanged.*/
	bool getNearest(const Node& n, Node& newNode) {

		if(nodes.size() == 0) {

			// The output grid is empty, so return false
			return false;
		} else {

			Sphere s(n.vertex(), r);
			double minDist = inf();

            Array<Node> neighbors;
            nodes.getIntersectingMembers(s, neighbors);

			Node nearest;
			bool nodeFound = false;
			for(int i = 0; i < neighbors.size(); ++i) {
				double distanceSq = distanceSquared(n, neighbors[i]);
				if(distanceSq <= minDist) {
					nodeFound = true;
					nearest = neighbors[i];
					minDist = distanceSq;
				}
			}

			if(nodeFound && distanceSquared(n, nearest) <= 1.0) {
				newNode = nearest;
				return true;
			} else {
				return false;
			}
		}
	}

	/** Called by mergeNodes.
        Returns the index in the outputArrays which contains either the passed 
        node or a node which it will be merged into.*/
	int getMatchingNodeIndex(Node& n) {

        // Check if a similar node has already been put into the output grid
        Node nearestNode;
        if(getNearest(n, nearestNode)) {

			// n is being merged, so return the index of the node it is being 
            // merged into
			return nearestNode.index();
        } else {

			// n is not being merged, so put it into the output arrays and the 
            // output grid
			outputArrays.append(n);

            // the index for n is the last index of the output arrays
			int index = outputArrays.size() - 1;				
			nodes.insert(Node(index, &outputArrays));
			return index;

		}
	}

	/** Called by the constructor.
        Finds similar indices in unrolledArrays, then merges them and fills the 
        outputArrays. 
        Assumes that unroll and computeFlatNormals have been called.*/
	void mergeNodes() {
		debugAssert(unrolledArrays.size() > 0);

        // Clear the grid before any output nodes will be put there
        nodes.clearData();
        outputArrays.fastClearAll();
        outputIndex.resize(unrolledIndex.size());

		// Iterate through each vertex of the unrolled arrays
		for(int i = 0; i < unrolledArrays.size(); ++i) {
			Node n(i, &unrolledArrays);

			// Get the index in the output arrays where this vertex (or the 
            // vertex it is being merged into) is now stored
			int newIndex = getMatchingNodeIndex(n);

			// add to the end of the output index array to store the vertex
			outputArrays.indexArray->append(newIndex);

            // remember where index i in the output arrays was sent
            outputIndex[i] = newIndex;
		}
	}


	/** Called by getNearest.
        Computes the squared distance between two nodes.
		Formula: d^2 =	 (1/r^2)(||vertex(a) - vertex(b)||^2)
						+([1 - (normal(a)*normal(b))]/[1 - cos(theta)])^2
						+(1/s^2)(||texCoord(a) - texCoord(b)||^2)
						
		NOTE: Some meshes contain zero-area faces to eliminate "holes" in the 
        mesh due to round-off error.
		If either vertex has a zero normal vector, the normals should be ignored 
        when computing the distance.
		Since all nonzero normals have unit length, we do this by multiplying by 
        the magnitudes of a and b in the second line of the formula.*/
	float distanceSquared(const Node& a, const Node& b) {

        if(a == b) {

            // a and b specify the same index in the same arrays, so their
            // distance should be 0.
            return 0.0;
        } else {
		    float vertexMagnitude2 = (a.vertex() - b.vertex()).squaredLength();
		    float texCoordMagnitude2 = (a.texCoord() - b.texCoord()).squaredLength();
            float normalDist = 0.0f;

            // If either normal is the zero vector, disregard the normals
            // in the distance computation.
            if(!(a.normal().isZero() || b.normal().isZero())) {
                float dotProd = a.normal().dot(b.normal());
                normalDist = (1.0 - dotProd) / oneMinusCosineTheta;
            }

		    return rSquaredInverse * vertexMagnitude2 + square(normalDist) +
                sSquaredInverse * texCoordMagnitude2;
        }
	}

	/** Called by the constructor.
        Creates nodes from the entries of unrolledArrays and puts them into the
        inputGrid.*/
	void buildInputGrid() {
		for(int i = 0; i < unrolledArrays.size(); ++i) {
			Node n(i, &unrolledArrays);
			nodes.insert(n);
		}
        nodes.balance();
	}

    /** Called by the constructor.

        Computes an array which maps indices in the input arrays to indices in 
        the output arrays.*/
    void computeOldToNew(Array<int>& oldToNewArray) {
        oldToNewArray.resize(unrolledIndex.size());
        for(int i = 0; i < oldToNewArray.size(); ++i) {
            int unrolled = unrolledIndex[i];
            int output = outputIndex[unrolled];
            oldToNewArray[i] = output;
        }
    }

public:

    /** Identifies and merges similar vertices in a triangle mesh, and performs
        normal smoothing.
        
        Vertices, normals, texCoords, and indices should be the vertex, normal,
        texCoord, and index arrays of the input mesh.

        _r, _s, and _theta indicate the maximum allowed vertex, texCoord, and 
        normal angle differences for vertices to be merged.
        
        Normal smoothing will be applied on all angles with a cosine less than 
        cosNormalThreshold.

        After the merging takes place, vertices, normals, texCoords, and 
        indices will store the new mesh, and the ith value in oldToNewIndex
        will be the new index of index i in the old mesh.*/
	MeshHelper(
        Array<Vector3>& vertices, 
        Array<Vector3>& normals, 
        Array<TexCoord>& texCoords, 
        Array<int>& indices, 
        const float _r, 
        const float _s, 
        const float _theta, 
        const float cosNormalThreshold, 
        Array<int>& oldToNewIndex, 
        bool recomputeNormals = true) : 
		inputArrays(&indices, &vertices, &normals, &texCoords), 
        outputArrays(&indices, &vertices, &normals, &texCoords) {

		r = _r;
		rSquaredInverse = 1.0 / square(r);
		sSquaredInverse = 1.0 / square(_s);
		oneMinusCosineTheta = 1.0 - cos(_theta);

        oldToNewIndex.resize(vertices.size());

        // Unroll the arrays and compute the normals.
		unroll();
		computeFlatNormals();

		// For each array index, create a node and put it into the inputGrid
		buildInputGrid();
        
        if (recomputeNormals) {
            mergeNormals(cosNormalThreshold);
        }

		mergeNodes();

        computeOldToNew(oldToNewIndex);
	}
		
};

class WeldNode {
public:
    bool operator==(const WeldNode* other) { return index == other->index; }

    int         index;
    Vector3*    vec;
    Vector3*    norm;
    Vector2*    tex;
};

class WeldNodeTraits {
public:
    static void getPosition(const WeldNode* v, G3D::Vector3& p) {
        p = *(v->vec);
    }

    static size_t hashCode(const WeldNode* key) {
        return key->vec->hashCode();
    }

    static bool equals(const WeldNode* a, const WeldNode* b) {
        return *(a->vec) == *(b->vec);
    }
};

} // _internal


void MeshAlg::weld(
    Array<Vector3>&     vertices,
    Array<Vector2>&     textureCoords, 
    Array<Vector3>&     normals,
    Array<int>&         indices,
    float               normalSmoothingAngle,
    float               vertexWeldRadius,
    float               textureWeldRadius,
    float               normalWeldRadius) {

/*
    // validate array sizes
    debugAssert(normals.length() == 0 || vertices.length() == normals.length());
    debugAssert(textureCoords.length() == 0 || vertices.length() == textureCoords.length());

    // unroll geometry into triangles and generate normals and texture coordinates if necessary
    Array<Vector3> unrolledVertices(indices.length());
    Array<Vector2> unrolledTextureCoords(indices.length());
    Array<Vector3> unrolledNormals(indices.length());

    Array<_internal::WeldNode*> weldNodes;

    for (int i = 0; i < indices.length(); i += 3) {
        _internal::WeldNode* weldNode = new _internal::WeldNode;
        _internal::WeldNode* weldNode2 = new _internal::WeldNode;
        _internal::WeldNode* weldNode3 = new _internal::WeldNode;

        weldNode->index = i;
        weldNode2->index = i + 1;
        weldNode3->index = i + 2;

        int vertexIndex = indices[i];
        int vertexIndex2 = indices[i + 1];
        int vertexIndex3 = indices[i + 2];

        unrolledVertices[vertexIndex] = vertices[vertexIndex];
        unrolledVertices[vertexIndex2] = vertices[vertexIndex2];
        unrolledVertices[vertexIndex3] = vertices[vertexIndex3];

        weldNode->vec = &unrolledVertices[vertexIndex];
        weldNode2->vec = &unrolledVertices[vertexIndex2];
        weldNode3->vec = &unrolledVertices[vertexIndex3];

        if (textureCoords.length() > 0) {
            unrolledTextureCoords[vertexIndex] = textureCoords[vertexIndex];
            unrolledTextureCoords[vertexIndex2] = textureCoords[vertexIndex2];
            unrolledTextureCoords[vertexIndex3] = textureCoords[vertexIndex3];
        } else {
            unrolledTextureCoords[vertexIndex] = Vector2::zero();
            unrolledTextureCoords[vertexIndex2] = Vector2::zero();
            unrolledTextureCoords[vertexIndex3] = Vector2::zero();
        }

        weldNode->tex = &unrolledTextureCoords[vertexIndex];
        weldNode2->tex = &unrolledTextureCoords[vertexIndex2];
        weldNode3->tex = &unrolledTextureCoords[vertexIndex3];


        if (normals.length() > 0) {
            unrolledNormals[vertexIndex] = normals[vertexIndex];
            unrolledNormals[vertexIndex2] = normals[vertexIndex2];
            unrolledNormals[vertexIndex3] = normals[vertexIndex3];
        } else {
            const Vector3& n0 = vertices[vertexIndex];
            const Vector3& n1 = vertices[vertexIndex2];
            const Vector3& n2 = vertices[vertexIndex3];

            Vector3 faceNormal = (n1 - n0).cross(n2 - n0);
            faceNormal = faceNormal.directionOrZero();

            unrolledNormals[vertexIndex] = faceNormal;
            unrolledNormals[vertexIndex2] = faceNormal;
            unrolledNormals[vertexIndex3] = faceNormal;
        }

        weldNode->norm = &unrolledNormals[vertexIndex];
        weldNode2->norm = &unrolledNormals[vertexIndex2];
        weldNode3->norm = &unrolledNormals[vertexIndex3];

        weldNodes.append(weldNode);
        weldNodes.append(weldNode2);
        weldNodes.append(weldNode3);
    }

    // create tree of vertices to query neighbors
    PointAABSPTree<_internal::WeldNode*, _internal::WeldNodeTraits, _internal::WeldNodeTraits, _internal::WeldNodeTraits> unrolledTree;
    unrolledTree.insert(weldNodes);
    unrolledTree.balance();

    // merge unique vertices within vertex weld radius (same vertex count, just not unique)
    Array<_internal::WeldNode*> neighbors;
    for (int nodeIndex = 0; nodeIndex < weldNodes.length(); ++nodeIndex) {
        neighbors.clear(false);
        unrolledTree.getIntersectingMembers(Sphere(*(weldNodes[nodeIndex]->vec), vertexWeldRadius), neighbors);

        for (int neighborIndex = 0; neighborIndex < neighbors.length(); ++neighborIndex) {
            *(neighbors[neighborIndex]->vec) = *(weldNodes[nodeIndex]->vec);
        }

        // update tree    
        unrolledTree.update(weldNodes[nodeIndex]);
    }

    // merge texture coordinates
    for (int nodeIndex = 0; nodeIndex < weldNodes.length(); ++nodeIndex) {
        neighbors.clear(false);
        unrolledTree.getIntersectingMembers(Sphere(*(weldNodes[nodeIndex]->vec), textureWeldRadius), neighbors);

        for (int neighborIndex = 0; neighborIndex < neighbors.length(); ++neighborIndex) {
            *(neighbors[neighborIndex]->tex) = *(weldNodes[nodeIndex]->tex);
        }
    }

    // merge normals
    float cosSmoothAngle = cos(normalSmoothingAngle);
    for (int nodeIndex = 0; nodeIndex < weldNodes.length(); ++nodeIndex) {
        neighbors.clear(false);
        unrolledTree.getIntersectingMembers(Sphere(*(weldNodes[nodeIndex]->vec), normalWeldRadius), neighbors);

        for (int neighborIndex = 0; neighborIndex < neighbors.length(); ++neighborIndex) {
            if (weldNodes[nodeIndex]->norm->dot(*(neighbors[neighborIndex]->norm)) >= cosSmoothAngle) {
                *(neighbors[neighborIndex]->norm) = *(weldNodes[nodeIndex]->norm);
            }
        }
    }

    unrolledTree.clearData();

    vertices.clear();
    textureCoords.clear();
    normals.clear();
    indices.clear();

    for (int nodeIndex = 0; nodeIndex < weldNodes.length(); ++nodeIndex) {
        neighbors.clear(false);
        unrolledTree.getIntersectingMembers(Sphere(*(weldNodes[nodeIndex]->vec), 0.0f), neighbors);

        if (neighbors.length() == 0) {
            weldNodes[nodeIndex]->index = vertices.length();

            vertices.append(*(weldNodes[nodeIndex]->vec));
            textureCoords.append(*(weldNodes[nodeIndex]->tex));
            normals.append(*(weldNodes[nodeIndex]->norm));

            indices.append(weldNodes[nodeIndex]->index);

            unrolledTree.insert(weldNodes[nodeIndex]);
        } else {
            for (int neighborIndex = 0; neighborIndex < neighbors.length(); ++neighborIndex) {
                if (*(neighbors[neighborIndex]->vec) == *(weldNodes[nodeIndex]->vec) &&
                    *(neighbors[neighborIndex]->tex) == *(weldNodes[nodeIndex]->tex) &&
                    *(neighbors[neighborIndex]->norm) == *(weldNodes[nodeIndex]->norm)) {

                    indices.append(neighbors[neighborIndex]->index);
                    break;
                }
            }
        }
    }
*/
}

} // G3D
