#include "G3D/G3DAll.h"

/**
 An AABSPTree that can render itself for debugging purposes.
 */
class VisibleBSP : public AABSPTree<Vector3> {
protected:

    VisibleBSP() {
        int N = 200;
        for (int i = 0; i < N; ++i) {
            insert(Vector3(uniformRandom(0, app->renderDevice->width()), uniformRandom(0, app->renderDevice->height()), 0));
        }
        balance();
    }

    void drawPoint(RenderDevice* rd, const Vector2& pt, float radius, const Color3& col) {
        Draw::rect2D(Rect2D::xywh(pt.x - radius, pt.y - radius, radius * 2, radius * 2), rd, col);
    }

    void drawNode(RenderDevice* rd, Node* node, float radius, int level) {
        
        Color3 color = Color3::white();

        // Draw the points at this node
        for (int i = 0; i < node->valueArray.size(); ++i) {
            const Vector3& pt = node->valueArray[i]->value;
            drawPoint(rd, pt.xy(), radius, Color3::cyan());
        }

        if (node->splitAxis != 2) {
            // Draw the splitting plane
            const AABox& bounds = node->splitBounds;
            Vector2 v1 = bounds.low().xy();
            Vector2 v2 = bounds.high().xy();

            // Make the line go horizontal or vertical based on the split axis
            v1[node->splitAxis] = node->splitLocation;
            v2[node->splitAxis] = node->splitLocation;

            rd->setLineWidth(radius / 2.0f);
            rd->setColor(color);
            rd->beginPrimitive(RenderDevice::LINES);
                rd->sendVertex(v1);
                rd->sendVertex(v2);
            rd->endPrimitive();
        }

        // Shrink radius
        float nextRad = max(0.5f, radius / 2.0f);

        for (int i = 0;i < 2; ++i) {
            if (node->child[i]) {
                drawNode(rd, node->child[i], nextRad, level + 1);
            }
        }
    }

public:

    /**
     Draw a 2D projected version; ignore splitting planes in z
     */
    void render2D(RenderDevice* rd) {
        rd->push2D();
            drawNode(rd, root, 20, 0);
        rd->pop2D();
    }
    
};


static void testSerialize() {
    AABSPTree<Vector3> tree;
    int N = 1000;

    for (int i = 0; i < N; ++i) {
        tree.insert(Vector3::random());
    }
    tree.balance();

    // Save the struture
    BinaryOutput b("test-bsp.dat", G3D_LITTLE_ENDIAN);
    tree.serializeStructure(b);
    b.commit();

}


static void testBoxIntersect() {

	AABSPTree<Vector3> tree;

	// Make a tree containing a regular grid of points
	for (int x = -5; x <= 5; ++x) {
		for (int y = -5; y <= 5; ++y) {
			for (int z = -5; z <= 5; ++z) {
				tree.insert(Vector3(x, y, z));
			}
		}
	}
	tree.balance();

	AABox box(Vector3(-1.5, -1.5, -1.5), Vector3(1.5, 1.5, 1.5));

	AABSPTree<Vector3>::BoxIntersectionIterator it = tree.beginBoxIntersection(box);
	const AABSPTree<Vector3>::BoxIntersectionIterator end = tree.endBoxIntersection();

	int hits = 0;
	while (it != end) { 
		const Vector3& v = *it;

		debugAssert(box.contains(v));

		++hits;
		++it;
	}

	debugAssertM(hits == 3*3*3, "Wrong number of intersections found in testBoxIntersect for AABSPTree");
}


void perfAABSPTree() {

    Array<AABox>                array;
    AABSPTree<AABox>            tree;
    
    const int NUM_POINTS = 1000000;
    
    for (int i = 0; i < NUM_POINTS; ++i) {
        Vector3 pt = Vector3(uniformRandom(-10, 10), uniformRandom(-10, 10), uniformRandom(-10, 10));
        AABox box(pt, pt + Vector3(.1f, .1f, .1f));
        array.append(box);
        tree.insert(box);
    }

    RealTime t0 = System::time();
    tree.balance();
    RealTime t1 = System::time();
    printf("AABSPTree<AABox>::balance() time for %d boxes: %gs\n\n", NUM_POINTS, t1 - t0);

    uint64 bspcount = 0, arraycount = 0, boxcount = 0;

    // Run twice to get cache issues out of the way
    for (int it = 0; it < 2; ++it) {
        Array<Plane> plane;
        plane.append(Plane(Vector3(-1, 0, 0), Vector3(3, 1, 1)));
        plane.append(Plane(Vector3(1, 0, 0), Vector3(1, 1, 1)));
        plane.append(Plane(Vector3(0, 0, -1), Vector3(1, 1, 3)));
        plane.append(Plane(Vector3(0, 0, 1), Vector3(1, 1, 1)));
        plane.append(Plane(Vector3(0,-1, 0), Vector3(1, 3, 1)));
        plane.append(Plane(Vector3(0, 1, 0), Vector3(1, -3, 1)));

        AABox box(Vector3(1, 1, 1), Vector3(3,3,3));

        Array<AABox> point;

        System::beginCycleCount(bspcount);
        tree.getIntersectingMembers(plane, point);
        System::endCycleCount(bspcount);

        point.clear();

        System::beginCycleCount(boxcount);
        tree.getIntersectingMembers(box, point);
        System::endCycleCount(boxcount);

        point.clear();

        System::beginCycleCount(arraycount);
        for (int i = 0; i < array.size(); ++i) {
            if (! array[i].culledBy(plane)) {
                point.append(array[i]);
            }
        }
        System::endCycleCount(arraycount);
    }

    printf("AABSPTree<AABox>::getIntersectingMembers(plane) %g Mcycles\n"
           "AABSPTree<AABox>::getIntersectingMembers(box)   %g Mcycles\n"
           "Culled by on Array<AABox>                       %g Mcycles\n\n", 
           bspcount / 1e6, 
           boxcount / 1e6,
           arraycount / 1e6);
}


void testAABSPTree() {
	printf("AABSPTree ");

	testBoxIntersect();
	testSerialize();

	printf("passed\n");
}
