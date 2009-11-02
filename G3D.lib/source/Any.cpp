/**
 @file Any.cpp
  
 @created 2006-06-11
 @edited  2009-11-09

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
 */

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

Any& Any::operator=(const Any& x) {
    dropReference();

    m_type        = x.m_type;
    m_simpleValue = x.m_simpleValue;

    if (x.m_data != 0) {
        m_data = const_cast<Data*>(x.m_data);
        m_data->referenceCount.increment();
    }
    else {
        m_data = NULL;
    }
}

void Any::dropReference() {
    if (m_data && m_data->referenceCount.decrement() <= 0) {
        // This was the last reference to the shared data
        delete m_data;
        m_data = NULL;
    }
}

void Any::checkType(Type e) const {
    if (m_type != e)
        throw WrongType(e,m_type);
}

const Any& Any::operator[](int i) const {
    checkType(ARRAY);
    Array<Any>& array = *(m_data->value.a);
    return array[i];
}


Any& Any::operator[](int i) {
    checkType(ARRAY);
    Array<Any>& array = *(m_data->value.a);
    return array[i];
}


const Array<Any>& Any::array() const {
    checkType(ARRAY);
    return *(m_data->value.a);
}


void Any::append(const Any& x0) {
    checkType(ARRAY);
    m_data->value.a->append(x0);
}


void Any::append(const Any& x0, const Any& x1) {
    append(x0);
    append(x1);
}


void Any::append(const Any& x0, const Any& x1, const Any& x2) {
    append(x0);
    append(x1);
    append(x2);
}


void Any::append(const Any& x0, const Any& x1, const Any& x2, const Any& x3) {
    append(x0);
    append(x1);
    append(x2);
    append(x3);
}


const Table<std::string, Any>& Any::table() const {
    checkType(TABLE);
    return *(m_data->value.t);
}


const Any& Any::operator[](const std::string& x) const {
    checkType(TABLE);
    const Table<std::string,Any>& table = *(m_data->value.t);
    Any* value = table.getPointer(x);
    if( value == NULL )
        throw KeyNotFound(x);
    return *value;
}


Any& Any::operator[](const std::string& x) {
    checkType(TABLE);
    Table<std::string,Any>& table = *(m_data->value.t);
    return table.getCreate(x);

/*
    try {
        Any& any = table[x];
        return any;
    } catch(...) {
        table.set(x,Any());
        Any& any = table[x];
        return any;
    }
*/
}


void Any::load(const std::string& filename) {
    TextInput::Settings settings;
    settings.cppBlockComments = false;
    settings.cppLineComments = false;
    settings.otherLineComments = true;
    settings.otherCommentCharacter = '#';
    settings.generateCommentTokens = true;
    settings.singleQuotedStrings = false;
    settings.msvcSpecials = false;
    settings.caseSensitive = false;

    TextInput ti(filename,settings);
    deserialize(ti);
}


void Any::save(const std::string& filename) const {
}


void Any::serialize(TextOutput& t) const {
}


void Any::deserialize(TextInput& ti) {
    Token token = ti.read();
    switch (token.type())
    {
    case Token::SYMBOL:
        dropReference();

        if (token.string() == "{") {
            // Become a TABLE.
            m_type = TABLE;
            m_data = new Data( Table<std::string,Any>() );

            // Create Anys until an end-of-table symbol "}" is read.
            for (token = ti.read(); token.type() != Token::END; token = ti.read()) {

                if (token.type() == Token::SYMBOL) {
                    if (token.string() == ",")
                        continue;    // Skip comma.
                    if (token.string() == "}")
                        return;    // Done creating the TABLE.
                }
                else {
                    throw CorruptText("Expected a symbol name.",token);
                }

                ti.readSymbol("=");
                operator[](token.string()).deserialize(ti);
            }
            throw CorruptText("Table ended unexpectedly.",token);
        }

        else if (token.string() == "(") {
            // Become an ARRAY.
            m_type = ARRAY;
            m_data = new Data( Array<Any>() );

            // Create Anys until an end-of-array symbol ")" is read.
            for (token = ti.read(); token.type() != Token::END; token = ti.read()) {

                if (token.type() == Token::SYMBOL) {
                    if (token.string() == ",")
                        continue;    // Skip comma.
                    if (token.string() == ")")
                        return;    // Done creating the ARRAY.
                    throw CorruptText("Unexpected symbol name.",token);
                }

                ti.push(token);
                append(Any(ti));
            }
            throw CorruptText("Array ended unexpectedly.",token);
        }

        else    // symbol name
        {
            deserialize(ti);
            if ( m_data == NULL )
                throw CorruptText("Expected a named table or a named array.",token);
            m_data->name = token.string();
            return;
        }
        return;    // Never reached.

    case Token::STRING:
        dropReference();

        // Become a STRING.
        m_type = STRING;
        m_data = new Data(token.string());

        return;

    case Token::NUMBER:
        dropReference();

        // Become a NUMBER.
        m_type = NUMBER;
        m_simpleValue.n = token.number();

        return;

    case Token::BOOLEAN:
        dropReference();

        // Become a BOOLEAN.
        m_type = BOOLEAN;
        m_simpleValue.b = token.boolean();

        return;

    case Token::COMMENT:
        // Become whatever the next expression is, then prepend this comment to that.
        deserialize(ti);
        if ( m_data == NULL )
            m_data = new Data();
        m_data->comment = token.string()+m_data->comment;

        return;

    case Token::NEWLINE:
        // NEWLINE ignored.
        return;

    default:
        throw CorruptText("Unexpected token type.",token);
    };    // switch(token.type())
}


}    // namespace G3D
