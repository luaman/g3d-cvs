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

class WeldNode {
public:
    bool operator==(const WeldNode& other)  { return index == other.index; }

    int         index;
};

class WeldNodeTraits {
public:
    static Array<Vector3>* vertices;

    static void getPosition(const WeldNode& v, G3D::Vector3& p) {
        p = (*vertices)[v.index];
    }

    static size_t hashCode(const WeldNode& key) {
        return (*vertices)[key.index].hashCode();
    }

    static bool equals(const WeldNode& a, const WeldNode& b) {
        return a.index == b.index;
    }
};

Array<Vector3>* WeldNodeTraits::vertices = NULL;


void MeshAlg::weld(
    Array<Vector3>&     vertices,
    Array<Vector2>&     textureCoords, 
    Array<Vector3>&     normals,
    Array<int>&         indices,
    float               normalSmoothingAngle,
    float               vertexWeldRadius,
    float               textureWeldRadius,
    float               normalWeldRadius) {

    // validate array sizes
    debugAssert((indices.length() % 3) == 0);
    debugAssert(normals.length() == 0 || vertices.length() == normals.length());
    debugAssert(textureCoords.length() == 0 || vertices.length() == textureCoords.length());

    WeldNodeTraits::vertices = &vertices;

    // create tree nodes with indices
    Array<WeldNode> weldNodes;
    for (int i = 0; i < indices.length(); ++i) {
        WeldNode node;
        node.index = indices[i];
        weldNodes.append(node);
    }

    // generate texture coordinates if required
    if (textureCoords.length() == 0) {
        textureCoords.resize(indices.length());
        for (int i = 0; i < indices.length(); ++i) {
            textureCoords.append(Vector2::zero());
        }
    }

    // generate normals if required
    if (normals.length() == 0) {
        normals.resize(indices.length());
        for (int i = 0; i < indices.length(); i += 3) {
            const Vector3& n0 = vertices[indices[i]];
            const Vector3& n1 = vertices[indices[i]];
            const Vector3& n2 = vertices[indices[i]];

            Vector3 faceNormal = (n1 - n0).cross(n2 - n0).directionOrZero();
            normals.append(faceNormal);
            normals.append(faceNormal);
            normals.append(faceNormal);
        }
    }

    // create tree of vertex indices to query neighbors
    PointAABSPTree<WeldNode, WeldNodeTraits, WeldNodeTraits, WeldNodeTraits> nodeTree;
    nodeTree.insert(weldNodes);
    nodeTree.balance();

    // merge vertices
    Array<WeldNode> neighbors;
    for (int i = 0; i < weldNodes.length(); ++i) {
        neighbors.clear(false);

        nodeTree.getIntersectingMembers(Sphere(vertices[weldNodes[i].index], vertexWeldRadius), neighbors);

        for (int n = 0; n < neighbors.length(); ++n) {
            vertices[neighbors[n].index] = vertices[weldNodes[i].index];
            nodeTree.update(neighbors[n]);
        }
    }

    // merge texture coordinates
    for (int i = 0; i < weldNodes.length(); ++i) {
        neighbors.clear(false);

        nodeTree.getIntersectingMembers(Sphere(vertices[weldNodes[i].index], textureWeldRadius), neighbors);

        for (int n = 0; n < neighbors.length(); ++n) {
            textureCoords[neighbors[n].index] = textureCoords[weldNodes[i].index];
        }
    }

    // merge normals
    for (int i = 0; i < weldNodes.length(); ++i) {
        neighbors.clear(false);

        nodeTree.getIntersectingMembers(Sphere(vertices[weldNodes[i].index], normalWeldRadius), neighbors);

        // find average of all normals within distance and angle
        Vector3 sum = normals[weldNodes[i].index];

        float cosSmoothAngle = cos(normalSmoothingAngle);
        for (int n = 0; n < neighbors.length(); ++n) {
            if (normals[weldNodes[i].index].dot(normals[neighbors[n].index]) >= cosSmoothAngle) {
                sum += normals[neighbors[n].index];
            }
        }

        // set the current normal to the average
        sum = sum.directionOrZero();
        normals[weldNodes[i].index] = sum;
    }

    // clear tree and indices
    nodeTree.clearData();
    indices.clear();

    // find matching points and create new indices collapsing the points used
    for (int i = 0; i < weldNodes.length(); ++i) {
        neighbors.clear(false);

        nodeTree.getIntersectingMembers(Sphere(vertices[weldNodes[i].index], 0.0f), neighbors);

        if (neighbors.length() == 0) {
            nodeTree.insert(weldNodes[i]);
            indices.append(weldNodes[i].index);
        } else {
            for (int n = 0; n < neighbors.length(); ++n) {
                if (vertices[neighbors[n].index] == vertices[weldNodes[i].index] &&
                    textureCoords[neighbors[n].index] == textureCoords[weldNodes[i].index] &&
                    normals[neighbors[n].index] == normals[weldNodes[i].index]) {

                    indices.append(neighbors[n].index);
                    break;
                }
            }
        }
    }
}

} // G3D
