/**
  @file empty/main.cpp

  This is a sample main.cpp to get you started with G3D.  It is
  designed to make writing an application easy.  Although the
  GApp/GApplet infrastructure is helpful for most projects,
  you are not restricted to using it-- choose the level of
  support that is best for your project.

  @author Morgan McGuire, morgan3d@users.sf.net
 */

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include <conio.h>


/**
 An AABSPTree that can render itself for debugging purposes.
 */
class VisibleBSP : public AABSPTree<Vector3> {
protected:

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
    VisibleBSP(int w, int h) {
        int N = 200;
        for (int i = 0; i < N; ++i) {
            insert(Vector3(uniformRandom(0, w), uniformRandom(0, h), 0));
        }
        balance();
    }

    /**
     Draw a 2D projected version; ignore splitting planes in z
     */
    void render2D(RenderDevice* rd) {
        rd->push2D();
            drawNode(rd, root, 20, 0);
        rd->pop2D();
    }
    
};



#if defined(G3D_VER) && (G3D_VER < 70000)
    #error Requires G3D 7.00
#endif

/**
 This simple demo applet uses the debug mode as the regular
 rendering mode so you can fly around the scene.
 */
class Demo : public GApplet {
public:

    VisibleBSP          bsp;

    // Add state that should be visible to this applet.
    // If you have multiple applets that need to share
    // state, put it in the App.

    class App*          app;

    Demo(App* app);

    virtual ~Demo() {}

    virtual void onInit();

    virtual void onLogic();

	virtual void onNetwork();

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

    virtual void onGraphics(RenderDevice* rd);

    virtual void onUserInput(UserInput* ui);

    virtual void onCleanup();

};



class App : public GApp {
protected:
    int main();
public:
    SkyRef              sky;

    Demo*               applet;

    App(const GApp::Settings& settings);

    ~App();
};


Demo::Demo(App* _app) : GApplet(_app), app(_app), bsp(_app->renderDevice->width(), _app->renderDevice->height()) {
}


void Demo::onInit()  {
    // Called before Demo::run() beings
    app->debugCamera.setPosition(Vector3(0, 2, 10));
    app->debugCamera.lookAt(Vector3(0, 2, 0));

    GApplet::onInit();
}


void Demo::onCleanup() {
    // Called when Demo::run() exits
  GApplet::onCleanup();
}


void Demo::onLogic() {
    // Add non-simulation game logic and AI code here
}


void Demo::onNetwork() {
	// Poll net messages here
}


void Demo::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
	// Add physical simulation here.  You can make your time advancement
    // based on any of the three arguments.
}


void Demo::onUserInput(UserInput* ui) {
    if (ui->keyPressed(GKey::ESCAPE)) {
        // Even when we aren't in debug mode, quit on escape.
        endApplet = true;
        app->endProgram = true;
    }

	// Add other key handling here
	
	//must call GApplet::onUserInput
	GApplet::onUserInput(ui);
}


void Demo::onGraphics(RenderDevice* rd) {
    rd->clear();

    bsp.render2D(rd);
}


int App::main() {
	setDebugMode(true);
	debugController->setActive(false);
    debugShowRenderingStats = false;
    
    applet->run();

    return 0;
}


App::App(const GApp::Settings& settings) : GApp(settings) {
    applet = new Demo(this);
}


App::~App() {
    delete applet;
}

G3D_START_AT_MAIN();






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

int main(int argc, char** argv) {
    
    perfAABSPTree();
    getch();
    return 0;
    
	GApp::Settings settings;
    settings.useNetwork = false;

    return App(settings).run();
}
