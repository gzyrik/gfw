#include "config.h"
#include "xjson.h"
#include "xmsg.h"
#include "xtest.h"
#include <string.h>
static xssize json_parse(xJsonParser *parser, xcstr js, xsize len,
                         xJsonToken *tokens, xsize num_tokens);
static xssize json_parse_strict(xJsonParser *parser, xcstr js, xsize len,
                                xJsonToken *tokens, xsize num_tokens);

xssize x_json_parse_full (xJsonParser *parser, xcstr js, xssize len,
                          xJsonToken *tokens, xsize num_tokens, xbool nonstrict)
{
    x_return_val_if_fail(parser != NULL, X_JSON_ERROR_INVAL);
    x_return_val_if_fail(js != NULL, X_JSON_ERROR_INVAL);
    if (len < 0) len = strlen(js);
    if (nonstrict)
        return json_parse(parser, js, len, tokens, num_tokens);
    else
        return json_parse_strict(parser, js, len, tokens, num_tokens);
}
xint x_json_keycmp(xcstr json, xJsonToken *tok, xcstr s)
{
    xsize i;
    x_assert(tok->type >= X_JSON_PRIMITIVE && tok->type <= X_JSON_OBJECT);
    x_assert(json);

    i = tok->start;
    while (i<tok->end && *s && json[i] == *s)
        ++i, ++s;
    return i>=tok->end ?  0 - *s : json[i] - *s;
}
xsize x_json_skip(const xJsonToken *t, const xJsonToken* end)
{
    xsize i;
    const xJsonToken * const tag = t;
    const xsize size = t->size;
    const xJsonType type = t->type;
    ++t;
    for (i=0;i<size && t < end;++i) {
        if (type == X_JSON_OBJECT) {
            if (t->type != X_JSON_STRING)
                return end - tag;
            ++t;
        }
        t += x_json_skip(t,end);
    }
    return t - tag;
}
#define JSON_PARENT_LINKS
#undef JSON_STRICT
#include "json.h"
#define JSON_STRICT
#define json_alloc_token        json_alloc_token_strict
#define json_fill_token         json_fill_token_strict
#define json_parse_primitive    json_parse_primitive_strict
#define json_parse_string       json_parse_string_strict
#define json_parse              json_parse_strict
#include "json.h"
