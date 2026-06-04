/*
 * pyte.c — A minimal Python runtime.
 *
 * One C file. One-pass compiler. Tiny bytecode VM.
 * Subset: int, str, bool, None, list, def/return, if/elif/else,
 *         while, for/range, print, len, arithmetic, comparisons.
 *
 * Compile: gcc -O2 -s -o pyte pyte.c
 * Use:     ./pyte file.py
 *         ./pyte -e "print(1+2)"
 *
 * ~40KB compiled (stripped).  One-pass compiler, no AST, no JIT.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <setjmp.h>

/* ================================================================
 * Configuration
 * ================================================================ */
#define STACK_SIZE       4096
#define LOCALS_MAX       64
#define GLOBALS_MAX      128
#define FRAMES_MAX       64
#define BYTECODE_MAX     (128 * 1024)
#define STRING_POOL_MAX  (8 * 1024)
#define LIST_ARENA_MAX   (128 * 1024)
#define IDENT_MAX        64
#define INDENT_SIZE      4
#define SOURCE_MAX       (256 * 1024)

/* ================================================================
 * Value type
 * ================================================================ */
typedef enum {
    V_NONE, V_BOOL, V_INT, V_FLOAT, V_STR, V_LIST,
    V_FUNC, V_BUILTIN, V_RANGE_ITER, V_LIST_ITER
} VType;

typedef struct Value Value;
typedef struct ListObj  { int len, cap; Value* items; } ListObj;
typedef struct FuncObj  { int bc_start, nparams, nlocals; } FuncObj;
typedef struct RangeIter { int start, stop, step, cur; } RangeIter;
typedef struct ListIter { ListObj* list; int index; } ListIter;
typedef struct StrObj { int len; char s[]; } StrObj;

struct Value {
    VType type;
    union {
        int       as_bool;
        int64_t   as_int;
        double    as_float;
        StrObj*   as_str;
        ListObj*  as_list;
        FuncObj*  as_func;
        int       as_builtin_id;
        RangeIter as_range;
        ListIter  as_list_iter;
    };
};

/* Value constructors */
static Value v_none(void)         { Value v; v.type = V_NONE; return v; }
static Value v_bool(int b)        { Value v; v.type = V_BOOL; v.as_bool = b; return v; }
static Value v_int(int64_t i)     { Value v; v.type = V_INT; v.as_int = i; return v; }
static Value v_float(double f)    { Value v; v.type = V_FLOAT; v.as_float = f; return v; }
static Value v_str(StrObj* s)     { Value v; v.type = V_STR; v.as_str = s; return v; }
static Value v_list(ListObj* l)   { Value v; v.type = V_LIST; v.as_list = l; return v; }
static Value v_func(FuncObj* f)   { Value v; v.type = V_FUNC; v.as_func = f; return v; }
/* v_builtin omitted */
static Value v_range_iter(int a,int b,int s) {
    Value v; v.type = V_RANGE_ITER;
    v.as_range.start=a; v.as_range.stop=b;
    v.as_range.step=s; v.as_range.cur=a;
    return v;
}
static Value v_list_iter(ListObj* l) {
    Value v; v.type = V_LIST_ITER;
    v.as_list_iter.list = l; v.as_list_iter.index = 0;
    return v;
}

static int is_truthy(Value v) {
    switch (v.type) {
    case V_NONE:  return 0;
    case V_BOOL:  return v.as_bool;
    case V_INT:   return v.as_int != 0;
    case V_FLOAT: return v.as_float != 0.0;
    case V_STR:   return v.as_str->len > 0;
    case V_LIST:  return v.as_list->len > 0;
    case V_LIST_ITER: return 1;
    default:      return 1;
    }
}

/* ================================================================
 * Arenas (string pool, list heap)
 * ================================================================ */
static char str_arena[STRING_POOL_MAX];
static int  str_arena_pos;

static StrObj* str_alloc(int len) {
    int n = sizeof(StrObj) + len + 1;
    if (str_arena_pos + n > STRING_POOL_MAX) { fprintf(stderr,"Error: string pool full\n"); exit(1); }
    StrObj* s = (StrObj*)(str_arena + str_arena_pos);
    str_arena_pos += n;
    s->len = len;
    s->s[len] = 0;
    return s;
}

static StrObj* str_intern(const char* data, int len) {
    int pos = 0;
    while (pos < str_arena_pos) {
        StrObj* s = (StrObj*)(str_arena + pos);
        if (s->len == len && memcmp(s->s, data, len) == 0) return s;
        pos += sizeof(StrObj) + s->len + 1;
    }
    StrObj* s = str_alloc(len);
    memcpy(s->s, data, len);
    return s;
}

/* str_from_cstr omitted */

/* List arena */
static char list_arena[LIST_ARENA_MAX];
static int  list_arena_pos;

static void* lalloc(int sz) {
    if (list_arena_pos + sz > LIST_ARENA_MAX) { fprintf(stderr,"Error: list arena full\n"); exit(1); }
    void* p = list_arena + list_arena_pos;
    list_arena_pos += sz;
    return p;
}

static ListObj* list_new(void) {
    ListObj* l = (ListObj*)lalloc(sizeof(ListObj));
    l->len = 0; l->cap = 0; l->items = NULL;
    return l;
}

static void list_ensure(ListObj* l, int n) {
    if (n <= l->cap) return;
    int nc = n < 8 ? 8 : n;
    Value* ni = (Value*)lalloc(nc * sizeof(Value));
    if (l->items && l->len) memcpy(ni, l->items, l->len * sizeof(Value));
    l->items = ni; l->cap = nc;
}

/* ================================================================
 * Lexer
 * ================================================================ */
typedef enum {
    T_EOF, T_NEWLINE, T_INDENT, T_DEDENT,
    T_IDENT, T_INT, T_FLOAT, T_STR,
    T_PLUS, T_MINUS, T_STAR, T_SLASH, T_PERCENT,
    T_EQ, T_EQEQ, T_NE, T_LT, T_GT, T_LE, T_GE,
    T_LPAREN, T_RPAREN, T_COMMA, T_COLON,
    T_LBRACKET, T_RBRACKET, T_DOT,
    /* Keywords */
    T_KW_AND=128, T_KW_OR, T_KW_NOT,
    T_KW_IF, T_KW_ELIF, T_KW_ELSE,
    T_KW_WHILE, T_KW_FOR, T_KW_IN,
    T_KW_DEF, T_KW_RETURN,
    T_KW_TRUE, T_KW_FALSE, T_KW_NONE,
    T_KW_PRINT, T_KW_LEN, T_KW_RANGE,
    T_KW_APPEND,
    T_KW_BREAK, T_KW_CONTINUE,
} TokenType;

static const char* tok_name(int t) {
    switch(t) {
    case T_EOF: return "EOF";
    case T_NEWLINE: return "NEWLINE";
    case T_INDENT: return "INDENT";
    case T_DEDENT: return "DEDENT";
    case T_IDENT: return "IDENT";
    case T_INT: return "INT";
    case T_FLOAT: return "FLOAT";
    case T_STR: return "STR";
    case T_PLUS: return "+";
    case T_MINUS: return "-";
    case T_STAR: return "*";
    case T_SLASH: return "//";
    case T_PERCENT: return "%";
    case T_EQ: return "=";
    case T_EQEQ: return "==";
    case T_NE: return "!=";
    case T_LT: return "<";
    case T_GT: return ">";
    case T_LE: return "<=";
    case T_GE: return ">=";
    case T_LPAREN: return "(";
    case T_RPAREN: return ")";
    case T_COMMA: return ",";
    case T_COLON: return ":";
    case T_LBRACKET: return "[";
    case T_RBRACKET: return "]";
    case T_DOT: return ".";
    case T_KW_AND: return "and";
    case T_KW_OR: return "or";
    case T_KW_NOT: return "not";
    case T_KW_IF: return "if";
    case T_KW_ELIF: return "elif";
    case T_KW_ELSE: return "else";
    case T_KW_WHILE: return "while";
    case T_KW_FOR: return "for";
    case T_KW_IN: return "in";
    case T_KW_DEF: return "def";
    case T_KW_RETURN: return "return";
    case T_KW_TRUE: return "True";
    case T_KW_FALSE: return "False";
    case T_KW_NONE: return "None";
    case T_KW_PRINT: return "print";
    case T_KW_LEN: return "len";
    case T_KW_RANGE: return "range";
    case T_KW_APPEND: return "append";
    case T_KW_BREAK: return "break";
    case T_KW_CONTINUE: return "continue";
    default: return "???";
    }
}

typedef struct {
    const char* src;
    int pos, line, col;
    int prev_indent;
    int indent_stack[64];
    int indent_top;
    int pending_dedents;  /* DEDENT tokens to emit before next real token */
    /* Pending values for token */
    int64_t int_val;
    double  float_val;
    StrObj* str_val;
    char    ident[IDENT_MAX];
} Lexer;

static int lex_next(Lexer* L);

static void lex_init(Lexer* L, const char* src) {
    L->src = src; L->pos = 0;
    L->line = 1; L->col = 1;
    L->prev_indent = 0;
    L->indent_stack[0] = 0; L->indent_top = 0;
    L->pending_dedents = 0;
}

static int lex_peek(Lexer* L) { return L->src[L->pos]; }
static int lex_adv(Lexer* L) {
    int c = L->src[L->pos++];
    L->col++;
    if (c == '\n') { L->line++; L->col = 1; }
    return c;
}

/* Skip whitespace and comments, return indent level if at line start */
static int lex_skip_ws(Lexer* L, int* at_line_start) {
    int indent = 0;
    while (1) {
        int c = lex_peek(L);
        if (c == ' ') { lex_adv(L); if (*at_line_start) indent++; continue; }
        if (c == '\t') { fprintf(stderr,"Error: tabs not supported line %d\n",L->line); exit(1); }
        if (c == '#') { while (lex_peek(L) && lex_peek(L) != '\n') lex_adv(L); continue; }
        if (c == '\n') {
            lex_adv(L);
            *at_line_start = 1;
            indent = 0;
            continue;
        }
        if (c == '\\' && L->src[L->pos+1] == '\n') { lex_adv(L); lex_adv(L); *at_line_start = 1; indent = 0; continue; }
        break;
    }
    return indent;
}

static int lex_next(Lexer* L) {
    int c;

    /* Emit pending DEDENTs from previous indent calculation */
    if (L->pending_dedents > 0) {
        L->pending_dedents--;
        return T_DEDENT;
    }

    int at_line_start = (L->col == 1);

    /* Handle indentation */
    if (at_line_start) {
        int indent = lex_skip_ws(L, &at_line_start) / INDENT_SIZE;
        if (indent > L->indent_stack[L->indent_top]) {
            if (L->indent_top < 63) L->indent_stack[++L->indent_top] = indent;
            L->prev_indent = indent;
            return T_INDENT;
        }
        if (indent < L->indent_stack[L->indent_top]) {
            while (L->indent_top > 0 && indent < L->indent_stack[L->indent_top]) {
                L->indent_top--;
                L->pending_dedents++;
            }
            L->prev_indent = L->indent_stack[L->indent_top];
            L->pending_dedents--;
            return T_DEDENT;
        }
        if (indent != L->indent_stack[L->indent_top]) {
            fprintf(stderr,"Error: inconsistent indent at line %d\n",L->line); exit(1);
        }
        L->prev_indent = indent;
        /* Same level: leading whitespace already consumed, continue to tokens */
    }

    /* Skip inline spaces */
    while (lex_peek(L) == ' ') lex_adv(L);

    c = lex_peek(L);

    /* Comment */
    if (c == '#') {
        while (lex_peek(L) && lex_peek(L) != '\n') lex_adv(L);
        /* Return NEWLINE so the compiler sees a line break */
        if (lex_peek(L) == '\n') { lex_adv(L); return T_NEWLINE; }
        if (L->indent_top > 0) { L->indent_top--; return T_DEDENT; }
        return T_EOF;
    }

    /* EOF — emit DEDENTs for remaining levels */
    if (c == 0) {
        if (L->indent_top > 0) { L->indent_top--; return T_DEDENT; }
        return T_EOF;
    }

    /* Newline */
    if (c == '\n') { lex_adv(L); return T_NEWLINE; }

    /* String */
    if (c == '"' || c == '\'') {
        int q = c; lex_adv(L);
        int start = L->pos;
        while (lex_peek(L) && lex_peek(L) != q) {
            if (lex_peek(L) == '\\') { lex_adv(L); if (!lex_peek(L)) break; }
            lex_adv(L);
        }
        if (!lex_peek(L)) { fprintf(stderr,"Error: unterminated string line %d\n",L->line); exit(1); }
        int len = L->pos - start;
        char buf[1024];
        if (len >= 1023) { fprintf(stderr,"Error: string too long\n"); exit(1); }
        memcpy(buf, L->src + start, len); buf[len] = 0;
        lex_adv(L); /* skip closing quote */
        /* Simple unescape */
        char out[1024]; int o = 0;
        for (int i = 0; i < len; i++) {
            if (buf[i] == '\\' && i+1 < len) {
                i++;
                switch (buf[i]) {
                case 'n': out[o++] = '\n'; break;
                case 't': out[o++] = '\t'; break;
                case 'r': out[o++] = '\r'; break;
                case '\\': out[o++] = '\\'; break;
                case '"': out[o++] = '"'; break;
                case '\'': out[o++] = '\''; break;
                default: out[o++] = buf[i]; break;
                }
            } else out[o++] = buf[i];
        }
        L->str_val = str_intern(out, o);
        return T_STR;
    }

    /* Number */
    if (c >= '0' && c <= '9') {
        int64_t val = 0;
        while (lex_peek(L) >= '0' && lex_peek(L) <= '9') val = val*10 + (lex_adv(L)-'0');
        if (lex_peek(L) == '.') {
            lex_adv(L); /* skip . */
            double frac = 0.0, div = 10.0;
            while (lex_peek(L) >= '0' && lex_peek(L) <= '9') { frac += (lex_adv(L)-'0')/div; div *= 10.0; }
            L->float_val = (double)val + frac;
            return T_FLOAT;
        }
        L->int_val = val;
        return T_INT;
    }

    /* Identifier / keyword */
    if (isalpha(c) || c == '_') {
        int start = L->pos;
        while (isalnum(lex_peek(L)) || lex_peek(L) == '_') lex_adv(L);
        int len = L->pos - start;
        if (len >= IDENT_MAX-1) { fprintf(stderr,"Error: identifier too long\n"); exit(1); }
        memcpy(L->ident, L->src + start, len); L->ident[len] = 0;

        struct { const char* w; int t; } kw[] = {
            {"and",T_KW_AND},{"or",T_KW_OR},{"not",T_KW_NOT},
            {"if",T_KW_IF},{"elif",T_KW_ELIF},{"else",T_KW_ELSE},
            {"while",T_KW_WHILE},{"for",T_KW_FOR},{"in",T_KW_IN},
            {"def",T_KW_DEF},{"return",T_KW_RETURN},
            {"True",T_KW_TRUE},{"False",T_KW_FALSE},{"None",T_KW_NONE},
            {"print",T_KW_PRINT},{"len",T_KW_LEN},{"range",T_KW_RANGE},
            {"append",T_KW_APPEND},{"break",T_KW_BREAK},{"continue",T_KW_CONTINUE},
            {NULL,0}
        };
        for (int i = 0; kw[i].w; i++)
            if (strcmp(L->ident, kw[i].w) == 0) return kw[i].t;
        return T_IDENT;
    }

    /* Multi-char operators */
    if (c == '=' && L->src[L->pos+1] == '=') { lex_adv(L); lex_adv(L); return T_EQEQ; }
    if (c == '!' && L->src[L->pos+1] == '=') { lex_adv(L); lex_adv(L); return T_NE; }
    if (c == '<' && L->src[L->pos+1] == '=') { lex_adv(L); lex_adv(L); return T_LE; }
    if (c == '>' && L->src[L->pos+1] == '=') { lex_adv(L); lex_adv(L); return T_GE; }
    if (c == '/' && L->src[L->pos+1] == '/') { lex_adv(L); lex_adv(L); return T_SLASH; }
    if (c == '*' && L->src[L->pos+1] == '*') { lex_adv(L); lex_adv(L); return T_STAR; } /* ** not used, treat as * */

    /* Single char */
    switch (c) {
    case '+': lex_adv(L); return T_PLUS;
    case '-': lex_adv(L); return T_MINUS;
    case '*': lex_adv(L); return T_STAR;
    case '%': lex_adv(L); return T_PERCENT;
    case '=': lex_adv(L); return T_EQ;
    case '<': lex_adv(L); return T_LT;
    case '>': lex_adv(L); return T_GT;
    case '(': lex_adv(L); return T_LPAREN;
    case ')': lex_adv(L); return T_RPAREN;
    case ',': lex_adv(L); return T_COMMA;
    case ':': lex_adv(L); return T_COLON;
    case '[': lex_adv(L); return T_LBRACKET;
    case ']': lex_adv(L); return T_RBRACKET;
    case '.': lex_adv(L); return T_DOT;
    }
    fprintf(stderr,"Error: unexpected char '%c'(%d) at line %d\n",c,c,L->line);
    exit(1);
    return T_EOF;
}

/* ================================================================
 * Bytecode opcodes
 * ================================================================ */
typedef enum {
    OP_HALT, OP_NONE, OP_TRUE, OP_FALSE,
    OP_INT,  /* +8 bytes int64 */
    OP_FLOAT,/* +8 bytes double */
    OP_STR,  /* +2 bytes string arena offset */
    OP_LOAD, OP_STORE,   /* +1 byte slot (local) */
    OP_GLOAD, OP_GSTORE, /* +1 byte slot (global) */
    OP_DUP, OP_POP, OP_SWAP,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_NEG,
    OP_LT, OP_GT, OP_EQ, OP_NE, OP_LE, OP_GE,
    OP_JUMP,        /* +2 bytes offset */
    OP_JF,          /* jump if false +2 bytes */
    OP_JT,          /* jump if true  +2 bytes */
    OP_CALL,        /* +1 byte nargs */
    OP_RET,
    OP_MKFUNC,      /* +2 bytes body_offset +1 byte nparams +1 byte nlocals */
    OP_PRINT,       /* +1 byte nargs */
    OP_LIST,        /* +1 byte n (build list) */
    OP_GET,         /* list[index] */
    OP_SET,         /* list[index] = value */
    OP_FORITER,     /* +2 bytes end offset */
    OP_NOT,         /* unary not */
    OP_LEN,         /* len() */
    OP_RANGE,       /* build range from top of stack: stop or start,stop */
    OP_APPEND,      /* list.append(value) */
    OP_POP_LIST,    /* list.pop() */
    OP_UPPER,       /* str.upper() */
    OP_LOWER,       /* str.lower() */
    OP_STRIP,       /* str.strip() */
    OP_IN,          /* x in list/string */
    OP_ITER,        /* convert top to iterator */
} Opcode;

/* ================================================================
 * Compiler
 * ================================================================ */
typedef struct {
    int continue_target;   /* bcpos for continue to jump to */
    int break_patches[64]; /* positions of JUMP instructions to patch for break */
    int n_break_patches;
} LoopContext;

typedef struct {
    uint8_t* bc;
    int bc_len, bc_cap;
    struct { char name[32]; } globals[GLOBALS_MAX];
    int nglobals;
    struct { char name[32]; } locals[LOCALS_MAX];
    int nlocals;
    Lexer lex;
    int tok;
    int in_func; /* >0 when compiling function body */
    int line, col;
    LoopContext loops[64];
    int depth; /* current loop depth */
} Compiler;

static void comp_init(Compiler* C, uint8_t* buf, int cap, const char* src) {
    C->bc = buf; C->bc_len = 0; C->bc_cap = cap;
    C->nglobals = 0; C->nlocals = 0; C->in_func = 0;
    C->depth = 0;
    lex_init(&C->lex, src);
}

static void emit(Compiler* C, int b) {
    if (C->bc_len >= C->bc_cap) { fprintf(stderr,"Error: bytecode overflow\n"); exit(1); }
    C->bc[C->bc_len++] = b;
}
static void emit64(Compiler* C, int64_t v) {
    for (int i = 0; i < 8; i++) emit(C, (v>>(i*8))&0xFF);
}
static void emit64d(Compiler* C, double v) {
    uint64_t b; memcpy(&b, &v, 8);
    for (int i = 0; i < 8; i++) emit(C, (b>>(i*8))&0xFF);
}
static void emit16(Compiler* C, int v) {
    emit(C, v&0xFF); emit(C, (v>>8)&0xFF);
}
static int bcpos(Compiler* C) { return C->bc_len; }
static void patch16(Compiler* C, int pos) {
    int t = C->bc_len;
    C->bc[pos] = t&0xFF; C->bc[pos+1] = (t>>8)&0xFF;
}

static void next(Compiler* C) {
    C->tok = lex_next(&C->lex);
}
static void expect(Compiler* C, int t) {
    if (C->tok != t) {
        fprintf(stderr,"Error: expected '%s' got '%s' at line %d\n",tok_name(t),tok_name(C->tok),C->lex.line);
        exit(1);
    }
    next(C);
}

/* Symbol table helpers */
static int gslot(Compiler* C, const char* name) {
    for (int i = 0; i < C->nglobals; i++)
        if (strcmp(C->globals[i].name, name)==0) return i;
    int s = C->nglobals++;
    size_t sl = strlen(name); if (sl>31) sl=31;
    memcpy(C->globals[s].name, name, sl); C->globals[s].name[sl]=0;
    return s;
}
static int lslot(Compiler* C, const char* name) {
    for (int i = 0; i < C->nlocals; i++)
        if (strcmp(C->locals[i].name, name)==0) return i;
    int s = C->nlocals++;
    size_t sl = strlen(name); if (sl>31) sl=31;
    memcpy(C->locals[s].name, name, sl); C->locals[s].name[sl]=0;
    return s;
}
static int find_local(Compiler* C, const char* name) {
    for (int i = 0; i < C->nlocals; i++)
        if (strcmp(C->locals[i].name, name)==0) return i;
    return -1;
}

/* Forward declarations */
static void stmt(Compiler* C);
static void expr(Compiler* C);

/* Precedence table */
static int prec(int tok) {
    switch (tok) {
    case T_KW_OR:  return 1;
    case T_KW_AND: return 2;
    case T_KW_NOT: return 3;
    case T_EQEQ: case T_NE: case T_LT: case T_GT: case T_LE: case T_GE: case T_KW_IN: return 4;
    case T_PLUS: case T_MINUS: return 5;
    case T_STAR: case T_SLASH: case T_PERCENT: return 6;
    default: return 0;
    }
}

/* ================================================================
 * Expression parser (precedence climbing)
 * ================================================================ */
static void primary(Compiler* C) {
    switch (C->tok) {
    case T_INT:
        emit(C, OP_INT); emit64(C, C->lex.int_val);
        next(C); break;
    case T_FLOAT:
        emit(C, OP_FLOAT); emit64d(C, C->lex.float_val);
        next(C); break;
    case T_STR:
        emit(C, OP_STR); emit16(C, (char*)C->lex.str_val - str_arena);
        next(C); break;
    case T_KW_TRUE:  emit(C, OP_TRUE); next(C); break;
    case T_KW_FALSE: emit(C, OP_FALSE); next(C); break;
    case T_KW_NONE:  emit(C, OP_NONE); next(C); break;
    case T_LPAREN:
        next(C); expr(C); expect(C, T_RPAREN);
        break;
    case T_LBRACKET: {
        /* List literal */
        next(C);
        int n = 0;
        if (C->tok != T_RBRACKET) {
            expr(C); n++;
            while (C->tok == T_COMMA) { next(C); expr(C); n++; }
        }
        expect(C, T_RBRACKET);
        emit(C, OP_LIST); emit(C, n);
        break;
    }
    case T_IDENT:
    case T_KW_LEN:
    case T_KW_RANGE:
    case T_KW_PRINT: {
        char name[IDENT_MAX];
        strcpy(name, C->lex.ident);
        next(C);

        /* Check for call: name(...) */
        if (C->tok == T_LPAREN) {
            /* Determine if it's a known builtin we handle specially */
            if (strcmp(name,"print")==0) {
                next(C);
                int n = 0;
                if (C->tok != T_RPAREN) { expr(C); n++; while (C->tok==T_COMMA) { next(C); expr(C); n++; } }
                expect(C, T_RPAREN);
                emit(C, OP_PRINT); emit(C, n);
            } else if (strcmp(name,"len")==0) {
                next(C); expr(C); expect(C, T_RPAREN);
                emit(C, OP_LEN);
            } else if (strcmp(name,"range")==0) {
                next(C);
                if (C->tok == T_RPAREN) {
                    /* range() — empty range, not useful but supported */
                    emit(C, OP_INT); emit64(C, 0);
                    emit(C, OP_INT); emit64(C, 0);
                    emit(C, OP_RANGE);
                } else {
                    expr(C);
                    if (C->tok == T_COMMA) {
                        next(C); expr(C);
                        emit(C, OP_RANGE);
                    } else {
                        /* range(stop) = range(0, stop) */
                        emit(C, OP_SWAP);
                        emit(C, OP_INT); emit64(C, 0);
                        emit(C, OP_SWAP);
                        emit(C, OP_RANGE);
                    }
                }
                expect(C, T_RPAREN);
            } else {
                /* User function or builtin call */
                int s;
                if (C->in_func && (s = find_local(C,name)) >= 0) { emit(C, OP_LOAD); emit(C,s); }
                else { s = gslot(C,name); emit(C, OP_GLOAD); emit(C,s); }
                next(C);
                int n = 0;
                if (C->tok != T_RPAREN) { expr(C); n++; while (C->tok==T_COMMA) { next(C); expr(C); n++; } }
                expect(C, T_RPAREN);
                emit(C, OP_CALL); emit(C, n);
            }
        }
        /* Bare identifier (variable reference) */
        else {
            int s;
            if (C->in_func && (s = find_local(C,name)) >= 0) { emit(C, OP_LOAD); emit(C,s); }
            else { s = gslot(C,name); emit(C, OP_GLOAD); emit(C,s); }
        }
        break;
    }
    default:
        fprintf(stderr,"Error: unexpected token '%s' in expression at line %d\n",
                tok_name(C->tok), C->lex.line);
        exit(1);
    }

    /* Postfix operations: subscript [...] */
    while (C->tok == T_LBRACKET) {
        next(C); expr(C); expect(C, T_RBRACKET);
        emit(C, OP_GET);
    }
}

static void expr_bp(Compiler* C, int minp) {
    /* Handle unary prefix: minus and not.
       After the prefix, fall through to the infix loop so binary
       operators at the current precedence level are still parsed.
       Use a high binding power for the prefix operand so it binds
       tighter than any binary operator. */
    int handled_prefix = 0;
    if (C->tok == T_MINUS) {
        next(C);
        expr_bp(C, 8);  /* right operand binds tighter than any binop */
        emit(C, OP_NEG);
        handled_prefix = 1;
    } else if (C->tok == T_KW_NOT) {
        next(C);
        expr_bp(C, 3);
        emit(C, OP_NOT);
        handled_prefix = 1;
    }

    if (!handled_prefix) {
        primary(C);
    }

    while (1) {
        int p = prec(C->tok);
        if (p == 0 || p < minp) break;
        int op = C->tok;

        /* Handle `not in` as a single operator: `x not in y` → not(x in y) */
        if (op == T_KW_NOT) {
            /* Consume 'not' ourselves, then check for 'in' */
            next(C);
            if (C->tok == T_KW_IN) {
                next(C); /* consume 'in' */
                expr_bp(C, p + 1);
                emit(C, OP_IN);
                emit(C, OP_NOT);
                continue;
            } else {
                fprintf(stderr,"Error: 'not' must be followed by 'in' in this context\n"); exit(1);
            }
        }

        /* and/or need short-circuit: emit jump BEFORE right is compiled,
           then patch after. Handle in-line before the normal pattern. */
        if (op == T_KW_AND || op == T_KW_OR) {
            next(C);
            emit(C, OP_DUP);                   /* dup left for short-circuit check */
            int patch = bcpos(C);
            if (op == T_KW_AND) {
                emit(C, OP_JF); emit16(C, 0);  /* if left falsy → keep left on stack, skip right */
            } else {
                emit(C, OP_JT); emit16(C, 0);  /* if left truthy → keep left on stack, skip right */
            }
            emit(C, OP_POP);                   /* condition failed, discard left */
            expr_bp(C, p + 1);                 /* compile right operand */
            patch16(C, patch + 1);             /* patch jump target (offset at patch+1) */
            continue;
        }

        next(C);
        expr_bp(C, p + 1);

        switch (op) {
        case T_PLUS:  emit(C, OP_ADD); break;
        case T_MINUS: emit(C, OP_SUB); break;
        case T_STAR:  emit(C, OP_MUL); break;
        case T_SLASH: emit(C, OP_DIV); break;
        case T_PERCENT: emit(C, OP_MOD); break;
        case T_LT:    emit(C, OP_LT); break;
        case T_GT:    emit(C, OP_GT); break;
        case T_EQEQ:  emit(C, OP_EQ); break;
        case T_NE:    emit(C, OP_NE); break;
        case T_LE:    emit(C, OP_LE); break;
        case T_GE:    emit(C, OP_GE); break;
        case T_KW_IN: emit(C, OP_IN); break;
        default: break;
        }
    }
}

static void expr(Compiler* C) { expr_bp(C, 0); }

/* ================================================================
 * Statement parser
 * ================================================================ */
static void block(Compiler* C, int end) {
    /* Compile statements until end token, EOF, or DEDENT below starting level. */
    int base_indent = C->lex.indent_top;
    if (C->tok == T_INDENT) { base_indent = C->lex.indent_top; next(C); }
    while (C->tok != T_EOF && C->tok != end) {
        if (C->tok == T_DEDENT) {
            if (C->lex.indent_top < base_indent) break;
            next(C); continue;
        }
        if (C->tok == T_INDENT) { next(C); continue; }
        if (C->tok == T_NEWLINE) { next(C); continue; }
        stmt(C);
        if (C->tok == T_NEWLINE) next(C);
    }
}

static void stmt(Compiler* C) {
    switch (C->tok) {
    case T_KW_PRINT: {
        next(C); expect(C, T_LPAREN);
        int n = 0;
        if (C->tok != T_RPAREN) { expr(C); n++; while (C->tok==T_COMMA) { next(C); expr(C); n++; } }
        expect(C, T_RPAREN);
        emit(C, OP_PRINT); emit(C, n);
        break;
    }
    case T_KW_IF: {
        next(C); expr(C); expect(C, T_COLON);
        int p_false = bcpos(C);
        emit(C, OP_JF); emit16(C, 0);
        if (C->tok == T_NEWLINE) next(C);
        int then_indent = C->lex.indent_top;
        block(C, T_KW_ELIF);
        /* Consume DEDENTs back to the if statement's own level */
        while (C->tok == T_DEDENT && C->lex.indent_top >= then_indent - 1) {
            next(C);
        }
        if (C->tok == T_KW_ELIF || C->tok == T_KW_ELSE) {
            int p_end = bcpos(C);
            emit(C, OP_JUMP); emit16(C, 0);
            patch16(C, p_false + 1);  /* if's JF → jump to after then-body (at JUMP) */
            /* Track JUMP positions for all elifs so they can be patched at the end. */
            int elif_jumps[64]; int n_elif_jumps = 0;
            while (C->tok == T_KW_ELIF) {
                next(C); expr(C); expect(C, T_COLON);
                p_false = bcpos(C);
                emit(C, OP_JF); emit16(C, 0);
                if (C->tok == T_NEWLINE) next(C);
                int elif_indent = C->lex.indent_top;
                block(C, T_KW_ELIF);
                /* Consume DEDENTs back to the elif statement's own level */
                while (C->tok == T_DEDENT && C->lex.indent_top >= elif_indent - 1) {
                    next(C);
                }
                if (C->tok == T_KW_ELIF || C->tok == T_KW_ELSE) {
                    int p = bcpos(C);
                    emit(C, OP_JUMP); emit16(C, 0);
                    patch16(C, p_false + 1);  /* elif's JF → jump to start of next chain */
                    if (n_elif_jumps < 64) elif_jumps[n_elif_jumps++] = p + 1;
                    p_false = p;
                } else {
                    patch16(C, p_false + 1);  /* no more elif/else → JF jumps to end */
                }
            }
            if (C->tok == T_KW_ELSE) {
                next(C); expect(C, T_COLON);
                if (C->tok == T_NEWLINE) next(C);
                int else_base = C->lex.indent_top;
                if (C->tok == T_INDENT) { else_base = C->lex.indent_top; next(C); }
                while (C->tok != T_EOF) {
                    if (C->tok == T_DEDENT) {
                        if (C->lex.indent_top < else_base) break;
                        next(C); continue;
                    }
                    if (C->tok == T_INDENT) { next(C); continue; }
                    if (C->tok == T_NEWLINE) { next(C); continue; }
                    stmt(C);
                    if (C->tok == T_NEWLINE) next(C);
                }
            }
            /* Patch all elif JUMPs and the IF-then JUMP to go past else/end */
            for (int i = 0; i < n_elif_jumps; i++) patch16(C, elif_jumps[i]);
            patch16(C, p_end + 1);  /* patch the if-then JUMP to go past else/elif chain */
        } else {
            patch16(C, p_false + 1);  /* no else/elif → JF jumps to end */
        }
        break;
    }
    case T_KW_WHILE: {
        next(C);
        int start = bcpos(C);
        expr(C); expect(C, T_COLON);
        int p_exit = bcpos(C);
        emit(C, OP_JF); emit16(C, 0);
        if (C->tok == T_NEWLINE) next(C);
        /* Set up loop context for break/continue */
        int saved_depth = C->depth;
        C->loops[C->depth].continue_target = start;
        C->loops[C->depth].n_break_patches = 0;
        C->depth++;
        block(C, T_EOF);
        C->depth = saved_depth;
        emit(C, OP_JUMP); emit16(C, start);
        patch16(C, p_exit + 1);
        /* Patch all break jumps to exit here */
        for (int i = 0; i < C->loops[saved_depth].n_break_patches; i++)
            patch16(C, C->loops[saved_depth].break_patches[i]);
        break;
    }
    case T_KW_FOR: {
        /* for var in range(...): */
        next(C);
        if (C->tok != T_IDENT) { fprintf(stderr,"Error: expected identifier after 'for'\n"); exit(1); }
        char vname[IDENT_MAX]; strcpy(vname, C->lex.ident);
        next(C);
        expect(C, T_KW_IN);

        /* Handle range(...) and general iterables (lists, strings) */
        if (C->tok == T_KW_RANGE) {
            next(C); expect(C, T_LPAREN);
            if (C->tok == T_RPAREN) {
                emit(C, OP_INT); emit64(C, 0);
                emit(C, OP_INT); emit64(C, 0);
                emit(C, OP_RANGE);
            } else {
                expr(C);
                if (C->tok == T_COMMA) {
                    next(C); expr(C);
                    emit(C, OP_RANGE);
                } else {
                    /* range(stop) → push start=0 then stop, so OP_RANGE gets (0, stop) */
                    emit(C, OP_INT); emit64(C, 0);
                    emit(C, OP_SWAP);
                    emit(C, OP_RANGE);
                }
            }
            expect(C, T_RPAREN);
        } else {
            /* General iterable: compile expression, then OP_ITER converts to iterator */
            expr(C);
            emit(C, OP_ITER);
        }
        expect(C, T_COLON);

        int loop_start = bcpos(C);
        emit(C, OP_FORITER); emit16(C, 0); /* end offset placeholder */
        /* Stack now has: range_iter, value */

        /* Store value into variable */
        if (C->in_func) { int s = lslot(C,vname); emit(C, OP_STORE); emit(C,s); }
        else { int s = gslot(C,vname); emit(C, OP_GSTORE); emit(C,s); }

        /* Set up loop context for break/continue */
        int saved_depth2 = C->depth;
        C->loops[C->depth].continue_target = loop_start;
        C->loops[C->depth].n_break_patches = 0;
        C->depth++;

        if (C->tok == T_NEWLINE) next(C);
        block(C, T_EOF);

        C->depth = saved_depth2;

        emit(C, OP_JUMP); emit16(C, loop_start);
        patch16(C, loop_start + 1); /* patch FORITER end offset */

        /* Patch all break jumps to execute the POP (clean up iterator) */
        for (int i = 0; i < C->loops[saved_depth2].n_break_patches; i++)
            patch16(C, C->loops[saved_depth2].break_patches[i]);

        emit(C, OP_POP); /* pop iterator — executed by both normal exit and break */
        break;
    }
    case T_KW_DEF: {
        /* def name(params): body */
        next(C);
        if (C->tok != T_IDENT) { fprintf(stderr,"Error: expected function name\n"); exit(1); }
        char fname[IDENT_MAX]; strcpy(fname, C->lex.ident);
        int slot = gslot(C, fname);
        next(C); expect(C, T_LPAREN);

        /* Parse param names */
        char params[LOCALS_MAX][IDENT_MAX];
        int nparams = 0;
        if (C->tok != T_RPAREN) {
            while (1) {
                if (C->tok != T_IDENT) { fprintf(stderr,"Error: expected parameter name\n"); exit(1); }
                strcpy(params[nparams++], C->lex.ident);
                next(C);
                if (C->tok != T_COMMA) break;
                next(C);
            }
        }
        expect(C, T_RPAREN); expect(C, T_COLON);

        /* Compile function body */
        int saved_locals = C->nlocals;
        int saved_in_func = C->in_func;
        C->nlocals = 0;
        C->in_func = 1;

        /* Register params as locals */
        for (int i = 0; i < nparams; i++) lslot(C, params[i]);

        /* Emit JUMP over function body (patch later) */
        int jump_skip = bcpos(C);
        emit(C, OP_JUMP); emit16(C, 0);

        int body_start = bcpos(C);

        if (C->tok == T_NEWLINE) next(C);
        block(C, T_EOF);

        /* Implicit return None */
        emit(C, OP_NONE); emit(C, OP_RET);

        /* Patch the initial JUMP to skip past the function body */
        patch16(C, jump_skip + 1); /* +1 to skip the opcode byte, patch the target */

        /* def compiled */

        emit(C, OP_MKFUNC); emit16(C, body_start); emit(C, nparams); emit(C, C->nlocals);
        emit(C, OP_GSTORE); emit(C, slot);

        C->nlocals = saved_locals;
        C->in_func = saved_in_func;
        break;
    }
    case T_KW_RETURN: {
        next(C);
        if (C->tok == T_NEWLINE || C->tok == T_DEDENT || C->tok == T_EOF)
            emit(C, OP_NONE);
        else
            expr(C);
        emit(C, OP_RET);
        break;
    }
    case T_KW_BREAK: {
        next(C);
        if (C->depth == 0) { fprintf(stderr,"Error: break outside loop\n"); exit(1); }
        int p = bcpos(C);
        emit(C, OP_JUMP); emit16(C, 0);
        LoopContext* lc = &C->loops[C->depth - 1];
        if (lc->n_break_patches < 64) lc->break_patches[lc->n_break_patches++] = p + 1;
        break;
    }
    case T_KW_CONTINUE: {
        next(C);
        if (C->depth == 0) { fprintf(stderr,"Error: continue outside loop\n"); exit(1); }
        emit(C, OP_JUMP); emit16(C, C->loops[C->depth - 1].continue_target);
        break;
    }
    case T_IDENT: {
        char name[IDENT_MAX]; strcpy(name, C->lex.ident);
        next(C);

        if (C->tok == T_EQ) {
            /* Assignment */
            next(C); expr(C);
            if (C->in_func) { int s = lslot(C,name); emit(C, OP_STORE); emit(C,s); }
            else { int s = gslot(C,name); emit(C, OP_GSTORE); emit(C,s); }
        } else if (C->tok == T_LBRACKET) {
            /* x[index] = expr */
            int s;
            if (C->in_func && (s = find_local(C,name)) >= 0) { emit(C, OP_LOAD); emit(C,s); }
            else { s = gslot(C,name); emit(C, OP_GLOAD); emit(C,s); }
            next(C); expr(C); expect(C, T_RBRACKET);
            expect(C, T_EQ); expr(C);
            emit(C, OP_SET);
        } else if (C->tok == T_LPAREN) {
            /* Call as statement */
            if (strcmp(name,"print")==0) {
                next(C);
                int n = 0;
                if (C->tok != T_RPAREN) { expr(C); n++; while (C->tok==T_COMMA) { next(C); expr(C); n++; } }
                expect(C, T_RPAREN);
                emit(C, OP_PRINT); emit(C, n);
            } else {
                int s;
                if (C->in_func && (s = find_local(C,name)) >= 0) { emit(C, OP_LOAD); emit(C,s); }
                else { s = gslot(C,name); emit(C, OP_GLOAD); emit(C,s); }
                next(C);
                int n = 0;
                if (C->tok != T_RPAREN) { expr(C); n++; while (C->tok==T_COMMA) { next(C); expr(C); n++; } }
                expect(C, T_RPAREN);
                emit(C, OP_CALL); emit(C, n);
                emit(C, OP_POP); /* discard return value */
            }
        } else if (C->tok == T_DOT) {
            /* x.append(y) or x.pop() */
            next(C);
            if (strcmp(C->lex.ident,"append")==0) {
                next(C); expect(C, T_LPAREN); expr(C); expect(C, T_RPAREN);
                int s;
                if (C->in_func && (s = find_local(C,name)) >= 0) { emit(C, OP_LOAD); emit(C,s); }
                else { s = gslot(C,name); emit(C, OP_GLOAD); emit(C,s); }
                emit(C, OP_SWAP); /* swap: value, list -> list, value */
                /* Actually OP_APPEND expects (list, value) */
                /* The list is already on stack from LOAD, then value pushed from expr */
                /* So stack is: list, value. That's correct for OP_APPEND.
                   But we need to make sure: after lexing 'append', we parsed expr.
                   So: LOAD list, expr(value). Stack: list, value.
                   OP_APPEND should pop value and list, push result.
                   But for 'append as statement', we just want to discard result. */
                emit(C, OP_APPEND);
                emit(C, OP_POP);
            } else if (strcmp(C->lex.ident,"pop")==0) {
                next(C); expect(C, T_LPAREN); expect(C, T_RPAREN);
                int s;
                if (C->in_func && (s = find_local(C,name)) >= 0) { emit(C, OP_LOAD); emit(C,s); }
                else { s = gslot(C,name); emit(C, OP_GLOAD); emit(C,s); }
                emit(C, OP_POP_LIST);
                emit(C, OP_POP); /* discard return value in statement context */
            } else if (strcmp(C->lex.ident,"upper")==0 || strcmp(C->lex.ident,"lower")==0) {
                int is_upper = strcmp(C->lex.ident,"upper")==0;
                next(C); expect(C, T_LPAREN); expect(C, T_RPAREN);
                int s;
                if (C->in_func && (s = find_local(C,name)) >= 0) { emit(C, OP_LOAD); emit(C,s); }
                else { s = gslot(C,name); emit(C, OP_GLOAD); emit(C,s); }
                emit(C, is_upper ? OP_UPPER : OP_LOWER);
                if (C->in_func) { emit(C, OP_STORE); emit(C,s); }
                else { emit(C, OP_GSTORE); emit(C,s); }
            } else if (strcmp(C->lex.ident,"strip")==0) {
                next(C); expect(C, T_LPAREN); expect(C, T_RPAREN);
                int s;
                if (C->in_func && (s = find_local(C,name)) >= 0) { emit(C, OP_LOAD); emit(C,s); }
                else { s = gslot(C,name); emit(C, OP_GLOAD); emit(C,s); }
                emit(C, OP_STRIP);
                if (C->in_func) { emit(C, OP_STORE); emit(C,s); }
                else { emit(C, OP_GSTORE); emit(C,s); }
            } else {
                fprintf(stderr,"Error: unknown method '%s'\n",C->lex.ident); exit(1);
            }
        } else {
            /* Expression as statement - discard */
            fprintf(stderr,"Warning: expression as statement at line %d\n",C->lex.line);
        }
        break;
    }
    case T_NEWLINE:
        next(C);
        break;
    default:
        fprintf(stderr,"Error: unexpected '%s' at line %d\n",tok_name(C->tok),C->lex.line);
        exit(1);
    }
}

/* ================================================================
 * VM
 * ================================================================ */
static Value    stack[STACK_SIZE];
static Value*   sp;
static Value    globals[GLOBALS_MAX];
static uint8_t* bc;
static int      bc_len;

/* Call frames */
typedef struct {
    int pc;
    Value* sp;
    Value locals[LOCALS_MAX];
    int nlocals;
    int nargs;
} Frame;
static Frame frames[FRAMES_MAX];
static int frame_count;

static jmp_buf vm_err;

static void print_val(Value v) {
    switch (v.type) {
    case V_NONE:  printf("None"); break;
    case V_BOOL:  printf(v.as_bool?"True":"False"); break;
    case V_INT:   printf("%lld",(long long)v.as_int); break;
    case V_FLOAT: printf("%g",v.as_float); break;
    case V_STR:   printf("%s",v.as_str->s); break;
    case V_LIST: {
        printf("[");
        for (int i=0;i<v.as_list->len;i++) {
            if(i) printf(", ");
            print_val(v.as_list->items[i]);
        }
        printf("]");
        break;
    }
    case V_FUNC:  printf("<function>"); break;
    case V_BUILTIN: printf("<builtin %d>",v.as_builtin_id); break;
    case V_RANGE_ITER: printf("<range>"); break;
    case V_LIST_ITER: printf("<list_iter>"); break;
    }
}

static void print_vals(Value* a, int n) {
    for (int i=0;i<n;i++) { if(i) printf(" "); print_val(a[i]); }
    printf("\n");
}

static Value add_val(Value a, Value b) {
    if (a.type==V_INT && b.type==V_INT) return v_int(a.as_int+b.as_int);
    if (a.type==V_FLOAT && b.type==V_FLOAT) return v_float(a.as_float+b.as_float);
    if (a.type==V_INT && b.type==V_FLOAT) return v_float(a.as_int+b.as_float);
    if (a.type==V_FLOAT && b.type==V_INT) return v_float(a.as_float+b.as_int);
    if (a.type==V_STR && b.type==V_STR) {
        int l = a.as_str->len + b.as_str->len;
        char buf[4096]; if (l>=4095) { fprintf(stderr,"String too long\n"); longjmp(vm_err,1); }
        memcpy(buf,a.as_str->s,a.as_str->len);
        memcpy(buf+a.as_str->len,b.as_str->s,b.as_str->len);
        return v_str(str_intern(buf,l));
    }
    fprintf(stderr,"Error: unsupported +\n"); longjmp(vm_err,1);
    return v_none();
}
static Value sub_val(Value a, Value b) {
    if (a.type==V_INT && b.type==V_INT) return v_int(a.as_int-b.as_int);
    if (a.type==V_FLOAT && b.type==V_FLOAT) return v_float(a.as_float-b.as_float);
    if (a.type==V_INT && b.type==V_FLOAT) return v_float(a.as_int-b.as_float);
    if (a.type==V_FLOAT && b.type==V_INT) return v_float(a.as_float-b.as_int);
    fprintf(stderr,"Error: unsupported -\n"); longjmp(vm_err,1);
    return v_none();
}
static Value mul_val(Value a, Value b) {
    if (a.type==V_INT && b.type==V_INT) return v_int(a.as_int*b.as_int);
    if (a.type==V_FLOAT && b.type==V_FLOAT) return v_float(a.as_float*b.as_float);
    if (a.type==V_INT && b.type==V_FLOAT) return v_float(a.as_int*b.as_float);
    if (a.type==V_FLOAT && b.type==V_INT) return v_float(a.as_float*b.as_int);
    fprintf(stderr,"Error: unsupported *\n"); longjmp(vm_err,1);
    return v_none();
}
static Value div_val(Value a, Value b) {
    if (a.type==V_INT && b.type==V_INT) {
        if (b.as_int==0) { fprintf(stderr,"Division by zero\n"); longjmp(vm_err,1); }
        return v_int(a.as_int/b.as_int);
    }
    if (a.type==V_FLOAT && b.type==V_FLOAT) {
        if (b.as_float==0.0) { fprintf(stderr,"Division by zero\n"); longjmp(vm_err,1); }
        return v_float((double)((int)(a.as_float/b.as_float)));
    }
    if (a.type==V_INT && b.type==V_FLOAT) {
        if (b.as_float==0.0) { fprintf(stderr,"Division by zero\n"); longjmp(vm_err,1); }
        return v_float((double)((int)(a.as_int/b.as_float)));
    }
    if (a.type==V_FLOAT && b.type==V_INT) {
        if (b.as_int==0) { fprintf(stderr,"Division by zero\n"); longjmp(vm_err,1); }
        return v_float((double)((int)(a.as_float/b.as_int)));
    }
    fprintf(stderr,"Error: unsupported //\n"); longjmp(vm_err,1);
    return v_none();
}
static Value mod_val(Value a, Value b) {
    if (a.type==V_INT && b.type==V_INT) {
        if (b.as_int==0) { fprintf(stderr,"Modulo by zero\n"); longjmp(vm_err,1); }
        return v_int(a.as_int%b.as_int);
    }
    fprintf(stderr,"Error: unsupported %%\n"); longjmp(vm_err,1);
    return v_none();
}
static Value neg_val(Value a) {
    if (a.type==V_INT) return v_int(-a.as_int);
    if (a.type==V_FLOAT) return v_float(-a.as_float);
    fprintf(stderr,"Error: unsupported negation\n"); longjmp(vm_err,1);
    return v_none();
}
static Value cmp_val(int op, Value a, Value b) {
    int r = 0;
    if (a.type==V_INT && b.type==V_INT) {
        int64_t va=a.as_int,vb=b.as_int;
        switch(op){case OP_LT:r=va<vb;break;case OP_GT:r=va>vb;break;case OP_EQ:r=va==vb;break;case OP_NE:r=va!=vb;break;case OP_LE:r=va<=vb;break;case OP_GE:r=va>=vb;break;}
    } else if (a.type==V_FLOAT && b.type==V_FLOAT) {
        double va=a.as_float,vb=b.as_float;
        switch(op){case OP_LT:r=va<vb;break;case OP_GT:r=va>vb;break;case OP_EQ:r=va==vb;break;case OP_NE:r=va!=vb;break;case OP_LE:r=va<=vb;break;case OP_GE:r=va>=vb;break;}
    } else if (a.type==V_INT && b.type==V_FLOAT) {
        double va=a.as_int,vb=b.as_float;
        switch(op){case OP_LT:r=va<vb;break;case OP_GT:r=va>vb;break;case OP_EQ:r=va==vb;break;case OP_NE:r=va!=vb;break;case OP_LE:r=va<=vb;break;case OP_GE:r=va>=vb;break;}
    } else if (a.type==V_FLOAT && b.type==V_INT) {
        double va=a.as_float,vb=b.as_int;
        switch(op){case OP_LT:r=va<vb;break;case OP_GT:r=va>vb;break;case OP_EQ:r=va==vb;break;case OP_NE:r=va!=vb;break;case OP_LE:r=va<=vb;break;case OP_GE:r=va>=vb;break;}
    } else if (a.type==V_STR && b.type==V_STR) {
        int c = strcmp(a.as_str->s, b.as_str->s);
        switch(op){case OP_LT:r=c<0;break;case OP_GT:r=c>0;break;case OP_EQ:r=c==0;break;case OP_NE:r=c!=0;break;case OP_LE:r=c<=0;break;case OP_GE:r=c>=0;break;}
    } else if (a.type==V_BOOL && b.type==V_BOOL) {
        switch(op){case OP_EQ:r=a.as_bool==b.as_bool;break;case OP_NE:r=a.as_bool!=b.as_bool;break;default:goto unsup_cmp;}
    } else if (a.type==V_BOOL && b.type==V_INT) {
        switch(op){case OP_EQ:r=a.as_bool==b.as_int;break;case OP_NE:r=a.as_bool!=b.as_int;break;default:goto unsup_cmp;}
    } else if (a.type==V_INT && b.type==V_BOOL) {
        switch(op){case OP_EQ:r=a.as_int==b.as_bool;break;case OP_NE:r=a.as_int!=b.as_bool;break;default:goto unsup_cmp;}
    } else if (a.type==V_NONE && b.type==V_NONE) {
        switch(op){case OP_EQ:r=1;break;case OP_NE:r=0;break;default:goto unsup_cmp;}
    } else if ((a.type==V_NONE) != (b.type==V_NONE)) {
        /* One is None, the other is not */
        switch(op){case OP_EQ:r=0;break;case OP_NE:r=1;break;default:goto unsup_cmp;}
    } else {
        unsup_cmp:
        fprintf(stderr,"Error: unsupported comparison\n"); longjmp(vm_err,1);
    }
    return v_bool(r);
}

/* ================================================================
 * VM dispatch
 * ================================================================ */
static void vm_exec(int start_pc) {
    int pc = start_pc;
    volatile int nlocals = 0;
    Value locals[LOCALS_MAX];
    memset(locals,0,sizeof(locals));

    if (setjmp(vm_err)) { sp = stack; frame_count = 0; return; }

#define RD8()  (bc[pc++])
#define RD16() ({ int v = bc[pc] | (bc[pc+1]<<8); pc+=2; v; })
#define RD64() ({ int64_t v=0; for(int i=0;i<8;i++) v|=(int64_t)bc[pc++]<<(i*8); v; })
#define RD64F() ({ uint64_t b=0; for(int i=0;i<8;i++) b|=(uint64_t)bc[pc++]<<(i*8); double d; memcpy(&d,&b,8); d; })
#define PUSH(v) (*sp++ = (v))
#define POP()   (*--sp)
#define TOP()   (sp[-1])

    while (1) {
        int op = RD8();
        switch (op) {
        case OP_HALT: return;

        case OP_NONE:  PUSH(v_none()); break;
        case OP_TRUE:  PUSH(v_bool(1)); break;
        case OP_FALSE: PUSH(v_bool(0)); break;

        case OP_INT:   { int64_t v = RD64(); PUSH(v_int(v)); break; }
        case OP_FLOAT: { double v = RD64F(); PUSH(v_float(v)); break; }
        case OP_STR:   { int off = RD16(); PUSH(v_str((StrObj*)(str_arena+off))); break; }

        case OP_LOAD:  { int s = RD8(); PUSH(locals[s]); break; }
        case OP_STORE: { int s = RD8(); locals[s] = POP(); break; }
        case OP_GLOAD: { int s = RD8(); PUSH(globals[s]); break; }
        case OP_GSTORE:{ int s = RD8(); globals[s] = POP(); break; }

        case OP_DUP:  { Value v = TOP(); PUSH(v); break; }
        case OP_POP:  { (void)POP(); break; }
        case OP_SWAP: { Value a = POP(); Value b = POP(); PUSH(a); PUSH(b); break; }

        case OP_ADD: { Value b=POP();Value a=POP();PUSH(add_val(a,b)); break; }
        case OP_SUB: { Value b=POP();Value a=POP();PUSH(sub_val(a,b)); break; }
        case OP_MUL: { Value b=POP();Value a=POP();PUSH(mul_val(a,b)); break; }
        case OP_DIV: { Value b=POP();Value a=POP();PUSH(div_val(a,b)); break; }
        case OP_MOD: { Value b=POP();Value a=POP();PUSH(mod_val(a,b)); break; }
        case OP_NEG: { Value a=POP();PUSH(neg_val(a)); break; }

        case OP_LT: { Value b=POP();Value a=POP();PUSH(cmp_val(OP_LT,a,b)); break; }
        case OP_GT: { Value b=POP();Value a=POP();PUSH(cmp_val(OP_GT,a,b)); break; }
        case OP_EQ: { Value b=POP();Value a=POP();PUSH(cmp_val(OP_EQ,a,b)); break; }
        case OP_NE: { Value b=POP();Value a=POP();PUSH(cmp_val(OP_NE,a,b)); break; }
        case OP_LE: { Value b=POP();Value a=POP();PUSH(cmp_val(OP_LE,a,b)); break; }
        case OP_GE: { Value b=POP();Value a=POP();PUSH(cmp_val(OP_GE,a,b)); break; }

        case OP_NOT: { Value a = POP(); PUSH(v_bool(!is_truthy(a))); break; }

        case OP_JUMP: { pc = RD16(); break; }
        case OP_JF:  { int t = RD16(); if (!is_truthy(POP())) pc = t; break; }
        case OP_JT:  { int t = RD16(); if (is_truthy(POP())) pc = t; break; }

        case OP_PRINT: {
            int n = RD8();
            Value* args = sp - n;
            print_vals(args, n);
            sp -= n;
            break;
        }

        case OP_LEN: {
            Value v = POP();
            if (v.type == V_STR) PUSH(v_int(v.as_str->len));
            else if (v.type == V_LIST) PUSH(v_int(v.as_list->len));
            else { fprintf(stderr,"Error: len() unsupported type\n"); longjmp(vm_err,1); }
            break;
        }

        case OP_RANGE: {
            Value b = POP();
            Value a = POP();
            if (a.type != V_INT || b.type != V_INT) {
                fprintf(stderr,"Error: range() expects int\n"); longjmp(vm_err,1);
            }
            PUSH(v_range_iter((int)a.as_int, (int)b.as_int, 1));
            break;
        }

        case OP_IN: {
            Value container = POP();
            Value item = POP();
            int found = 0;
            if (container.type == V_LIST) {
                for (int i = 0; i < container.as_list->len; i++) {
                    Value r = cmp_val(OP_EQ, item, container.as_list->items[i]);
                    if (is_truthy(r)) { found = 1; break; }
                }
            } else if (container.type == V_STR) {
                if (item.type != V_STR) { found = 0; }
                else { found = strstr(container.as_str->s, item.as_str->s) != NULL; }
            } else {
                fprintf(stderr,"Error: in not supported on this type\n"); longjmp(vm_err,1);
            }
            PUSH(v_bool(found));
            break;
        }

        case OP_ITER: {
            /* Convert top-of-stack to an iterator */
            Value* vp = &TOP();
            if (vp->type == V_LIST) {
                ListObj* lst = vp->as_list;
                *vp = v_list_iter(lst);
            } else {
                fprintf(stderr,"Error: not iterable\n"); longjmp(vm_err,1);
            }
            break;
        }

        case OP_FORITER: {
            int end = RD16();
            /* Get the iterator from the stack, update it in place */
            Value* itp = &TOP();
            if (itp->type == V_RANGE_ITER) {
                if (itp->as_range.step > 0 && itp->as_range.cur < itp->as_range.stop) {
                    PUSH(v_int(itp->as_range.cur));
                    itp->as_range.cur += itp->as_range.step;
                } else if (itp->as_range.step < 0 && itp->as_range.cur > itp->as_range.stop) {
                    PUSH(v_int(itp->as_range.cur));
                    itp->as_range.cur += itp->as_range.step;
                } else {
                    pc = end;
                }
            } else if (itp->type == V_LIST_ITER) {
                ListIter* li = &itp->as_list_iter;
                if (li->index < li->list->len) {
                    PUSH(li->list->items[li->index]);
                    li->index++;
                } else {
                    pc = end;
                }
            } else {
                fprintf(stderr,"Error: not iterable\n"); longjmp(vm_err,1);
            }
            break;
        }

        case OP_LIST: {
            int n = RD8();
            ListObj* l = list_new();
            list_ensure(l, n);
            l->len = n;
            Value* items = sp - n;
            memcpy(l->items, items, n * sizeof(Value));
            sp -= n;
            PUSH(v_list(l));
            break;
        }

        case OP_GET: {
            Value idx = POP();
            Value obj = POP();
            if (idx.type!=V_INT) { fprintf(stderr,"Error: index must be int\n"); longjmp(vm_err,1); }
            int i = (int)idx.as_int;
            if (obj.type==V_STR) {
                if (i < 0) i += obj.as_str->len;
                if (i<0||i>=obj.as_str->len) { fprintf(stderr,"Error: string index out of range\n"); longjmp(vm_err,1); }
                char buf[2] = {obj.as_str->s[i], 0};
                PUSH(v_str(str_intern(buf,1)));
            } else if (obj.type==V_LIST) {
                if (i < 0) i += obj.as_list->len;
                if (i<0||i>=obj.as_list->len) { fprintf(stderr,"Error: list index out of range\n"); longjmp(vm_err,1); }
                PUSH(obj.as_list->items[i]);
            } else {
                fprintf(stderr,"Error: subscript on non-subscriptable\n"); longjmp(vm_err,1);
            }
            break;
        }

        case OP_SET: {
            Value val = POP();
            Value idx = POP();
            Value lst = POP();
            if (lst.type!=V_LIST) { fprintf(stderr,"Error: subscript on non-list\n"); longjmp(vm_err,1); }
            if (idx.type!=V_INT) { fprintf(stderr,"Error: index must be int\n"); longjmp(vm_err,1); }
            int i = (int)idx.as_int;
            if (i < 0) i += lst.as_list->len;
            if (i<0||i>=lst.as_list->len) { fprintf(stderr,"Error: list index out of range\n"); longjmp(vm_err,1); }
            lst.as_list->items[i] = val;
            break;
        }

        case OP_APPEND: {
            Value val = POP();
            Value lst = POP();
            if (lst.type!=V_LIST) { fprintf(stderr,"Error: append on non-list\n"); longjmp(vm_err,1); }
            list_ensure(lst.as_list, lst.as_list->len + 1);
            lst.as_list->items[lst.as_list->len++] = val;
            PUSH(lst);
            break;
        }

        case OP_POP_LIST: {
            Value lst = POP();
            if (lst.type!=V_LIST) { fprintf(stderr,"Error: pop on non-list\n"); longjmp(vm_err,1); }
            if (lst.as_list->len == 0) { fprintf(stderr,"Error: pop from empty list\n"); longjmp(vm_err,1); }
            Value ret = lst.as_list->items[--lst.as_list->len];
            PUSH(ret);
            break;
        }

        case OP_UPPER: {
            Value s = POP();
            if (s.type != V_STR) { fprintf(stderr,"Error: expected string\n"); longjmp(vm_err,1); }
            int len = s.as_str->len;
            char* buf = (char*)alloca(len + 1);
            for (int i = 0; i < len; i++) {
                char c = s.as_str->s[i];
                buf[i] = (c >= 'a' && c <= 'z') ? c - 32 : c;
            }
            buf[len] = 0;
            PUSH(v_str(str_intern(buf, len)));
            break;
        }

        case OP_LOWER: {
            Value s = POP();
            if (s.type != V_STR) { fprintf(stderr,"Error: expected string\n"); longjmp(vm_err,1); }
            int len = s.as_str->len;
            char* buf = (char*)alloca(len + 1);
            for (int i = 0; i < len; i++) {
                char c = s.as_str->s[i];
                buf[i] = (c >= 'A' && c <= 'Z') ? c + 32 : c;
            }
            buf[len] = 0;
            PUSH(v_str(str_intern(buf, len)));
            break;
        }

        case OP_STRIP: {
            Value s = POP();
            if (s.type != V_STR) { fprintf(stderr,"Error: expected string\n"); longjmp(vm_err,1); }
            int len = s.as_str->len;
            const char* data = s.as_str->s;
            int start = 0;
            while (start < len && (data[start] == ' ' || data[start] == '\t' || data[start] == '\n' || data[start] == '\r')) start++;
            int end = len;
            while (end > start && (data[end-1] == ' ' || data[end-1] == '\t' || data[end-1] == '\n' || data[end-1] == '\r')) end--;
            PUSH(v_str(str_intern(data + start, end - start)));
            break;
        }

        case OP_CALL: {
            int nargs = RD8();
            Value* args = sp - nargs;
            Value callee = args[-1]; /* callee is below args */

            if (callee.type == V_FUNC) {
                FuncObj* f = callee.as_func;
                /* Save frame */
                if (frame_count >= FRAMES_MAX) { fprintf(stderr,"Error: call stack overflow\n"); longjmp(vm_err,1); }
                int n = nargs < f->nparams ? nargs : f->nparams;
                /* Save frame */
                if (frame_count >= FRAMES_MAX) { fprintf(stderr,"Error: call stack overflow\n"); longjmp(vm_err,1); }
                /* Save frame */
                if (frame_count >= FRAMES_MAX) { fprintf(stderr,"Error: call stack overflow\n"); longjmp(vm_err,1); }
                Frame* fr = &frames[frame_count++];
                fr->pc = pc;
                fr->sp = sp;         /* save BEFORE adjusting sp */
                fr->nargs = nargs;
                memcpy(fr->locals, locals, sizeof(Value)*nlocals);
                fr->nlocals = nlocals;

                /* Set up new frame */
                memset(locals, 0, sizeof(locals));
                for (int i = 0; i < n; i++) locals[i] = args[i];
                nlocals = f->nlocals;

                sp -= nargs + 1; /* pop args and callee */
                pc = f->bc_start;
            } else if (callee.type == V_BUILTIN) {
                /* Builtin functions: print, len, range */
                switch (callee.as_builtin_id) {
                case 0: { /* print */
                    /* already handled as OP_PRINT, but if called as function */
                    if (nargs > 0) {
                        /* print function should be in globals... but we didn't set it up that way */
                        /* For now, just handle via the OP_PRINT opcode */
                    }
                    sp -= nargs + 1;
                    PUSH(v_none());
                    break;
                }
                case 1: { /* range */
                    if (nargs == 1) {
                        if (args[0].type != V_INT) { fprintf(stderr,"range() expects int\n"); longjmp(vm_err,1); }
                        sp -= 2; /* pop arg and callee */
                        PUSH(v_range_iter(0,(int)args[0].as_int,1));
                    } else if (nargs == 2) {
                        if (args[0].type!=V_INT||args[1].type!=V_INT) { fprintf(stderr,"range() expects int\n"); longjmp(vm_err,1); }
                        sp -= 3;
                        PUSH(v_range_iter((int)args[0].as_int,(int)args[1].as_int,1));
                    } else { fprintf(stderr,"range() bad args\n"); longjmp(vm_err,1); }
                    break;
                }
                default:
                    sp -= nargs + 1;
                    PUSH(v_none());
                    break;
                }
            } else {
                fprintf(stderr,"Error: calling non-function\n");
                longjmp(vm_err,1);
            }
            break;
        }

        case OP_RET: {
            Value ret = POP();
            if (frame_count == 0) {
                /* Top-level return */
                PUSH(ret);
                return;
            }
            Frame* fr = &frames[--frame_count];
            pc = fr->pc;
            /* Restore sp to before the call (just past the last arg),
               then back up to place retval at the callee position */
            sp = fr->sp - fr->nargs - 1;
            *sp = ret;   /* write retval where the callee was */
            sp += 1;     /* advance past retval */
            memcpy(locals, fr->locals, sizeof(Value)*fr->nlocals);
            nlocals = fr->nlocals;
            break;
        }

        case OP_MKFUNC: {
            int body = RD16();
            int nparams = RD8();
            int nloc = RD8();
            FuncObj* f = (FuncObj*)malloc(sizeof(FuncObj));
            if (!f) { fprintf(stderr,"Out of memory\n"); exit(1); }
            f->bc_start = body;
            f->nparams = nparams;
            f->nlocals = nloc;
            PUSH(v_func(f));
            break;
        }

        default:
            fprintf(stderr,"Error: unknown opcode %d at pc=%d\n",op,pc-1);
            longjmp(vm_err,1);
        }
    }
}

/* ================================================================
 * Top-level execution
 * ================================================================ */
static void run_ext(const char* src, int keep_globals);

static void run(const char* src) {
    run_ext(src, 0);
}

static void run_ext(const char* src, int keep_globals) {
    /* Compile */
    uint8_t* bytecode = (uint8_t*)malloc(BYTECODE_MAX);
    if (!bytecode) { fprintf(stderr,"Out of memory\n"); exit(1); }

    Compiler comp;
    comp_init(&comp, bytecode, BYTECODE_MAX, src);
    next(&comp);

    /* Compile top-level statements */
    while (comp.tok != T_EOF) {
        if (comp.tok == T_NEWLINE || comp.tok == T_DEDENT) { next(&comp); continue; }
        stmt(&comp);
        if (comp.tok == T_NEWLINE) next(&comp);
    }
    emit(&comp, OP_HALT);

    /* Setup */
    bc = bytecode;
    bc_len = comp.bc_len;
    sp = stack;
    if (!keep_globals) memset(globals, 0, sizeof(globals));
    frame_count = 0;

    /* Execute */
    vm_exec(0);

    free(bytecode);
}

/* ================================================================
 * Main
 * ================================================================ */
static char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { perror(path); return NULL; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz >= SOURCE_MAX) { fprintf(stderr,"File too large\n"); fclose(f); return NULL; }
    rewind(f);
    char* buf = (char*)malloc(sz+1);
    if (!buf) { fprintf(stderr,"Out of memory\n"); fclose(f); return NULL; }
    size_t n = fread(buf, 1, sz, f);
    buf[n] = 0;
    fclose(f);
    return buf;
}

static void repl(void) {
    printf("pyte 0.1 (type 'exit()' to quit)\n");
    char inbuf[16384];
    int inpos = 0;
    int need_more = 0;

    while (1) {
        printf(need_more ? ".  ": "> ");
        fflush(stdout);

        if (!fgets(inbuf + inpos, (int)(sizeof(inbuf) - inpos), stdin)) {
            printf("\n");
            break;
        }
        int len = (int)strlen(inbuf + inpos);
        inpos += len;

        /* Quick exit check — strip trailing newline first */
        if (inpos > 0 && inbuf[inpos-1] == '\n') inbuf[--inpos] = 0;
        if (strcmp(inbuf, "exit()") == 0) break;
        /* Put newline back for compilation */
        if (inbuf[inpos] == 0 && inpos < (int)sizeof(inbuf)-1) {
            inbuf[inpos++] = '\n';
            inbuf[inpos] = 0;
        }

        /* Count leading whitespace on the last line to track indentation */
        int last_start = inpos - len;
        if (last_start < 0) last_start = 0;
        int indent = 0;
        for (int i = last_start; i < inpos && inbuf[i] == ' '; i++) indent++;

        /* Check if last meaningful line ends with ':' (block start) */
        int ends_with_colon = 0;
        for (int i = last_start; i < inpos; i++) {
            if (inbuf[i] == ':') {
                int j = i + 1;
                while (j < inpos && (inbuf[j] == ' ' || inbuf[j] == '\t')) j++;
                if (j >= inpos || inbuf[j] == '\n' || inbuf[j] == 0) {
                    ends_with_colon = 1;
                    break;
                }
            }
        }

        if (ends_with_colon) {
            need_more = 1;
            continue;
        }

        /* If we needed more and got a non-indented line, execute */
        if (need_more && indent == 0 && len > 1) {
            need_more = 0;
        }

        if (!need_more) {
            /* Strip trailing newline for compilation */
            while (inpos > 0 && (inbuf[inpos-1] == '\n' || inbuf[inpos-1] == '\r')) inpos--;
            inbuf[inpos] = 0;

            if (inpos == 0) continue;

            /* Compile and execute (keep globals across lines) */
            run_ext(inbuf, 1);

            inpos = 0;
            inbuf[0] = 0;
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        repl();
        return 0;
    }

    const char* src;
    char* buf = NULL;

    if (argc == 3 && strcmp(argv[1], "-e") == 0) {
        src = argv[2];
    } else {
        buf = read_file(argv[1]);
        if (!buf) return 1;
        src = buf;
    }

    run(src);
    free(buf);
    return 0;
}
