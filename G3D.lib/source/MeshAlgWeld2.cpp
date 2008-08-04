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

class NodeArrays;

typedef Vector2 TexCoord;


/**
	Class containing arrays which specify vertices, normals, and texture coordinates of mesh vertices.
	vertexArray, normalArray, and texCoordArray are parallel arrays, so vertexArray[i], normalArray[i], and
	texCoordArray[i] together describe one vertex.
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
	NodeArrays() : indexArray(new Array<int>()), vertexArray(new Array<Vector3>()), normalArray(new Array<Vector3>()), 
		texCoordArray(new Array<TexCoord>()) {
		deleteArrays = true;
	}

	/** Constructor that uses pre-existing Arrays.*/
	NodeArrays(Array<int>* const _indexArray, Array<Vector3>* const _vertexArray, Array<Vector3>* const _normalArray, 
		Array<TexCoord>* const _texCoordArray) : 
		indexArray(_indexArray), vertexArray(_vertexArray), normalArray(_normalArray), texCoordArray(_texCoordArray) {
		deleteArrays = false;
	}

	/** If the constructor allocated arrays, delete them here.*/
	~NodeArrays() {
		if(deleteArrays) {
			delete indexArray;
			delete vertexArray;
			delete normalArray;
			delete texCoordArray;
		}
	}
};

/** A pointer to one index in parallel vertex, normal, and texCoord arrays. This describes one vertex of a mesh face.*/
class Node {

private:
	int					m_index;
	NodeArrays*			m_arrays;

public:
	class Position {
	public:
		static void getPosition(const Node& obj, G3D::Vector3& p) {
			p = obj.vertex();
		}
	};

	class Equals {
	public:
		static bool equals(const Node& a, const Node& b) {
			return a == b;
		}
	};

    class HashFunc {
    public:
        static size_t hashCode(const Node& key) {
            return HashTrait<Vector3>::hashCode(key.vertex());
        }
    };

	bool operator==(const Node& other) const {
		return m_index == other.m_index && m_arrays == other.m_arrays;
	}
	
	Node(const int index, NodeArrays* const arrays) : m_index(index), m_arrays(arrays) {}
    Node() : m_index(0), m_arrays(NULL) {}

	Vector3& vertex () {
		return (*m_arrays->vertexArray)[m_index];
	}
	Vector3& normal() {
		return (*m_arrays->normalArray)[m_index];
	}
	TexCoord texCoord() {
        if(m_arrays->texCoordArray->size() > 0) {
		    return (*m_arrays->texCoordArray)[m_index];
        } else {
            return TexCoord::zero();
        }
	}
	int index() {
		return m_index;
	}

	const Vector3& vertex() const {
		return (*(m_arrays->vertexArray))[m_index];
	}
	const Vector3& normal() const {
		return (*(m_arrays->normalArray))[m_index];
	}
	const TexCoord texCoord() const {
        if(m_arrays->texCoordArray->size() > 0) {
		    return (*m_arrays->texCoordArray)[m_index];
        } else {
            return TexCoord::zero();
        }
	}
	const int index() const {
		return m_index;
	}

	/** Returns a node that is the average of the values in an array of nodes*/
	static void average(const Array<Node>& nodes, Node& n, NodeArrays* const averageNode) {
		debugAssert(nodes.size() > 0);
		Vector3 vertexSum;
		Vector3 normalSum;
		TexCoord texCoordSum;
		int numNodes = nodes.size();
		for(int i = 0; i < numNodes; ++i) {
			vertexSum += nodes[i].vertex();
			normalSum += nodes[i].normal();
			texCoordSum += nodes[i].texCoord();
		}

		// Put the averages into the first index of averageNode and return a node which refers to that index
		(*averageNode->vertexArray)[0] = vertexSum / numNodes;
		(*averageNode->normalArray)[0] = normalSum / numNodes;
		(*averageNode->texCoordArray)[0] = texCoordSum / numNodes;

		// TODO: why does n(0, averageNode); cause a compiler error?
		n = Node(0, averageNode);
	}
};



class MeshHelper {

public:

private:
	

    typedef PointAABSPTree<Node, Node::Position, Node::HashFunc, Node::Equals> Grid;

	/** Constructed from the arrays passed to the constructor.*/
	NodeArrays		inputArrays;
	/** The input arrays are "unrolled" (so that each face has its own vertex, normal, and texCoords) here.*/
	NodeArrays		unrolledArrays;
	/** This is a temporary instance to store neighborhood averages. Its arrays should only be length 1.*/
	NodeArrays		averageNode;
	/** Contains the arrays after similar nodes have been merged.*/
	NodeArrays		outputArrays;
	/** The nodes from the unrolled arrays are placed here. Used for finding nearby vertices when computing neighborhoods.*/
	Grid			inputNodes;
	/** After nodes have been placed in the outputArrays, they are placed here so that similar nodes are not created.*/
	Grid			outputNodes;

    /** Tracks which index in the unrolled arrays has been assigned to each index in the input arrays. */
    Array<int> unrolledIndex;

    /** Tracks which index in the output arrays has been assigned to each index in the unrolled arrays. */
    Array<int> outputIndex;

	double			r;
	/** 1/(r^2), for use in the distance formula.*/
	double			rSquaredInverse;
	/** 1/(s^2), for use in the distance formula.*/
	double			sSquaredInverse;
	/** 1-cos(theta), for use in the distance formula.*/
	double			oneMinusCosineTheta;

    /** Computes the average of an array of Vector3s. */
    Vector3 average(const Array<Vector3>& v) {
        if(v.size() == 0) return Vector3::zero();

        Vector3 sum = Vector3::zero();
        for(int i = 0; i < v.size(); ++i) {
            sum += v[i];
        }
        return sum / v.size();
    }

	/** "Unrolls" the arrays in inputArrays according to the index array. The unrolled arrays are stored in unrolledArrays,
		and any old data in the vertex and texCoord arrays of unrolledArrays is lost.*/
	void unroll() {
        alwaysAssertM((inputArrays.vertexArray->size() == inputArrays.texCoordArray->size()) || (inputArrays.texCoordArray->size() == 0), 
            "Mesh is textured, but vertex and texCoord arrays are different lengths");
       
        unrolledIndex.resize(inputArrays.vertexArray->size());

		unrolledArrays.vertexArray->fastClear();
		unrolledArrays.texCoordArray->fastClear();

        if(inputArrays.texCoordArray->size() > 0) {

            // Textured mesh
		    for(int i = 0; i < inputArrays.indexArray->size(); ++i) {
			    unrolledArrays.vertexArray->append((*inputArrays.vertexArray)[(*inputArrays.indexArray)[i]]);
                unrolledArrays.texCoordArray->append((*inputArrays.texCoordArray)[(*inputArrays.indexArray)[i]]);
                
                int oldIndex = (*inputArrays.indexArray)[i];
                unrolledIndex[oldIndex] = i;
		    }
        } else {

            // Untextured mesh
            for(int i = 0; i < inputArrays.indexArray->size(); ++i) {
                unrolledArrays.vertexArray->append((*inputArrays.vertexArray)[(*inputArrays.indexArray)[i]]);

                int oldIndex = (*inputArrays.indexArray)[i];
                unrolledIndex[oldIndex] = i;
            }
        }
	}

	/** Uses the data in unrolledArrays.vertexArray to fill unrolledArrays.normalArray with the vector normals before smoothing.
		Assumes unroll has been called, and overwrites any old data in the normal array of unrolledArrays.*/
	void computeFlatNormals() {
		debugAssert(unrolledArrays.vertexArray->size() > 0);
		unrolledArrays.normalArray->fastClear();
		for(int i = 0; i < unrolledArrays.vertexArray->size(); i += 3) {
			Vector3 f = ((*unrolledArrays.vertexArray)[i+1] - (*unrolledArrays.vertexArray)[i]).cross(
				(*unrolledArrays.vertexArray)[i+2] - (*unrolledArrays.vertexArray)[i]);
			Vector3 normal = f;
			if(normal != Vector3::zero()) {
				normal = f.direction();
			}
			for(int j = 0; j < 3; ++j) {
				unrolledArrays.normalArray->append(normal);
			}
		}
	}

	/** Finds all neighboring nodes (as dictated by vertex position) which also have similar normals. Any old data in similarNormals 
        will be lost. The normals of neighboring nodes with similar normals will be put into similarNormals. */
	void getSimilarNormals(const Node& n, Array<Vector3>& similarNormals, const float cosThreshold) {
		similarNormals.fastClear();
		Sphere s(n.vertex(), r);
        Array<Node> neighbors;
        inputNodes.getIntersectingMembers(s, neighbors);

		for(int i = 0; i < neighbors.size(); ++i) {
            float cosAngle = n.normal().dot(neighbors[i].normal());

			if (cosAngle > cosThreshold) {
				similarNormals.append(neighbors[i].normal());
            }
		}
	}

    /** Uses the data in unrolledArrays.normalArray to merge the normals of nearby vertices with similar normals.
        Assumes that computeFlatNormals has been called, and changes the data in unrolledArrays->normalArray.*/
    void mergeNormals(const float cosThreshold) {
        Array<Vector3>* newNormals = new Array<Vector3>();

        for(int i = 0; i < unrolledArrays.vertexArray->size(); ++i) {
            Node n(i, &unrolledArrays);
            Array<Vector3> neighbors;

            // Get the neighbors of this node
            getSimilarNormals(n, neighbors, cosThreshold);
            // The new normal of this node is the average of the vectors in neighbors
            Vector3 newNormal = average(neighbors).direction();
            newNormals->append(newNormal);
        }

        delete unrolledArrays.normalArray;
        unrolledArrays.normalArray = newNormals;
    }

	/** Find all neighbors (nodes which have a distance <= 1) of a node, and put them into nodeArray. Any old data in nodeArray will be lost.
		In this case, "distance" is a measure of how similar two nodes are, and its square is the value returned by distanceSquared.*/
	void getNeighbors(const Node& n, Array<Node>& nodeArray) {
		nodeArray.fastClear();
		Sphere s(n.vertex(), r);
        Array<Node> possibleNeighbors;
        inputNodes.getIntersectingMembers(s, possibleNeighbors);

		for(int i = 0; i < possibleNeighbors.size(); ++i) {
			if(distanceSquared(n, possibleNeighbors[i]) <= 1.0) {
				nodeArray.append(possibleNeighbors[i]);
			}
		}
	}

	/** If there is at least one node in outputGrid within a distance of 1 (per the distanceSquared function), 
		returns true and and stores the nearest node in newNode.
		If there is no such node, returns false and leaves newNode unchanged.*/
	bool getNearest(const Node& n, Node& newNode) {

		if(outputNodes.size() == 0) {

			// The output grid is empty, so return false
			return false;
		} else {

			Sphere s(n.vertex(), r);
			double minDist = inf();

            Array<Node> neighbors;
            outputNodes.getIntersectingMembers(s, neighbors);

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

	/** Creates an entry in the output grid that is equivalent to a node, putting its values into the outputArrays. Called by MergeNodes.*/
	void createOutputNode(Node& n) {
		outputArrays.vertexArray->append(n.vertex());
		outputArrays.normalArray->append(n.normal());
		outputArrays.texCoordArray->append(n.texCoord());
	}

	/** Returns the index in the outputArrays which contains either the passed node or a node which it will be merged into.*/
	int getMatchingNodeIndex(Node& n) {
		Array<Node> neighbors;
		getNeighbors(n, neighbors);
		Node average;
		Node::average(neighbors, average, &averageNode);
		Node nearestNode;
		bool merge = getNearest(average, nearestNode);
		if(!merge) {

			// n is not being merged, so put it into the output arrays and the output grid
			createOutputNode(n);
			int index = outputArrays.vertexArray->size() - 1;				// the index for n is the last index of the output arrays
			outputNodes.insert(Node(index, &outputArrays));
			return index;
		} else {

			// n is being merged, so return the index of the node it is being merged into
			return nearestNode.index();
		}
	}

	/** Finds similar indices in unrolledArrays, then merges them and fills the outputArrays. Assumes that unroll and computeFlatNormals have been called.*/
	void mergeNodes() {
		debugAssert(unrolledArrays.vertexArray->size() > 0);

		outputArrays.indexArray->fastClear();
		outputArrays.vertexArray->fastClear();
		outputArrays.normalArray->fastClear();
		outputArrays.texCoordArray->fastClear();

		averageNode.vertexArray->fastResize(1);
		averageNode.normalArray->fastResize(1);
		averageNode.texCoordArray->fastResize(1);

        //"Prime" the tree so that the next step is fast
        for(int i = 0; i < unrolledArrays.vertexArray->size(); ++i) {
            Node n(i, &unrolledArrays);
            outputNodes.insert(n);
        }
        outputNodes.balance();
        outputNodes.clearData();

		//Iterate through each vertex of the unrolled arrays
        outputIndex.resize(unrolledIndex.size());
		Array<Node> neighbors;
		for(int i = 0; i < unrolledArrays.vertexArray->size(); ++i) {
			Node n(i, &unrolledArrays);

			//get the index in the output arrays where this vertex (or the vertex it is being merged into) is stored
			int index = getMatchingNodeIndex(n);

			//add to the end of the output index array to store the vertex
			outputArrays.indexArray->append(index);

            //remember where index i in the output arrays was sent
            outputIndex[i] = index;
		}
	}


	/** Computes the squared distance between two nodes.
		Formula: d^2 =	 (1/r^2)(||vertex(a) - vertex(b)||^2)
						+([1 - (normal(a)*normal(b))]/[1 - cos(theta)])^2
						+(1/s^2)(||texCoord(a) - texCoord(b)||^2)
						
		NOTE: Some meshes contain zero-area faces to eliminate "holes" in the mesh due to round-off error.
		If either vertex has a zero normal vector, the normals should be ignored when computing the distance.
		Since all nonzero normals have unit length, we do this by multiplying by the magnitudes of a and b in the
		second line of the formula.*/
	double distanceSquared(const Node& a, const Node& b) {
		double vertexMagnitude2 = (a.vertex() - b.vertex()).squaredLength();
		double texCoordMagnitude2 = (a.texCoord() - b.texCoord()).squaredLength();
        double dotProd = a.normal().dot(b.normal());
        double normalDist = (1.0 - dotProd) * a.normal().magnitude() * b.normal().magnitude() / oneMinusCosineTheta;
        if(!isFinite(normalDist)) {
            normalDist = 0.0;
        }

		return rSquaredInverse * square(vertexMagnitude2) + square(normalDist) + sSquaredInverse * texCoordMagnitude2;
	}

	/** Creates nodes from the entries of unrolledArrays and puts them into the inputGrid.*/
	void buildInputGrid() {
		for(int i = 0; i < unrolledArrays.vertexArray->size(); ++i) {
			Node n(i, &unrolledArrays);
			inputNodes.insert(n);
		}
        inputNodes.balance();
	}

    /** Computes an array which maps indices in the input arrays to indices in the output arrays.*/
    void computeOldToNew(Array<int>& oldToNewArray) {
        oldToNewArray.resize(unrolledIndex.size());
        for(int i = 0; i < oldToNewArray.size(); ++i) {
            int unrolled = unrolledIndex[i];
            int output = outputIndex[unrolled];
            oldToNewArray[i] = output;
        }
    }

public:

	MeshHelper(Array<Vector3>& vertices, Array<Vector3>& normals, Array<TexCoord>& texCoords, Array<int>& indices, 
		const double _r, const double _s, const double _theta, const double cosNormalThreshold, Array<int>& oldToNewIndex, bool recomputeNormals = true) : 
		inputArrays(&indices, &vertices, &normals, &texCoords), outputArrays(&indices, &vertices, &normals, &texCoords) {

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