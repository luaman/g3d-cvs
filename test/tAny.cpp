/**
 @file tAny.cpp
  
 @author Shawn Yarbrough
  
 @created 2009-11-03
 @edited  2009-11-03

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
 */

#include "G3D/G3DAll.h"
#include <sstream>


static void testConstruct() {

    {
        Any x = 3;
        debugAssert(x.type() == Any::NUMBER);
    }

    {
        Any x = 3.1;
        debugAssert(x.type() == Any::NUMBER);
    }

    {
        Any x = 3.1f;
        debugAssert(x.type() == Any::NUMBER);
    }

    {
        Any x = true;
        debugAssert(x.type() == Any::BOOLEAN);
    }

    {
        Any x = "hello";
        debugAssert(x.type() == Any::STRING);
    }

    {
        Any x = std::string("hello");
        debugAssert(x.type() == Any::STRING);
    }

    {
        Any y = "hello";
        Any x = y;
        debugAssert(x.type() == Any::STRING);
    }

}

void testAny() {

    printf("G3D::Any ");

    testConstruct();

    std::stringstream errss;

    try {

        Any any, any2;

        any.load("Any-load.txt");
        any2 = any;
        if( any != any2 ) {
            any2.save("Any-failed.txt");
            throw "Two objects of class Any differ after assigning one to the other.";
        }

        any .save("Any-save.txt");
        any2.load("Any-save.txt");
        if( any != any2 ) {
            any2.save("Any-failed.txt");
            throw "Any-load.txt and Any-save.txt differ.";
        }

    } catch( const Any::WrongType& err ) {
        errss << "failed: Any::WrongType expected=" << Any::stringType(err.expected).c_str() << " actual=" << Any::stringType(err.actual).c_str();
    } catch( const Any::KeyNotFound& err ) {
        errss << "failed: Any::KeyNotFound key=" << err.key.c_str();
    } catch( const Any::IndexOutOfBounds& err ) {
        errss << "failed: Any::IndexOutOfBounds index=" << err.index << " size=" << err.size;
    } catch( const Any::CorruptText& err ) {
        errss << "failed: Any::CorruptText message=\"" << err.message.c_str() << "\" token=\"" << err.token.string().c_str() << "\" line=" << err.token.line() << " character=" << err.token.character();
    } catch( const std::exception& err ) {
        errss << "failed: std::exception \"" << err.what() << "\"\n";
    } catch( const std::string& err ) {
        errss << "failed: std::string \"" << err.c_str() << "\"\n";
    } catch( const char* err ) {
        errss << "failed: const char* \"" << err << "\"\n";
    }

    if( !errss.str().empty() )
        debugAssertM(false, errss.str());

    printf("passed\n");

};    // void testAny()
