#if 0
#include "G3D/Any.h"
#include "G3D/TextOutput.h"

namespace G3D {
   

Any::Data::Data(const std::string& x) : type(STRING), referenceCount(1) {
    value.s = new std::string(x);
}
    

Any::Data::Data(const Array<Any>& x) : type(ARRAY), referenceCount(1) {
    value.a = new Array<Any>(x);
}


Any::Data::Data(const Table<std::string, Any>& x) : type(TABLE), referenceCount(1) {
    value.t = new Table<std::string, Any>(x);
}


Any::Data::~Data() {
    alwaysAssertM(referenceCount.value() == 0, "Incorrect delete in Any::Data");

    switch (type) {
    case STRING:
        delete value.s;
        break;
    case ARRAY:
        delete value.a;
        break;
    case TABLE:
        delete value.t;
        break;
    default:
        alwaysAssertM(value.s == NULL, "Corrupt Any::Data::Value");
    }
    value.s = NULL;
}

//////////////////////////////////////////////////////////////

Any::Any() : m_type(NONE), m_data(NULL) {
}


Any::Any(TextInput& t) : m_type(NONE), m_data(NULL) {
    deserialize(t);
}


Any::Any(const Any& x) : m_type(NONE), m_data(NULL) {
    *this = x;
}


Any::Any(double x) : m_type(NUMBER), m_simpleValue(x), m_data(NULL) {
}


Any::Any(int x) : m_type(NUMBER), m_simpleValue((double)x), m_data(NULL) {
}


Any::Any(bool x) : m_type(BOOLEAN), m_simpleValue(x), m_data(NULL) {
}


Any::Any(const std::string& s) : m_type(STRING), m_data(new Data(s)) {
}


Any::Any(Type t, const std::string& name) : m_type(t), m_data(NULL) {
    alwaysAssertM(t == ARRAY || t == TABLE,
                  "Illegal type with Any(Type) constructor");

    if (name != "") {
        m_data = new Data();
        m_data->name = name;
    }
}


Any::~Any() {
    dropReference();
}


void Any::dropReference() {
    if (m_data && m_data->referenceCount.decrement() <= 0) {
        // This was the last reference to the shared data
        delete m_data;
        m_data = NULL;
    }
}



}
#endif
