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
	NodeArrays(Array<int>* const _indexArray, 
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
    indexArray(new Array<int>()),
    vertexArray(new Array<Vector3>()), 
    normalArray(new Array<Vector3>()), 
	texCoordArray(new Array<TexCoord>()),
    deleteArrays(true) {}

NodeArrays::NodeArrays(Array<int>* const _indexArray, 
    Array<Vector3>* const _vertexArray, 
    Array<Vector3>* const _normalArray, 
    Array<TexCoord>* const _texCoordArray) : 
    indexArray(_indexArray),
    vertexArray(_vertexArray), 
    normalArray(_normalArray), 
    texCoordArray(_texCoordArray), 
    deleteArrays(false) {}

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
	if(deleteArrays) {
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

		//Iterate through each vertex of the unrolled arrays
		Array<Node> neighbors;
		for(int i = 0; i < unrolledArrays.size(); ++i) {
			Node n(i, &unrolledArrays);

			//get the index in the output arrays where this vertex (or the 
            //vertex it is being merged into) is stored
			int index = getMatchingNodeIndex(n);

			//add to the end of the output index array to store the vertex
			outputArrays.indexArray->append(index);

            //remember where index i in the output arrays was sent
            outputIndex[i] = index;
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
        vertices, normals, texCoords, and indices should be the vertex, normal,
        texCoord, and index arrays of the input mesh.
        _r, _s, and _theta indicate the maximum allowed vertex, texCoord, and 
        normal angle differences for vertices to be merged.
        normal smoothing will be applied on all angles with a cosine less than 
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

        // Unroll the arrays and compute the normals.
		unroll();
		computeFlatNormals();

		// For each array index, create a node and put it into the inputGrid
		buildInputGrid();
        if(recomputeNormals) {
            mergeNormals(cosNormalThreshold);
        }
		mergeNodes();

        computeOldToNew(oldToNewIndex);
	}
		
};
} // _internal


void MeshAlg::weld(
    Geometry&             geometry, 
    Array<Vector2>&       texCoord, 
    Array<int>&           indexArray,
    Array<int>&           oldToNewIndex, 
    bool                  recomputeNormals,
    float                 normalSmoothingAngle,
    float                 vertexWeldRadius,
    float                 texCoordWeldRadius,
    float                 normalWeldRadius) {

    
    _internal::MeshHelper helper(  geometry.vertexArray, 
                        geometry.normalArray, 
                        texCoord, 
                        indexArray, 
                        vertexWeldRadius, 
                        texCoordWeldRadius,
                        normalWeldRadius,
                        cos(normalSmoothingAngle),
                        oldToNewIndex,
                        recomputeNormals);



}

} // G3D
