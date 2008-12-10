#include "G3D/G3DAll.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;
#include <string>

static int      sThreadedValue = 0;
static GMutex   sThreadedMutex;

static void incThreadedValue() {
    GMutexLock lock(&sThreadedMutex);
    ++sThreadedValue;

//    debugAssert(sThreadedMutex.tryLock());
}

static void threadProc(void* param) {
    ++sThreadedValue;
}

static void lockProc(void* param) {
    debugAssert(sThreadedMutex.tryLock() == false);
    ++sThreadedValue;
}

void testGThread() {
    printf("G3D::GThread ");

    {
        GThreadRef gthread = GThread::create("GThread", threadProc, NULL);

        bool started = gthread->start();
        debugAssert(started);

        gthread->waitForCompletion();
        debugAssert(gthread->completed());

        debugAssert(sThreadedValue == 1);

        incThreadedValue();
        debugAssert(sThreadedValue == 2);

//        sThreadedMutex.lock();
//        gthread = GThread::create("GMutex", lockProc, NULL);
//        gthread->start();
//        while (!gthread->running() && !gthread->completed()) {
//            System::sleep(0.01);
//        }
//        sThreadedMutex.unlock();
    }

    printf("passed\n");
}

