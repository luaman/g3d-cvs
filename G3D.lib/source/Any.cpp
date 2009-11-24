/**
 @file Any.cpp

 @author Morgan McGuire
 @author Shawn Yarbrough
  
 @created 2006-06-11
 @edited  2009-11-15

 Copyright 2000-2009, Morgan McGuire.
 All rights reserved.
 */

#include "G3D/Any.h"
#include "G3D/TextOutput.h"
#include "G3D/TextInput.h"
#include "G3D/stringutils.h"
#include <deque>
#include <iostream>

namespace G3D {
   
Any::Data* Any::Data::create(const Data* d) {
    Data* p = create(d->type);

    p->comment = d->comment;
    p->name = d->name;

    switch (d->type) {
    case NONE:
    case BOOLEAN:
    case NUMBER:
        // No clone needed
        break;

    case STRING:
        *(p->value.s) = *(d->value.s);
        break;

    case ARRAY:
        *(p->value.a) = *(d->value.a);
        break;

    case TABLE:
        *(p->value.t) = *(d->value.t);
        break;
    }

    return p;
}


Any::Data* Any::Data::create(Any::Type t) {
    size_t s = sizeof(Data);

    switch (t) {
    case NONE:
    case BOOLEAN:
    case NUMBER:
        // No extra space needed
        break;

    case STRING:
        s += sizeof(std::string);
        break;

    case ARRAY:
        s += sizeof(AnyArray);
        break;

    case TABLE:
        s += sizeof(AnyTable);
        break;
    }

    // Allocate the data object
    Data* p = new (MemoryManager::create()->alloc(s)) Data(t);

    // Create the (empyt) value object at the end of the Data object
    switch (t) {
    case NONE:
    case BOOLEAN:
    case NUMBER:
        // No value
        break;

    case STRING:
        p->value.s = new (p + 1) std::string();
        break;

    case ARRAY:
        p->value.a = new (p + 1) AnyArray();
        break;

    case TABLE:
        p->value.t = new (p + 1) AnyTable();
        break;
    }    

    return p;
}


void Any::Data::destroy(Data* d) {
    if (d != NULL) {
        d->~Data();
        MemoryManager::create()->free(d);
    }
}


Any::Data::~Data() {
    debugAssertM(referenceCount.value() <= 0, "Tried to deallocate an Any::Data with a positive reference count.");

    // Destruct but do not deallocate children
    switch (type) {
    case STRING:
        debugAssert(value.s != NULL);
        value.s->~basic_string();
        break;

    case ARRAY:
        debugAssert(value.a != NULL);
        value.a->~Array();
        break;

    case TABLE:
        debugAssert(value.t != NULL);
        value.t->~Table();
        break;

    default:
        // All other types should have a NULL value pointer (i.e., they were used just for name and comment fields)
        debugAssertM(value.s == NULL, "Corrupt Any::Data::Value");
    }

    value.s = NULL;
}


//////////////////////////////////////////////////////////////

bool Any::containsKey(const std::string& x) const {
    verifyType(TABLE);
    return m_data->value.t->containsKey(x);
}


void Any::dropReference() {
    if (m_data && m_data->referenceCount.decrement() <= 0) {
        // This was the last reference to the shared data
        Data::destroy(m_data);
    }
    m_data = NULL;
}


void Any::ensureMutable() {
    if (m_data && (m_data->referenceCount.value() >= 1)) {
        // Copy the data.  We must do this before dropping the reference
        // to avoid a race condition
        Data* d = Data::create(m_data);
        dropReference();
        m_data = d;
    }
}


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


#ifdef G3D_32BIT
Any::Any(int64 x) : m_type(NUMBER), m_simpleValue((double)x), m_data(NULL) {
}
#endif    // G3D_32BIT


#if 0
Any::Any(int32 x) : m_type(NUMBER), m_simpleValue((double)x), m_data(NULL) {
}
#endif    // 0


Any::Any(long x) : m_type(NUMBER), m_simpleValue((double)x), m_data(NULL) {
}


Any::Any(int x) : m_type(NUMBER), m_simpleValue((double)x), m_data(NULL) {
}


Any::Any(short x) : m_type(NUMBER), m_simpleValue((double)x), m_data(NULL) {
}


Any::Any(bool x) : m_type(BOOLEAN), m_simpleValue(x), m_data(NULL) {
}


Any::Any(const std::string& s) : m_type(STRING), m_data(Data::create(STRING)) {
    *(m_data->value.s) = s;
}


Any::Any(const char* s) : m_type(STRING), m_data(NULL) {
    if (s == NULL) {
        m_type = NONE;
    } else {
        ensureData();
        *(m_data->value.s) = s;
    }
}


Any::Any(Type t, const std::string& name) : m_type(t), m_data(NULL) {
    alwaysAssertM(t == ARRAY || t == TABLE,
                  "Illegal type with Any(Type) constructor");

    ensureData();
    if (name != "") {
        m_data->name = name;
    }
}


Any::~Any() {
    dropReference();
}


Any& Any::operator=(const Any& x) {
    if (this == &x) {
        return *this;
    }

    dropReference();

    m_type        = x.m_type;
    m_simpleValue = x.m_simpleValue;

    if (x.m_data != NULL) {
        x.m_data->referenceCount.increment();
        m_data = x.m_data;
    }

    return *this;
}


Any& Any::operator=(double x) {
    *this = Any(x);
    return *this;
}


Any& Any::operator=(int x) {
    return (*this = Any(x));
}


Any& Any::operator=(bool x) {
    *this = Any(x);
    return *this;
}


Any& Any::operator=(const std::string& x) {
    *this = Any(x);
    return *this;
}


Any& Any::operator=(const char* x) {
    *this = Any(x);
    return *this;
}


Any& Any::operator=(Type t) {
    switch (t) {
    case NONE:  
        *this = Any();
        break;

    case TABLE: 
    case ARRAY: 
        *this = Any(t);
        break;

    default:
        alwaysAssertM(false, "Any = Any::Type must take NONE, TABLE, or ARRAY as an argument");
    }

    return *this;
}


Any::Type Any::type() const {
    return m_type;
}


const std::string& Any::comment() const {
    static const std::string blank;
    if (m_data != NULL) {
        return m_data->comment;
    } else {
        return blank;
    }
}


void Any::setComment(const std::string& c) {
    ensureData();
    m_data->comment = c;
}


bool Any::isNone() const {
    return (m_type == NONE);
}


double Any::number() const {
    verifyType(NUMBER);
    return m_simpleValue.n;
}


const std::string& Any::string() const {
    verifyType(STRING);
    return *(m_data->value.s);
}


bool Any::boolean() const {
    verifyType(BOOLEAN);
    return m_simpleValue.b;
}


const std::string& Any::name() const {
    static const std::string blank;
    if (m_data != NULL) {
        return m_data->name;
    } else {
        return blank;
    }
}


void Any::setName(const std::string& n) {
    ensureData();
    m_data->name = n;
}


int Any::size() const {
    verifyType(ARRAY, TABLE);
    switch (m_type) {
    case TABLE:
        return m_data->value.t->size();

    case ARRAY:
        return m_data->value.a->size();

    default:;
    } // switch (m_type)
}


int Any::length() const {
    return size();
}


void Any::resize(int n) {
    alwaysAssertM(n >= 0, "Argument to resize must be non-negative");
    verifyType(ARRAY);
    m_data->value.a->resize(n);
}


void Any::clear() {
    verifyType(ARRAY, TABLE);
    switch (m_type) {
    case ARRAY:
        m_data->value.a->clear();
        break;

    case TABLE:
        m_data->value.t->clear();
        break;

    default:;
    }
}


const Any& Any::operator[](int i) const {
    verifyType(ARRAY);
    debugAssertM(m_data != NULL, "NULL m_data");
    Array<Any>& array = *(m_data->value.a);
    return array[i];
}


Any& Any::next() {
    verifyType(ARRAY);
    int n = size();
    resize(n + 1);
    return (*this)[n];
}


Any& Any::operator[](int i) {
    verifyType(ARRAY);
    debugAssertM(m_data != NULL,"NULL m_data");
    Array<Any>& array = *(m_data->value.a);
    return array[i];
}


const Array<Any>& Any::array() const {
    verifyType(ARRAY);
    debugAssertM(m_data != NULL,"NULL m_data");
    return *(m_data->value.a);
}


void Any::append(const Any& x0) {
    verifyType(ARRAY);
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
    verifyType(TABLE);
    debugAssertM(m_data != NULL,"NULL m_data");
    return *(m_data->value.t);
}


const Any& Any::operator[](const char* x) const {
    return (*this)[std::string(x)];
}


const Any& Any::operator[](const std::string& x) const {
    verifyType(TABLE);
    debugAssertM(m_data != NULL, "NULL m_data");
    const Table<std::string, Any>& table = *(m_data->value.t);
    Any* value = table.getPointer(x);
    if (value == NULL) {
        throw KeyNotFound(x);
    }
    return *value;
}


void Any::set(const std::string& k, const Any& v) {
    verifyType(TABLE);
    debugAssertM(m_data != NULL,"NULL m_data");
    Table<std::string, Any>& table = *(m_data->value.t);
    table.set(k, v);
}


const Any& Any::get(const std::string& x, const Any& defaultVal) const {
    try {
        return operator[](x);
    } catch(KeyNotFound) {
        return defaultVal;
    }
}


bool Any::operator==(const Any& x) const {
    if (m_type != x.m_type) {
        return false;
    }

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
        if (size() != x.size()) {
            return false;
        }
        debugAssertM(m_data != NULL,"NULL m_data");
        if (m_data->name != x.m_data->name) {
            return false;
        }
        Table<std::string, Any>& cmptable  = *(  m_data->value.t);
        Table<std::string, Any>& xcmptable = *(x.m_data->value.t);
        for (Table<std::string,Any>::Iterator it1 = cmptable.begin(), it2 = xcmptable.begin();
             it1 != cmptable.end() && it2 != xcmptable.end();
             ++it1, ++it2) {
             if (*it1 != *it2) {
                return false;
             }
        }
        return true;
    }

    case ARRAY: {
        if (size() != x.size()) {
            return false;
        }
        debugAssertM(m_data != NULL,"NULL m_data");
        if (m_data->name != x.m_data->name) {
            return false;
        }

        Array<Any>& cmparray  = *(  m_data->value.a);
        Array<Any>& xcmparray = *(x.m_data->value.a);

        for (int ii = 0; ii < size(); ++ii) {
            if (cmparray[ii] != xcmparray[ii]) {
                return false;
            }
        }
        return true;
    }

    default:
        alwaysAssertM(false, "Corrupt Any");
    }    // switch (m_type)
}


bool Any::operator!=(const Any& x) const {
    return !operator==(x);
}


static void getDeserializeSettings(TextInput::Settings& settings) {
    settings.cppBlockComments = true;
    settings.cppLineComments = true;
    settings.otherLineComments = true;
    settings.otherCommentCharacter = '#';
    settings.generateCommentTokens = true;
    settings.singleQuotedStrings = false;
    settings.msvcSpecials = false;
    settings.caseSensitive = false;
}


std::string Any::unparse() const {
    TextOutput::Settings settings;
    TextOutput to(settings);
    serialize(to);
    return to.commitString();
}


void Any::parse(const std::string& src) {
    TextInput::Settings settings;
    getDeserializeSettings(settings);

    TextInput ti(TextInput::FROM_STRING, src, settings);
    deserialize(ti);
}


void Any::load(const std::string& filename) {
    TextInput::Settings settings;
    getDeserializeSettings(settings);

    TextInput ti(filename, settings);
    deserialize(ti);
}


void Any::save(const std::string& filename) const {
    TextOutput::Settings settings;
    settings.wordWrap = TextOutput::Settings::WRAP_NONE;

    TextOutput to(filename,settings);
    serialize(to);
    to.commit();
}


static bool needsQuotes(const std::string& s) {
    if (! isLetter(s[0]) && (s[0] != '_')) {
        return true;
    }

    for (int i = 0; i < (int)s.length(); ++i) {
        char c = s[i];
        
        // peek character
        char p = (i == s.length() - 1) ? '_' : s[i + 1];

        // Identify separators
        if ((c == '-' && p == '>') ||
            (c == ':' && p == ':')) {            
            // Skip over this symbol
            ++i;
            continue;
        }

        if (! isDigit(c) && ! isLetter(c) & (c != '.')) {
            // This is an illegal character for an identifier, so we need quotes
            return true;
        }
    }

    return false;
}


// TODO: if the output will fit on one line, compress tables and arrays into a single line
void Any::serialize(TextOutput& to) const {
    if (m_data && ! m_data->comment.empty()) {
        to.printf("\n/* %s */\n", m_data->comment.c_str());
    }

    switch (m_type) {
    case NONE:
        to.writeSymbol("NONE");
        break;

    case BOOLEAN:
        to.writeBoolean(m_simpleValue.b);
        break;

    case NUMBER:
        to.writeNumber(m_simpleValue.n);
        break;

    case STRING:
        debugAssertM(m_data != NULL, "NULL m_data");
        to.writeString(*(m_data->value.s));
        break;

    case TABLE: {
        debugAssertM(m_data != NULL, "NULL m_data");
        if (! m_data->name.empty()) {
            if (needsQuotes(m_data->name)) {
                to.writeString(m_data->name);
            } else {
                to.writeSymbol(m_data->name);
            }
        }
        to.writeSymbol("{");
        to.writeNewline();
        to.pushIndent();
        AnyTable& table = *(m_data->value.t);
        Array<std::string> keys;
        table.getKeys(keys);
        keys.sort();

        for (int i = 0; i < keys.size(); ++i) {

            to.writeSymbol(keys[i]);
            to.writeSymbol("=");
            table[keys[i]].serialize(to);

            if (i < keys.size() - 1) {
                to.writeSymbol(",");
            }
            to.writeNewline();

            // Skip a line between table entries
            to.writeNewline();
        }

        to.popIndent();
        to.writeSymbol("}");
        break;
    }

    case ARRAY: {
            debugAssertM(m_data != NULL, "NULL m_data");
            if (! m_data->name.empty()) {
                // For arrays, leave no trailing space between the name and the paren
                to.writeSymbol(format("%s(", m_data->name.c_str()));
            } else {
                to.writeSymbol("(");
            }
            to.writeNewline();
            to.pushIndent();
            Array<Any>& array = *(m_data->value.a);
            for (int ii = 0; ii < size(); ++ii) {
                array[ii].serialize(to);
                if (ii < size() - 1) {
                    to.writeSymbol(",");
                    to.writeNewline();
                }

                // Put the close paren on an array right behind the last element
            }
            to.popIndent();
            to.writeSymbol(")");
            break;
        }
    }
}


void Any::deserializeComment(TextInput& ti, Token& token, std::string& comment) {
    // Parse comments
    while (token.type() == Token::COMMENT) {
        comment += trimWhitespace(token.string()) + "\n";

        // Allow comments to contain newlines.
        do {
            token = ti.read();
            comment += "\n";
        } while (token.type() == Token::NEWLINE);
    }

    comment = trimWhitespace(comment);
}

/** True if \a c is an open paren of some form */
static bool isOpen(const char c) {
    return c == '(' || c == '[' || c == '{';
}


/** True if \a c is an open paren of some form */
static bool isClose(const char c) {
    return c == ')' || c == ']' || c == '}';
}


/** True if \a s is a C++ name operator */
static bool isNameOperator(const std::string& s) {
    return s == "." || s == "::" || s == "->";
}


void Any::deserializeName(TextInput& ti, Token& token, std::string& name) {
    debugAssert(token.type() == Token::SYMBOL);
    std::string s = token.string();
    while (! isOpen(s[0])) {
        name += s;

        // Skip newlines and comments
        token = ti.readSignificant();

        if (token.type() != Token::SYMBOL) {
            throw ParseError(ti.filename(), token.line(), token.character(), 
                "Expected symbol while parsing Any");
        }
        s = token.string();
    }
}


void Any::deserialize(TextInput& ti) {
    Token token = ti.read();
    deserialize(ti, token);
    // Restore the last token
    ti.push(token);
}


void Any::deserialize(TextInput& ti, Token& token) {
    // Deallocate old data
    dropReference();
    m_type = NONE;
    m_simpleValue.b = false;

    // Skip leading newlines
    while (token.type() == Token::NEWLINE) {
        token = ti.read();
    } 

    std::string comment;
    if (token.type() == Token::COMMENT) {
        deserializeComment(ti, token, comment);
    }

    if (token.type() == Token::END) {
        // There should never be a comment without an Any following it; even
        // if the file ends with some commented out stuff,
        // that should not happen after a comma, so we'd never read that 
        // far in a proper file.
        throw ParseError(ti.filename(), token.line(), token.character(), 
            "File ended without a properly formed Any");
    }

    switch (token.type()) {
    case Token::STRING:
        m_type = STRING;
        ensureData();
        *(m_data->value.s) = token.string();
        m_data->source.set(ti, token);
        break;

    case Token::NUMBER:
        m_type = NUMBER;
        m_simpleValue.n = token.number();
        ensureData();
        m_data->source.set(ti, token);
        break;

    case Token::BOOLEAN:
        m_type = BOOLEAN;
        m_simpleValue.b = token.boolean();
        ensureData();
        m_data->source.set(ti, token);
        break;

    case Token::SYMBOL:
        // Named Array, Named Table, Array, Table, or NONE
        if (toUpper(token.string()) == "NONE") {
            // Nothing left to do; we initialized to NONE originally
            ensureData();
            m_data->source.set(ti, token);
        } else {
            // Array or Table

            // Parse the name

            // s must have at least one element or this would not have
            // been parsed as a symbol
            std::string name;
            deserializeName(ti, token, name);
            if (token.type() != Token::SYMBOL) {
                throw ParseError(ti.filename(), token.line(), token.character(), 
                    "Malformed Any TABLE or ARRAY; must start with [, (, or {");
            }

            if (isOpen(token.string()[0])) {
                // Array or table
                deserializeBody(ti, token);
            } else {
                throw ParseError(ti.filename(), token.line(), token.character(), 
                    "Malformed Any TABLE or ARRAY; must start with [, (, or {");
            }

            if (! name.empty()) {
                ensureData();
                m_data->name = name;
            }
        } // if NONE
        break;

    default:
        throw ParseError(ti.filename(), token.line(), token.character(), 
            "Unexpected token");

    } // switch

    if (! comment.empty()) {
        ensureData();
        m_data->comment = comment;
    }

    if (m_type != ARRAY && m_type != TABLE) {
        // Array and table already consumed their last token
        token = ti.read();
    }
}


void Any::ensureData() {
    if (m_data == NULL) {
        m_data = Data::create(m_type);
    }
}


static bool isSeparator(char c) {
    return c == ',' || c == ';';
}


void Any::readUntilCommaOrClose(TextInput& ti, Token& token) {
    while (! ((token.type() == Token::SYMBOL) && 
              (isClose(token.string()[0])) || 
               isSeparator(token.string()[0]))) {
        switch (token.type()) {
        case Token::NEWLINE:
        case Token::COMMENT:
            // Consume
            token = ti.read();
            break;

        default:
            throw ParseError(ti.filename(), token.line(), token.character(), 
                "Expected a comma or close paren");
        }
    }
}


void Any::deserializeBody(TextInput& ti, Token& token) {
    char closeSymbol = '}';
    m_type = TABLE;
    
    const char c = token.string()[0];
    
    if (c != '{') {
        m_type = ARRAY;
        // Chose the appropriate close symbol
        closeSymbol = (c == '(') ? ')' : ']';
    }

    // Allocate the underlying data structure
    ensureData();
    m_data->source.set(ti, token);

    // Consume the open token
    token = ti.read();

    while (! ((token.type() == Token::SYMBOL) && (token.string()[0] == closeSymbol))) {

        // Read any leading comment.  This must be done here (and not in the recursive deserialize
        // call) in case the body contains only a comment.
        std::string comment;
        deserializeComment(ti, token, comment);

        if ((token.type() == Token::SYMBOL) && (token.string()[0] == closeSymbol)) {
            // We're done; this catches the case where the array is empty
            break;
        }

        // Pointer the value being read
        Any a = NULL;
        std::string key;
        
        if (m_type == TABLE) {
            // Read the key
            if (token.type() != Token::SYMBOL && token.type() != Token::STRING) {
                throw ParseError(ti.filename(), token.line(), token.character(), "Expected a name");
            } 
            
            key = token.string();
            // Consume everything up to the = sign
            token = ti.readSignificant();

            if ((token.type() != Token::SYMBOL) || (token.string() != "=")) {
                throw ParseError(ti.filename(), token.line(), token.character(), "Expected =");
            } else {
                // Consume (don't consume comments--we want the value pointed to by a to get those).
                token = ti.read();
            }
        }
        a.deserialize(ti, token);

        if (! comment.empty()) {
            // Prepend the comment we read earlier
            a.ensureData();
            a.m_data->comment = trimWhitespace(comment + "\n" + a.m_data->comment);
        }
        
        if (m_type == TABLE) {
            set(key, a);
        } else {
            append(a);
        }

        // Read until the comma or close paren, discarding trailing comments and newlines
        readUntilCommaOrClose(ti, token);

        // Consume the comma
        if (isSeparator(token.string()[0])) {
            token = ti.read();
        }
    }

    // Consume the close paren (to match other deserialize methods)
    token = ti.read();
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


const Any::Source& Any::source() const {
    static Source s;
    if (m_data) {
        return m_data->source;
    } else {
        return s;
    }
}


void Any::verify(bool value, const std::string& message) const {
    if (! value) {
        ParseError p;
        if (m_data) {
            p.filename  = m_data->source.filename;
            p.line      = m_data->source.line;
            p.character = m_data->source.character;
        }

        if (name().empty()) {
            p.message = "Parse error";
        } else {
            p.message = "Parse error while reading the contents of " + name();
        }

        if (! message.empty()) {
            p.message = p.message + ": " + message;
        }

        throw p;
    }
}


void Any::verifyName(const std::string& n) const {
    verify(beginsWith(toUpper(name()), toUpper(n)), "Name must begin with " + n);
}


void Any::verifyType(Type t) const {
    if (type() != t) {
        verify(false, "Must have type " + toString(t));
    }
}


void Any::verifyType(Type t0, Type t1) const {
    if (type() != t0 && type() != t1) {
        verify(false, "Must have type " + toString(t0) + " or " + toString(t1));
    }
}


void Any::verifySize(int low, int high) const {
    verifyType(ARRAY, TABLE);
    if (size() < low || size() > high) {
        verify(false, format("Size must be between %d and %d", low, high));
    }
}


void Any::verifySize(int s) const {
    verifyType(ARRAY, TABLE);
    if (size() != s) {
        verify(false, format("Size must be %d", s));
    }
}


std::string Any::toString(Type t) {
    switch(t) {
    case NONE:    return "NONE";
    case BOOLEAN: return "BOOLEAN";
    case NUMBER:  return "NUMBER";
    case STRING:  return "STRING";
    case ARRAY:   return "ARRAY";
    case TABLE:   return "TABLE";
    default:
        alwaysAssertM(false, "Illegal Any::Type");
    }
}

}    // namespace G3D

