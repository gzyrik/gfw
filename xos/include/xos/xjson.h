#ifndef __X_JSON_H__
#define __X_JSON_H__
#include "xtype.h"
X_BEGIN_DECLS

/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
typedef enum {
    X_JSON_PRIMITIVE    = 0,
    X_JSON_STRING       = 1,
    X_JSON_ARRAY        = 2,
    X_JSON_OBJECT       = 3,
} xJsonType;

enum {
    /* Not enough tokens were provided */
    X_JSON_ERROR_NOMEM    = -1,
    /* Invalid character inside JSON string */
    X_JSON_ERROR_INVAL    = -2,
    /* The string is not a full JSON packet, more bytes expected */
    X_JSON_ERROR_PART     = -3
};

/**
 * JSON token description.
 * @param		type	type (JSON_OBJECT, JSON_ARRAY, JSON_STRING etc.)
 * @param		start	start position in JSON data string
 * @param		end		end position in JSON data string
 */
struct _xJsonToken
{
    xJsonType       type;
    xsize           start;
    xsize           end;
    xsize           size;
    xssize          parent;
};

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string
 */
struct _xJsonParser
{
    /*< private >*/
    xsize pos; /* offset in the JSON string */
    xsize toknext; /* next token to allocate */
    xsize toksuper1; /* superior token node, e.g parent object or array, start from 1, 0 is no parent */
};

#define x_json_init(parser)   memset(parser, 0, sizeof(xJsonParser))

#define x_json_parse(parser, json, tokens) \
    x_json_parse_full(parser, json, -1, tokens, X_N_ELEMENTS(tokens), FALSE)

#define x_json_nparse(parser, json, len, tokens) \
    x_json_parse_full(parser, json, len, tokens, X_N_ELEMENTS(tokens), FALSE)

#define x_json_parse_nonstrict(parser, json, tokens) \
    x_json_parse_full(parser, json, -1, tokens, X_N_ELEMENTS(tokens), TRUE)

#define x_json_nparse_nonstrict(parser, json, len, tokens) \
    x_json_parse_full(parser, json, len, tokens, X_N_ELEMENTS(tokens), TRUE)

#define x_json_n_tokens(parser, json) \
    x_json_parse_full(parser, json, -1, NULL, 0, TRUE)

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each describing
 * a single JSON object.
 * A non-negative value is the number of tokens actually used by the parser, on succeed. 
 * otherwise JSON_ERROR_* on failed.
 */
xssize x_json_parse_full(xJsonParser *parser,
                         xcstr json, xssize len,
                         xJsonToken *tokens, xsize num_tokens,
                         xbool nonstrict);

/**
 * Like strcmp, compare token key name
 */
xint x_json_keycmp(xcstr json, xJsonToken *token, xcstr key);

/**
 * Skip token
 */
xsize x_json_skip(const xJsonToken *token, const xJsonToken* token_end);

X_END_DECLS
#endif /* __X_JSON_H__ */
