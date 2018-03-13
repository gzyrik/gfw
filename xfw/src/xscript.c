#include "config.h"
#include "xarchive.h"
#include "xscript.h"
#include "xresource.h"
#include <string.h>
#include <ctype.h>
struct _xScript
{
    xHashTable      *children;
    xScriptEnter    on_enter;
    xScriptVisit    on_visit;
    xScriptLeave    on_leave;
};
#define MAX_NAME_LEN  255
struct parser{
    xchar       key[MAX_NAME_LEN+1];
    xchar       val[MAX_NAME_LEN+1];
    xTrash      *stack;
    xcstr       group;
};

struct context {
    xTrash      *next;
    xQuark      name;
    xptr        object;
    xScript     *script;
};
static xScript _root;
static xptr dft_enter(xptr object, xcstr val, xcstr group, xQuark name)
{
    return object;
}
static void dft_visit(xptr object, xcstr key, xcstr val)
{
    x_warning("visit unknown script `:%s %s'", key, val);
}
static void dft_leave(xptr object, xptr parent, xcstr group)
{
}

static xScript* script_get (xScript *parent, xQuark name)
{
    if (name != 0) {
        if (!parent)
            parent = &_root;
        if (parent->children)
            return x_hash_table_lookup (parent->children, X_SIZE_TO_PTR(name));
    }
    return NULL;
}

static void
cache_read_val          (xCache         *cache,
                         xchar          *buf);
static void
parser_read_chunk       (xCache         *cache,
                         struct parser  *parser);
static int
cache_skip_space        (xCache         *cache)
{
    xint    ch;

    while ((ch = x_cache_getc (cache)) != -1) {
        if (ch == ';') {/* skip comment */
            do {
                if ((ch = x_cache_getc (cache)) == -1)
                    return ch;
            } while (ch != '\n');
        }
        else if (!isspace (ch)) {
            x_cache_back (cache, 1);
            break;
        }
    }
    return ch;
}

static void
cache_read_key          (xCache         *cache,
                         xchar          *buf)
{
    xint    ch;

    ch = cache_skip_space (cache);
    if (ch == ':') {
        x_cache_skip (cache, 1);
        cache_read_val (cache, buf);
    }
    else  {
        buf[0] = '\0';
    }
}

static xint
unespace                (xCache         *cache)
{
    xint i, ch;

    switch (ch = x_cache_getc (cache)) {
    default:    return ch;
    case 'n':   return '\n';
    case 'r':   return '\r';
    case 't':   return '\t';
    case 'x':
                i=0;
                while ((ch = x_cache_getc (cache)) != -1) {
                    if ( ch >= '0' && ch <= '9' )
                        i = (i<<4)+ (ch - '0');
                    else if( ch >= 'a' && ch <= 'f' )
                        i = (i<<4) + (ch - 'a') + 10;
                    else if( ch >= 'A' && ch <= 'F' )
                        i = (i<<4) + (ch - 'A') + 10;
                    else
                        break;
                }
                break;
    case 'o':
    case 'O':
                i=0;
                while ((ch = x_cache_getc (cache)) != -1) {
                    if(ch < '0' || ch > '7')
                        break;
                    i = (i << 3) + ch - '0';
                }
                break;
    case 'b':
    case 'B':
                i=0;
                while ((ch = x_cache_getc (cache)) != -1) {
                    if(ch != '0' && ch != '1') break;
                    i = (i<<1) + ch-'0'; 
                }
                break;
    }
    if (ch != -1)
        x_cache_back (cache, 1);
    return i;
}

static void
cache_read_str          (xCache         *cache,
                         xchar          *buf)
{
    xint    ch, i = 0;
    xchar   quot = x_cache_getc (cache);

    while ((ch = x_cache_getc (cache)) != -1) {
        if (ch == '\\') {
            ch = unespace(cache);
            if (ch == -1)
                break;
        }
        else if (ch == quot)
            break;

        if (i>=MAX_NAME_LEN)
            x_error ("override name len");
        buf[i++] = ch;
    }
    buf[i++] = '\0';
}

static void
cache_read_exp          (xCache         *cache,
                         xchar          *buf)
{
    xint ch, quot, i = 0;

    buf[i++] = x_cache_getc (cache);
    quot = buf[0] + 2;/* '['->']' or '{'->'}' or '<'->'>' */
    while ((ch = cache_skip_space (cache)) != -1) {
        if (ch == quot)
            break;
        while (TRUE) {
            ch = x_cache_getc (cache);
            if (ch == quot || ch == -1)
                goto finish;
            else if (isspace (ch))
                break;
            if (i>=MAX_NAME_LEN)
                x_error ("override name len");
            buf[i++] = ch;
        }
        buf[i++] = ' ';
    }
finish:
    buf[i++]=quot;
    buf[i] = '\0';
}

static void
cache_read_sym          (xCache         *cache,
                         xchar          *buf)
{
    xint    ch, i = 0;

    while ((ch = x_cache_getc (cache)) != -1
           && !isspace (ch)) {
        if (ch == '(' || ch == ')') {
            x_cache_back(cache, 1);
            break;
        }
        if (i>=MAX_NAME_LEN)
            x_error ("override name len");
        buf[i++] = ch;
    }
    buf[i++] = '\0';
}

static void
cache_read_val          (xCache         *cache,
                         xchar          *buf)
{
    xint ch = cache_skip_space (cache);
    if (ch == '"' || ch == '\'')
        cache_read_str (cache, buf);
    else if (ch == '[' || ch == '{' || ch == '<')
        cache_read_exp (cache, buf);
    else if (ch != '(' && ch != ')' && ch != -1)
        cache_read_sym (cache, buf);
    else
        buf[0] = '\0';
}


static void
parser_inlude           (struct parser  *parser)
{
    xCache      *cache;
    xFile       *file;
    xTrash      *stack;
    struct context *cxt;

    file = x_repository_open (parser->group, parser->val, NULL);
    x_return_if_fail (file != NULL);


    cache = x_file_cache_new (file, 64);
    stack = parser->stack;
    while (cache_skip_space (cache) != -1)
        parser_read_chunk (cache, parser);

    while (stack != parser->stack) {
        cxt = (struct context*)parser->stack;
        parser->stack = cxt->next;
        x_slice_free (struct context, cxt);
    }
}

static void
parser_on_enter         (struct parser  *parser)
{
    struct context *cxt, *parent;

    parent = x_trash_peek (&parser->stack);
    cxt = x_slice_new0 (struct context);
    cxt->name   = x_quark_try (parser->key);
    cxt->object = parent->object;
    if (cxt->name)
        cxt->script = script_get (parent->script, cxt->name);
    x_trash_push (&parser->stack, cxt);

    if (cxt->script)
        cxt->object = cxt->script->on_enter (cxt->object, parser->val, parser->group, parent->name);
    else 
        x_warning("enter unknown script `:%s %s'",parser->key, parser->val);
}

static void
parser_on_visit         (struct parser  *parser)
{
    xScript *script;
    struct context *cxt;

    if (!x_strcmp (parser->key, "include")) {
        parser_inlude (parser);
        return;
    }
    cxt = x_trash_peek (&parser->stack);
    if (!cxt->script || !cxt->object)
        return; 

    script = script_get (cxt->script, x_quark_try (parser->key));
    if (script) {
        xptr object = script->on_enter (cxt->object, parser->val, parser->group, cxt->name);
        script->on_leave (object, cxt->object, parser->group);
    }
    else {
        cxt->script->on_visit (cxt->object, parser->key, parser->val);
    }
}

static void
parser_on_leave         (struct parser  *parser)
{
    struct context *cxt, *parent;
    cxt = x_trash_pop (&parser->stack);
    parent = x_trash_peek (&parser->stack);
    if (cxt->script) 
        cxt->script->on_leave (cxt->object, parent->object, parser->group);

    x_slice_free (struct context, cxt);
}

static void
parser_read_chunk       (xCache         *cache,
                         struct parser  *parser)
{
    xchar ch;
    cache_read_key (cache, parser->key);
    cache_read_val (cache, parser->val);

    if (cache_skip_space (cache) == '(') {
        x_cache_skip (cache, 1);
        parser_on_enter (parser);
        while ((ch = cache_skip_space (cache)) != ')') {
            if (ch == -1)
                x_error ("invalid syntax");
            parser_read_chunk (cache, parser);
        }
        parser_on_leave(parser);
        x_cache_skip (cache, 1);
    }
    else if (parser->key[0] || parser->val[0]) {
        parser_on_visit (parser);
    }
}
void
x_script_import         (xCache         *cache,
                         xcstr          group,
                         xptr           object)
{
    struct parser  parser;
    struct context *cxt;

    cxt = x_slice_new0 (struct context);
    cxt->object = object;
    cxt->script = &_root;
    parser.group = group;
    parser.stack = (xTrash*)cxt;

    while (cache_skip_space (cache) != -1)
        parser_read_chunk (cache, &parser);

    while (parser.stack) {
        cxt = (struct context*)parser.stack;
        parser.stack = cxt->next;
        x_slice_free (struct context, cxt);
    }
}
void
x_script_import_str     (xcstr          str,
                         xcstr          group,
                         xptr           object)
{
    xCache         *cache;
    xsize           slen;

    slen = strlen (str);

    x_return_if_fail (slen >2);

    cache = x_cache_new ((xbyte*)str, slen);

    x_script_import (cache, group, object);

    x_cache_delete (cache);
}

void
x_script_import_file    (xFile          *file,
                         xcstr          group,
                         xptr           object)
{
    xCache         *cache;

    x_return_if_fail (file != NULL);

    cache = x_file_cache_new (file, 64);

    x_script_import (cache, group, object);

    x_cache_delete (cache);
}

xScript*
x_script_set            (xScript        *parent,
                         xQuark         name,
                         xScriptEnter   on_enter,
                         xScriptVisit   on_visit,
                         xScriptLeave   on_leave)
{
    xScript *script;
    x_return_val_if_fail (name != 0, NULL);

    if (!parent)
        parent = &_root;
    if (!parent->children)
        parent->children = x_hash_table_new(x_direct_hash, x_direct_cmp);

    script = x_hash_table_lookup (parent->children, X_SIZE_TO_PTR(name));
    x_return_val_if_fail (script == NULL, NULL);

    if (!on_enter) on_enter = dft_enter;
    if (!on_visit) on_visit = dft_visit;
    if (!on_leave) on_leave = dft_leave;

    script = x_slice_new0 (xScript);
    script->on_enter = on_enter;
    script->on_visit = on_visit;
    script->on_leave = on_leave;

    x_hash_table_insert (parent->children, X_SIZE_TO_PTR(name), script);
    return script;
}

void
x_script_link           (xScript        *parent,
                         xQuark         name,
                         xScript        *child)
{
    xScript *script;
    x_return_if_fail (name != 0);
    x_return_if_fail (child != NULL);

    if (!parent)
        parent = &_root;
    if (!parent->children)
        parent->children = x_hash_table_new(x_direct_hash, x_direct_cmp);

    script = x_hash_table_lookup (parent->children, X_SIZE_TO_PTR(name));
    x_return_if_fail (script == NULL);

    x_hash_table_insert (parent->children, X_SIZE_TO_PTR(name), child);
}

xScript*
x_script_get            (xcstr          path)
{
    xsize i;
    xstrv name;
    xScript *script = &_root;
    x_return_val_if_fail (path != NULL, NULL);

    name = x_strsplit (path, "/", -1);
    x_return_val_if_fail (name != NULL, NULL);

    for (i=0; name[i]; ++i) {
        script = script_get (script, x_quark_try (name[i]));
        x_goto_clean_if_fail (script != NULL);
    }
clean:
    x_strv_free (name);
    return script;
}

struct script_indent
{
    xsize depth;
    xbool newline;
};
static xQuark indent_quark()
{
    static xQuark q;
    if X_UNLIKELY (q == 0)
        q = x_quark_from_static("indent");
    return q;
}
#define INDENT  "    "
void
x_script_begin_valist   (xFile         *file,
                         xcstr          key,
                         xcstr          value,
                         va_list        argv)
{
    xsize i;
    struct script_indent* indent;
    indent = x_object_get_qdata (file, indent_quark());
    if (!indent) {
        indent = x_new0(struct script_indent, 1);
        x_object_set_qdata_full (file, indent_quark(),
                                 indent, x_free);
    }
    else {
        if (!indent->newline)
            x_file_putc (file, '\n');
        for(i=0;i<indent->depth; ++i)
            x_file_puts(file, INDENT, FALSE);
    }

    if (key) {
        x_file_putc (file, ':');
        x_file_puts (file, key, FALSE);
    }
    if (value) {
        x_file_putc (file, ' ');
        x_file_print_valist(file, FALSE, value, argv);
    }
    x_file_puts (file, "(\n", FALSE);
    indent->newline = TRUE;
    indent->depth++;
}

void
x_script_begin          (xFile          *file,
                         xcstr          key,
                         xcstr          value,
                         ...)
{
    va_list args;
    x_return_if_fail (key != NULL);

    va_start (args, value);
    x_script_begin_valist (file, key, value, args);
    va_end (args);
}

void
x_script_end            (xFile          *file)
{
    struct script_indent* indent;
    indent = x_object_get_qdata (file, indent_quark());
    x_return_if_fail (indent != NULL && indent->depth > 0);
    x_file_puts (file, ")", FALSE);
    indent->newline = FALSE;
    indent->depth--;
}

void
x_script_write_valist   (xFile         *file,
                         xcstr          key,
                         xcstr          value,
                         va_list        argv)
{
    xsize i;
    struct script_indent* indent;
    x_return_if_fail (key != NULL);
    x_return_if_fail (value != NULL);

    indent = x_object_get_qdata (file, indent_quark());
    x_return_if_fail (indent != NULL && indent->depth > 0);

    if (!indent->newline)
        x_file_putc (file, '\n');
    for(i=0;i<indent->depth; ++i)
        x_file_puts(file, INDENT, FALSE);

    x_file_putc (file, ':');
    x_file_puts (file, key, FALSE);

    x_file_putc (file, ' ');
    x_file_print_valist(file, FALSE, value, argv);

    indent->newline = FALSE;
}

void
x_script_write          (xFile          *file,
                         xcstr          key,
                         xcstr          value,
                         ...)
{
    va_list args;
    x_return_if_fail (key != NULL);
    x_return_if_fail (value != NULL);

    va_start (args, value);
    x_script_write_valist (file, key, value, args);
    va_end (args);
}

void
x_script_comment        (xFile          *file,
                         xcstr          comment)
{
    struct script_indent* indent;
    x_return_if_fail (comment != NULL);

    indent = x_object_get_qdata (file, indent_quark());
    x_return_if_fail (indent != NULL && indent->depth > 0);

    if (indent->newline) {
        xsize i;
        for(i=0;i<indent->depth; ++i)
            x_file_puts(file, INDENT, FALSE);
    }

    x_file_putc (file, ';');
    x_file_puts (file, comment, FALSE);
    x_file_putc (file, '\n');
    indent->newline = TRUE;
}
