#include "Hit.h"

Hit::Hit() : position(Vector3::inf()), material(NULL) {
}


void Hit::setFromIntersector(const Tri::Intersector& intersector) {
    if (intersector.tri != NULL) {
        intersector.getResult(position, normal, texCoord);
        material = intersector.tri->material();
    } else {
        position = Vector3::inf();
    }
}
