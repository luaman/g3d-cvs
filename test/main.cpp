/**
 @file Test/main.cpp

 This file runs unit conformance and performance tests for G3D.  
 To write a new test, add a file named t<class>.cpp to the project
 and provide two entry points: test<class> and perf<class> (even if they are
 empty).

 Call those two methods from main() in main.cpp.

 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 @created 2002-01-01
 @edited  2010-03-03
 */

#include "G3D/G3D.h"
#include "GLG3D/GLG3D.h"
#ifdef main
#    undef main
#endif

using namespace G3D;
#include <iostream>

using namespace G3D;

#ifdef WIN32
#    include "conio.h"
#endif
#include <string>

using G3D::uint64;
using G3D::uint32;
using G3D::uint8;

//#define RUN_SLOW_TESTS

// Forward declarations
void testImageConvert();

void perfArray();
void testArray();
void testSmallArray();

void testMatrix();

void testFileSystem();

void testMatrix3();
void perfMatrix3();

void testMatrix4() {
   float L = -1.0f;
    float R =  4.0f;
    float B = -2.0f;
    float T =  3.0f;
    float N =  1.5f;
    float F = 100.2f;
    Matrix4 P = Matrix4::perspectiveProjection(L, R, B, T, N, F);

    float L2 = -1.0f;
    float R2 =  4.0f;
    float B2 = -2.0f;
    float T2 =  3.0f;
    float N2 =  1.5f;
    float F2 = 100.2f;
    P.getPerspectiveProjectionParameters(L2, R2, B2, T2, N2, F2);
    
    debugAssert(fuzzyEq(L, L2));
    debugAssert(fuzzyEq(R, R2));
    debugAssert(fuzzyEq(B, B2));
    debugAssert(fuzzyEq(T, T2));
    debugAssert(fuzzyEq(N, N2));
    debugAssert(fuzzyEq(F, F2));
}

void testZip();

void testuint128();

void testCollisionDetection();
void perfCollisionDetection();

void testWeakCache();
void testCallback();

void testSpline();

void testGChunk();

void testQuat();

void perfKDTree();
void testKDTree();

void testSphere();

void testAABox();

void testReliableConduit(NetworkDevice*);

void perfSystemMemcpy();
void testSystemMemcpy();
void testSystemMemset();

void testMap2D();

void testReferenceCount();

void testRandom();

void perfTextOutput();

void testMeshAlgTangentSpace();

void perfQueue();
void testQueue();

void testBinaryIO();
void testHugeBinaryIO();
void perfBinaryIO();

void testTextInput();
void testTextInput2();

void testTable();
void testAdjacency();

void perfTable();

void testAtomicInt32();

void testCoordinateFrame();

void testGThread();

void testfilter();

void testAny();


void testPointHashGrid();
void perfPointHashGrid();


void testTableTable() {

    // Test making tables out of tables
    typedef Table<std::string, int> StringTable;
    Table<int, StringTable> table;

    table.set(3, StringTable());
    table.set(0, StringTable());
    table[3].set("Hello", 3);
}

void testGCamera() {
    printf("GCamera...");
    GCamera camera;
    camera.setCoordinateFrame(CFrame());
    camera.setFieldOfView(toRadians(90), GCamera::HORIZONTAL);
    camera.setNearPlaneZ(-1);
    camera.setFarPlaneZ(-100);

    Rect2D viewport = Rect2D::xywh(0,0,200,100);
    Array<Plane> plane;
    camera.getClipPlanes(viewport, plane);
    debugAssertM(plane.size() == 6, "Missing far plane");
    debugAssertM(plane[0].fuzzyContains(Vector3(0,0,-1)), plane[0].center().toString());
    debugAssertM(plane[0].normal() == Vector3(0,0,-1), plane[0].normal().toString());

    debugAssertM(plane[5].fuzzyContains(Vector3(0,0,-100)), plane[1].center().toString());
    debugAssertM(plane[5].normal() == Vector3(0,0,1), plane[1].normal().toString());

    debugAssertM(plane[1].normal().fuzzyEq(Vector3(-1,0,-1).direction()), plane[1].normal().toString());
    debugAssertM(plane[2].normal().fuzzyEq(Vector3(1,0,-1).direction()), plane[2].normal().toString());

    // top
    debugAssertM(plane[3].normal().fuzzyEq(Vector3(0,-0.894427f,-0.447214f).direction()), plane[3].normal().toString());
    debugAssertM(plane[4].normal().fuzzyEq(Vector3(0,0.894427f,-0.447214f).direction()), plane[4].normal().toString());

    Vector3 ll, lr, ul, ur;
    camera.getNearViewportCorners(viewport, ur, ul, ll, lr);
    debugAssertM(ur == Vector3(1, 0.5, -1), ur.toString());
    debugAssertM(lr == Vector3(1, -0.5, -1), lr.toString());
    debugAssertM(ll == Vector3(-1, -0.5, -1), ll.toString());
    debugAssertM(ul == Vector3(-1, 0.5, -1), ul.toString());
    printf("passed\n");
}

void testConvexPolygon2D() {
    printf("ConvexPolygon2D\n");
    Array<Vector2> v;
    v.append(Vector2(0, 0), Vector2(1,1), Vector2(2, 0));
    ConvexPolygon2D C(v);
    debugAssert(! C.contains(Vector2(10, 2)));
    debugAssert(C.contains(Vector2(1, 0.5)));
    printf("  passed\n");
}

void testBox2D() {
    printf("Box2D\n");
    Box2D b(Vector2(0,0), Vector2(2,3));
    debugAssert(b.contains(Vector2(0,0)));
    debugAssert(b.contains(Vector2(2,3)));
    debugAssert(b.contains(Vector2(1,1.5)));
    debugAssert(! b.contains(Vector2(-1,1.5)));
    debugAssert(! b.contains(Vector2(3,1.5)));
    debugAssert(! b.contains(Vector2(1,-1.5)));
    debugAssert(! b.contains(Vector2(1,4)));
}


void testWildcards() {
    printf("filenameContainsWildcards\n");
    debugAssert(!filenameContainsWildcards("file1.exe"));
	debugAssert(filenameContainsWildcards("file?.exe"));
	debugAssert(filenameContainsWildcards("f*.exe"));
	debugAssert(filenameContainsWildcards("f*.e?e"));
	debugAssert(filenameContainsWildcards("*1.exe"));
	debugAssert(filenameContainsWildcards("?ile1.exe"));
}


void testBox() {
    printf("Box\n");
    Box box = Box(Vector3(0,0,0), Vector3(1,1,1));

    debugAssert(box.contains(Vector3(0,0,0)));
    debugAssert(box.contains(Vector3(1,1,1)));
    debugAssert(box.contains(Vector3(0.5,0.5,0.5)));
    debugAssert(! box.contains(Vector3(1.5,0.5,0.5)));
    debugAssert(! box.contains(Vector3(0.5,1.5,0.5)));
    debugAssert(! box.contains(Vector3(0.5,0.5,1.5)));
    debugAssert(! box.contains(-Vector3(0.5,0.5,0.5)));
    debugAssert(! box.contains(-Vector3(1.5,0.5,0.5)));
    debugAssert(! box.contains(-Vector3(0.5,1.5,0.5)));
    debugAssert(! box.contains(-Vector3(0.5,0.5,1.5)));

    Vector3 v0, v1, v2, v3, n1, n2;

    v0 = box.corner(0);
    v1 = box.corner(1);
    v2 = box.corner(2);
    v3 = box.corner(3);

    debugAssert(v0 == Vector3(0,0,1));
    debugAssert(v1 == Vector3(1,0,1));
    debugAssert(v2 == Vector3(1,1,1));
    debugAssert(v3 == Vector3(0,1,1));

    Vector3 n[2] = {Vector3(0,0,1), Vector3(1,0,0)};
    (void)n;

    int i;
    for (i = 0; i < 2; ++i) {
        box.getFaceCorners(i, v0, v1, v2, v3);
        n1 = (v1 - v0).cross(v3 - v0);
        n2 = (v2 - v1).cross(v0 - v1);

        debugAssert(n1 == n2);
        debugAssert(n1 == n[i]);
    }

}



void testAABoxCollision() {
    printf("intersectionTimeForMovingPointFixedAABox\n");

    Vector3 boxlocation, aaboxlocation, normal;

    for (int i = 0; i < 1000; ++i) {

        Vector3 pt1 = Vector3::random() * uniformRandom(0, 10);
        Vector3 vel1 = Vector3::random();

        Vector3 low = Vector3::random() * 5;
        Vector3 extent(uniformRandom(0,4), uniformRandom(0,4), uniformRandom(0,4));
        AABox aabox(low, low + extent);
        Box   box = aabox;

        double boxTime = CollisionDetection::collisionTimeForMovingPointFixedBox(
            pt1, vel1, box, boxlocation, normal);
        (void)boxTime;

        double aaTime = CollisionDetection::collisionTimeForMovingPointFixedAABox(
            pt1, vel1, aabox, aaboxlocation);
        (void)aaTime;

        Ray ray = Ray::fromOriginAndDirection(pt1, vel1);
        double rayboxTime = ray.intersectionTime(box);
        (void)rayboxTime;

        double rayaaTime = ray.intersectionTime(aabox);
        (void)rayaaTime;

        debugAssert(fuzzyEq(boxTime, aaTime));
        if (boxTime < finf()) {
            debugAssert(boxlocation.fuzzyEq(aaboxlocation));
        }

        debugAssert(fuzzyEq(rayboxTime, rayaaTime));
    }
}


void testPlane() {
    printf("Plane\n");
    {
        Plane p(Vector3(1, 0, 0),
                Vector3(0, 1, 0),
                Vector3(0, 0, 0));

        Vector3 n = p.normal();
        debugAssert(n == Vector3(0,0,1));
    }

    {
        Plane p(Vector3(4, 6, .1f),
                Vector3(-.2f, 6, .1f),
                Vector3(-.2f, 6, -.1f));

        Vector3 n = p.normal();
        debugAssert(n.fuzzyEq(Vector3(0,-1,0)));
    }

    {
        Plane p(Vector4(1,0,0,0),
                Vector4(0,1,0,0),
                Vector4(0,0,0,1));
        Vector3 n = p.normal();
        debugAssert(n.fuzzyEq(Vector3(0,0,1)));
    }

    {
        Plane p(
                Vector4(0,0,0,1),
                Vector4(1,0,0,0),
                Vector4(0,1,0,0));
        Vector3 n = p.normal();
        debugAssert(n.fuzzyEq(Vector3(0,0,1)));
    }

    {
        Plane p(Vector4(0,1,0,0),
                Vector4(0,0,0,1),
                Vector4(1,0,0,0));
        Vector3 n = p.normal();
        debugAssert(n.fuzzyEq(Vector3(0,0,1)));
    }
}



class A {
public:
    int x;

    A() : x(0) {
        printf("Default constructor\n");
    }

    A(int y) : x(y) {
        printf("Construct %d\n", x);
    }

    A(const A& a) : x(a.x) {
        printf("Copy %d\n", x);
    }

    A& operator=(const A& other) {
        printf("Assign %d\n", other.x);
        x = other.x;
        return *this;
    }

    ~A() {
        printf("Destruct %d\n", x);
    }
};



void measureMemsetPerformance() {
    printf("----------------------------------------------------------\n");

    uint64 native = 0, g3d = 0;

    int n = 1024 * 1024;
    void* m1 = malloc(n);

    // First iteration just primes the system
    for (int i=0; i < 2; ++i) {
        System::beginCycleCount(native);
            memset(m1, 31, n);
        System::endCycleCount(native);
        System::beginCycleCount(g3d);
            System::memset(m1, 31, n);
        System::endCycleCount(g3d);
    }
    free(m1);

    printf("System::memset:                     %d cycles/kb\n", (int)(g3d / (n / 1024)));
    printf("::memset      :                     %d cycles/kb\n", (int)(native / (n / 1024)));
}





void measureNormalizationPerformance() {
    printf("----------------------------------------------------------\n");
    uint64 raw = 0, opt = 0, overhead = 0;
    int n = 1024 * 1024;

    double y;
    Vector3 x = Vector3(10,-20,3);

    int i, j;

    for (j = 0; j < 2; ++j) {
        x = Vector3(10,-20,3);
		y = 0;
        System::beginCycleCount(overhead);
        for (i = n - 1; i >= 0; --i) {
            x.z = i;
            y += x.z;
        }
        System::endCycleCount(overhead);
    }

    x = Vector3(10,-20,3);
    y = 0;
    System::beginCycleCount(raw);
    for (i = n - 1; i >= 0; --i) {
        x.z = i;
        y += x.direction().z;
        y += x.direction().z;
        y += x.direction().z;
    }
    System::endCycleCount(raw);
    
    x = Vector3(10,-20,3);
    y = 0;
    System::beginCycleCount(opt);
    for (i = n - 1; i >= 0; --i) {
        x.z = i;
        y += x.fastDirection().z;
        y += x.fastDirection().z;
        y += x.fastDirection().z;
    }
    System::endCycleCount(opt);

    double r = raw;
    double o = opt;
    double h = overhead;

    printf("%g %g %g\n", r-h, o-h, h);

    printf("Vector3::direction():               %d cycles\n", (int)((r-h)/(n*3.0)));
    printf("Vector3::fastDirection():           %d cycles\n", (int)((o-h)/(n*3.0)));

}


void testColor3uint8Array() {
    printf("Array<Color3uint8>\n");
    Array<Color3uint8> x;
    x.resize(2);

    debugAssert(sizeof(Color3uint8) == 3);
    x[0].r = 60;
    x[0].g = 61;
    x[0].b = 62;
    x[1].r = 63;
    x[1].g = 64;
    x[1].b = 65;

    uint8* y = (uint8*)x.getCArray();
    (void)y;
    debugAssert(y[0] == 60);
    debugAssert(y[1] == 61);
    debugAssert(y[2] == 62);
    debugAssert(y[3] == 63);
    debugAssert(y[4] == 64);
    debugAssert(y[5] == 65);
}



void testFloat() {
    printf("Test Float\n");
    /* changed from "nan" by ekern.  does this work on windows? */
    double x = nan();
    bool b1  = (x < 0.0);
    bool b2  = (x >= 0.0);
    bool b3  = !(b1 || b2);
    (void)b1;
    (void)b2;
    (void)b3;
    debugAssert(isNaN(nan()));
    debugAssert(! isNaN(4));
    debugAssert(! isNaN(0));
    debugAssert(! isNaN(inf()));
    debugAssert(! isNaN(-inf()));
    debugAssert(! isFinite(nan()));
    debugAssert(! isFinite(-inf()));
    debugAssert(! isFinite(inf()));
    debugAssert(isFinite(0.0f));
		    
}

void testglFormatOf() {
    printf("glFormatOf\n");

    debugAssert(glFormatOf(Color3) == GL_FLOAT);
    debugAssert(glFormatOf(Color3uint8) == GL_UNSIGNED_BYTE);
    debugAssert(glFormatOf(Vector3int16) == GL_SHORT);
    debugAssert(glFormatOf(float) == GL_FLOAT);
    debugAssert(glFormatOf(int16) == GL_SHORT);
    debugAssert(glFormatOf(int32) == GL_INT);

    debugAssert(sizeOfGLFormat(GL_FLOAT) == 4);
}



void testSwizzle() {
    Vector4 v1(1,2,3,4);
    Vector2 v2;

    v2 = v1.xy() + v1.yz();
}


void testSphere() {
    printf("Sphere...");
    Sphere a(Vector3(0,3,0), 2);
    Sphere b(Vector3(0,2,0), 0.5f);

    debugAssert(a.contains(b));
    debugAssert(! b.contains(a));

    Sphere s = a;
    s.merge(b);
    debugAssert(s == a);

    Sphere c(Vector3(1,0,0), 2);
    s = a;
    s.merge(c);
    debugAssert(s.contains(a));
    debugAssert(s.contains(c));

    printf("passed\n");
}



void testCoordinateFrame() {
    printf("CoordinateFrame ");

    {
        // Easy case
        CoordinateFrame c;
        c.lookAt(Vector3(-1, 0, -1));
        float h = c.getHeading();
        debugAssert(fuzzyEq(h, G3D::pi() / 4));
    }

    // Test getHeading at a variety of angles
    for (int i = -175; i <= 175; i += 5) {
        CoordinateFrame c;
        float h = c.getHeading();
        debugAssert(h == 0);

        c.rotation = Matrix3::fromAxisAngle(Vector3::unitY(), toRadians(i));

        h = c.getHeading();
        debugAssert(fuzzyEq(h, toRadians(i)));
    }

    printf("passed\n");
}

void measureRDPushPopPerformance(RenderDevice* rd) {
    uint64 identityCycles;

    int N = 500;
    rd->pushState();
    rd->popState();

    System::beginCycleCount(identityCycles);
    for (int i = 0; i < N; ++i) {
        rd->pushState();
        rd->popState();
    }
    System::endCycleCount(identityCycles);

    printf("RenderDevice::push+pop:             %g cycles\n", identityCycles / (double)N);
}

void testGLight() {
    GLight L = GLight::point(Vector3(1,2,3), Color3::white(), 1,0,0);
    Sphere s;
    
    s = L.effectSphere();
    debugAssert(s.contains(Vector3(1,2,3)));
    debugAssert(s.contains(Vector3(0,0,0)));
    debugAssert(s.contains(Vector3(100,100,100)));

    {
        GLight L = GLight::point(Vector3(1,2,3), Color3::white(), 1,0,1);
        Sphere s;
        
        s = L.effectSphere();
        debugAssert(s.contains(Vector3(1,2,3)));
        debugAssert(s.contains(Vector3(1,1,3)));
        debugAssert(! s.contains(Vector3(100,100,100)));
    }
}


void testLineSegment2D() {
    LineSegment2D A = LineSegment2D::fromTwoPoints(Vector2(1,1), Vector2(2,2));
    LineSegment2D B = LineSegment2D::fromTwoPoints(Vector2(2,1), Vector2(1,2));
    LineSegment2D C = LineSegment2D::fromTwoPoints(Vector2(2,1), Vector2(3,-1));
    LineSegment2D D = LineSegment2D::fromTwoPoints(Vector2(1,1.2f), Vector2(2,1.2f));

    Vector2 i0 = A.intersection(B);
    debugAssert(i0.fuzzyEq(Vector2(1.5f, 1.5f)));

    Vector2 i1 = A.intersection(C);
    debugAssert(i1 == Vector2::inf());

    Vector2 i2 = D.intersection(A);
    debugAssert(i2.fuzzyEq(Vector2(1.2f, 1.2f)));
}

int main(int argc, char* argv[]) {


    (void)argc;
    (void)argv;

#   ifdef G3D_WIN32
    {
        // Change to the executable directory
        chdir(filenamePath(argv[0]).c_str());
    }
#   endif


    char x[2000];
    getcwd(x, sizeof(x));
    
	debugAssertM(FileSystem::exists("apiTest.zip", false), 
        format("Tests are being run from the wrong directory.  cwd = %s", x));

    RenderDevice* renderDevice = NULL;

    std::string s;
    System::describeSystem(s);
    printf("%s\n", s.c_str());

    NetworkDevice::instance()->describeSystem(s);
    printf("%s\n", s.c_str());


#    ifndef _DEBUG
        printf("Performance analysis:\n\n");

        perfKDTree();

        // Pause so that we can see the values in the debugger
//	        getch();

        perfCollisionDetection();

        perfArray();

        perfTable();

        printf("%s\n", System::mallocPerformance().c_str());

        perfQueue();

        perfMatrix3();

        perfTextOutput();

        perfSystemMemcpy();

        perfBinaryIO();

        perfPointHashGrid();


        measureMemsetPerformance();
        measureNormalizationPerformance();

        OSWindow::Settings settings;
        settings.width = 800;
        settings.height = 600;
        settings.alphaBits = 0;
        settings.rgbBits = 8;
        settings.stencilBits = 0;
        settings.msaaSamples = 1;

        if (!renderDevice) {
            renderDevice = new RenderDevice();
        }
    
        renderDevice->init(settings);

        if (renderDevice) {
            renderDevice->describeSystem(s);
            printf("%s\n", s.c_str());
        }

        measureRDPushPopPerformance(renderDevice);

#       ifdef G3D_WIN32
            // Pause so that we can see the values in the debugger
//	        getch();
#       endif

#   else

    printf("\n\nTests:\n\n");

    testAny();

    testFileSystem();

    testCollisionDetection();  

    testTextInput();
    testTextInput2();
    printf("  passed\n");


    testSphere();

    testImageConvert();

    testKDTree();

    testMatrix();

    testLineSegment2D();

    testGLight();

    testZip();

    testMap2D();

    testfilter();

    testArray();

    testSmallArray();

    testSpline();

    testMatrix3();
    testMatrix4();

    testTable();

    testTableTable();
  

    testCoordinateFrame();

    testReliableConduit(NetworkDevice::instance());

    testQuat();

    testReferenceCount();

    testAtomicInt32();

    testGThread();
    
    testWeakCache();
    
    testSystemMemset();

    testSystemMemcpy();

    testuint128();

    testQueue();

    testMeshAlgTangentSpace();

    testConvexPolygon2D();

    testPlane();
    printf("  passed\n");

    testAABox();

    testRandom();
    printf("  passed\n");

    testAABoxCollision();
    printf("  passed\n");
    testAdjacency();
    printf("  passed\n");
    testWildcards();
    printf("  passed\n");

    testFloat();
    printf("  passed\n");
    
    testRandom();


    testBox();
    printf("  passed\n");

    testBox2D();
    printf("  passed\n");

    testColor3uint8Array();
    printf("  passed\n");
    testglFormatOf();
    printf("  passed\n");
    testSwizzle();

    testGCamera();

    testCallback();

    testPointHashGrid();

    testBinaryIO();

#   ifdef RUN_SLOW_TESTS
        testHugeBinaryIO();
        printf("  passed\n");
#   endif

    printf("%s\n", System::mallocPerformance().c_str());
    System::resetMallocPerformanceCounters();

    printf("\nAll tests succeeded.\n");
#endif

    if (renderDevice) {
        renderDevice->cleanup();
        delete renderDevice;
    }
    
    NetworkDevice::cleanup();

    return 0;
}

