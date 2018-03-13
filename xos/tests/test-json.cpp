#include "add-test.h"
#include <string.h>

#define TOKEN_EQ(t, tok_start, tok_end, tok_type) \
    ((t).start == tok_start \
     && (t).end == tok_end  \
     && (t).type == (tok_type))

#define TOKEN_STRING(js, t, s) (x_json_keycmp(js, &t, s) == 0)

static void test_empty()
{
    const char *js;
    int r;
    xJsonParser p;
    xJsonToken t[10];

    js = "{}";
    x_json_init(&p);
    r = x_json_parse(&p, js, t);
    x_assert(r >= 0);
    x_assert(t[0].type == X_JSON_OBJECT);
    x_assert(t[0].start == 0 && t[0].end == 2);

    js = "[]";
    x_json_init(&p);
    r = x_json_parse(&p, js, t);
    x_assert(r >= 0);
    x_assert(t[0].type == X_JSON_ARRAY);
    x_assert(t[0].start == 0 && t[0].end == 2);

    js = "{\"a\":[]}";
    x_json_init(&p);
    r = x_json_parse(&p, js, t);
    x_assert(r >= 0);
    x_assert(t[0].type == X_JSON_OBJECT && t[0].start == 0 && t[0].end == 8);
    x_assert(t[1].type == X_JSON_STRING && t[1].start == 2 && t[1].end == 3);
    x_assert(t[2].type == X_JSON_ARRAY && t[2].start == 5 && t[2].end == 7);

    js = "[{},{}]";
    x_json_init(&p);
    r = x_json_parse(&p, js, t);
    x_assert(r >= 0);
    x_assert(t[0].type == X_JSON_ARRAY && t[0].start == 0 && t[0].end == 7);
    x_assert(t[1].type == X_JSON_OBJECT && t[1].start == 1 && t[1].end == 3);
    x_assert(t[2].type == X_JSON_OBJECT && t[2].start == 4 && t[2].end == 6);
}
static void test_simple()
{
    const char *js;
    int r;
    xJsonParser p;
    xJsonToken tokens[10];

    js = "{\"a\": 0}";

    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r >= 0);
    x_assert(TOKEN_EQ(tokens[0], 0, 8, X_JSON_OBJECT));
    x_assert(TOKEN_EQ(tokens[1], 2, 3, X_JSON_STRING));
    x_assert(TOKEN_EQ(tokens[2], 6, 7, X_JSON_PRIMITIVE));

    x_assert(TOKEN_STRING(js, tokens[0], js));
    x_assert(TOKEN_STRING(js, tokens[1], "a"));
    x_assert(TOKEN_STRING(js, tokens[2], "0"));

    x_json_init(&p);
    js = "[\"a\":{},\"b\":{}]";
    r = x_json_parse(&p, js, tokens);
    x_assert(r >= 0);

    x_json_init(&p);
    js = "{\n \"Day\": 26,\n \"Month\": 9,\n \"Year\": 12\n }";
    r = x_json_parse(&p, js, tokens);
    x_assert(r >= 0);
}
static void test_primitive()
{
    int r;
    xJsonParser p;
    xJsonToken tok[10];
    const char *js;
    js = "\"boolVar\" : true";
    x_json_init(&p);
    r = x_json_parse_nonstrict(&p, js, tok);
    x_assert(r >= 0 && tok[0].type == X_JSON_STRING
             && tok[1].type == X_JSON_PRIMITIVE);
    x_assert(TOKEN_STRING(js, tok[0], "boolVar"));
    x_assert(TOKEN_STRING(js, tok[1], "true"));

    js = "\"boolVar\" : false";
    x_json_init(&p);
    r = x_json_parse_nonstrict(&p, js, tok);
    x_assert(r >= 0 && tok[0].type == X_JSON_STRING
             && tok[1].type == X_JSON_PRIMITIVE);
    x_assert(TOKEN_STRING(js, tok[0], "boolVar"));
    x_assert(TOKEN_STRING(js, tok[1], "false"));

    js = "\"intVar\" : 12345";
    x_json_init(&p);
    r = x_json_parse_nonstrict(&p, js, tok);
    x_assert(r >= 0 && tok[0].type == X_JSON_STRING
             && tok[1].type == X_JSON_PRIMITIVE);
    x_assert(TOKEN_STRING(js, tok[0], "intVar"));
    x_assert(TOKEN_STRING(js, tok[1], "12345"));

    js = "\"floatVar\" : 12.345";
    x_json_init(&p);
    r = x_json_parse_nonstrict(&p, js, tok);
    x_assert(r >= 0 && tok[0].type == X_JSON_STRING
             && tok[1].type == X_JSON_PRIMITIVE);
    x_assert(TOKEN_STRING(js, tok[0], "floatVar"));
    x_assert(TOKEN_STRING(js, tok[1], "12.345"));

    js = "\"nullVar\" : null";
    x_json_init(&p);
    r = x_json_parse_nonstrict(&p, js, tok);
    x_assert(r >= 0 && tok[0].type == X_JSON_STRING
             && tok[1].type == X_JSON_PRIMITIVE);
    x_assert(TOKEN_STRING(js, tok[0], "nullVar"));
    x_assert(TOKEN_STRING(js, tok[1], "null"));
}
static void test_string()
{
    int r;
    xJsonParser p;
    xJsonToken tok[10];
    const char *js;

    js = "\"strVar\" : \"hello world\"";
    x_json_init(&p);
    r = x_json_parse(&p, js, tok);
    x_assert(r >= 0 && tok[0].type == X_JSON_STRING
             && tok[1].type == X_JSON_STRING);
    x_assert(TOKEN_STRING(js, tok[0], "strVar"));
    x_assert(TOKEN_STRING(js, tok[1], "hello world"));

    js = "\"strVar\" : \"escapes: \\/\\r\\n\\t\\b\\f\\\"\\\\\"";
    x_json_init(&p);
    r = x_json_parse(&p, js, tok);
    x_assert(r >= 0 && tok[0].type == X_JSON_STRING
             && tok[1].type == X_JSON_STRING);
    x_assert(TOKEN_STRING(js, tok[0], "strVar"));
    x_assert(TOKEN_STRING(js, tok[1], "escapes: \\/\\r\\n\\t\\b\\f\\\"\\\\"));

    js = "\"strVar\" : \"\"";
    x_json_init(&p);
    r = x_json_parse(&p, js, tok);
    x_assert(r >= 0 && tok[0].type == X_JSON_STRING
             && tok[1].type == X_JSON_STRING);
    x_assert(TOKEN_STRING(js, tok[0], "strVar"));
    x_assert(TOKEN_STRING(js, tok[1], ""));
}
static void test_partial_string()
{
    int r;
    xJsonParser p;
    xJsonToken tok[10];
    const char *js;

    x_json_init(&p);
    js = "\"x\": \"va";
    r = x_json_parse(&p, js, tok);
    x_assert(r == X_JSON_ERROR_PART && tok[0].type == X_JSON_STRING);
    x_assert(TOKEN_STRING(js, tok[0], "x"));
    x_assert(p.toknext == 1);

    x_json_init(&p);
    {
        char js_slash[] = "\"x\": \"va\\";
        r = x_json_parse(&p, js_slash, tok);
    }
    x_assert(r == X_JSON_ERROR_PART);

    x_json_init(&p);
    {
        char js_unicode[] = "\"x\": \"va\\u";
        r = x_json_parse(&p, js_unicode, tok);
    }
    x_assert(r == X_JSON_ERROR_PART);

    js = "\"x\": \"valu";
    r = x_json_parse(&p, js, tok);
    x_assert(r == X_JSON_ERROR_PART && tok[0].type == X_JSON_STRING);
    x_assert(TOKEN_STRING(js, tok[0], "x"));
    x_assert(p.toknext == 1);

    js = "\"x\": \"value\"";
    r = x_json_parse(&p, js, tok);
    x_assert(r >= 0 && tok[0].type == X_JSON_STRING
             && tok[1].type == X_JSON_STRING);
    x_assert(TOKEN_STRING(js, tok[0], "x"));
    x_assert(TOKEN_STRING(js, tok[1], "value"));

    js = "\"x\": \"value\", \"y\": \"value y\"";
    r = x_json_parse(&p, js, tok);
    x_assert(r >= 0 && tok[0].type == X_JSON_STRING
             && tok[1].type == X_JSON_STRING && tok[2].type == X_JSON_STRING
             && tok[3].type == X_JSON_STRING);
    x_assert(TOKEN_STRING(js, tok[0], "x"));
    x_assert(TOKEN_STRING(js, tok[1], "value"));
    x_assert(TOKEN_STRING(js, tok[2], "y"));
    x_assert(TOKEN_STRING(js, tok[3], "value y"));
}
static void test_unquoted_keys()
{
    int r;
    xJsonParser p;
    xJsonToken tok[10];
    const char *js;

    x_json_init(&p);
    js = "key1: \"value\"\nkey2 : 123";

    r = x_json_parse_nonstrict(&p, js, tok);
    x_assert(r >= 0 && tok[0].type == X_JSON_PRIMITIVE
             && tok[1].type == X_JSON_STRING && tok[2].type == X_JSON_PRIMITIVE
             && tok[3].type == X_JSON_PRIMITIVE);
    x_assert(TOKEN_STRING(js, tok[0], "key1"));
    x_assert(TOKEN_STRING(js, tok[1], "value"));
    x_assert(TOKEN_STRING(js, tok[2], "key2"));
    x_assert(TOKEN_STRING(js, tok[3], "123"));
}
static void test_partial_array()
{
    int r;
    xJsonParser p;
    xJsonToken tok[10];
    const char *js;

    x_json_init(&p);
    js = "  [ 1, true, ";
    r = x_json_parse(&p, js, tok);
    x_assert(r == X_JSON_ERROR_PART && tok[0].type == X_JSON_ARRAY
             && tok[1].type == X_JSON_PRIMITIVE && tok[2].type == X_JSON_PRIMITIVE);

    js = "  [ 1, true, [123, \"hello";
    r = x_json_parse(&p, js, tok);
    x_assert(r == X_JSON_ERROR_PART && tok[0].type == X_JSON_ARRAY
             && tok[1].type == X_JSON_PRIMITIVE && tok[2].type == X_JSON_PRIMITIVE
             && tok[3].type == X_JSON_ARRAY && tok[4].type == X_JSON_PRIMITIVE);

    js = "  [ 1, true, [123, \"hello\"]";
    r = x_json_parse(&p, js, tok);
    x_assert(r == X_JSON_ERROR_PART && tok[0].type == X_JSON_ARRAY
             && tok[1].type == X_JSON_PRIMITIVE && tok[2].type == X_JSON_PRIMITIVE
             && tok[3].type == X_JSON_ARRAY && tok[4].type == X_JSON_PRIMITIVE
             && tok[5].type == X_JSON_STRING);
    /* x_assert child nodes of the 2nd array */
    x_assert(tok[3].size == 2);

    js = "  [ 1, true, [123, \"hello\"]]";
    r = x_json_parse(&p, js, tok);
    x_assert(r >= 0 && tok[0].type == X_JSON_ARRAY
             && tok[1].type == X_JSON_PRIMITIVE && tok[2].type == X_JSON_PRIMITIVE
             && tok[3].type == X_JSON_ARRAY && tok[4].type == X_JSON_PRIMITIVE
             && tok[5].type == X_JSON_STRING);
    x_assert(tok[3].size == 2);
    x_assert(tok[0].size == 3);
}
static void test_array_nomem()
{
    int i;
    int r;
    xJsonParser p;
    xJsonToken toksmall[10], toklarge[10];
    const char *js;

    js = "  [ 1, true, [123, \"hello\"]]";

    for (i = 0; i < 6; i++) {
        x_json_init(&p);
        memset(toksmall, 0, sizeof(toksmall));
        memset(toklarge, 0, sizeof(toklarge));

        r = x_json_parse_full(&p, js, -1, toksmall, i, FALSE);
        x_assert(r == X_JSON_ERROR_NOMEM);

        memcpy(toklarge, toksmall, sizeof(toksmall));

        r = x_json_parse(&p, js, toklarge);
        x_assert(r >= 0);

        x_assert(toklarge[0].type == X_JSON_ARRAY && toklarge[0].size == 3);
        x_assert(toklarge[3].type == X_JSON_ARRAY && toklarge[3].size == 2);
    }
}
static void test_objects_arrays()
{
    int r;
    xJsonParser p;
    xJsonToken tokens[10];
    const char *js;

    js = "[10}";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r == X_JSON_ERROR_INVAL);

    js = "[10]";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r >= 0);

    js = "{\"a\": 1]";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r == X_JSON_ERROR_INVAL);

    js = "{\"a\": 1}";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r >= 0);
}
static void test_issue_22()
{
    int r;
    xJsonParser p;
    xJsonToken tokens[128];
    const char *js;

    js = "{ \"height\":10, \"layers\":[ { \"data\":[6,6], \"height\":10, "
        "\"name\":\"Calque de Tile 1\", \"opacity\":1, \"type\":\"tilelayer\", "
        "\"visible\":true, \"width\":10, \"x\":0, \"y\":0 }], "
        "\"orientation\":\"orthogonal\", \"properties\": { }, \"tileheight\":32, "
        "\"tilesets\":[ { \"firstgid\":1, \"image\":\"..\\/images\\/tiles.png\", "
        "\"imageheight\":64, \"imagewidth\":160, \"margin\":0, \"name\":\"Tiles\", "
        "\"properties\":{}, \"spacing\":0, \"tileheight\":32, \"tilewidth\":32 }], "
        "\"tilewidth\":32, \"version\":1, \"width\":10 }";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r >= 0);
#if 0
    for (i = 1; tokens[i].end < tokens[0].end; i++) {
        if (tokens[i].type == X_JSON_STRING || tokens[i].type == X_JSON_PRIMITIVE) {
            printf("%.*s\n", tokens[i].end - tokens[i].start, js + tokens[i].start);
        } else if (tokens[i].type == X_JSON_ARRAY) {
            printf("[%d elems]\n", tokens[i].size);
        } else if (tokens[i].type == X_JSON_OBJECT) {
            printf("{%d elems}\n", tokens[i].size);
        } else {
            TOKEN_PRINT(tokens[i]);
        }
    }
#endif
}
static void test_unicode_characters()
{
    xJsonParser p;
    xJsonToken tokens[10];
    const char *js;

    int r;
    js = "{\"a\":\"\\uAbcD\"}";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r >= 0);

    js = "{\"a\":\"str\\u0000\"}";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r >= 0);

    js = "{\"a\":\"\\uFFFFstr\"}";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r >= 0);

    js = "{\"a\":\"str\\uFFGFstr\"}";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r == X_JSON_ERROR_INVAL);

    js = "{\"a\":\"str\\u@FfF\"}";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r == X_JSON_ERROR_INVAL);

    js = "{\"a\":[\"\\u028\"]}";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r == X_JSON_ERROR_INVAL);

    js = "{\"a\":[\"\\u0280\"]}";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r >= 0);
}
static void test_input_length()
{
    const char *js;
    int r;
    xJsonParser p;
    xJsonToken tokens[10];

    js = "{\"a\": 0}garbage";

    x_json_init(&p);
    r = x_json_nparse(&p, js, 8, tokens);
    x_assert(r == 3);
    x_assert(TOKEN_STRING(js, tokens[0], "{\"a\": 0}"));
    x_assert(TOKEN_STRING(js, tokens[1], "a"));
    x_assert(TOKEN_STRING(js, tokens[2], "0"));
}
static void test_count() {
    xJsonParser p;
    const char *js;
    xJsonToken tokens[10];

    js = "{}";
    x_json_init(&p);
    x_assert(x_json_n_tokens(&p, js) == 1);

    js = "[]";
    x_json_init(&p);
    x_assert(x_json_n_tokens(&p, js) == 1);

    js = "[[]]";
    x_json_init(&p);
    x_assert(x_json_n_tokens(&p, js) == 2);

    js = "[[], []]";
    x_json_init(&p);
    x_assert(x_json_n_tokens(&p, js) == 3);

    js = "[[], []]";
    x_json_init(&p);
    x_assert(x_json_n_tokens(&p, js) == 3);

    js = "[[], [[]], [[], []]]";
    x_json_init(&p);
    x_assert(x_json_n_tokens(&p, js) == 7);

    js = "[\"a\", [[], []]]";
    x_json_init(&p);
    x_assert(x_json_n_tokens(&p, js) == 5);

    js = "[[], \"[], [[]]\", [[]]]";
    x_json_init(&p);
    x_assert(x_json_n_tokens(&p, js) == 5);

    js = "[1, 2, 3]";
    x_json_init(&p);
    x_assert(x_json_n_tokens(&p, js) == 4);

    js = "[1, 2, [3, \"a\"], null]";
    x_json_init(&p);
    x_assert(x_json_n_tokens(&p, js) == 7);

    js = "{\"a\": 0, \"b\": \"c\"}";
    x_json_init(&p);
    x_assert(x_json_parse(&p, js, tokens) == p.toknext);
}

static void test_keyvalue() {
    const char *js;
    int r;
    xJsonParser p;
    xJsonToken tokens[10];

    js = "{\"a\": 0, \"b\": \"c\"}";

    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r == 5);
    x_assert(tokens[0].size == 2); /* two keys */
    x_assert(tokens[1].size == 1 && tokens[3].size == 1); /* one value per key */
    x_assert(tokens[2].size == 0 && tokens[4].size == 0); /* values have zero size */

    js = "{\"a\"\n0}";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r == X_JSON_ERROR_INVAL);

    js = "{\"a\", 0}";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r == X_JSON_ERROR_INVAL);

    js = "{\"a\": {2}}";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r == X_JSON_ERROR_INVAL);

    js = "{\"a\": {2: 3}}";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r == X_JSON_ERROR_INVAL);


    js = "{\"a\": {\"a\": 2 3}}";
    x_json_init(&p);
    r = x_json_parse(&p, js, tokens);
    x_assert(r == X_JSON_ERROR_INVAL);
}
static void test_nonstrict() {
    const char *js;
    int r;
    xJsonParser p;
    xJsonToken tokens[10];

    js = "a: 0garbage";

    x_json_init(&p);
    r = x_json_nparse_nonstrict(&p, js, 4, tokens);
    x_assert(r == 2);
    x_assert(TOKEN_STRING(js, tokens[0], "a"));
    x_assert(TOKEN_STRING(js, tokens[1], "0"));

    js = "Day : 26\nMonth : Sep\n\nYear: 12";
    x_json_init(&p);
    r = x_json_parse_nonstrict(&p, js, tokens);
    x_assert(r == 6);
}

ADD_TEST("/xos/json/empty", test_empty, "General test for a empty JSON objects/arrays");
ADD_TEST("/xos/json/simple", test_simple, "General test for a simple JSON string");
ADD_TEST("/xos/json/primitive", test_primitive, "Primitive JSON data parsing");
ADD_TEST("/xos/json/string", test_string, "String JSON data parsing");
ADD_TEST("/xos/json/partial-str", test_partial_string, "Partial JSON string parsing");
ADD_TEST("/xos/json/unquoted-keys", test_unquoted_keys, "Unquoted keys (like in JavaScript)");
ADD_TEST("/xos/json/partial-array", test_partial_array, "Partial array reading");
ADD_TEST("/xos/json/array-nomem", test_array_nomem, "Array reading with a smaller number of tokens");
ADD_TEST("/xos/json/objects-arrays", test_objects_arrays, "Test objects and arrays");
ADD_TEST("/xos/json/issue-22", test_issue_22, "Test issue #22");
ADD_TEST("/xos/json/unicode-characters", test_unicode_characters, "Test unicode characters");
ADD_TEST("/xos/json/input-length", test_input_length, "Test strings that are not null-terminated");
ADD_TEST("/xos/json/count", test_count, "Test tokens count estimation");
ADD_TEST("/xos/json/keyvalue", test_keyvalue, "Test for keys/values");
ADD_TEST("/xos/json/nonstrict", test_nonstrict, "Test for non-strict mode");
