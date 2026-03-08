/*
 * ============================================================================
 *  RAMPYAARYAN - Compiler Implementation
 *  Single-pass Pratt parser that directly emits bytecode
 *  Converts tokens -> bytecode (no separate AST pass)
 * ============================================================================
 */

#include "compiler.h"
#include "lexer.h"
#include "memory.h"
#include "vm.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

/* ============================================================================
 *  PRECEDENCE LEVELS (lowest to highest)
 * ============================================================================ */
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,   /* =                */
    PREC_OR,           /* ya               */
    PREC_AND,          /* aur              */
    PREC_NOT,          /* nahi             */
    PREC_EQUALITY,     /* == !=            */
    PREC_COMPARISON,   /* < > <= >=        */
    PREC_TERM,         /* + -              */
    PREC_FACTOR,       /* * / %            */
    PREC_POWER,        /* **               */
    PREC_UNARY,        /* - nahi           */
    PREC_CALL,         /* . () []          */
    PREC_PRIMARY,
} Precedence;

/* ============================================================================
 *  PARSER STATE
 * ============================================================================ */
typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
    Lexer lexer;
    VM* vm;
} Parser;

/* Parse function type */
typedef void (*ParseFn)(Parser* parser, bool canAssign);

/* Pratt parser rule */
typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

/* ============================================================================
 *  LOCAL VARIABLE
 * ============================================================================ */
typedef struct {
    Token name;
    int depth;
    bool isCaptured;
} Local;

/* Upvalue */
typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

/* ============================================================================
 *  FUNCTION COMPILER STATE
 * ============================================================================ */
typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT,
} FunctionType;

typedef struct CompilerState {
    struct CompilerState* enclosing;
    ObjFunction* function;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount;
    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth;

    /* Loop tracking for break/continue */
    int loopStart;
    int loopScopeDepth;
    int breakJumps[256];
    int breakCount;
} CompilerState;

/* ============================================================================
 *  GLOBAL COMPILER POINTER (for GC)
 * ============================================================================ */
static CompilerState* currentCompilerState = NULL;

/* ============================================================================
 *  ERROR REPORTING
 * ============================================================================ */
static void errorAt(Parser* parser, Token* token, const char* message) {
    if (parser->panicMode) return;
    parser->panicMode = true;
    parser->hadError = true;

    fprintf(stderr, "\n  \xe2\x9d\x8c [Line %d] Galti", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " file ke end mein");
    } else if (token->type == TOKEN_ERROR) {
        /* Nothing */
    } else {
        fprintf(stderr, " '%.*s' ke paas", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
}

static void error(Parser* parser, const char* message) {
    errorAt(parser, &parser->previous, message);
}

static void errorAtCurrent(Parser* parser, const char* message) {
    errorAt(parser, &parser->current, message);
}

/* ============================================================================
 *  TOKEN CONSUMPTION
 * ============================================================================ */
static void advanceParser(Parser* parser) {
    parser->previous = parser->current;

    for (;;) {
        parser->current = scanToken(&parser->lexer);
        if (parser->current.type != TOKEN_ERROR) break;
        errorAtCurrent(parser, parser->current.start);
    }
}

static void consume(Parser* parser, RamTokenType type, const char* message) {
    if (parser->current.type == type) {
        advanceParser(parser);
        return;
    }
    errorAtCurrent(parser, message);
}

static bool check(Parser* parser, RamTokenType type) {
    return parser->current.type == type;
}

static bool matchToken(Parser* parser, RamTokenType type) {
    if (!check(parser, type)) return false;
    advanceParser(parser);
    return true;
}

static void skipNewlines(Parser* parser) {
    while (parser->current.type == TOKEN_NEWLINE) {
        advanceParser(parser);
    }
}

static void consumeNewlineOrEnd(Parser* parser) {
    if (check(parser, TOKEN_NEWLINE) || check(parser, TOKEN_EOF) ||
        check(parser, TOKEN_RIGHT_BRACE)) {
        if (check(parser, TOKEN_NEWLINE)) advanceParser(parser);
        return;
    }
    errorAtCurrent(parser, "Statement ke baad nayi line ya '}' expected tha.");
}

/* ============================================================================
 *  BYTECODE EMISSION
 * ============================================================================ */
static CompilerState* currentCS(Parser* parser) {
    UNUSED(parser);
    return currentCompilerState;
}

static Chunk* currentChunk(Parser* parser) {
    return currentCS(parser)->function->chunk;
}

static void emitByte(Parser* parser, uint8_t byte) {
    writeChunk(currentChunk(parser), byte, parser->previous.line);
}

static void emitBytes(Parser* parser, uint8_t byte1, uint8_t byte2) {
    emitByte(parser, byte1);
    emitByte(parser, byte2);
}

static void emitReturn(Parser* parser) {
    emitByte(parser, OP_NULL);
    emitByte(parser, OP_RETURN);
}

static int emitJump(Parser* parser, uint8_t instruction) {
    emitByte(parser, instruction);
    emitByte(parser, 0xff);
    emitByte(parser, 0xff);
    return currentChunk(parser)->count - 2;
}

static void patchJump(Parser* parser, int offset) {
    int jump = currentChunk(parser)->count - offset - 2;
    if (jump > UINT16_MAX) {
        error(parser, "Bahut bada jump hai. Code chhota karo.");
    }
    currentChunk(parser)->code[offset] = (jump >> 8) & 0xff;
    currentChunk(parser)->code[offset + 1] = jump & 0xff;
}

static void emitLoop(Parser* parser, int loopStart) {
    emitByte(parser, OP_LOOP);
    int offset = currentChunk(parser)->count - loopStart + 2;
    if (offset > UINT16_MAX) {
        error(parser, "Loop body bahut bada hai.");
    }
    emitByte(parser, (offset >> 8) & 0xff);
    emitByte(parser, offset & 0xff);
}

static uint8_t makeConstant(Parser* parser, Value value) {
    int constant = addConstant(currentChunk(parser), value);
    if (constant > UINT8_MAX) {
        error(parser, "Ek chunk mein bahut saare constants hain.");
        return 0;
    }
    return (uint8_t)constant;
}

static void emitConstant(Parser* parser, Value value) {
    emitBytes(parser, OP_CONSTANT, makeConstant(parser, value));
}

/* ============================================================================
 *  COMPILER INITIALIZATION
 * ============================================================================ */
static void initCompilerState(Parser* parser, CompilerState* cs, FunctionType type) {
    cs->enclosing = currentCompilerState;
    cs->function = NULL;
    cs->type = type;
    cs->localCount = 0;
    cs->scopeDepth = 0;
    cs->loopStart = -1;
    cs->loopScopeDepth = 0;
    cs->breakCount = 0;

    cs->function = newFunction(parser->vm);
    currentCompilerState = cs;

    if (type != TYPE_SCRIPT) {
        cs->function->name = copyString(parser->vm,
            parser->previous.start, parser->previous.length);
    }

    /* Slot 0 is for the function itself */
    Local* local = &cs->locals[cs->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    local->name.start = "";
    local->name.length = 0;
}

static ObjFunction* endCompiler(Parser* parser) {
    emitReturn(parser);
    ObjFunction* function = currentCS(parser)->function;

#ifdef DEBUG_PRINT_CODE
    if (!parser->hadError) {
        disassembleChunk(currentChunk(parser),
            function->name != NULL ? function->name->chars : "<script>");
    }
#endif

    currentCompilerState = currentCS(parser)->enclosing;
    return function;
}

/* ============================================================================
 *  SCOPING
 * ============================================================================ */
static void beginScope(Parser* parser) {
    currentCS(parser)->scopeDepth++;
}

static void endScope(Parser* parser) {
    CompilerState* cs = currentCS(parser);
    cs->scopeDepth--;

    while (cs->localCount > 0 &&
           cs->locals[cs->localCount - 1].depth > cs->scopeDepth) {
        if (cs->locals[cs->localCount - 1].isCaptured) {
            emitByte(parser, OP_CLOSE_UPVALUE);
        } else {
            emitByte(parser, OP_POP);
        }
        cs->localCount--;
    }
}

/* ============================================================================
 *  VARIABLE RESOLUTION
 * ============================================================================ */
static bool identifiersEqual(Token* a, Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Parser* parser, CompilerState* cs, Token* name) {
    for (int i = cs->localCount - 1; i >= 0; i--) {
        Local* local = &cs->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error(parser, "Variable ko apni hi definition mein use nahi kar sakte.");
            }
            return i;
        }
    }
    return -1;
}

static int addUpvalue(Parser* parser, CompilerState* cs, uint8_t index, bool isLocal) {
    int upvalueCount = cs->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &cs->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error(parser, "Ek function mein bahut zyada closure variables hain.");
        return 0;
    }

    cs->upvalues[upvalueCount].isLocal = isLocal;
    cs->upvalues[upvalueCount].index = index;
    return cs->function->upvalueCount++;
}

static int resolveUpvalue(Parser* parser, CompilerState* cs, Token* name) {
    if (cs->enclosing == NULL) return -1;

    int local = resolveLocal(parser, cs->enclosing, name);
    if (local != -1) {
        cs->enclosing->locals[local].isCaptured = true;
        return addUpvalue(parser, cs, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(parser, cs->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(parser, cs, (uint8_t)upvalue, false);
    }

    return -1;
}

static void addLocal(Parser* parser, Token name) {
    CompilerState* cs = currentCS(parser);
    if (cs->localCount == UINT8_COUNT) {
        error(parser, "Ek scope mein bahut zyada variables hain.");
        return;
    }
    Local* local = &cs->locals[cs->localCount++];
    local->name = name;
    local->depth = -1; /* Mark as uninitialized */
    local->isCaptured = false;
}

static void declareVariable(Parser* parser) {
    CompilerState* cs = currentCS(parser);
    if (cs->scopeDepth == 0) return;

    Token* name = &parser->previous;
    for (int i = cs->localCount - 1; i >= 0; i--) {
        Local* local = &cs->locals[i];
        if (local->depth != -1 && local->depth < cs->scopeDepth) break;
        if (identifiersEqual(name, &local->name)) {
            error(parser, "Is scope mein ye variable pehle se hai.");
        }
    }

    addLocal(parser, *name);
}

static uint8_t identifierConstant(Parser* parser, Token* name) {
    return makeConstant(parser, OBJ_VAL(
        copyString(parser->vm, name->start, name->length)));
}

static void markInitialized(Parser* parser) {
    CompilerState* cs = currentCS(parser);
    if (cs->scopeDepth == 0) return;
    cs->locals[cs->localCount - 1].depth = cs->scopeDepth;
}

static uint8_t parseVariable(Parser* parser, const char* errorMessage) {
    consume(parser, TOKEN_IDENTIFIER, errorMessage);

    declareVariable(parser);
    if (currentCS(parser)->scopeDepth > 0) return 0;

    return identifierConstant(parser, &parser->previous);
}

static void defineVariable(Parser* parser, uint8_t global) {
    if (currentCS(parser)->scopeDepth > 0) {
        markInitialized(parser);
        return;
    }
    emitBytes(parser, OP_DEFINE_GLOBAL, global);
}

/* ============================================================================
 *  EXPRESSION PARSING (Forward declarations)
 * ============================================================================ */
static void expression(Parser* parser);
static void statement(Parser* parser);
static void declaration(Parser* parser);
static ParseRule* getRule(RamTokenType type);
static void parsePrecedence(Parser* parser, Precedence precedence);
static void block(Parser* parser);

/* ============================================================================
 *  EXPRESSION PARSERS
 * ============================================================================ */

/* --- NUMBER --- */
static void numberLiteral(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    double value = strtod(parser->previous.start, NULL);
    emitConstant(parser, NUMBER_VAL(value));
}

/* --- STRING --- */
static void processEscapes(const char* src, int srcLen, char* dest, int* destLen) {
    int j = 0;
    for (int i = 0; i < srcLen; i++) {
        if (src[i] == '\\' && i + 1 < srcLen) {
            i++;
            switch (src[i]) {
                case 'n': dest[j++] = '\n'; break;
                case 't': dest[j++] = '\t'; break;
                case 'r': dest[j++] = '\r'; break;
                case '\\': dest[j++] = '\\'; break;
                case '\'': dest[j++] = '\''; break;
                case '"': dest[j++] = '"'; break;
                case '0': dest[j++] = '\0'; break;
                default:
                    dest[j++] = '\\';
                    dest[j++] = src[i];
                    break;
            }
        } else {
            dest[j++] = src[i];
        }
    }
    *destLen = j;
}

static void stringLiteral(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    /* Skip surrounding quotes */
    const char* src = parser->previous.start + 1;
    int srcLen = parser->previous.length - 2;

    char* buffer = (char*)malloc(srcLen + 1);
    int destLen;
    processEscapes(src, srcLen, buffer, &destLen);
    buffer[destLen] = '\0';

    ObjString* str = copyString(parser->vm, buffer, destLen);
    free(buffer);
    emitConstant(parser, OBJ_VAL(str));
}

/* --- GROUPING --- */
static void grouping(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "')' expected tha expression ke baad.");
}

/* --- UNARY --- */
static void unary(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    RamTokenType operatorType = parser->previous.type;
    parsePrecedence(parser, PREC_UNARY);

    switch (operatorType) {
        case TOKEN_MINUS: emitByte(parser, OP_NEGATE); break;
        case TOKEN_NAHI:  emitByte(parser, OP_NOT); break;
        default: return;
    }
}

/* --- BINARY --- */
static void binary(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    RamTokenType operatorType = parser->previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence(parser, (Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_PLUS:          emitByte(parser, OP_ADD); break;
        case TOKEN_MINUS:         emitByte(parser, OP_SUBTRACT); break;
        case TOKEN_STAR:          emitByte(parser, OP_MULTIPLY); break;
        case TOKEN_SLASH:         emitByte(parser, OP_DIVIDE); break;
        case TOKEN_PERCENT:       emitByte(parser, OP_MODULO); break;
        case TOKEN_STAR_STAR:     emitByte(parser, OP_POWER); break;
        case TOKEN_EQUAL_EQUAL:   emitByte(parser, OP_EQUAL); break;
        case TOKEN_BANG_EQUAL:    emitByte(parser, OP_NOT_EQUAL); break;
        case TOKEN_GREATER:       emitByte(parser, OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitByte(parser, OP_GREATER_EQUAL); break;
        case TOKEN_LESS:          emitByte(parser, OP_LESS); break;
        case TOKEN_LESS_EQUAL:    emitByte(parser, OP_LESS_EQUAL); break;
        default: return;
    }
}

/* --- LITERALS --- */
static void literal(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    switch (parser->previous.type) {
        case TOKEN_SACH:   emitByte(parser, OP_TRUE); break;
        case TOKEN_JHOOTH: emitByte(parser, OP_FALSE); break;
        case TOKEN_KHALI:  emitByte(parser, OP_NULL); break;
        default: return;
    }
}

/* --- VARIABLE (get/set) --- */
static void namedVariable(Parser* parser, Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(parser, currentCS(parser), &name);

    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(parser, currentCS(parser), &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(parser, &name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && matchToken(parser, TOKEN_EQUAL)) {
        expression(parser);
        emitBytes(parser, setOp, (uint8_t)arg);
    } else if (canAssign && (check(parser, TOKEN_PLUS_EQUAL) ||
               check(parser, TOKEN_MINUS_EQUAL) ||
               check(parser, TOKEN_STAR_EQUAL) ||
               check(parser, TOKEN_SLASH_EQUAL))) {
        RamTokenType opType = parser->current.type;
        advanceParser(parser);
        emitBytes(parser, getOp, (uint8_t)arg);
        expression(parser);
        switch (opType) {
            case TOKEN_PLUS_EQUAL:  emitByte(parser, OP_ADD); break;
            case TOKEN_MINUS_EQUAL: emitByte(parser, OP_SUBTRACT); break;
            case TOKEN_STAR_EQUAL:  emitByte(parser, OP_MULTIPLY); break;
            case TOKEN_SLASH_EQUAL: emitByte(parser, OP_DIVIDE); break;
            default: break;
        }
        emitBytes(parser, setOp, (uint8_t)arg);
    } else {
        emitBytes(parser, getOp, (uint8_t)arg);
    }
}

static void variable(Parser* parser, bool canAssign) {
    namedVariable(parser, parser->previous, canAssign);
}

/* --- AND (aur) --- */
static void and_(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    int endJump = emitJump(parser, OP_JUMP_IF_FALSE);
    emitByte(parser, OP_POP);
    parsePrecedence(parser, PREC_AND);
    patchJump(parser, endJump);
}

/* --- OR (ya) --- */
static void or_(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    int elseJump = emitJump(parser, OP_JUMP_IF_FALSE);
    int endJump = emitJump(parser, OP_JUMP);

    patchJump(parser, elseJump);
    emitByte(parser, OP_POP);
    parsePrecedence(parser, PREC_OR);
    patchJump(parser, endJump);
}

/* --- FUNCTION CALL --- */
static uint8_t argumentList(Parser* parser) {
    uint8_t argCount = 0;
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            expression(parser);
            if (argCount == 255) {
                error(parser, "255 se zyada arguments nahi de sakte.");
            }
            argCount++;
        } while (matchToken(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RIGHT_PAREN, "')' lagao arguments ke baad.");
    return argCount;
}

static void call(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    uint8_t argCount = argumentList(parser);
    emitBytes(parser, OP_CALL, argCount);
}

/* --- LIST LITERAL --- */
static void listLiteral(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    int itemCount = 0;
    if (!check(parser, TOKEN_RIGHT_BRACKET)) {
        do {
            skipNewlines(parser);
            expression(parser);
            itemCount++;
            skipNewlines(parser);
        } while (matchToken(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RIGHT_BRACKET, "']' lagao list band karne ke liye.");
    emitBytes(parser, OP_LIST_NEW, (uint8_t)itemCount);
}

/* --- INDEX ACCESS --- */
static void index_(Parser* parser, bool canAssign) {
    expression(parser);
    consume(parser, TOKEN_RIGHT_BRACKET, "']' lagao index ke baad.");

    if (canAssign && matchToken(parser, TOKEN_EQUAL)) {
        expression(parser);
        emitByte(parser, OP_LIST_SET);
    } else {
        emitByte(parser, OP_LIST_GET);
    }
}

/* --- INPUT (pucho) --- */
static void inputExpr(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    consume(parser, TOKEN_LEFT_PAREN, "'(' lagao 'pucho' ke baad.");
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        expression(parser); /* prompt string */
    } else {
        emitConstant(parser, OBJ_VAL(copyString(parser->vm, "", 0)));
    }
    consume(parser, TOKEN_RIGHT_PAREN, "')' lagao 'pucho' band karne ke liye.");
    emitByte(parser, OP_INPUT);
}

/* ============================================================================
 *  PARSE RULES TABLE (Pratt parser)
 * ============================================================================ */
static ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, call,    PREC_CALL},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,    PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,    PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,    PREC_NONE},
    [TOKEN_LEFT_BRACKET]  = {listLiteral, index_, PREC_CALL},
    [TOKEN_RIGHT_BRACKET] = {NULL,     NULL,    PREC_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,    PREC_NONE},
    [TOKEN_DOT]           = {NULL,     NULL,    PREC_NONE},
    [TOKEN_COLON]         = {NULL,     NULL,    PREC_NONE},
    [TOKEN_SEMICOLON]     = {NULL,     NULL,    PREC_NONE},
    [TOKEN_MINUS]         = {unary,    binary,  PREC_TERM},
    [TOKEN_PLUS]          = {NULL,     binary,  PREC_TERM},
    [TOKEN_SLASH]         = {NULL,     binary,  PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,     binary,  PREC_FACTOR},
    [TOKEN_PERCENT]       = {NULL,     binary,  PREC_FACTOR},
    [TOKEN_STAR_STAR]     = {NULL,     binary,  PREC_POWER},
    [TOKEN_BANG]          = {unary,    NULL,    PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     binary,  PREC_EQUALITY},
    [TOKEN_EQUAL]         = {NULL,     NULL,    PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary,  PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL,     binary,  PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary,  PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,     binary,  PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary,  PREC_COMPARISON},
    [TOKEN_PLUS_EQUAL]    = {NULL,     NULL,    PREC_NONE},
    [TOKEN_MINUS_EQUAL]   = {NULL,     NULL,    PREC_NONE},
    [TOKEN_STAR_EQUAL]    = {NULL,     NULL,    PREC_NONE},
    [TOKEN_SLASH_EQUAL]   = {NULL,     NULL,    PREC_NONE},
    [TOKEN_IDENTIFIER]    = {variable, NULL,    PREC_NONE},
    [TOKEN_STRING]        = {stringLiteral, NULL, PREC_NONE},
    [TOKEN_NUMBER]        = {numberLiteral, NULL, PREC_NONE},
    [TOKEN_MAANO]         = {NULL,     NULL,    PREC_NONE},
    [TOKEN_LIKHO]         = {NULL,     NULL,    PREC_NONE},
    [TOKEN_PUCHO]         = {inputExpr, NULL,   PREC_NONE},
    [TOKEN_AGAR]          = {NULL,     NULL,    PREC_NONE},
    [TOKEN_WARNA]         = {NULL,     NULL,    PREC_NONE},
    [TOKEN_WARNA_AGAR]    = {NULL,     NULL,    PREC_NONE},
    [TOKEN_JAB_TAK]       = {NULL,     NULL,    PREC_NONE},
    [TOKEN_HAR]           = {NULL,     NULL,    PREC_NONE},
    [TOKEN_SE]            = {NULL,     NULL,    PREC_NONE},
    [TOKEN_TAK]           = {NULL,     NULL,    PREC_NONE},
    [TOKEN_KAAM]          = {NULL,     NULL,    PREC_NONE},
    [TOKEN_WAPAS_DO]      = {NULL,     NULL,    PREC_NONE},
    [TOKEN_RUKO]          = {NULL,     NULL,    PREC_NONE},
    [TOKEN_AGLA]          = {NULL,     NULL,    PREC_NONE},
    [TOKEN_SACH]          = {literal,  NULL,    PREC_NONE},
    [TOKEN_JHOOTH]        = {literal,  NULL,    PREC_NONE},
    [TOKEN_KHALI]         = {literal,  NULL,    PREC_NONE},
    [TOKEN_AUR]           = {NULL,     and_,    PREC_AND},
    [TOKEN_YA]            = {NULL,     or_,     PREC_OR},
    [TOKEN_NAHI]          = {unary,    NULL,    PREC_NONE},
    [TOKEN_NEWLINE]       = {NULL,     NULL,    PREC_NONE},
    [TOKEN_ERROR]         = {NULL,     NULL,    PREC_NONE},
    [TOKEN_EOF]           = {NULL,     NULL,    PREC_NONE},
};

static ParseRule* getRule(RamTokenType type) {
    return &rules[type];
}

/* ============================================================================
 *  PRATT PARSER CORE
 * ============================================================================ */
static void parsePrecedence(Parser* parser, Precedence precedence) {
    advanceParser(parser);
    ParseFn prefixRule = getRule(parser->previous.type)->prefix;
    if (prefixRule == NULL) {
        error(parser, "Expression expected tha.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(parser, canAssign);

    while (precedence <= getRule(parser->current.type)->precedence) {
        advanceParser(parser);
        ParseFn infixRule = getRule(parser->previous.type)->infix;
        if (infixRule != NULL) {
            infixRule(parser, canAssign);
        }
    }

    if (canAssign && matchToken(parser, TOKEN_EQUAL)) {
        error(parser, "Invalid assignment target.");
    }
}

static void expression(Parser* parser) {
    parsePrecedence(parser, PREC_ASSIGNMENT);
}

/* ============================================================================
 *  STATEMENT PARSING
 * ============================================================================ */

static void block(Parser* parser) {
    skipNewlines(parser);
    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        declaration(parser);
        skipNewlines(parser);
    }
    consume(parser, TOKEN_RIGHT_BRACE, "'}' lagao block band karne ke liye.");
}

/* --- PRINT (likho) --- */
static void printStatement(Parser* parser) {
    /* Parentheses are optional for likho */
    bool hasParens = matchToken(parser, TOKEN_LEFT_PAREN);

    if (hasParens && check(parser, TOKEN_RIGHT_PAREN)) {
        /* Empty print = newline */
        emitConstant(parser, OBJ_VAL(copyString(parser->vm, "", 0)));
        emitByte(parser, OP_PRINT);
        consume(parser, TOKEN_RIGHT_PAREN, "')' lagao.");
        consumeNewlineOrEnd(parser);
        return;
    }

    /* Check for empty print without parens (just newline) */
    if (!hasParens && (check(parser, TOKEN_NEWLINE) || check(parser, TOKEN_EOF))) {
        emitConstant(parser, OBJ_VAL(copyString(parser->vm, "\n", 1)));
        emitByte(parser, OP_PRINT);
        consumeNewlineOrEnd(parser);
        return;
    }

    expression(parser);
    int argCount = 1;

    while (matchToken(parser, TOKEN_COMMA)) {
        /* Print space separator then next expression */
        emitByte(parser, OP_PRINT);
        emitConstant(parser, OBJ_VAL(copyString(parser->vm, " ", 1)));
        emitByte(parser, OP_PRINT);
        expression(parser);
        argCount++;
    }

    /* Check for ';' separator (no newline at end) */
    if (matchToken(parser, TOKEN_SEMICOLON)) {
        emitByte(parser, OP_PRINT);
    } else {
        emitByte(parser, OP_PRINT);
        /* Print newline at end */
        emitConstant(parser, OBJ_VAL(copyString(parser->vm, "\n", 1)));
        emitByte(parser, OP_PRINT);
    }

    if (hasParens) {
        consume(parser, TOKEN_RIGHT_PAREN, "')' lagao 'likho' band karne ke liye.");
    }
    consumeNewlineOrEnd(parser);
}

/* --- IF (agar) --- */
static void ifStatement(Parser* parser) {
    expression(parser); /* condition */
    skipNewlines(parser);
    consume(parser, TOKEN_LEFT_BRACE, "'{' lagao 'agar' ke baad.");

    int thenJump = emitJump(parser, OP_JUMP_IF_FALSE);
    emitByte(parser, OP_POP); /* pop condition */

    beginScope(parser);
    block(parser);
    endScope(parser);
    skipNewlines(parser);

    int elseJump = emitJump(parser, OP_JUMP);
    patchJump(parser, thenJump);
    emitByte(parser, OP_POP); /* pop condition */

    /* Handle warna agar / warna */
    if (matchToken(parser, TOKEN_WARNA_AGAR)) {
        ifStatement(parser); /* Recursive for else-if */
    } else if (matchToken(parser, TOKEN_WARNA)) {
        skipNewlines(parser);
        consume(parser, TOKEN_LEFT_BRACE, "'{' lagao 'warna' ke baad.");
        beginScope(parser);
        block(parser);
        endScope(parser);
        skipNewlines(parser);
    }

    patchJump(parser, elseJump);
}

/* --- WHILE (jab tak) --- */
static void whileStatement(Parser* parser) {
    CompilerState* cs = currentCS(parser);
    int surroundingLoopStart = cs->loopStart;
    int surroundingLoopDepth = cs->loopScopeDepth;
    int surroundingBreakCount = cs->breakCount;

    int loopStart = currentChunk(parser)->count;
    cs->loopStart = loopStart;
    cs->loopScopeDepth = cs->scopeDepth;
    cs->breakCount = 0;

    expression(parser); /* condition */
    skipNewlines(parser);
    consume(parser, TOKEN_LEFT_BRACE, "'{' lagao 'jab tak' ke baad.");

    int exitJump = emitJump(parser, OP_JUMP_IF_FALSE);
    emitByte(parser, OP_POP);

    beginScope(parser);
    block(parser);
    endScope(parser);
    skipNewlines(parser);

    emitLoop(parser, loopStart);

    patchJump(parser, exitJump);
    emitByte(parser, OP_POP);

    /* Patch break jumps */
    for (int i = 0; i < cs->breakCount; i++) {
        patchJump(parser, cs->breakJumps[i]);
    }

    cs->loopStart = surroundingLoopStart;
    cs->loopScopeDepth = surroundingLoopDepth;
    cs->breakCount = surroundingBreakCount;
}

/* --- FOR (har x se start tak end) --- */
static void forStatement(Parser* parser) {
    beginScope(parser);

    /* C-style for: har i = 1; i <= 10; i = i + 1 { ... } */

    /* --- Initializer: variable declaration --- */
    consume(parser, TOKEN_IDENTIFIER, "Loop variable ka naam do 'har' ke baad.");
    declareVariable(parser);
    markInitialized(parser);

    consume(parser, TOKEN_EQUAL, "'=' lagao initial value dene ke liye.");
    expression(parser);
    /* defineVariable for local just leaves the value on stack */

    consume(parser, TOKEN_SEMICOLON, "';' lagao initializer ke baad.");

    /* --- Condition --- */
    CompilerState* cs = currentCS(parser);
    int surroundingLoopStart = cs->loopStart;
    int surroundingLoopDepth = cs->loopScopeDepth;
    int surroundingBreakCount = cs->breakCount;

    int loopStart = currentChunk(parser)->count;
    cs->loopStart = loopStart;
    cs->loopScopeDepth = cs->scopeDepth;
    cs->breakCount = 0;

    expression(parser); /* condition expression */
    int exitJump = emitJump(parser, OP_JUMP_IF_FALSE);
    emitByte(parser, OP_POP); /* pop condition */

    consume(parser, TOKEN_SEMICOLON, "';' lagao condition ke baad.");

    /* --- Increment: jump over it initially, execute after body --- */
    int bodyJump = emitJump(parser, OP_JUMP);
    int incrementStart = currentChunk(parser)->count;

    /* Parse the increment expression (e.g. i = i + 1) */
    expression(parser);
    emitByte(parser, OP_POP); /* discard increment result */

    emitLoop(parser, loopStart); /* loop back to condition */

    loopStart = incrementStart;
    cs->loopStart = incrementStart;

    patchJump(parser, bodyJump);

    /* --- Body --- */
    skipNewlines(parser);
    consume(parser, TOKEN_LEFT_BRACE, "'{' lagao loop body shuru karne ke liye.");

    beginScope(parser);
    block(parser);
    endScope(parser);
    skipNewlines(parser);

    emitLoop(parser, loopStart); /* jump to increment */

    patchJump(parser, exitJump);
    emitByte(parser, OP_POP); /* pop condition */

    /* Patch break jumps */
    for (int i = 0; i < cs->breakCount; i++) {
        patchJump(parser, cs->breakJumps[i]);
    }

    cs->loopStart = surroundingLoopStart;
    cs->loopScopeDepth = surroundingLoopDepth;
    cs->breakCount = surroundingBreakCount;

    endScope(parser);
}

/* --- FUNCTION (kaam) --- */
static void function(Parser* parser, FunctionType type) {
    CompilerState cs;
    initCompilerState(parser, &cs, type);
    beginScope(parser);

    consume(parser, TOKEN_LEFT_PAREN, "'(' lagao function parameters ke liye.");
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            currentCS(parser)->function->arity++;
            if (currentCS(parser)->function->arity > 255) {
                errorAtCurrent(parser, "255 se zyada parameters nahi rakh sakte.");
            }
            uint8_t constant = parseVariable(parser, "Parameter ka naam do.");
            defineVariable(parser, constant);
        } while (matchToken(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RIGHT_PAREN, "')' lagao parameters ke baad.");
    skipNewlines(parser);
    consume(parser, TOKEN_LEFT_BRACE, "'{' lagao function body shuru karne ke liye.");
    block(parser);
    skipNewlines(parser);

    ObjFunction* fn = endCompiler(parser);
    emitBytes(parser, OP_CLOSURE, makeConstant(parser, OBJ_VAL(fn)));

    for (int i = 0; i < fn->upvalueCount; i++) {
        emitByte(parser, cs.upvalues[i].isLocal ? 1 : 0);
        emitByte(parser, cs.upvalues[i].index);
    }
}

static void functionDeclaration(Parser* parser) {
    uint8_t global = parseVariable(parser, "Function ka naam do 'kaam' ke baad.");
    markInitialized(parser);
    function(parser, TYPE_FUNCTION);
    defineVariable(parser, global);
}

/* --- RETURN (wapas do) --- */
static void returnStatement(Parser* parser) {
    if (currentCS(parser)->type == TYPE_SCRIPT) {
        error(parser, "'wapas do' sirf function ke andar use kar sakte ho.");
    }

    if (check(parser, TOKEN_NEWLINE) || check(parser, TOKEN_EOF) ||
        check(parser, TOKEN_RIGHT_BRACE)) {
        emitReturn(parser);
    } else {
        expression(parser);
        emitByte(parser, OP_RETURN);
    }
    consumeNewlineOrEnd(parser);
}

/* --- BREAK (ruko) --- */
static void breakStatement(Parser* parser) {
    CompilerState* cs = currentCS(parser);
    if (cs->loopStart == -1) {
        error(parser, "'ruko' sirf loop ke andar use kar sakte ho.");
        return;
    }

    /* Pop locals in loop scope */
    for (int i = cs->localCount - 1; i >= 0 && cs->locals[i].depth > cs->loopScopeDepth; i--) {
        emitByte(parser, OP_POP);
    }

    if (cs->breakCount >= 256) {
        error(parser, "Ek loop mein 256 se zyada 'ruko' nahi laga sakte.");
        return;
    }
    cs->breakJumps[cs->breakCount++] = emitJump(parser, OP_JUMP);
    consumeNewlineOrEnd(parser);
}

/* --- CONTINUE (agla) --- */
static void continueStatement(Parser* parser) {
    CompilerState* cs = currentCS(parser);
    if (cs->loopStart == -1) {
        error(parser, "'agla' sirf loop ke andar use kar sakte ho.");
        return;
    }

    /* Pop locals in loop scope */
    for (int i = cs->localCount - 1; i >= 0 && cs->locals[i].depth > cs->loopScopeDepth; i--) {
        emitByte(parser, OP_POP);
    }

    emitLoop(parser, cs->loopStart);
    consumeNewlineOrEnd(parser);
}

/* --- VARIABLE DECLARATION (maano) --- */
static void varDeclaration(Parser* parser) {
    uint8_t global = parseVariable(parser, "Variable ka naam do 'maano' ke baad.");
    consume(parser, TOKEN_EQUAL, "'=' lagao variable ki value dene ke liye.");
    expression(parser);
    defineVariable(parser, global);
    consumeNewlineOrEnd(parser);
}

/* --- EXPRESSION STATEMENT --- */
static void expressionStatement(Parser* parser) {
    expression(parser);
    emitByte(parser, OP_POP);
    consumeNewlineOrEnd(parser);
}

/* ============================================================================
 *  SYNCHRONIZE (error recovery)
 * ============================================================================ */
static void synchronize(Parser* parser) {
    parser->panicMode = false;
    while (parser->current.type != TOKEN_EOF) {
        if (parser->previous.type == TOKEN_NEWLINE) return;
        switch (parser->current.type) {
            case TOKEN_MAANO:
            case TOKEN_KAAM:
            case TOKEN_HAR:
            case TOKEN_AGAR:
            case TOKEN_JAB_TAK:
            case TOKEN_LIKHO:
            case TOKEN_WAPAS_DO:
            case TOKEN_RUKO:
            case TOKEN_AGLA:
                return;
            default:
                break;
        }
        advanceParser(parser);
    }
}

/* ============================================================================
 *  STATEMENT DISPATCH
 * ============================================================================ */
static void statement(Parser* parser) {
    if (matchToken(parser, TOKEN_LIKHO)) {
        printStatement(parser);
    } else if (matchToken(parser, TOKEN_AGAR)) {
        ifStatement(parser);
    } else if (matchToken(parser, TOKEN_JAB_TAK)) {
        whileStatement(parser);
    } else if (matchToken(parser, TOKEN_HAR)) {
        forStatement(parser);
    } else if (matchToken(parser, TOKEN_WAPAS_DO)) {
        returnStatement(parser);
    } else if (matchToken(parser, TOKEN_RUKO)) {
        breakStatement(parser);
    } else if (matchToken(parser, TOKEN_AGLA)) {
        continueStatement(parser);
    } else if (matchToken(parser, TOKEN_LEFT_BRACE)) {
        beginScope(parser);
        block(parser);
        endScope(parser);
    } else {
        expressionStatement(parser);
    }
}

static void declaration(Parser* parser) {
    skipNewlines(parser);
    if (check(parser, TOKEN_EOF)) return;

    if (matchToken(parser, TOKEN_MAANO)) {
        varDeclaration(parser);
    } else if (matchToken(parser, TOKEN_KAAM)) {
        functionDeclaration(parser);
    } else {
        statement(parser);
    }

    if (parser->panicMode) synchronize(parser);
}

/* ============================================================================
 *  COMPILE ENTRY POINT
 * ============================================================================ */
ObjFunction* compile(VM* vm, const char* source, const char* filename) {
    Parser parser;
    parser.hadError = false;
    parser.panicMode = false;
    parser.vm = vm;

    initLexer(&parser.lexer, source, filename);

    CompilerState cs;
    initCompilerState(&parser, &cs, TYPE_SCRIPT);

    advanceParser(&parser);
    skipNewlines(&parser);

    while (!matchToken(&parser, TOKEN_EOF)) {
        declaration(&parser);
    }

    ObjFunction* function = endCompiler(&parser);
    return parser.hadError ? NULL : function;
}

/* Mark compiler roots for GC */
void markCompilerRoots(VM* vm) {
    UNUSED(vm);
    CompilerState* cs = currentCompilerState;
    while (cs != NULL) {
        Obj* obj = (Obj*)cs->function;
        if (obj != NULL && !obj->isMarked) {
            obj->isMarked = true;
        }
        cs = cs->enclosing;
    }
}
