#ifndef __JSON_PARSER_H__
#define __JSON_PARSER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
enum {
    JSON_PRIMITIVE      = 0,
    JSON_OBJECT         = 1,
    JSON_ARRAY          = 2,
    JSON_STRING         = 3
};

enum {
    /* Not enough tokens were provided */
    JSON_ERROR_NOMEM    = -1,
    /* Invalid character inside JSON string */
    JSON_ERROR_INVAL    = -2,
    /* The string is not a full JSON packet, more bytes expected */
    JSON_ERROR_PART     = -3
};

/**
 * JSON token description.
 * @param		type	type (JSON_OBJECT, JSON_ARRAY, JSON_STRING etc.)
 * @param		start	start position in JSON data string
 * @param		end		end position in JSON data string
 */
typedef struct {
    int         type;
    int         start;
    int         end;
    int         size;
    int         parent;
} json_t;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string
 */
typedef struct {
    /*< private >*/
    unsigned    _[3];
} json_p;

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each describing
 * a single JSON object.
 * A non-negative value is the number of tokens actually used by the parser, on succeed. 
 * otherwise JSON_ERROR_* on failed.
 */
int json_parse(json_p *parser, const char *json, unsigned len, json_t *tokens, unsigned num_tokens);


/**
 * Like strcmp, compare token key name
 */
int json_strcmp(const char *json, json_t *token, const char *key);


#ifdef __cplusplus
}
#endif

#endif /* __JSON_PARSER_H__ */
