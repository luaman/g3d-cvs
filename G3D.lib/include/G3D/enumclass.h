/**
  @file G3D/enumclass.h

  @maintainer Morgan McGuire, morgan@graphics3d.com
  @created 2007-01-27
  @edited  2007-01-27
*/
#ifndef G3D_ENUMCLASS_H
#define G3D_ENUMCLASS_H

/**
  Creates a series of methods that turn a class into a scoped enumeration.
  Uses the "Intelligent Enum" design pattern 
  http://www.codeguru.com/cpp/cpp/cpp_mfc/article.php/c4001/

  See GLG3D/GKey.h for an example.
 */
#define G3D_DECLARE_ENUM_CLASS_METHODS(Classname)\
    inline Classname(char v) : value((Value)v) {}\
\
    inline Classname() : value((Value)0) {}\
\
    inline Classname(const Value v) : value(v) {}\
\
    explicit inline Classname(int v) : value((Value)v) {}\
\
    /** Support cast back to the Value type, which is needed to allow implicit assignment inside unions. */\
    inline operator Value() const {\
        return value;\
    }\
\
    inline operator int() const {\
        return (int)value;\
    }\
\
    inline bool operator== (const Classname other) const {\
        return value == other.value;\
    }\
\
    inline bool operator!= (const Classname other) const {\
        return value != other.value;\
    }\
\
    inline bool operator< (const Classname other) const {\
        return value < other.value;\
    }\
\
    inline bool operator> (const Classname other) const {\
        return value > other.value;\
    }\
\
    inline bool operator>= (const Classname other) const {\
        return value >= other.value;\
    }\
\
    inline bool operator<= (const Classname other) const {\
        return value <= other.value;\
    }\
\
    inline bool operator< (const Value other) const {\
        return value < other;\
    }\
\
    inline bool operator> (const Value other) const {\
        return value > other;\
    }\
\
    inline bool operator<= (const Value other) const {\
        return value <= other;\
    }\
\
    inline bool operator>= (const Value other) const {\
        return value >= other;\
    }\
\
    inline Classname& operator-- () {\
        value = (Value)((int)value - 1);\
        return *this;\
    }\
\
    inline Classname& operator++ () {\
        value = (Value)((int)value + 1);\
        return *this;\
    }\
\
    inline Classname& operator+= (const int x) {\
        value = (Value)((int)value + x);\
        return *this;\
    }\
\
    inline Classname& operator-= (const int x) {\
        value = (Value)((int)value - x);\
        return *this;\
    }\
\
    inline Classname operator+ (const int x) const {\
        return Classname((int)value + x);\
    }\
\
    inline Classname operator- (const int x) const {\
        return Classname((int)value - x);\
    }
#endif
