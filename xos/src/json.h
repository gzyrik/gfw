/**
 * Allocates a fresh unused token from the token pull.
 */
static xJsonToken *json_alloc_token(xJsonParser *parser,
                                    xJsonToken *tokens, xsize num_tokens)
{
    xJsonToken *tok;
    if (parser->toknext >= num_tokens) {
        return NULL;
    }
    tok = &tokens[parser->toknext++];
    tok->start = tok->end = -1;
    tok->size = 0;
#ifdef JSON_PARENT_LINKS
    tok->parent = -1;
#endif
    return tok;
}

/**
 * Fills token type and boundaries.
 */
static void json_fill_token(xJsonToken *token, int type,
                            xsize start, xsize end)
{
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;
}

/**
 * Fills next available token with JSON primitive.
 */
static int json_parse_primitive(xJsonParser *parser, const char *js,
                                xsize len, xJsonToken *tokens, xsize num_tokens)
{
    xJsonToken *token;
    xsize start;

    start = parser->pos;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        switch (js[parser->pos]) {
#ifndef JSON_STRICT
            /* In strict mode primitive must be followed by "," or "}" or "]" */
        case ':':
#endif
        case '\t' : case '\r' : case '\n' : case ' ' :
        case ','  : case ']'  : case '}' :
            goto found;
        }
        if (js[parser->pos] < 32 || js[parser->pos] >= 127) {
            parser->pos = start;
            return X_JSON_ERROR_INVAL;
        }
    }
#ifdef JSON_STRICT
    /* In strict mode primitive must be followed by a comma/object/array */
    parser->pos = start;
    return X_JSON_ERROR_PART;
#endif

found:
    if (tokens == NULL) {
        parser->pos--;
        return 0;
    }
    token = json_alloc_token(parser, tokens, num_tokens);
    if (token == NULL) {
        parser->pos = start;
        return X_JSON_ERROR_NOMEM;
    }
    json_fill_token(token, X_JSON_PRIMITIVE, start, parser->pos);
#ifdef JSON_PARENT_LINKS
    token->parent = parser->toksuper1-1;
#endif
    parser->pos--;
    return 0;
}

/**
 * Filsl next token with JSON string.
 */
static int json_parse_string(xJsonParser *parser, const char *js,
                             xsize len, xJsonToken *tokens, xsize num_tokens)
{
    xJsonToken *token;

    xsize start = parser->pos;

    parser->pos++;

    /* Skip starting quote */
    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        char c = js[parser->pos];

        /* Quote: end of string */
        if (c == '\"') {
            if (tokens == NULL) {
                return 0;
            }
            token = json_alloc_token(parser, tokens, num_tokens);
            if (token == NULL) {
                parser->pos = start;
                return X_JSON_ERROR_NOMEM;
            }
            json_fill_token(token, X_JSON_STRING, start+1, parser->pos);
#ifdef JSON_PARENT_LINKS
            token->parent = parser->toksuper1-1;
#endif
            return 0;
        }

        /* Backslash: Quoted symbol expected */
        if (c == '\\' && parser->pos + 1 < len) {
            int i;
            parser->pos++;
            switch (js[parser->pos]) {
                /* Allowed escaped symbols */
            case '\"': case '/' : case '\\' : case 'b' :
            case 'f' : case 'r' : case 'n'  : case 't' :
                break;
                /* Allows escaped symbol \uXXXX */
            case 'u':
                parser->pos++;
                for(i = 0; i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++) {
                    /* If it isn't a hex character we have an error */
                    if(!((js[parser->pos] >= 48 && js[parser->pos] <= 57) || /* 0-9 */
                         (js[parser->pos] >= 65 && js[parser->pos] <= 70) || /* A-F */
                         (js[parser->pos] >= 97 && js[parser->pos] <= 102))) { /* a-f */
                        parser->pos = start;
                        return X_JSON_ERROR_INVAL;
                    }
                    parser->pos++;
                }
                parser->pos--;
                break;
                /* Unexpected symbol */
            default:
                parser->pos = start;
                return X_JSON_ERROR_INVAL;
            }
        }
    }
    parser->pos = start;
    return X_JSON_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */
static xssize json_parse(xJsonParser *parser, xcstr js, xsize len,
                         xJsonToken *tokens, xsize num_tokens)
{
    int r;
    xssize i;
    xJsonToken *token;
    xsize count = parser->toknext;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        char c;
        int type;

        c = js[parser->pos];
        switch (c) {
        case '{': case '[':
            count++;
            if (tokens == NULL) {
                break;
            }
            token = json_alloc_token(parser, tokens, num_tokens);
            if (token == NULL)
                return X_JSON_ERROR_NOMEM;
            if (parser->toksuper1 != 0) {
                tokens[parser->toksuper1-1].size++;
#ifdef JSON_PARENT_LINKS
                token->parent = parser->toksuper1-1;
#endif
            }
            token->type = (c == '{' ? X_JSON_OBJECT : X_JSON_ARRAY);
            token->start = parser->pos;
            parser->toksuper1 = parser->toknext;
            break;
        case '}': case ']':
            if (tokens == NULL)
                break;
            type = (c == '}' ? X_JSON_OBJECT : X_JSON_ARRAY);
#ifdef JSON_PARENT_LINKS
            if (parser->toknext < 1) {
                return X_JSON_ERROR_INVAL;
            }
            token = &tokens[parser->toknext - 1];
            for (;;) {
                if (token->start != -1 && token->end == -1) {
                    if (token->type != type) {
                        return X_JSON_ERROR_INVAL;
                    }
                    token->end = parser->pos + 1;
                    parser->toksuper1 = token->parent+1;
                    break;
                }
                if (token->parent == -1) {
                    break;
                }
                token = &tokens[token->parent];
            }
#else
            for (i = parser->toknext - 1; i >= 0; i--) {
                token = &tokens[i];
                if (token->start != -1 && token->end == -1) {
                    if (token->type != type) {
                        return X_JSON_ERROR_INVAL;
                    }
                    parser->toksuper1 = 0;
                    token->end = parser->pos + 1;
                    break;
                }
            }
            /* Error if unmatched closing bracket */
            if (i == -1) return X_JSON_ERROR_INVAL;
            for (; i >= 0; i--) {
                token = &tokens[i];
                if (token->start != -1 && token->end == -1) {
                    parser->toksuper1 = i+1;
                    break;
                }
            }
#endif
            break;
        case '\"':
            r = json_parse_string(parser, js, len, tokens, num_tokens);
            if (r < 0) return r;
            count++;
            if (parser->toksuper1 != 0 && tokens != NULL)
                tokens[parser->toksuper1-1].size++;
            break;
        case '\t' : case '\r' : case '\n' : case ' ':
            break;
        case ':':
            parser->toksuper1 = parser->toknext;
            break;
        case ',':
            if (tokens != NULL &&
                tokens[parser->toksuper1-1].type != X_JSON_ARRAY &&
                tokens[parser->toksuper1-1].type != X_JSON_OBJECT) {
#ifdef JSON_PARENT_LINKS
                parser->toksuper1 = tokens[parser->toksuper1-1].parent+1;
#else
                for (i = parser->toknext - 1; i >= 0; i--) {
                    if (tokens[i].type == X_JSON_ARRAY || tokens[i].type == X_JSON_OBJECT) {
                        if (tokens[i].start != -1 && tokens[i].end == -1) {
                            parser->toksuper1 = i+1;
                            break;
                        }
                    }
                }
#endif
            }
            break;
#ifdef JSON_STRICT
            /* In strict mode primitives are: numbers and booleans */
        case '-': case '0': case '1' : case '2': case '3' : case '4':
        case '5': case '6': case '7' : case '8': case '9':
        case 't': case 'f': case 'n' :
            /* And they must not be keys of the object */
            if (tokens != NULL) {
                xJsonToken *t = &tokens[parser->toksuper1-1];
                if (t->type == X_JSON_OBJECT ||
                    (t->type == X_JSON_STRING && t->size != 0)) {
                    return X_JSON_ERROR_INVAL;
                }
            }
#else
            /* In non-strict mode every unquoted value is a primitive */
        default:
#endif
            r = json_parse_primitive(parser, js, len, tokens, num_tokens);
            if (r < 0) return r;
            count++;
            if (parser->toksuper1 != 0 && tokens != NULL)
                tokens[parser->toksuper1-1].size++;
            break;

#ifdef JSON_STRICT
            /* Unexpected char in strict mode */
        default:
            return X_JSON_ERROR_INVAL;
#endif
        }
    }

    for (i = parser->toknext - 1; i >= 0; i--) {
        /* Unmatched opened object or array */
        if (tokens[i].start != -1 && tokens[i].end == -1) {
            return X_JSON_ERROR_PART;
        }
    }
    if(tokens && count != parser->toknext)
        return X_JSON_ERROR_INVAL;
    return count;
}
