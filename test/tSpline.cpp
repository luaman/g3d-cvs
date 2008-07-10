#include <G3D/G3DAll.h>

// unit-interval spline
static void unitTests() {
    Spline<float> spline;

    spline.append(0.0, 5.0);
    spline.append(1.0, 10.0);
    spline.cyclic = false;

    float d = spline.duration();
    debugAssert(fuzzyEq(d, 1.0));
    debugAssert(spline.size() == 2);
    
    int i;
    float u;

    spline.computeIndex(0, i, u);
    debugAssert(i == 0);
    debugAssert(u == 0);

    spline.computeIndex(0.5, i, u);
    debugAssert(i == 0);
    debugAssert(fuzzyEq(u, 0.5));

    spline.computeIndex(1, i, u);
    debugAssert(i == 1);
    debugAssert(u == 0);

    spline.computeIndex(-1, i, u);
    debugAssert(i == -1);
    debugAssert(u == 0);

    spline.computeIndex(-0.5, i, u);
    debugAssert(i == -1);
    debugAssert(fuzzyEq(u, 0.5));

    // Cyclic tests
    spline.cyclic = true;

    spline.computeIndex(0, i, u);
    debugAssert(i == 0);
    debugAssert(u == 0);

    spline.computeIndex(0.5, i, u);
    debugAssert(i == 0);
    debugAssert(fuzzyEq(u, 0.5));

    spline.computeIndex(1, i, u);
    debugAssert(i == 1);
    debugAssert(u == 0);

    spline.computeIndex(2, i, u);
    debugAssert(i == 2);
    debugAssert(u == 0);

    spline.computeIndex(1.5, i, u);
    debugAssert(i == 1);
    debugAssert(u == 0.5);

    spline.computeIndex(-1, i, u);
    debugAssert(i == -1);
    debugAssert(u == 0);

    spline.computeIndex(-0.5, i, u);
    debugAssert(i == -1);
    debugAssert(fuzzyEq(u, 0.5));
}


static void nonunitTests() {
    Spline<float> spline;

    spline.append(1.0, 5.0);
    spline.append(3.0, 10.0);
    spline.cyclic = false;

    float d = spline.duration();
    debugAssert(fuzzyEq(d, 2.0));
    debugAssert(spline.size() == 2);
    
    int i;
    float u;

    spline.computeIndex(1, i, u);
    debugAssert(i == 0);
    debugAssert(u == 0);

    spline.computeIndex(2, i, u);
    debugAssert(i == 0);
    debugAssert(fuzzyEq(u, 0.5));

    spline.computeIndex(3, i, u);
    debugAssert(i == 1);
    debugAssert(u == 0);

    spline.computeIndex(-1, i, u);
    debugAssert(i == -1);
    debugAssert(u == 0);

    spline.computeIndex(0, i, u);
    debugAssert(i == -1);
    debugAssert(fuzzyEq(u, 0.5));

    // Cyclic case
    spline.cyclic = true;
    spline.computeIndex(1, i, u);
    debugAssert(i == 0);
    debugAssert(u == 0);

    spline.computeIndex(2, i, u);
    debugAssert(i == 0);
    debugAssert(fuzzyEq(u, 0.5));

    spline.computeIndex(3, i, u);
    debugAssert(i == 1);
    debugAssert(u == 0);

    float fi = spline.getFinalInterval();
    debugAssert(fi == 2);

    spline.computeIndex(-1, i, u);
    debugAssert(i == -1);
    debugAssert(u == 0);

    spline.computeIndex(0, i, u);
    debugAssert(i == -1);
    debugAssert(fuzzyEq(u, 0.5));
}

// Hard case: irregular intervals, cyclic spline
static void irregularTests() {
    Spline<float> spline;
    spline.cyclic = true;
    spline.append(1.0, 1.0);
    spline.append(2.0, 1.0);
    spline.append(4.0, 1.0);

    float fi = spline.getFinalInterval();
    debugAssert(fuzzyEq(fi, 1.5f));

    debugAssert(fuzzyEq(spline.duration(), 4.5f));

    float u;
    int i;

    spline.computeIndex(1.0, i, u);
    debugAssert(i == 0);
    debugAssert(fuzzyEq(u, 0));

    spline.computeIndex(2.0, i, u);
    debugAssert(i == 1);
    debugAssert(fuzzyEq(u, 0));

    spline.computeIndex(4.0, i, u);
    debugAssert(i == 2);
    debugAssert(fuzzyEq(u, 0));

    spline.computeIndex(5.5, i, u);
    debugAssert(i == 3);
    debugAssert(fuzzyEq(u, 0));

    spline.computeIndex(-0.5, i, u);
    debugAssert(i == -1);
    debugAssert(fuzzyEq(u, 0));

    spline.computeIndex(0.25, i, u);
    debugAssert(i == -1);
    debugAssert(fuzzyEq(u, 0.5));

}

static void linearTest() {
    Spline<float> spline;

    spline.append(0.0, 0.0);
    spline.append(1.0, 1.0);
    spline.cyclic = false;

    // Points on the line y=x
    int N = 11;
    for (int i = 0; i < N; ++i) {
        float t = i / (N - 1.0f);
        
        float v = spline.evaluate(t);
        
        debugAssert(fuzzyEq(v, t));
    }

    // points on the line y = 1
    spline.control[0] = 1.0;
    spline.control[1] = 1.0;

    for (int i = 0; i < N; ++i) {
        float t = i / (N - 1.0f);
        
        float v = spline.evaluate(t);
        
        debugAssert(fuzzyEq(v, 1.0));
    }


    spline.time[0] = 0.0;
    spline.time[1] = 0.5;

    for (int i = 0; i < N; ++i) {
        float t = i / (N - 1.0f);
        
        float v = spline.evaluate(t);
        
        debugAssert(fuzzyEq(v, 1.0));
    }
}


static void curveTest() {
    Spline<float> spline;
    spline.cyclic = false;

    spline.append(0, 0);
    spline.append(0.25, 0);
    spline.append(1.0, 1.0);

    float t, v;
    t = 1.0;    
    v = spline.evaluate(t);
    debugAssert(fuzzyEq(v, 1.0));

    t = 1.5;
    v = spline.evaluate(t);
//    debugAssert(fuzzyEq(v, 1.66667));
}

void testSpline() {
    printf("Spline ");
    // Control point testing
    unitTests();
    nonunitTests();
    irregularTests();

    // Evaluate testing
    linearTest();
  
    curveTest();
    printf("passed\n");
}
