/** 
  @file Pointer.h
 
  @maintainer Morgan McGuire, graphics3d.com
 
  @created 2007-05-16
  @edited  2007-05-16

  Copyright 2000-2007, Morgan McGuire.
  All rights reserved.
 */
#ifndef G3D_POINTER_H
#define G3D_POINTER_H

#include "G3D/debugAssert.h"

namespace G3D {

/**
   Acts like a pointer to a value of type ValueType (i.e.,
   ValueType*), but can operate through accessor methods as well as on
   a value in memory.  This is useful for implementing scripting
   languages and other applications that need to connect existing APIs
   by reference.

   <pre>
   class Foo {
   public:
      void setEnabled(bool b);
      bool getEnabled() const;
   };

   Foo  f;
   bool b;
   
   Pointer<bool> p1(&b);
   Pointer<bool> p2(&f, &Foo::getEnabled, &Foo::setEnabled);

   *p1 = true;
   *p2 = false;
   *p2 = *p1; \/\/ Value assignment
   p2 = p1; \/\/ Pointer aliasing

   \/\/ Or, equivalently:
   p1.setValue(true);
   p2.setValue(false);

   p2.setValue(p1.getValue());
   p2 = p1;
   </pre>

   Note: Because of the way that dereference is implemented, you cannot pass <code>*p</code> through a function
   that takes varargs (...), e.g., <code>printf("%d", *p)</code> will produce a compile-time error.  Instead use
   <code>printf("%d",(bool)*p)</code> or <code>printf("%d", p.getValue())</code>.
   
 */
template<class ValueType>
class Pointer {
private:

    class Interface {
    public:
        virtual ~Interface() {};
        virtual void set(ValueType b) = 0;
        virtual ValueType get() const = 0;
        virtual Interface* clone() const = 0;
    };

    class Memory : public Interface {
    private:

        ValueType* value;

    public:
        
        Memory(ValueType* value) : value(value) {
            debugAssert(value != NULL);
        }

        virtual void set(ValueType v) {
            *value = v;
        }

        virtual ValueType get() const {
            return *value;
        }

        virtual Interface* clone() const {
            return new Memory(value);
        }
    };

    template<class T>
    class Accessor : public Interface {
    public:
        typedef ValueType (T::*GetMethod)() const;
        typedef void (T::*SetMethod)(ValueType);

    private:

        T*          object;
        GetMethod   getMethod;
        SetMethod   setMethod;

    public:
        
        Accessor(T* object, GetMethod getMethod, SetMethod setMethod) : object(object), getMethod(getMethod), setMethod(setMethod) {
            debugAssert(object != NULL);
        }

        virtual void set(ValueType v) {
            (object->*setMethod)(v);
        }

        virtual ValueType get() const {
            return (object->*getMethod)();
        }

        virtual Interface* clone() const {
            return new Accessor(object, getMethod, setMethod);
        }
    };

    Interface* interface;

public:

    Pointer() : interface(NULL) {};

    explicit Pointer(ValueType* v) : interface(new Memory(v)) {}

    inline Pointer& operator=(const Pointer& r) {
        delete interface;
        if (r.interface != NULL) {
            interface = r.interface->clone();
        } else {
            interface = NULL;
        }
        return this[0];
    }

    Pointer(const Pointer& p) : interface(NULL) {
        this[0] = p;
    }

    template<class Class>
    Pointer(Class* object,
          bool (Class::*getMethod)() const,
          void (Class::*setMethod)(bool)) : interface(new Accessor<Class>(object, getMethod, setMethod)) {}

    ~Pointer() {
        delete interface;
    }

    inline ValueType getValue() const {
        debugAssert(interface != NULL);
        return interface->get();
    }

    inline void setValue(ValueType v) {
        debugAssert(interface != NULL);
        interface->set(v);
    }

    class IndirectValue {
    private:

        friend class Pointer;
        Pointer* pointer;
        IndirectValue(Pointer* p) : pointer(p) {}

    public:

        void operator=(const ValueType& v) {
            pointer->setValue(v);
        }

        operator ValueType() const {
            return pointer->getValue();
        }

    };

    inline IndirectValue operator*() {
        return IndirectValue(this);
    }

    inline ValueType operator*() const {
        return getValue();
    }
};

}

#endif