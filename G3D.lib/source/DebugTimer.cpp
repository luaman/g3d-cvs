
#include "G3D/platform.h"
#include "G3D/DebugTimer.h"
#include "G3D/System.h"

namespace G3D {

DebugTimer::DebugTimer(const std::string& myName) : myName(myName) {
    reset();
}

void DebugTimer::reset() {
    prevTime = startTime = System::time();
    prevMark = "Start";
}


void DebugTimer::after(const std::string& s) {
    RealTime now = System::time();
    debugPrintf("%s: %10s - %8fs since %s (%fs since start)\n",
           myName.c_str(),
           s.c_str(),
           now - prevTime,
           prevMark.c_str(),
           now - startTime);
    prevTime = now;
    prevMark = s;
}

}
