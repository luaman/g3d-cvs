/**
 @file Any.cpp

 @author Morgan McGuire
 @author Shawn Yarbrough
  
 @created 2006-06-11
 @edited  2009-11-03

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
 */

#include "G3D/Any.h"
#include "G3D/TextOutput.h"
#include <deque>

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


Any::Data::Data(const Data &peer)    // Mutate.
  : type(peer.type),
    comment(peer.comment),
    name(peer.name),
    referenceCount(1)
{
    switch (type) {
    case STRING: value.s = new std::string(*peer.value.s);             break;
    case ARRAY:  value.a = new Array<Any>(*peer.value.a);              break;
    case TABLE:  value.t = new Table<std::string, Any>(*peer.value.t); break;
    default:                                                           throw WrongType(NONE,type);
    }
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


//void* Any::copyValue() const {
//    
//}


void Any::ensureMutable() {
    if (m_data && m_data->referenceCount.value() > 0) {
        Data* d = new Data(*m_data);
        m_data->referenceCount.decrement();
        m_data = d;
    }
}


//void Any::allocData() {
//}


//void Any::freeData() {
//}


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


Any::Any(const char* s) : m_type(STRING), m_data(new Data(std::string(s))) {
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
    if (this == &x)
        return *this;

    dropReference();

    m_type        = x.m_type;
    m_simpleValue = x.m_simpleValue;

    if (x.m_data != NULL) {
        m_data = const_cast<Data*>(x.m_data);
        m_data->referenceCount.increment();
    }
    else {
        m_data = NULL;
    }

    return (*this);
}


Any& Any::operator=(double x) {
    return (*this = Any(x));
}


Any& Any::operator=(int x) {
    return (*this = Any(x));
}


Any& Any::operator=(bool x) {
    return (*this = Any(x));
}


Any& Any::operator=(const std::string& x) {
    return (*this = Any(x));
}


//Any& Any::operator=(Type t) {
//}


Any::Type Any::type() const {
    return m_type;
}


const std::string& Any::comment() const {
    static const std::string blank;
    if (m_data != NULL)
        return m_data->comment;
    else
        return blank;
}


void Any::setComment(const std::string& c) {
    if (m_data == NULL)
        m_data = new Data();
    m_data->comment = c;
}


bool Any::isNone() const {
    return (m_type == NONE);
}


double Any::number() const {
    checkType(NUMBER);
    return m_simpleValue.n;
}


/*
double Any::number(double defaultVal) const {
    checkType(NUMBER);
    return m_simpleValue.n;
}
*/


const std::string& Any::string() const {
    checkType(STRING);
    return *(m_data->value.s);
}


/*
const std::string& Any::string(const std::string& defaultVal) const {
    checkType(STRING);
    return *(m_data->value.s);
}
*/


bool Any::boolean() const {
    checkType(BOOLEAN);
    return m_simpleValue.b;
}


/*
bool Any::boolean(bool defaultVal) const {
    checkType(BOOLEAN);
    return m_simpleValue.b;
}
*/


const std::string& Any::name() const {
    static const std::string blank;
    if (m_data != NULL)
        return m_data->name;
    else
        return blank;
}


void Any::setName(const std::string& n) {
    if (m_data == NULL)
        m_data = new Data();
    m_data->name = n;
}


int Any::size() const {
    switch (m_type) {
    case TABLE:
        debugAssertM(m_data != NULL,"NULL m_data");
        return m_data->value.t->size();
        break;
    case ARRAY:
        debugAssertM(m_data != NULL,"NULL m_data");
        return m_data->value.a->size();
        break;
    default:
        throw WrongType(NONE,m_type);    // TODO: Need a version of WrongType taking a list of valid expected types?
    };    // switch (m_type)
}


int Any::length() const {
    return size();
}


const Any& Any::operator[](int i) const {
    checkType(ARRAY);
    debugAssertM(m_data != NULL,"NULL m_data");
    Array<Any>& array = *(m_data->value.a);
    return array[i];
}


Any& Any::operator[](int i) {
    checkType(ARRAY);
    debugAssertM(m_data != NULL,"NULL m_data");
    Array<Any>& array = *(m_data->value.a);
    return array[i];
}


const Array<Any>& Any::array() const {
    checkType(ARRAY);
    debugAssertM(m_data != NULL,"NULL m_data");
    return *(m_data->value.a);
}


void Any::append(const Any& x0) {
    checkType(ARRAY);
    debugAssertM(m_data != NULL,"NULL m_data");
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
    debugAssertM(m_data != NULL,"NULL m_data");
    return *(m_data->value.t);
}


const Any& Any::operator[](const std::string& x) const {
    checkType(TABLE);
    debugAssertM(m_data != NULL,"NULL m_data");
    const Table<std::string,Any>& table = *(m_data->value.t);
    Any* value = table.getPointer(x);
    if( value == NULL )
        throw KeyNotFound(x);
    return *value;
}


Any& Any::operator[](const std::string& x) {
    checkType(TABLE);
    debugAssertM(m_data != NULL,"NULL m_data");
    Table<std::string,Any>& table = *(m_data->value.t);
    return table.getCreate(x);
}


const Any& Any::get(const std::string& x, const Any& defaultVal) const {
    try {
        return operator[](x);
    } catch(KeyNotFound) {
        return defaultVal;
    }
}


bool Any::operator==(const Any& x) const {
    if( m_type != x.m_type )
        return false;
    switch (m_type) {
    case NONE:
        return true;
    case BOOLEAN:
        return (m_simpleValue.b == x.m_simpleValue.b);
    case NUMBER:
        return (m_simpleValue.n == x.m_simpleValue.n);
    case STRING:
        debugAssertM(m_data != NULL,"NULL m_data");
        return (*(m_data->value.s) == *(x.m_data->value.s));
    case TABLE: {
        if (size() != x.size())
            return false;
        debugAssertM(m_data != NULL,"NULL m_data");
        if (m_data->name != x.m_data->name)
            return false;
        Table<std::string,Any>& cmptable  = *(  m_data->value.t);
        Table<std::string,Any>& xcmptable = *(x.m_data->value.t);
        for (Table<std::string,Any>::Iterator it1 = cmptable.begin(), it2 = xcmptable.begin();
             it1 != cmptable.end() && it2 != xcmptable.end();
             ++it1, ++it2)
            if (*it1 != *it2)
                return false;
        return true;
    }
    case ARRAY: {
        if (size() != x.size())
            return false;
        debugAssertM(m_data != NULL,"NULL m_data");
        if (m_data->name != x.m_data->name)
            return false;
        Array<Any>& cmparray  = *(  m_data->value.a);
        Array<Any>& xcmparray = *(x.m_data->value.a);
        for (int ii = 0; ii < size(); ++ii) {
            if (cmparray[ii] != xcmparray[ii])
                return false;
        }
        return true;
    }
    default:
        throw WrongType(NONE,m_type);    // TODO: Need a version of WrongType taking a list of valid expected types?
        break;
    };    // switch (m_type)
}


bool Any::operator!=(const Any& x) const {
    return !operator==(x);
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
    TextOutput::Settings settings;
    settings.wordWrap = TextOutput::Settings::WRAP_NONE;

    TextOutput to(filename,settings);
    serialize(to);
    to.commit();
}


void Any::serialize(TextOutput& to) const {
    // TODO: Need to output comment(s).
    switch (m_type) {
    case NONE:
        throw WrongType(NONE,m_type);
        break;
    case BOOLEAN:
        to.writeBoolean(m_simpleValue.b);
        break;
    case NUMBER:
        to.writeNumber(m_simpleValue.n);
        break;
    case STRING:
        debugAssertM(m_data != NULL,"NULL m_data");
        to.writeString(*(m_data->value.s));
        break;
    case TABLE: {
        debugAssertM(m_data != NULL,"NULL m_data");
        if (!m_data->name.empty())
            to.writeSymbol(m_data->name);
        to.writeSymbol("{");
        to.writeNewline();
        to.pushIndent();
        bool first = true;
        Table<std::string,Any>& table = *(m_data->value.t);
        for( Table<std::string,Any>::Iterator it = table.begin(); it != table.end(); ++it ) {
            if( !first ) {
                to.writeSymbol(",");
                to.writeNewline();
            }

            to.writeSymbol(it->key);
            to.writeSymbol("=");
            it->value.serialize(to);

            if( first )
                first = false;
        }
        to.popIndent();
        to.writeSymbol("}");
        to.writeNewline();
        break;
    }
    case ARRAY: {
        debugAssertM(m_data != NULL,"NULL m_data");
        if (!m_data->name.empty())
            to.writeSymbol(m_data->name);
        to.writeSymbol("(");
        to.writeNewline();
        to.pushIndent();
        bool first = true;
        Array<Any>& array = *(m_data->value.a);
        for( int ii = 0; ii < size(); ++ii ) {
            if( !first ) {
                to.writeSymbol(",");
                to.writeNewline();
            }

            array[ii].serialize(to);

            if( first )
                first = false;
        }
        to.popIndent();
        to.writeSymbol(")");
        to.writeNewline();
        break;
    }
    default:
        throw WrongType(NONE,m_type);    // TODO: Need a version of WrongType taking a list of valid expected types?
        break;
    };    // switch (m_type)
}


void Any::deserialize(TextInput& ti) {
    Token token = ti.read();
    switch (token.type()) {

    case Token::SYMBOL:
        dropReference();

        if (token.string() == "{") {
            // Become a TABLE.
            m_type = TABLE;
            m_data = new Data( Table<std::string,Any>() );
            deserializeTable(ti);
            return;
        }

        else if (token.string() == "(") {
            // Become an ARRAY.
            m_type = ARRAY;
            m_data = new Data( Array<Any>() );
            deserializeArray(ti,")");
            return;
        }

        else if (token.string() == "[") {
            // Become an ARRAY.
            m_type = ARRAY;
            m_data = new Data( Array<Any>() );
            deserializeArray(ti,"]");
            return;
        }

        else {    // Name of a table or of an array.
            deserialize(ti);
            if ( m_type != TABLE && m_type != ARRAY )
                throw CorruptText("Expected a named table or a named array.",token);
            debugAssertM(m_data != NULL,"NULL m_data");
            m_data->name = token.string();
            if ( m_data->name.empty() )
                throw CorruptText("Expected a named table or a named array.",token);
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
        // Become whatever the next expression is, prepending this comment to that.
        deserialize(ti);
        if (m_data == NULL)
            m_data = new Data();
        m_data->comment = token.string()+m_data->comment;

        return;

    case Token::NEWLINE:
        // NEWLINE ignored.
        return;

    default:
        throw CorruptText("Unexpected token type.",token);

    };    // switch (token.type())
}


void Any::deserializeTable(TextInput& ti) {
    // Create Anys until an end-of-table symbol "}" is read.
    bool previous_comma = true;
    Token token;
    for (token = ti.read(); token.type() != Token::END; token = ti.read()) {
        // Remember comments for later insertion into the new Any object.
        std::deque<std::string> comments;
        while(token.type() == Token::COMMENT) {
            comments.push_back(token.string());
            token = ti.read();
        }

        // Validate the key name or comma or closing parenthesis.
        std::string key;
        if (token.type() == Token::SYMBOL) {
            if (token.string() == ",") {
                if (previous_comma)
                    throw CorruptText("Expected a symbol name.",token);
                previous_comma = true;
                continue;    // Skip comma.
            }
            if (token.string() == "}")
                return;    // Done creating the TABLE.
            if (!previous_comma)
                throw CorruptText("Expected a comma or a closing parenthesis.",token);
            key = token.string();
            previous_comma = false;
        }
        else {
            throw CorruptText("Expected a symbol name.",token);
        }

        // Usually an equal sign "=", but could be a comment.
        token = ti.read();
        while(token.type() == Token::COMMENT) {
            comments.push_back(token.string());
            token = ti.read();
        }

        // Create the new Any inside the table.
        Any& sub = operator[](key);
        sub.deserialize(ti);

        // Insert comments into the object we created.
        if (!comments.empty() && sub.m_data == NULL)
            sub.m_data = new Data();
        while (!comments.empty()) {
            sub.m_data->comment = comments.front()+m_data->comment;
            comments.pop_front();
        }
    }
    throw CorruptText("Table ended unexpectedly.",token);
}


void Any::deserializeArray(TextInput& ti,const std::string& term) {
    // Create Anys until an end-of-table symbol "}" is read.
    bool previous_comma = true;
    Token token;
    for (token = ti.read(); token.type() != Token::END; token = ti.read()) {
        // Remember comments for later insertion into the new Any object.
        std::deque<std::string> comments;
        while(token.type() == Token::COMMENT) {
            comments.push_back(token.string());
            token = ti.read();
        }

        // Validate the array value or comma or closing parenthesis.
        if (token.type() == Token::SYMBOL) {
            if (token.string() == ",") {
                if (previous_comma)
                    throw CorruptText("Expected an array value.",token);
                previous_comma = true;
                continue;    // Skip comma.
            }
            if (token.string() == term)
                return;    // Done creating the TABLE.
            if (!previous_comma)
                throw CorruptText("Expected a comma or a closing parenthesis.",token);
        }
        else if (token.type() == Token::NUMBER) {
        }
        else if (token.type() == Token::STRING) {
        }
        else if (token.type() == Token::BOOLEAN) {
        }
        else {
            throw CorruptText("Expected an array value.",token);
        }
        previous_comma = false;

        // Might be comment lines here before the comma or before the end of array.
        Token cmttoken = ti.read();
        while(cmttoken.type() == Token::COMMENT) {
            comments.push_back(cmttoken.string());
            cmttoken = ti.read();
        }
        ti.push(cmttoken);

        // Create the new Any inside the array.
        ti.push(token);
        append(Any(ti));

        // Insert comments into the object we created.
        debugAssertM(m_data != NULL,"NULL m_data");
        Any& sub = m_data->value.a->back();
        if (!comments.empty() && sub.m_data == NULL)
            sub.m_data = new Data();
        while (!comments.empty()) {
            sub.m_data->comment = comments.front()+m_data->comment;
            comments.pop_front();
        }
    }
    throw CorruptText("Table ended unexpectedly.",token);
}


Any::operator int() const {
    return iRound(number());
}


Any::operator float() const {
    return float(number());
}


Any::operator double() const {
    return number();
}


Any::operator bool() const {
    return boolean();
}


Any::operator std::string() const {
    return string();
}


}    // namespace G3D

