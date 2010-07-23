#ifndef View_h
#define View_h

#include <G3D/G3DAll.h>

class View {
public:
    Vector3 position;
    float   heading;
    
    View(const Vector3& p = Vector3::zero(), float h = -4.188792f);

    /** The string must have the same format as the output of parse() */
    View(const std::string& base);

    void parse(const std::string& s);
    
    std::string unparse() const;

    CFrame toCFrame() const;

    View left(BSPMapRef map) const;

    View right(BSPMapRef map) const;

    View forward(BSPMapRef map) const;
};

#endif
