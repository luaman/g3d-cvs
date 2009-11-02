/**
 @file Any.h
  
 @created 2006-06-11
 @edited  2009-11-09

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
 */

#ifndef G3D_Any_h
#define G3D_Any_h

#include "G3D/platform.h"
#include "G3D/Table.h"
#include "G3D/Array.h"
#include "G3D/TextInput.h"
#include "G3D/AtomicInt32.h"
#include <string>

namespace G3D {

class TextOutput;

/** 
Encodes typed, structured data and can serialize it to a human
readable format that is very similar to the Python language's data
syntax.  Well-suited for quickly creating human-readable file formats,
especially since deserialization and serialization preserves comments.

The class is designed so that copying Anys generally is fast, even if
it is a large array or table.  This is because data is shared between
copies until it is mutated, at which point an actual copy occurs.

\section Example
Sample File:
<pre>
{
   shape = "round",

   # in meters
   radius = 3.7,

   position = Vector3(1.0, -1.0, 0.0),
   texture = { format = "RGB8", size = (320, 200)}
}
</pre>

Sample code using:
<pre>
Any x;
x.load("ball.txt");
if (x["shape"].string() == "round") {
    x["density"] = 3;
}
x.save("ball.txt");
</pre>

The custom serialization format was chosen to be terse, easy for
humans to read, and easy for machines to parse. It was specifically
chosen over formats like XML, YAML, JSON, S-expressions, and Protocol
Buffers, although there is no reason you could not write readers and
writers for G3D::Any that support those.

G3D::Any assumes that structures do not contain cycles; it is an 
error to create a structure like:

<pre>
Any x(Any::ARRAY);
x.array().append(x);    // don't do this!
</pre>

although no exception wil be thrown at runtime during the append.

Serialized format BNF:

<pre>
identifier  ::= (letter | "_") (letter | digit | "_")*

comment     ::= "#" <any characters> "\n"

number      ::= <legal C printf number format>
string      ::= <legal C double-quoted string; backslashes must be escaped>
boolean     ::= "True" | "False"
none        ::= "None"
array       ::= "(" [value ("," value)*] ")"
pair        ::= identifier "=" value
table       ::= "{" [pair ("," pair)*] "}"
named-array ::= identifier tuple
named-table ::= identifier dict

value       ::= [comment] (none | number | boolean | string | array | table | named-array | named-table)
</pre>

Except for single-line comments, whitespace is not significant.

The deserializer allows the substitution of [] for () when writing
tuples.

The serializer indents four spaces for each level of nesting.  It
places newlines between array or table elements only if the entire
entry would not fit on a single line.  Tables are written with the
keys in alphabetic order.
*/
class Any {
public:

    enum Type {NONE, BOOLEAN, NUMBER, STRING, ARRAY, TABLE};

private:

    /** NONE, BOOLEAN, and NUMBER are stored directly in the Any */
    union SimpleValue {
        bool                     b;
        double                   n;
        inline SimpleValue() : n(0.0) {}
        inline SimpleValue(bool x) : b(x) {}
        inline SimpleValue(double x) : n(x) {}
    };

    class Data {
    public:
        /** ARRAY, TABLE, or STRING value only.  NULL otherwise. */
        union Value {
            std::string*             s;
            Array<Any>*              a;
            Table<std::string, Any>* t;
            inline Value() : s(NULL) {}
        };

        // Needed so that the destructor knows what is in Value
        // and can call its destructor. 
        Type                         type;
        
        Value                        value;
        
        std::string                  comment;

        std::string                  name;

        /** For STRING, ARRAY and TABLE types, m_value is shared between
            multiple instances.  Mutation is allowed only if the reference
            count is exactly 1, otherwise the mutating instance must copy
            the value.  This is not used for other types.
        */
        AtomicInt32                  referenceCount;

        /** When this was allocated solely for comments and names */
        inline Data() : type(NONE), referenceCount(1) {}
        Data(const std::string& x);
        Data(const Array<Any>& x);
        Data(const Table<std::string, Any>& x);
        ~Data();
    };

    Type        m_type;
    SimpleValue m_simpleValue;
    Data*       m_data;

    /** Decrements the reference count (if there is one).  If the
    reference count is zero or does not exist, calls delete on @a
    m_data and sets it to NULL.
    */
    void dropReference();

    /** Throws a WrongType exception if this Any is not of the
        expected type e.*/
    void checkType(Type e) const;

    /** Returns a copy of the m_value. */
    void* copyValue() const;

    /** Ensures that this has a unique reference and
        contains a valid m_data.*/
    void ensureMutable();

    void allocData();
    void freeData();

public:

    /** Base class for all Any exceptions.*/
    class Exception {
    public:
        virtual ~Exception() {}
    };

    /** Thrown when an inappropriate operation is performed (e.g., operator[] on a number) */
    class WrongType : public Exception {
    public:
        Type        expected;
        Type        actual;
        inline WrongType() : expected(NONE), actual(NONE) {}
        inline WrongType(Type e, Type a) : expected(e), actual(a) {}
    };

    /** Thrown by operator[] when a key is not present. */
    class KeyNotFound : public Exception {
    public:
        std::string key;
        inline KeyNotFound() {}
        inline KeyNotFound(const std::string& k) : key(k) {}
    };

    class IndexOutOfBounds : public Exception {
    public:
        int     index;
        int     size;
        inline IndexOutOfBounds() : index(0), size(0) {}
        inline IndexOutOfBounds(int i, int s) : index(i), size(s) {}
    };

    /** Thrown when deserialize() when the input is incorrectly formatted. */
    class CorruptText : public Exception {
    public:
        std::string      message;

        /** Token where the problem occurred.*/
        Token            token;

        inline CorruptText() {}
        inline CorruptText(const std::string& s, const Token& t) : message(s), token(t) {}
    };

    /** NONE constructor */
    Any();

    /** Deserialize */
    explicit Any(TextInput& t);

    Any(const Any& x);

    /** NUMBER constructor */
    Any(double x);

    /** NUMBER constructor */
    Any(int x);

    /** BOOLEAN constructor */
    Any(bool x);

    /** STRING constructor */
    Any(const std::string& x);

    /** \a t must be ARRAY or TABLE */
    Any(Type t, const std::string& name = "");
    
    ~Any();

    /** Removes the comment and name */
    Any& operator=(const Any& x);

    /** Removes the comment and name */
    Any& operator=(double x);

    /** Removes the comment and name */
    Any& operator=(int x);

    /** Removes the comment and name */
    Any& operator=(bool x);

    /** Removes the comment and name */
    Any& operator=(const std::string& x);
    /** \a t must be ARRAY, TABLE, or NONE. Removes the comment and name */
    Any& operator=(Type t);

    Type type() const;
    
    /** Comments appear before values when they are in serialized form.*/
    const std::string& comment() const;
    void setComment(const std::string& c);

    /** True if this is the NONE value */
    bool isNone() const;

    /** Throws a WrongType exception if this is not a number */
    double number() const;
    double number(double defaultVal) const;
    const std::string& string() const;
    const std::string& string(const std::string& defaultVal) const;
    bool boolean() const;
    bool boolean(bool defaultVal) const;

    /** If this is named ARRAY or TABLE, returns the name. */
    const std::string& name() const;
    /** Only legal for ARRAY or TABLE */
    void setName(const std::string& n);

    /** Number of elements if this is an ARRAY or TABLE */
    int length() const;

    /** For an array, returns the ith element */
    const Any& operator[](int i) const;
    Any& operator[](int i);

    /** Directly exposes the underlying data structure for an ARRAY. */
    const Array<Any>& array() const;
    void append(const Any& x0);
    void append(const Any& x0, const Any& x1);
    void append(const Any& x0, const Any& x1, const Any& x2);
    void append(const Any& x0, const Any& x1, const Any& x2, const Any& x3);

    /** Directly exposes the underlying data structure for table.*/
    const Table<std::string, Any>& table() const;

    /** For a table, returns the element for key x. Throws KeyNotFound exception if the element does not exist. */ 
    const Any& operator[](const std::string& x) const;
    
    /** For a table, returns the element for key x, creating it if it does not exist. */
    Any& operator[](const std::string& x);

    /** For a table, returns the element for key \a x and \a defaultVal if it does not exist. */
    const Any& get(const std::string& x, const Any& defaultVal) const;

    /** True if the Anys are exactly equal, ignoring comments.  Applies deeply on arrays and tables. */
    bool operator==(const Any& x) const;
    bool operator!=(const Any& x) const;

    /** Uses the deserialize method */
    void load(const std::string& filename);

    /** Uses the serialize method */
    void save(const std::string& filename) const;

    void serialize(TextOutput& t) const;
    void deserialize(TextInput& t);
};    // class Any

}    // namespace G3D

#endif
