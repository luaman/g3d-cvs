#include "G3D/G3DAll.h"

static void tfunc1();
static void tfunc2();
static void tCommentTokens();
static void tNewlineTokens();

void testTextInput() {
    printf("TextInput\n");

    {
        // Parse floats
 		TextInput ti(TextInput::FROM_STRING, "1.2f");
        debugAssert(ti.readNumber() == 1.2);
        debugAssert(! ti.hasMore());
    }
    {
        // Parse floats
 		TextInput ti(TextInput::FROM_STRING, ".1");
        debugAssert(ti.readNumber() == 0.1);
    }
    {
 		TextInput ti(TextInput::FROM_STRING, "..1");
        debugAssert(ti.readSymbol() == "..");
        debugAssert(ti.readNumber() == 1);
    }

    {
        // Quoted string with escapes.  The actual expression we are parsing looks like:
        // "\\"
 		TextInput ti(TextInput::FROM_STRING, "\"\\\\\"");
        Token t;

		ti.readString("\\");

        t = ti.read();
        debugAssert(t.type() == Token::END);
        debugAssert(!ti.hasMore());
	}

    {
        // Quoted string without escapes: read as two backslashes
        // (the test itself has to escape the backslashes, just to write them).
        // The actual expression we are parsing is:
        // "\"
        TextInput::Settings opt;
        Token t;

        opt.escapeSequencesInStrings = false;
 		TextInput ti(TextInput::FROM_STRING, "\"\\\"", opt);
		ti.readString("\\");

        t = ti.read();
        debugAssert(t.type() == Token::END);
        debugAssert(!ti.hasMore());
	}

	{
 		TextInput ti(TextInput::FROM_STRING, "a \'foo\' bar");
        Token t;

		ti.readSymbol("a");

		t = ti.read();
		debugAssert(t.extendedType() == Token::SINGLE_QUOTED_TYPE);
		debugAssert(t.string() == "foo");
		ti.readSymbol("bar");

        t = ti.read();
        debugAssert(t.type() == Token::END);
        debugAssert(!ti.hasMore());
	}

    {
        TextInput ti(TextInput::FROM_STRING, "2.x");
        Token t;

        debugAssert(ti.readNumber() == 2);
        ti.readSymbol("x");

        t = ti.read();
        debugAssert(t.type() == Token::END);
        debugAssert(!ti.hasMore());
    }
    {
        TextInput ti(TextInput::FROM_STRING, "1.E7");
        Token t;

        debugAssert(ti.readNumber() == 1.E7);

        t = ti.read();
        debugAssert(t.type() == Token::END);
        debugAssert(!ti.hasMore());
    }

    {
        TextInput ti(TextInput::FROM_STRING, "\\123");
        Token t;
        t = ti.read();
        debugAssert(t.type() == Token::SYMBOL);
        debugAssert(t.string() == "\\");
        t = ti.read();
        debugAssert(t.type() == Token::NUMBER);
        debugAssert(t.number() == 123);

        t = ti.read();
        debugAssert(t.type() == Token::END);
        debugAssert(!ti.hasMore());
    }

    {
        TextInput::Settings options;
        options.otherCommentCharacter = '#';

        TextInput ti(TextInput::FROM_STRING, "1#23\nA\\#2", options);
        Token t;
        t = ti.read();
        debugAssert(t.type() == Token::NUMBER);
        debugAssert(t.number() == 1);

        // skip the comment
        t = ti.read();
        debugAssert(t.type() == Token::SYMBOL);
        debugAssert(t.string() == "A");

        // Read escaped comment character
        t = ti.read();
        debugAssert(t.type() == Token::SYMBOL);
        debugAssert(t.string() == "#");

        t = ti.read();
        debugAssert(t.type() == Token::NUMBER);
        debugAssert(t.number() == 2);

        t = ti.read();
        debugAssert(t.type() == Token::END);
        debugAssert(!ti.hasMore());
    }

    {
        TextInput ti(TextInput::FROM_STRING, "0xFEED");

        Token t;
   
        t = ti.peek();
        debugAssert(t.type() == Token::NUMBER);
        double n = ti.readNumber();
        (void)n;
        debugAssert((int)n == 0xFEED);

        t = ti.read();
        debugAssert(t.type() == Token::END);
        debugAssert(!ti.hasMore());
    }

    {
        TextInput::Settings opt;
        opt.cppLineComments = false;
        TextInput ti(TextInput::FROM_STRING, 
                     "if/*comment*/(x->y==-1e6){cout<<\"hello world\"}; // foo\nbar",
                     opt);

        Token a = ti.read();
        Token b = ti.read();
        Token c = ti.read();
        Token d = ti.read();
        Token e = ti.read();
        Token f = ti.read();
        double g = ti.readNumber();
        (void)g;
        Token h = ti.read();
        Token i = ti.read();
        Token j = ti.read();
        Token k = ti.read();
        Token L = ti.read();
        Token m = ti.read();
        Token n = ti.read();
        Token p = ti.read();
        Token q = ti.read();
        Token r = ti.read();
        Token s = ti.read();
        Token t = ti.read();

        debugAssert(a.type() == Token::SYMBOL);
        debugAssert(a.string() == "if");

        debugAssert(b.type() == Token::SYMBOL);
        debugAssert(b.string() == "(");

        debugAssert(c.type() == Token::SYMBOL);
        debugAssert(c.string() == "x");

        debugAssert(d.type() == Token::SYMBOL);
        debugAssert(d.string() == "->");

        debugAssert(e.type() == Token::SYMBOL);
        debugAssert(e.string() == "y");

        debugAssert(f.type() == Token::SYMBOL);
        debugAssert(f.string() == "==");

        debugAssert(g == -1e6);

        debugAssert(h.type() == Token::SYMBOL);
        debugAssert(h.string() == ")");

        debugAssert(i.type() == Token::SYMBOL);
        debugAssert(i.string() == "{");

        debugAssert(j.type() == Token::SYMBOL);
        debugAssert(j.string() == "cout");

        debugAssert(k.type() == Token::SYMBOL);
        debugAssert(k.string() == "<<");

        debugAssert(L.type() == Token::STRING);
        debugAssert(L.string() == "hello world");

        debugAssert(m.type() == Token::SYMBOL);
        debugAssert(m.string() == "}");

        debugAssert(n.type() == Token::SYMBOL);
        debugAssert(n.string() == ";");

        debugAssert(p.type() == Token::SYMBOL);
        debugAssert(p.string() == "/");

        debugAssert(q.type() == Token::SYMBOL);
        debugAssert(q.string() == "/");

        debugAssert(r.type() == Token::SYMBOL);
        debugAssert(r.string() == "foo");

        debugAssert(s.type() == Token::SYMBOL);
        debugAssert(s.string() == "bar");

        debugAssert(t.type() == Token::END);
    }
    
    {
        TextInput ti(TextInput::FROM_STRING, "-1 +1 2.6");

        Token t;
   
        t = ti.peek();
        debugAssert(t.type() == Token::NUMBER);
        double n = ti.readNumber();
        debugAssert(n == -1);

        t = ti.peek();
        debugAssert(t.type() == Token::NUMBER);
        n = ti.readNumber();
        debugAssert(n == 1);

        t = ti.peek();
        debugAssert(t.type() == Token::NUMBER);
        n = ti.readNumber();
        debugAssert(n == 2.6);
    }

    {
        TextInput ti(TextInput::FROM_STRING, "- 1 ---.51");

        Token t;
   
        t = ti.peek();
        debugAssert(t.type() == Token::SYMBOL);
        ti.readSymbol("-");

        t = ti.peek();
        debugAssert(t.type() == Token::NUMBER);
        double n = ti.readNumber();
        debugAssert(n == 1);

        t = ti.peek();
        debugAssert(t.type() == Token::SYMBOL);
        ti.readSymbol("--");

        t = ti.peek();
        debugAssert(t.type() == Token::NUMBER);
        n = ti.readNumber();
        debugAssert(n == -.51);
    }


    {
        G3D::TextInput::Settings ti_opts;
        const std::string str = "'";
        ti_opts.singleQuotedStrings = false;

        G3D::TextInput ti(G3D::TextInput::FROM_STRING, str, ti_opts);

        G3D::Token t = ti.read();

        /*
        printf("t.string       = %s\n", t.string().c_str());
        printf("t.type         = %d\n", t.type());
        printf("t.extendedType = %d\n", t.extendedType());

        printf("\n");
        printf("SYMBOL         = %d\n", G3D::Token::SYMBOL);
        printf("END            = %d\n", G3D::Token::END);

        printf("\n");
        printf("SYMBOL_TYPE    = %d\n", G3D::Token::SYMBOL_TYPE);
        printf("END_TYPE       = %d\n", G3D::Token::END_TYPE);
        */

        alwaysAssertM(t.type() == G3D::Token::SYMBOL, "");
        alwaysAssertM(t.extendedType() == G3D::Token::SYMBOL_TYPE, "");
    }
    
    tfunc1();
    tfunc2();
    
    tCommentTokens();
    tNewlineTokens();
}

    // these defines are duplicated in tTextInput2.cpp
#define CHECK_EXC_POS(e, lnum, chnum)                                       \
        alwaysAssertM((int)(e).line == (int)(lnum) && (int)(e).character == (int)(chnum), "");

#define CHECK_TOKEN_POS(t, lnum, chnum)                                     \
        alwaysAssertM((int)(t).line() == (int)(lnum)                        \
                      && (int)(t).character() == (int)(chnum), "");         

#define CHECK_TOKEN_TYPE(t, typ, etyp)                                      \
        alwaysAssertM((t).type() == (typ), "");                             \
        alwaysAssertM((t).extendedType() == (etyp), "");                    

#define CHECK_SYM_TOKEN(ti, str, lnum, chnum)                               \
    {                                                                       \
        Token _t;                                                           \
        _t = (ti).read();                                                   \
        CHECK_TOKEN_TYPE(_t, Token::SYMBOL, Token::SYMBOL_TYPE);            \
                                                                            \
        CHECK_TOKEN_POS(_t, (lnum), (chnum));                               \
        alwaysAssertM(_t.string() == (str), "");                            \
    }

#define CHECK_END_TOKEN(ti, lnum, chnum)                                    \
    {                                                                       \
        Token _t;                                                           \
        _t = (ti).read();                                                   \
        CHECK_TOKEN_TYPE(_t, Token::END, Token::END_TYPE);                  \
        CHECK_TOKEN_POS(_t, (lnum), (chnum));                               \
    }


#define CHECK_ONE_SPECIAL_SYM(s)                                            \
    {                                                                       \
        TextInput ti(TextInput::FROM_STRING, "\n a" s "b\n ");              \
        CHECK_SYM_TOKEN(ti, "a", 2, 2);                                     \
        CHECK_SYM_TOKEN(ti,   s, 2, 3);                                     \
        CHECK_SYM_TOKEN(ti, "b", 2, 3 + strlen(s));                         \
        CHECK_END_TOKEN(ti,      3, 2);                                     \
    }

#define CHECK_LINE_COMMENT_TOKEN(ti, str, lnum, chnum)                      \
    {                                                                       \
        Token _t;                                                           \
        _t = (ti).read();                                                   \
        CHECK_TOKEN_TYPE(_t, Token::COMMENT, Token::LINE_COMMENT_TYPE);     \
                                                                            \
        CHECK_TOKEN_POS(_t, (lnum), (chnum));                               \
        alwaysAssertM(_t.string() == (str), "");                            \
    }

#define CHECK_BLOCK_COMMENT_TOKEN(ti, str, lnum, chnum)                     \
    {                                                                       \
        Token _t;                                                           \
        _t = (ti).read();                                                   \
        CHECK_TOKEN_TYPE(_t, Token::COMMENT, Token::BLOCK_COMMENT_TYPE);    \
                                                                            \
        CHECK_TOKEN_POS(_t, (lnum), (chnum));                               \
        alwaysAssertM(_t.string() == (str), "");                            \
    }

#define CHECK_NEWLINE_TOKEN(ti, str, lnum, chnum)                     \
    {                                                                       \
        Token _t;                                                           \
        _t = (ti).read();                                                   \
        CHECK_TOKEN_TYPE(_t, Token::NEWLINE, Token::NEWLINE_TYPE);    \
                                                                            \
        CHECK_TOKEN_POS(_t, (lnum), (chnum));                               \
        alwaysAssertM(_t.string() == (str), "");                            \
    }


static void tfunc1() {
    // Basic line number checking test.  Formerly would skip over line
    // numbers (i.e., report 1, 3, 5, 7 as the lines for the tokens), because
    // the newline would be consumed, pushed back to the input stream, then
    // consumed again (reincrementing the line number.)
    {
        TextInput ti(TextInput::FROM_STRING, "foo\nbar\nbaz\n");
        CHECK_SYM_TOKEN(ti, "foo", 1, 1);
        CHECK_SYM_TOKEN(ti, "bar", 2, 1);
        CHECK_SYM_TOKEN(ti, "baz", 3, 1);
        CHECK_END_TOKEN(ti,        4, 1);
    }
	
    CHECK_ONE_SPECIAL_SYM("@");
    CHECK_ONE_SPECIAL_SYM("(");
    CHECK_ONE_SPECIAL_SYM(")");
    CHECK_ONE_SPECIAL_SYM(",");
    CHECK_ONE_SPECIAL_SYM(";");
    CHECK_ONE_SPECIAL_SYM("{");
    CHECK_ONE_SPECIAL_SYM("}");
    CHECK_ONE_SPECIAL_SYM("[");
    CHECK_ONE_SPECIAL_SYM("]");
    CHECK_ONE_SPECIAL_SYM("#");
    CHECK_ONE_SPECIAL_SYM("$");
    CHECK_ONE_SPECIAL_SYM("?");
}

static void tfunc2() {
    CHECK_ONE_SPECIAL_SYM("-");
    CHECK_ONE_SPECIAL_SYM("--");
    CHECK_ONE_SPECIAL_SYM("-=");
    CHECK_ONE_SPECIAL_SYM("->");

    CHECK_ONE_SPECIAL_SYM("+");
    CHECK_ONE_SPECIAL_SYM("++");
    CHECK_ONE_SPECIAL_SYM("+=");
}	

static void tCommentTokens() {
    TextInput::Settings settings;
    settings.generateCommentTokens = true;

    {
        TextInput ti(TextInput::FROM_STRING, "/* comment 1 */  //comment 2", settings);
        CHECK_BLOCK_COMMENT_TOKEN(ti, " comment 1 ", 1, 1);
        CHECK_LINE_COMMENT_TOKEN(ti, "comment 2", 1, 18);
    }

    {
        TextInput ti(TextInput::FROM_STRING, "/*\n comment\n 1 */  //comment 2", settings);
        CHECK_BLOCK_COMMENT_TOKEN(ti, "\n comment\n 1 ", 1, 1);
        CHECK_LINE_COMMENT_TOKEN(ti, "comment 2", 3, 8);
    }

    settings.otherCommentCharacter = '#';
    settings.otherCommentCharacter2 = ';';

    {
        TextInput ti(TextInput::FROM_STRING, "/* comment 1 */\n;comment 2\n#comment 3  //some text", settings);
        CHECK_BLOCK_COMMENT_TOKEN(ti, " comment 1 ", 1, 1);
        CHECK_LINE_COMMENT_TOKEN(ti, "comment 2", 2, 1);
        CHECK_LINE_COMMENT_TOKEN(ti, "comment 3  //some text", 3, 1);
    }
}

static void tNewlineTokens() {
    TextInput::Settings settings;
    settings.generateNewlineTokens  = true;

    {
        TextInput ti(TextInput::FROM_STRING, "foo\nbar\r\nbaz\r", settings);
        CHECK_SYM_TOKEN(    ti, "foo",  1, 1);
        CHECK_NEWLINE_TOKEN(ti, "\n",   1, 4);
        CHECK_SYM_TOKEN(    ti, "bar",  2, 1);
        CHECK_NEWLINE_TOKEN(ti, "\r\n", 2, 4);
        CHECK_SYM_TOKEN(    ti, "baz",  3, 1);
        CHECK_NEWLINE_TOKEN(ti, "\r",   3, 4);
        CHECK_END_TOKEN(    ti,         4, 1);
    }

    settings.generateCommentTokens  = true;
    settings.otherCommentCharacter  = '#';
    settings.otherCommentCharacter2 = ';';

    {
        TextInput ti(TextInput::FROM_STRING, "/* comment 1 */\n;comment 2\r\n#comment 3  //some text\r", settings);
        CHECK_BLOCK_COMMENT_TOKEN(ti, " comment 1 ", 1, 1);
        CHECK_NEWLINE_TOKEN(      ti, "\n",          1, strlen("/* comment 1 */") + 1);

        CHECK_LINE_COMMENT_TOKEN( ti, "comment 2",   2, 1);
        CHECK_NEWLINE_TOKEN(      ti, "\r\n",        2, strlen(";comment 2") + 1);

        CHECK_LINE_COMMENT_TOKEN( ti, "comment 3  //some text", 3, 1);
        CHECK_NEWLINE_TOKEN(      ti, "\r",                     3, strlen("#comment 3  //some text") + 1);
    }

    // test newlines without tokens
    {
        TextInput ti(TextInput::FROM_STRING, "\n\rtext\rtext\ntext\r\n");
        CHECK_SYM_TOKEN(ti, "text", 3, 1);
        CHECK_SYM_TOKEN(ti, "text", 4, 1);
        CHECK_SYM_TOKEN(ti, "text", 5, 1);
        CHECK_END_TOKEN(ti,         6, 1);
    }
}