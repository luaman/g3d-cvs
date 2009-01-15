#ifndef G3D_DebugTimer_h
#define G3D_DebugTimer_h

#include <string>

namespace G3D {

/** For crude profiling */
class DebugTimer {
private:
    std::string  myName;
    double       startTime;
    std::string  prevMark;
    double       prevTime;

public:

    DebugTimer(const std::string& myName = "Timer");

    void reset();

    /** Call after an operation has completed, with the name of the operation */
    void after(const std::string& s = "");
};

}

#endif