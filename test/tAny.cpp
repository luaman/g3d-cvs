/**
 @file tAny.cpp
  
 @author Shawn Yarbrough
 @author Morgan McGuire  

 @created 2009-11-03
 @edited  2009-12-06

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
 */

#include "G3D/G3DAll.h"
#include <sstream>

static void testRefCount1() {

    // Explicit allocation so that we can control destruction
    Any* a = new Any(Any::ARRAY);

    Any* b = new Any();
    // Create alias
    *b = *a;

    // a->m_data->referenceCount.m_value should now be 2
    
    delete b;
    b = NULL;

    // Reference count should now be 1

    delete a;
    a = NULL;

    // Reference count should now be zero (and the object deallocated)
}

static void testRefCount2() {

    // Explicit allocation so that we can control destruction
    Any* a = new Any(Any::TABLE);

    // Put something complex the table, so that we have chains of dependencies
    a->set("x", Any(Any::TABLE));

    Any* b = new Any();
    // Create alias
    *b = *a;

    // a->m_data->referenceCount.m_value should now be 2
    
    delete b;
    b = NULL;

    // Reference count should now be 1

    delete a;
    a = NULL;

    // Reference count should now be zero (and the object deallocated)
}

static void testConstruct() {

    {
        Any x = char(3);
        debugAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x = short(3);
        debugAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x = int(3);
        debugAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x = long(3);
        debugAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x = int32(3);
        debugAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x = int64(3);
        debugAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x = 3.1;
        debugAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x = 3.1f;
        debugAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x = NULL;
        debugAssertM(x.type() == Any::NUMBER, Any::toString(x.type())+" when expecting NUMBER");
    }

    {
        Any x = true;
        debugAssertM(x.type() == Any::BOOLEAN, Any::toString(x.type())+" when expecting BOOLEAN");
    }

    {
        Any x = "hello";
        debugAssertM(x.type() == Any::STRING, Any::toString(x.type())+" when expecting STRING");
    }

    {
        Any x = std::string("hello");
        debugAssertM(x.type() == Any::STRING, Any::toString(x.type())+" when expecting STRING");
    }

    {
        Any y = "hello";
        Any x = y;
        debugAssertM(x.type() == Any::STRING, Any::toString(x.type())+" when expecting STRING");
    }

}

static void testCast() {
    {
        Any a = 3;
        int x = int(a.number());
        debugAssert(x == 3);
    }
    {
        Any a = 3;
        int x = a;
        debugAssert(x == 3);
    }
    {
        Any a = 3.1;
        double x = a;
        debugAssert(x == 3.1);
    }
    {
        Any a = 3.1f;
        float x = a;
        debugAssert(fuzzyEq(x, 3.1f));
    }
    {
        Any a = true;
        bool x = a;
        debugAssert(x == true);
    }
    {
        Any a = "hello";
        std::string x = a;
        debugAssert(x == "hello");
    }
}


static void testPlaceholder() {
    Any t(Any::TABLE);
    
    debugAssert(! t.containsKey("hello"));

    // Verify exceptions
    try {
        Any t(Any::TABLE);
        Any a = t["hello"]; 
        debugAssertM(false, "Placeholder failed to throw KeyNotFound exception.");
    } catch (const Any::KeyNotFound& e) {
        // Supposed to be thrown
        (void)e;
    } catch (...) {
        debugAssertM(false, "Threw wrong exception.");
    }

    try {
        Any t(Any::TABLE);
        t["hello"].number();
        debugAssert(false);
    } catch (const Any::KeyNotFound& e) {
    (void)e;}

    try {
        Any t(Any::TABLE);
        Any& a = t["hello"];
    } catch (const Any::KeyNotFound& e) { 
        debugAssert(false);
        (void)e;
    }

    try {
        Any t(Any::TABLE);
        t["hello"] = 3;
    } catch (const Any::KeyNotFound& e) { 
        debugAssert(false);
        (void)e;
    }
}


static void testParse() {
    const std::string& src =  
    "{\n\
       val0 = (1),\n\
       \n\
       // Comment 1\n\
       val1 = 3,\n\
       \
       // Comment 2\n\
       // Comment 3\n\
       val2 = true\n\
    }";

    Any a;
    a.parse(src);
    debugAssert(a.type() == Any::TABLE);
    debugAssert(a.size() == 3);

    Any val1 = a["val1"];
    debugAssert(val1.type() == Any::NUMBER);
    debugAssert(val1.number() == 3);
    debugAssert(val1.comment() == "Comment 1");
}

void testAny() {

    printf("G3D::Any ");

    testRefCount1();
    testRefCount2();
    testParse();
    testConstruct();
    testCast();
    testPlaceholder();

    std::stringstream errss;

    try {

        Any any, any2;

        any.load("Any-load.txt");
        any2 = any;
        if (any != any2) {
            any2.save("Any-failed.txt");
            throw "Two objects of class Any differ after assigning one to the other.";
        }

        any .save("Any-save.txt");
        any2.load("Any-save.txt");
        if (any != any2) {
            any2.save("Any-failed.txt");
            throw "Any-load.txt and Any-save.txt differ.";
        }

        // Trigger the destructors explicitly to help test reference counting
        any = Any();
        any2 = Any();

    } catch( const Any::KeyNotFound& err ) {
        errss << "failed: Any::KeyNotFound key=" << err.key.c_str();
    } catch( const ParseError& err ) {
//      (void)err;
        errss << "failed: ParseError: \"" << err.message << "\" " << err.filename << " line " << err.line << ":" << err.character;
    } catch( const Any::IndexOutOfBounds& err ) {
        errss << "failed: Any::IndexOutOfBounds index=" << err.index << " size=" << err.size;
    } catch( const std::exception& err ) {
        errss << "failed: std::exception \"" << err.what() << "\"\n";
    } catch( const std::string& err ) {
        errss << "failed: std::string \"" << err << "\"\n";
    } catch( const char* err ) {
        errss << "failed: const char* \"" << err << "\"\n";
    }

    if (! errss.str().empty()) {
        debugAssertM(false, errss.str());
    }

    printf("passed\n");

};    // void testAny()
