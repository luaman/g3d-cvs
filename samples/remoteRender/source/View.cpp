#include "View.h"

static float TURN_INCREMENT = 30.0f * units::degrees();
static float MOVE_INCREMENT = 4.0f  * units::meters();

View::View(const Vector3& p, float h) : position(p), heading(h) {}


View::View(const std::string& base) : heading(-4.188792f) {
    try {
        parse(base);
    } catch (...) {
        // If something goes wrong during parsing ignore the error
    }
}


void View::parse(const std::string& s) {
    Any any;
    any.parse(s);
    position = any.get("position", Vector3::zero());
    heading = any.get("heading", 0.0f);
}

    
std::string View::unparse() const {
    return format("View{position=Vector3(%f,%f,%f),heading=%f}", position.x, position.y, position.z, heading);
}


CFrame View::toCFrame() const {
    return CFrame::fromXYZYPRRadians(position.x, position.y, position.z, heading);
}


View View::left(BSPMapRef map) const {
    return View(position, heading + TURN_INCREMENT);
}


View View::right(BSPMapRef map) const {
    return View(position, heading - TURN_INCREMENT);
}


View View::forward(BSPMapRef map) const {
    Vector3 delta = -Vector3(sin(heading), 0, cos(heading)) * MOVE_INCREMENT;
    if (true || map.isNull()) {
        return View(position + delta, heading);
    } else {
        View after(position, heading);
        int N = 10;
        delta /= float(N);
        for (int i = 0; i < N; ++i) {
            map->slideCollision(after.position, delta, Vector3(1.0f, 1.0f, 1.0f));
        }
        return after;
    }
}
