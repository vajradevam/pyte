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

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <setjmp.h>

#include <unistd.h>
#include <fcntl.h>

static void pstr(const char* s) { write(1, s, strlen(s)); }
static void perr(const char* s) { write(2, s, strlen(s)); }
static void perr_int(long long n) {
    char buf[32], *p = buf + sizeof(buf);
    unsigned long long u;
    if (n < 0) { u = 0ULL - (unsigned long long)n; } else { u = n; }
    *--p = 0;
    do { *--p = '0' + u % 10; u /= 10; } while (u);
    if (n < 0) *--p = '-';
    write(2, p, buf + sizeof(buf) - 1 - p);
}
static void newline(void) { write(1, "\n", 1); }

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
    V_FUNC, V_BUILTIN, V_RANGE_ITER
} VType;

typedef struct Value Value;
typedef struct ListObj  { int len, cap; Value* items; } ListObj;
typedef struct FuncObj  { int bc_start, nparams, nlocals; } FuncObj;
typedef struct RangeIter { int start, stop, step, cur; } RangeIter;
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

static int is_truthy(Value v) {
    switch (v.type) {
    case V_NONE:  return 0;
    case V_BOOL:  return v.as_bool;
    case V_INT:   return v.as_int != 0;
    case V_FLOAT: return v.as_float != 0.0;
    case V_STR:   return v.as_str->len > 0;
    case V_LIST:  return v.as_list->len > 0;
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
    if (str_arena_pos + n > STRING_POOL_MAX) { perr("pool\n"); exit(1); }
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
    if (list_arena_pos + sz > LIST_ARENA_MAX) { perr("lstfull\n"); exit(1); }
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
        if (c == '\t') { perr("tabs ");perr_int(L->line);perr("\n"); exit(1); }
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
            perr("indent ");perr_int(L->line);perr("\n"); exit(1);
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
        if (!lex_peek(L)) { perr("unterm ");perr_int(L->line);perr("\n"); exit(1); }
        int len = L->pos - start;
        char buf[1024];
        if (len >= 1023) { perr("strlong\n"); exit(1); }
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
    if (((c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_') || c == '_') {
        int start = L->pos;
        while (((lex_peek(L)>='a'&&lex_peek(L)<='z')||(lex_peek(L)>='A'&&lex_peek(L)<='Z')||(lex_peek(L)>='0'&&lex_peek(L)<='9')||lex_peek(L)=='_') || lex_peek(L) == '_') lex_adv(L);
        int len = L->pos - start;
        if (len >= IDENT_MAX-1) { perr("idlong\n"); exit(1); }
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
    perr("badchar\n");
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
    OP_IN,          /* x in list/string */
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
    if (C->bc_len >= C->bc_cap) { perr("bcfull\n"); exit(1); }
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
        perr("expect\n");
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
        perr("badtok\n");
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
                perr("notin\n"); exit(1);
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
        if (C->tok != T_IDENT) { perr("forid\n"); exit(1); }
        char vname[IDENT_MAX]; strcpy(vname, C->lex.ident);
        next(C);
        expect(C, T_KW_IN);

        /* Only range(...) is supported */
        if (C->tok != T_KW_RANGE) { perr("norange\n"); exit(1); }
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
        emit(C, OP_POP); /* pop range iterator */

        /* Patch all break jumps to exit here */
        for (int i = 0; i < C->loops[saved_depth2].n_break_patches; i++)
            patch16(C, C->loops[saved_depth2].break_patches[i]);
        break;
    }
    case T_KW_DEF: {
        /* def name(params): body */
        next(C);
        if (C->tok != T_IDENT) { perr("fnname\n"); exit(1); }
        char fname[IDENT_MAX]; strcpy(fname, C->lex.ident);
        int slot = gslot(C, fname);
        next(C); expect(C, T_LPAREN);

        /* Parse param names */
        char params[LOCALS_MAX][IDENT_MAX];
        int nparams = 0;
        if (C->tok != T_RPAREN) {
            while (1) {
                if (C->tok != T_IDENT) { perr("param\n"); exit(1); }
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
        if (C->depth == 0) { perr("brkout\n"); exit(1); }
        int p = bcpos(C);
        emit(C, OP_JUMP); emit16(C, 0);
        LoopContext* lc = &C->loops[C->depth - 1];
        if (lc->n_break_patches < 64) lc->break_patches[lc->n_break_patches++] = p + 1;
        break;
    }
    case T_KW_CONTINUE: {
        next(C);
        if (C->depth == 0) { perr("contout\n"); exit(1); }
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
            } else {
                perr("method\n"); exit(1);
            }
        } else {
            /* Expression as statement - discard */
            
        }
        break;
    }
    case T_NEWLINE:
        next(C);
        break;
    default:
        perr("badtok\n");
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
    case V_NONE:  pstr("None"); break;
    case V_BOOL:  pstr(v.as_bool ? "True" : "False"); break;
    case V_INT:   { char b[32],*p=b+32;unsigned long long u=v.as_int<0?0ULL-(unsigned long long)v.as_int:(unsigned long long)v.as_int;*--p=0;do{*--p='0'+u%10;u/=10;}while(u);if(v.as_int<0)*--p='-';write(1,p,b+31-p);} break;
    case V_FLOAT: {
        double f = v.as_float;
        char b[64], *p = b;
        if (f < 0) { *p++ = '-'; f = -f; }
        if (f != f) { pstr("nan"); break; }
        if (f > 1e15) { pstr("inf"); break; }
        long long ip = (long long)f;
        double fp = f - (double)ip;
        /* Print integer part */
        char ibuf[32], *ip2 = ibuf + sizeof(ibuf);
        { unsigned long long u = ip < 0 ? 0ULL - (unsigned long long)ip : (unsigned long long)ip;
          *--ip2 = 0; do { *--ip2 = '0' + u % 10; u /= 10; } while (u);
          while (*ip2) *p++ = *ip2++; }
        if (fp != 0.0) {
            *p++ = '.';
            for (int i = 0; i < 10; i++) {
                fp *= 10; int d = (int)fp;
                *p++ = '0' + d; fp -= d;
                if (fp == 0.0) break;
            }
            while (p > b+1 && p[-1] == '0') p--;
        }
        write(1, b, p - b);
        break;
    }
    case V_STR:   pstr(v.as_str->s); break;
    case V_LIST: {
        pstr("[");
        for (int i=0;i<v.as_list->len;i++) {
            if(i) pstr(", ");
            print_val(v.as_list->items[i]);
        }
        pstr("]");
        break;
    }
    case V_FUNC:  pstr("<function>"); break;
    case V_BUILTIN: pstr("<builtin ");{char b[32],*p=b+32;unsigned long long u=v.as_builtin_id;*--p=0;do{*--p='0'+u%10;u/=10;}while(u);write(1,p,b+31-p);}pstr(">"); break;
    case V_RANGE_ITER: pstr("<range>"); break;
    }
}

static void print_vals(Value* a, int n) {
    for (int i=0;i<n;i++) { if(i) pstr(" "); print_val(a[i]); }
    newline();
}

static Value add_val(Value a, Value b) {
    if (a.type==V_INT && b.type==V_INT) return v_int(a.as_int+b.as_int);
    if (a.type==V_FLOAT && b.type==V_FLOAT) return v_float(a.as_float+b.as_float);
    if (a.type==V_INT && b.type==V_FLOAT) return v_float(a.as_int+b.as_float);
    if (a.type==V_FLOAT && b.type==V_INT) return v_float(a.as_float+b.as_int);
    if (a.type==V_STR && b.type==V_STR) {
        int l = a.as_str->len + b.as_str->len;
        char buf[4096]; if (l>=4095) { perr("strlong\n"); longjmp(vm_err,1); }
        memcpy(buf,a.as_str->s,a.as_str->len);
        memcpy(buf+a.as_str->len,b.as_str->s,b.as_str->len);
        return v_str(str_intern(buf,l));
    }
    perr("bad+\n"); longjmp(vm_err,1);
    return v_none();
}
static Value sub_val(Value a, Value b) {
    if (a.type==V_INT && b.type==V_INT) return v_int(a.as_int-b.as_int);
    if (a.type==V_FLOAT && b.type==V_FLOAT) return v_float(a.as_float-b.as_float);
    if (a.type==V_INT && b.type==V_FLOAT) return v_float(a.as_int-b.as_float);
    if (a.type==V_FLOAT && b.type==V_INT) return v_float(a.as_float-b.as_int);
    perr("bad-\n"); longjmp(vm_err,1);
    return v_none();
}
static Value mul_val(Value a, Value b) {
    if (a.type==V_INT && b.type==V_INT) return v_int(a.as_int*b.as_int);
    if (a.type==V_FLOAT && b.type==V_FLOAT) return v_float(a.as_float*b.as_float);
    if (a.type==V_INT && b.type==V_FLOAT) return v_float(a.as_int*b.as_float);
    if (a.type==V_FLOAT && b.type==V_INT) return v_float(a.as_float*b.as_int);
    perr("bad*\n"); longjmp(vm_err,1);
    return v_none();
}
static Value div_val(Value a, Value b) {
    if (a.type==V_INT && b.type==V_INT) {
        if (b.as_int==0) { perr("div0\n"); longjmp(vm_err,1); }
        return v_int(a.as_int/b.as_int);
    }
    if (a.type==V_FLOAT && b.type==V_FLOAT) {
        if (b.as_float==0.0) { perr("div0\n"); longjmp(vm_err,1); }
        return v_float((double)((int)(a.as_float/b.as_float)));
    }
    if (a.type==V_INT && b.type==V_FLOAT) {
        if (b.as_float==0.0) { perr("div0\n"); longjmp(vm_err,1); }
        return v_float((double)((int)(a.as_int/b.as_float)));
    }
    if (a.type==V_FLOAT && b.type==V_INT) {
        if (b.as_int==0) { perr("div0\n"); longjmp(vm_err,1); }
        return v_float((double)((int)(a.as_float/b.as_int)));
    }
    perr("bad//\n"); longjmp(vm_err,1);
    return v_none();
}
static Value mod_val(Value a, Value b) {
    if (a.type==V_INT && b.type==V_INT) {
        if (b.as_int==0) { perr("mod0\n"); longjmp(vm_err,1); }
        return v_int(a.as_int%b.as_int);
    }
    perr("bad%%\n"); longjmp(vm_err,1);
    return v_none();
}
static Value neg_val(Value a) {
    if (a.type==V_INT) return v_int(-a.as_int);
    if (a.type==V_FLOAT) return v_float(-a.as_float);
    perr("badneg\n"); longjmp(vm_err,1);
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
        perr("badcmp\n"); longjmp(vm_err,1);
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
            else { perr("lentype\n"); longjmp(vm_err,1); }
            break;
        }

        case OP_RANGE: {
            Value b = POP();
            Value a = POP();
            if (a.type != V_INT || b.type != V_INT) {
                perr("rangeint\n"); longjmp(vm_err,1);
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
                else { {int fl=container.as_str->len,sl=item.as_str->len;found=0;for(int si=0;si<=fl-sl;si++){int m=1;for(int sj=0;sj<sl;sj++){if(container.as_str->s[si+sj]!=item.as_str->s[sj]){m=0;break;}}if(m){found=1;break;}}} }
            } else {
                perr("intype\n"); longjmp(vm_err,1);
            }
            PUSH(v_bool(found));
            break;
        }

        case OP_FORITER: {
            int end = RD16();
            /* Get the range iterator from the stack, update it in place */
            Value* itp = &TOP();
            if (itp->type != V_RANGE_ITER) { perr("noiter\n"); longjmp(vm_err,1); }
            if (itp->as_range.step > 0 && itp->as_range.cur < itp->as_range.stop) {
                PUSH(v_int(itp->as_range.cur));
                itp->as_range.cur += itp->as_range.step;
            } else if (itp->as_range.step < 0 && itp->as_range.cur > itp->as_range.stop) {
                PUSH(v_int(itp->as_range.cur));
                itp->as_range.cur += itp->as_range.step;
            } else {
                pc = end;
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
            if (idx.type!=V_INT) { perr("idxint\n"); longjmp(vm_err,1); }
            int i = (int)idx.as_int;
            if (obj.type==V_STR) {
                if (i < 0) i += obj.as_str->len;
                if (i<0||i>=obj.as_str->len) { perr("stridx\n"); longjmp(vm_err,1); }
                char buf[2] = {obj.as_str->s[i], 0};
                PUSH(v_str(str_intern(buf,1)));
            } else if (obj.type==V_LIST) {
                if (i < 0) i += obj.as_list->len;
                if (i<0||i>=obj.as_list->len) { perr("lstidx\n"); longjmp(vm_err,1); }
                PUSH(obj.as_list->items[i]);
            } else {
                perr("nosub\n"); longjmp(vm_err,1);
            }
            break;
        }

        case OP_SET: {
            Value val = POP();
            Value idx = POP();
            Value lst = POP();
            if (lst.type!=V_LIST) { perr("nosubl\n"); longjmp(vm_err,1); }
            if (idx.type!=V_INT) { perr("idxint\n"); longjmp(vm_err,1); }
            int i = (int)idx.as_int;
            if (i < 0) i += lst.as_list->len;
            if (i<0||i>=lst.as_list->len) { perr("lstidx\n"); longjmp(vm_err,1); }
            lst.as_list->items[i] = val;
            PUSH(val);
            break;
        }

        case OP_APPEND: {
            Value val = POP();
            Value lst = POP();
            if (lst.type!=V_LIST) { perr("noapp\n"); longjmp(vm_err,1); }
            list_ensure(lst.as_list, lst.as_list->len + 1);
            lst.as_list->items[lst.as_list->len++] = val;
            PUSH(lst);
            break;
        }

        case OP_POP_LIST: {
            Value lst = POP();
            if (lst.type!=V_LIST) { perr("nopop\n"); longjmp(vm_err,1); }
            if (lst.as_list->len == 0) { perr("popemp\n"); longjmp(vm_err,1); }
            Value ret = lst.as_list->items[--lst.as_list->len];
            PUSH(ret);
            break;
        }

        case OP_CALL: {
            int nargs = RD8();
            Value* args = sp - nargs;
            Value callee = args[-1]; /* callee is below args */

            if (callee.type == V_FUNC) {
                FuncObj* f = callee.as_func;
                /* Save frame */
                if (frame_count >= FRAMES_MAX) { perr("stkovf\n"); longjmp(vm_err,1); }
                int n = nargs < f->nparams ? nargs : f->nparams;
                /* Save frame */
                if (frame_count >= FRAMES_MAX) { perr("stkovf\n"); longjmp(vm_err,1); }
                /* Save frame */
                if (frame_count >= FRAMES_MAX) { perr("stkovf\n"); longjmp(vm_err,1); }
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
                        if (args[0].type != V_INT) { perr("rangeint\n"); longjmp(vm_err,1); }
                        sp -= 2; /* pop arg and callee */
                        PUSH(v_range_iter(0,(int)args[0].as_int,1));
                    } else if (nargs == 2) {
                        if (args[0].type!=V_INT||args[1].type!=V_INT) { perr("rangeint\n"); longjmp(vm_err,1); }
                        sp -= 3;
                        PUSH(v_range_iter((int)args[0].as_int,(int)args[1].as_int,1));
                    } else { perr("rangebad\n"); longjmp(vm_err,1); }
                    break;
                }
                default:
                    sp -= nargs + 1;
                    PUSH(v_none());
                    break;
                }
            } else {
                perr("nofn\n");
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
            if (!f) { perr("oom\n"); exit(1); }
            f->bc_start = body;
            f->nparams = nparams;
            f->nlocals = nloc;
            PUSH(v_func(f));
            break;
        }

        default:
            perr("badop\n");
            longjmp(vm_err,1);
        }
    }
}

/* ================================================================
 * Top-level execution
 * ================================================================ */
static void run(const char* src) {
    /* Compile */
    uint8_t* bytecode = (uint8_t*)malloc(BYTECODE_MAX);
    if (!bytecode) { perr("oom\n"); exit(1); }

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
    memset(globals, 0, sizeof(globals));
    frame_count = 0;

    /* execute */

    /* Execute */
    vm_exec(0);

    free(bytecode);
}

/* ================================================================
 * Main
 * ================================================================ */
static char* read_file(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) { perr("noopen\n"); return NULL; }
    long sz = lseek(fd, 0, SEEK_END);
    if (sz >= SOURCE_MAX) { perr("bigfile\n"); close(fd); return NULL; }
    lseek(fd, 0, SEEK_SET);
    char* buf = (char*)malloc(sz+1);
    if (!buf) { perr("oom\n"); close(fd); return NULL; }
    int n = (int)read(fd, buf, sz);
    buf[n > 0 ? n : 0] = 0;
    close(fd);
    return buf;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        perr("Usage: "); perr(argv[0]); perr(" file.py\n");
        perr("       "); perr(argv[0]); perr(" -e code\n");
        return 1;
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
