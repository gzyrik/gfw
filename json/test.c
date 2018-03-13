#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_passed = 0;
static int test_failed = 0;
#define json_parse2(p, js, len, t, ts)  json_parse((json_p*)p, js, len, t, ts)
#define json_init(p)    memset(p, 0, sizeof(json_parser))
/* Terminate current test with error */
#define fail()	return __LINE__

/* Successfull end of the test case */
#define done() return 0

/* Check single condition */
#define check(cond) do { if (!(cond)) fail(); } while (0)

/* Test runner */
static void test(int (*func)(), const char *name) {
    int r = func();
    if (r == 0) {
        test_passed++;
    } else {
        test_failed++;
        printf("FAILED: %s (at line %d)\n", name, r);
    }
}

#define TOKEN_EQ(t, tok_start, tok_end, tok_type) \
    ((t).start == tok_start \
     && (t).end == tok_end  \
     && (t).type == (tok_type))

#define TOKEN_STRING(js, t, s) \
    (strncmp(js+(t).start, s, (t).end - (t).start) == 0 \
     && strlen(s) == (t).end - (t).start)

#define TOKEN_PRINT(t) \
    printf("start: %d, end: %d, type: %d, size: %d\n", \
           (t).start, (t).end, (t).type, (t).size)

#define JSON_STRICT
#include "json.c"

int test_empty() {
    const char *js;
    int r;
    json_parser p;
    json_t t[10];

    js = "{}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), t, 10);
    check(r >= 0);
    check(t[0].type == JSON_OBJECT);
    check(t[0].start == 0 && t[0].end == 2);

    js = "[]";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), t, 10);
    check(r >= 0);
    check(t[0].type == JSON_ARRAY);
    check(t[0].start == 0 && t[0].end == 2);

    js = "{\"a\":[]}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), t, 10);
    check(r >= 0);
    check(t[0].type == JSON_OBJECT && t[0].start == 0 && t[0].end == 8);
    check(t[1].type == JSON_STRING && t[1].start == 2 && t[1].end == 3);
    check(t[2].type == JSON_ARRAY && t[2].start == 5 && t[2].end == 7);

    js = "[{},{}]";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), t, 10);
    check(r >= 0);
    check(t[0].type == JSON_ARRAY && t[0].start == 0 && t[0].end == 7);
    check(t[1].type == JSON_OBJECT && t[1].start == 1 && t[1].end == 3);
    check(t[2].type == JSON_OBJECT && t[2].start == 4 && t[2].end == 6);
    return 0;
}

int test_simple() {
    const char *js;
    int r;
    json_parser p;
    json_t tokens[10];

    js = "{\"a\": 0}";

    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r >= 0);
    check(TOKEN_EQ(tokens[0], 0, 8, JSON_OBJECT));
    check(TOKEN_EQ(tokens[1], 2, 3, JSON_STRING));
    check(TOKEN_EQ(tokens[2], 6, 7, JSON_PRIMITIVE));

    check(TOKEN_STRING(js, tokens[0], js));
    check(TOKEN_STRING(js, tokens[1], "a"));
    check(TOKEN_STRING(js, tokens[2], "0"));

    json_init(&p);
    js = "[\"a\":{},\"b\":{}]";
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r >= 0);

    json_init(&p);
    js = "{\n \"Day\": 26,\n \"Month\": 9,\n \"Year\": 12\n }";
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r >= 0);

    return 0;
}

int test_primitive() {
#ifndef JSON_STRICT
    int r;
    json_parser p;
    json_t tok[10];
    const char *js;
    js = "\"boolVar\" : true";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r >= 0 && tok[0].type == JSON_STRING
          && tok[1].type == JSON_PRIMITIVE);
    check(TOKEN_STRING(js, tok[0], "boolVar"));
    check(TOKEN_STRING(js, tok[1], "true"));

    js = "\"boolVar\" : false";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r >= 0 && tok[0].type == JSON_STRING
          && tok[1].type == JSON_PRIMITIVE);
    check(TOKEN_STRING(js, tok[0], "boolVar"));
    check(TOKEN_STRING(js, tok[1], "false"));

    js = "\"intVar\" : 12345";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r >= 0 && tok[0].type == JSON_STRING
          && tok[1].type == JSON_PRIMITIVE);
    check(TOKEN_STRING(js, tok[0], "intVar"));
    check(TOKEN_STRING(js, tok[1], "12345"));

    js = "\"floatVar\" : 12.345";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r >= 0 && tok[0].type == JSON_STRING
          && tok[1].type == JSON_PRIMITIVE);
    check(TOKEN_STRING(js, tok[0], "floatVar"));
    check(TOKEN_STRING(js, tok[1], "12.345"));

    js = "\"nullVar\" : null";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r >= 0 && tok[0].type == JSON_STRING
          && tok[1].type == JSON_PRIMITIVE);
    check(TOKEN_STRING(js, tok[0], "nullVar"));
    check(TOKEN_STRING(js, tok[1], "null"));
#endif
    return 0;
}

int test_string() {
    int r;
    json_parser p;
    json_t tok[10];
    const char *js;

    js = "\"strVar\" : \"hello world\"";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r >= 0 && tok[0].type == JSON_STRING
          && tok[1].type == JSON_STRING);
    check(TOKEN_STRING(js, tok[0], "strVar"));
    check(TOKEN_STRING(js, tok[1], "hello world"));

    js = "\"strVar\" : \"escapes: \\/\\r\\n\\t\\b\\f\\\"\\\\\"";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r >= 0 && tok[0].type == JSON_STRING
          && tok[1].type == JSON_STRING);
    check(TOKEN_STRING(js, tok[0], "strVar"));
    check(TOKEN_STRING(js, tok[1], "escapes: \\/\\r\\n\\t\\b\\f\\\"\\\\"));

    js = "\"strVar\" : \"\"";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r >= 0 && tok[0].type == JSON_STRING
          && tok[1].type == JSON_STRING);
    check(TOKEN_STRING(js, tok[0], "strVar"));
    check(TOKEN_STRING(js, tok[1], ""));

    return 0;
}

int test_partial_string() {
    int r;
    json_parser p;
    json_t tok[10];
    const char *js;

    json_init(&p);
    js = "\"x\": \"va";
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r == JSON_ERROR_PART && tok[0].type == JSON_STRING);
    check(TOKEN_STRING(js, tok[0], "x"));
    check(p.toknext == 1);

    json_init(&p);
    {
        char js_slash[9] = "\"x\": \"va\\";
        r = json_parse2(&p, js_slash, sizeof(js_slash), tok, 10);
    }
    check(r == JSON_ERROR_PART);

    json_init(&p);
    {
        char js_unicode[10] = "\"x\": \"va\\u";
        r = json_parse2(&p, js_unicode, sizeof(js_unicode), tok, 10);
    }
    check(r == JSON_ERROR_PART);

    js = "\"x\": \"valu";
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r == JSON_ERROR_PART && tok[0].type == JSON_STRING);
    check(TOKEN_STRING(js, tok[0], "x"));
    check(p.toknext == 1);

    js = "\"x\": \"value\"";
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r >= 0 && tok[0].type == JSON_STRING
          && tok[1].type == JSON_STRING);
    check(TOKEN_STRING(js, tok[0], "x"));
    check(TOKEN_STRING(js, tok[1], "value"));

    js = "\"x\": \"value\", \"y\": \"value y\"";
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r >= 0 && tok[0].type == JSON_STRING
          && tok[1].type == JSON_STRING && tok[2].type == JSON_STRING
          && tok[3].type == JSON_STRING);
    check(TOKEN_STRING(js, tok[0], "x"));
    check(TOKEN_STRING(js, tok[1], "value"));
    check(TOKEN_STRING(js, tok[2], "y"));
    check(TOKEN_STRING(js, tok[3], "value y"));

    return 0;
}

int test_unquoted_keys() {
#ifndef JSON_STRICT
    int r;
    json_parser p;
    json_t tok[10];
    const char *js;

    json_init(&p);
    js = "key1: \"value\"\nkey2 : 123";

    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r >= 0 && tok[0].type == JSON_PRIMITIVE
          && tok[1].type == JSON_STRING && tok[2].type == JSON_PRIMITIVE
          && tok[3].type == JSON_PRIMITIVE);
    check(TOKEN_STRING(js, tok[0], "key1"));
    check(TOKEN_STRING(js, tok[1], "value"));
    check(TOKEN_STRING(js, tok[2], "key2"));
    check(TOKEN_STRING(js, tok[3], "123"));
#endif
    return 0;
}

int test_partial_array() {
    int r;
    json_parser p;
    json_t tok[10];
    const char *js;

    json_init(&p);
    js = "  [ 1, true, ";
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r == JSON_ERROR_PART && tok[0].type == JSON_ARRAY
          && tok[1].type == JSON_PRIMITIVE && tok[2].type == JSON_PRIMITIVE);

    js = "  [ 1, true, [123, \"hello";
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r == JSON_ERROR_PART && tok[0].type == JSON_ARRAY
          && tok[1].type == JSON_PRIMITIVE && tok[2].type == JSON_PRIMITIVE
          && tok[3].type == JSON_ARRAY && tok[4].type == JSON_PRIMITIVE);

    js = "  [ 1, true, [123, \"hello\"]";
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r == JSON_ERROR_PART && tok[0].type == JSON_ARRAY
          && tok[1].type == JSON_PRIMITIVE && tok[2].type == JSON_PRIMITIVE
          && tok[3].type == JSON_ARRAY && tok[4].type == JSON_PRIMITIVE
          && tok[5].type == JSON_STRING);
    /* check child nodes of the 2nd array */
    check(tok[3].size == 2);

    js = "  [ 1, true, [123, \"hello\"]]";
    r = json_parse2(&p, js, strlen(js), tok, 10);
    check(r >= 0 && tok[0].type == JSON_ARRAY
          && tok[1].type == JSON_PRIMITIVE && tok[2].type == JSON_PRIMITIVE
          && tok[3].type == JSON_ARRAY && tok[4].type == JSON_PRIMITIVE
          && tok[5].type == JSON_STRING);
    check(tok[3].size == 2);
    check(tok[0].size == 3);
    return 0;
}

int test_array_nomem() {
    int i;
    int r;
    json_parser p;
    json_t toksmall[10], toklarge[10];
    const char *js;

    js = "  [ 1, true, [123, \"hello\"]]";

    for (i = 0; i < 6; i++) {
        json_init(&p);
        memset(toksmall, 0, sizeof(toksmall));
        memset(toklarge, 0, sizeof(toklarge));
        r = json_parse2(&p, js, strlen(js), toksmall, i);
        check(r == JSON_ERROR_NOMEM);

        memcpy(toklarge, toksmall, sizeof(toksmall));

        r = json_parse2(&p, js, strlen(js), toklarge, 10);
        check(r >= 0);

        check(toklarge[0].type == JSON_ARRAY && toklarge[0].size == 3);
        check(toklarge[3].type == JSON_ARRAY && toklarge[3].size == 2);
    }
    return 0;
}

int test_objects_arrays() {
    int r;
    json_parser p;
    json_t tokens[10];
    const char *js;

    js = "[10}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r == JSON_ERROR_INVAL);

    js = "[10]";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r >= 0);

    js = "{\"a\": 1]";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r == JSON_ERROR_INVAL);

    js = "{\"a\": 1}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r >= 0);

    return 0;
}

int test_issue_22() {
    int r;
    json_parser p;
    json_t tokens[128];
    const char *js;

    js = "{ \"height\":10, \"layers\":[ { \"data\":[6,6], \"height\":10, "
        "\"name\":\"Calque de Tile 1\", \"opacity\":1, \"type\":\"tilelayer\", "
        "\"visible\":true, \"width\":10, \"x\":0, \"y\":0 }], "
        "\"orientation\":\"orthogonal\", \"properties\": { }, \"tileheight\":32, "
        "\"tilesets\":[ { \"firstgid\":1, \"image\":\"..\\/images\\/tiles.png\", "
        "\"imageheight\":64, \"imagewidth\":160, \"margin\":0, \"name\":\"Tiles\", "
        "\"properties\":{}, \"spacing\":0, \"tileheight\":32, \"tilewidth\":32 }], "
        "\"tilewidth\":32, \"version\":1, \"width\":10 }";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 128);
    check(r >= 0);
#if 0
    for (i = 1; tokens[i].end < tokens[0].end; i++) {
        if (tokens[i].type == JSON_STRING || tokens[i].type == JSON_PRIMITIVE) {
            printf("%.*s\n", tokens[i].end - tokens[i].start, js + tokens[i].start);
        } else if (tokens[i].type == JSON_ARRAY) {
            printf("[%d elems]\n", tokens[i].size);
        } else if (tokens[i].type == JSON_OBJECT) {
            printf("{%d elems}\n", tokens[i].size);
        } else {
            TOKEN_PRINT(tokens[i]);
        }
    }
#endif
    return 0;
}

int test_unicode_characters() {
    json_parser p;
    json_t tokens[10];
    const char *js;

    int r;
    js = "{\"a\":\"\\uAbcD\"}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r >= 0);

    js = "{\"a\":\"str\\u0000\"}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r >= 0);

    js = "{\"a\":\"\\uFFFFstr\"}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r >= 0);

    js = "{\"a\":\"str\\uFFGFstr\"}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r == JSON_ERROR_INVAL);

    js = "{\"a\":\"str\\u@FfF\"}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r == JSON_ERROR_INVAL);

    js = "{\"a\":[\"\\u028\"]}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r == JSON_ERROR_INVAL);

    js = "{\"a\":[\"\\u0280\"]}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r >= 0);

    return 0;
}

int test_input_length() {
    const char *js;
    int r;
    json_parser p;
    json_t tokens[10];

    js = "{\"a\": 0}garbage";

    json_init(&p);
    r = json_parse2(&p, js, 8, tokens, 10);
    check(r == 3);
    check(TOKEN_STRING(js, tokens[0], "{\"a\": 0}"));
    check(TOKEN_STRING(js, tokens[1], "a"));
    check(TOKEN_STRING(js, tokens[2], "0"));

    return 0;
}

int test_count() {
    json_parser p;
    const char *js;
    json_t tokens[10];

    js = "{}";
    json_init(&p);
    check(json_parse2(&p, js, strlen(js), NULL, 0) == 1);

    js = "[]";
    json_init(&p);
    check(json_parse2(&p, js, strlen(js), NULL, 0) == 1);

    js = "[[]]";
    json_init(&p);
    check(json_parse2(&p, js, strlen(js), NULL, 0) == 2);

    js = "[[], []]";
    json_init(&p);
    check(json_parse2(&p, js, strlen(js), NULL, 0) == 3);

    js = "[[], []]";
    json_init(&p);
    check(json_parse2(&p, js, strlen(js), NULL, 0) == 3);

    js = "[[], [[]], [[], []]]";
    json_init(&p);
    check(json_parse2(&p, js, strlen(js), NULL, 0) == 7);

    js = "[\"a\", [[], []]]";
    json_init(&p);
    check(json_parse2(&p, js, strlen(js), NULL, 0) == 5);

    js = "[[], \"[], [[]]\", [[]]]";
    json_init(&p);
    check(json_parse2(&p, js, strlen(js), NULL, 0) == 5);

    js = "[1, 2, 3]";
    json_init(&p);
    check(json_parse2(&p, js, strlen(js), NULL, 0) == 4);

    js = "[1, 2, [3, \"a\"], null]";
    json_init(&p);
    check(json_parse2(&p, js, strlen(js), NULL, 0) == 7);

    js = "{\"a\": 0, \"b\": \"c\"}";
    json_init(&p);
    check(json_parse2(&p, js, strlen(js), tokens, 10) == p.toknext);
    return 0;
}

int test_keyvalue() {
    const char *js;
    int r;
    json_parser p;
    json_t tokens[10];

    js = "{\"a\": 0, \"b\": \"c\"}";

    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r == 5);
    check(tokens[0].size == 2); /* two keys */
    check(tokens[1].size == 1 && tokens[3].size == 1); /* one value per key */
    check(tokens[2].size == 0 && tokens[4].size == 0); /* values have zero size */

    js = "{\"a\"\n0}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r == JSON_ERROR_INVAL);

    js = "{\"a\", 0}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r == JSON_ERROR_INVAL);

    js = "{\"a\": {2}}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r == JSON_ERROR_INVAL);

    js = "{\"a\": {2: 3}}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r == JSON_ERROR_INVAL);


    js = "{\"a\": {\"a\": 2 3}}";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r == JSON_ERROR_INVAL);
    return 0;
}

/** A huge redefinition of everything to include json in non-script mode */
#define json_parser         json_parser_nonstrict
#define json_parse          json_parse_nonstrict
#define json_alloc_token    json_alloc_token_nonstrict
#define json_fill_token     json_fill_token_nonstrict
#define json_parse_primitive json_parse_primitive_nonstrict
#define json_parse_string   json_parse_string_nonstrict
#define json_strcmp         json_strcmp_nonstrict
#undef JSON_STRICT
#include "json.c"

int test_nonstrict() {
    const char *js;
    int r;
    json_parser p;
    json_t tokens[10];

    js = "a: 0garbage";

    json_init(&p);
    r = json_parse2(&p, js, 4, tokens, 10);
    check(r == 2);
    check(TOKEN_STRING(js, tokens[0], "a"));
    check(TOKEN_STRING(js, tokens[1], "0"));

    js = "Day : 26\nMonth : Sep\n\nYear: 12";
    json_init(&p);
    r = json_parse2(&p, js, strlen(js), tokens, 10);
    check(r == 6);
    return 0;
}

int main() {
    test(test_empty, "general test for a empty JSON objects/arrays");
    test(test_simple, "general test for a simple JSON string");
    test(test_primitive, "test primitive JSON data types");
    test(test_string, "test string JSON data types");
    test(test_partial_string, "test partial JSON string parsing");
    test(test_partial_array, "test partial array reading");
    test(test_array_nomem, "test array reading with a smaller number of tokens");
    test(test_unquoted_keys, "test unquoted keys (like in JavaScript)");
    test(test_objects_arrays, "test objects and arrays");
    test(test_unicode_characters, "test unicode characters");
    test(test_input_length, "test strings that are not null-terminated");
    test(test_issue_22, "test issue #22");
    test(test_count, "test tokens count estimation");
    test(test_nonstrict, "test for non-strict mode");
    test(test_keyvalue, "test for keys/values");
    printf("\nPASSED: %d\nFAILED: %d\n", test_passed, test_failed);
    return 0;
}

