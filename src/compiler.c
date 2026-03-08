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
    PREC_BIT_OR,       /* |                */
    PREC_BIT_XOR,      /* ^                */
    PREC_BIT_AND,      /* &                */
    PREC_SHIFT,        /* << >>            */
    PREC_TERM,         /* + -              */
    PREC_FACTOR,       /* * / %            */
    PREC_POWER,        /* **               */
    PREC_UNARY,        /* - nahi ~         */
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
    TYPE_METHOD,
    TYPE_INITIALIZER,
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
 *  CLASS COMPILER STATE
 * ============================================================================ */
typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;
    bool hasSuperclass;
} ClassCompiler;

static ClassCompiler* currentClass = NULL;

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
    if (currentCS(parser)->type == TYPE_INITIALIZER) {
        emitBytes(parser, OP_GET_LOCAL, 0); /* return 'yeh' */
    } else {
        emitByte(parser, OP_NULL);
    }
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

static int makeConstant(Parser* parser, Value value) {
    int constant = addConstant(currentChunk(parser), value);
    if (constant > 65535) {
        error(parser, "Ek chunk mein bahut saare constants hain.");
        return 0;
    }
    return constant;
}

static void emitConstant(Parser* parser, Value value) {
    int constant = makeConstant(parser, value);
    if (constant <= UINT8_MAX) {
        emitBytes(parser, OP_CONSTANT, (uint8_t)constant);
    } else {
        emitByte(parser, OP_CONSTANT_LONG);
        emitByte(parser, (uint8_t)(constant & 0xff));
        emitByte(parser, (uint8_t)((constant >> 8) & 0xff));
    }
}

/* Emit an opcode that takes a constant index operand, auto-selecting
   short (1-byte) or long (2-byte little-endian) variant. */
static void emitConstantOp(Parser* parser, uint8_t shortOp, uint8_t longOp, int constant) {
    if (constant <= UINT8_MAX) {
        emitBytes(parser, shortOp, (uint8_t)constant);
    } else {
        emitByte(parser, longOp);
        emitByte(parser, (uint8_t)(constant & 0xff));
        emitByte(parser, (uint8_t)((constant >> 8) & 0xff));
    }
}

/* Emit an invoke-style opcode (constant + argCount), auto-selecting variant. */
static void emitInvokeOp(Parser* parser, uint8_t shortOp, uint8_t longOp, int constant, uint8_t argCount) {
    if (constant <= UINT8_MAX) {
        emitBytes(parser, shortOp, (uint8_t)constant);
        emitByte(parser, argCount);
    } else {
        emitByte(parser, longOp);
        emitByte(parser, (uint8_t)(constant & 0xff));
        emitByte(parser, (uint8_t)((constant >> 8) & 0xff));
        emitByte(parser, argCount);
    }
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

    /* Slot 0 is for the function itself (or 'yeh' for methods) */
    Local* local = &cs->locals[cs->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    if (type == TYPE_METHOD || type == TYPE_INITIALIZER) {
        local->name.start = "yeh";
        local->name.length = 3;
    } else {
        local->name.start = "";
        local->name.length = 0;
    }
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

static int identifierConstant(Parser* parser, Token* name) {
    return makeConstant(parser, OBJ_VAL(
        copyString(parser->vm, name->start, name->length)));
}

static void markInitialized(Parser* parser) {
    CompilerState* cs = currentCS(parser);
    if (cs->scopeDepth == 0) return;
    cs->locals[cs->localCount - 1].depth = cs->scopeDepth;
}

static int parseVariable(Parser* parser, const char* errorMessage) {
    consume(parser, TOKEN_IDENTIFIER, errorMessage);

    declareVariable(parser);
    if (currentCS(parser)->scopeDepth > 0) return 0;

    return identifierConstant(parser, &parser->previous);
}

static void defineVariable(Parser* parser, int global) {
    if (currentCS(parser)->scopeDepth > 0) {
        markInitialized(parser);
        return;
    }
    if (global <= UINT8_MAX) {
        emitBytes(parser, OP_DEFINE_GLOBAL, (uint8_t)global);
    } else {
        emitByte(parser, OP_DEFINE_GLOBAL_LONG);
        emitByte(parser, (uint8_t)(global & 0xff));
        emitByte(parser, (uint8_t)((global >> 8) & 0xff));
    }
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
static void function(Parser* parser, FunctionType type);
static void forInLoop(Parser* parser, Token varName);
static Token syntheticToken(const char* text);
static void importStatement(Parser* parser);

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

static void tripleStringLiteral(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    /* Skip surrounding triple quotes (""" or ''') */
    const char* src = parser->previous.start + 3;
    int srcLen = parser->previous.length - 6;

    char* buffer = (char*)malloc(srcLen + 1);
    int destLen;
    processEscapes(src, srcLen, buffer, &destLen);
    buffer[destLen] = '\0';

    ObjString* str = copyString(parser->vm, buffer, destLen);
    free(buffer);
    emitConstant(parser, OBJ_VAL(str));
}

/* Helper: emit the text part of an interpolation token. */
static void emitInterpSegment(Parser* parser) {
    const char* src = parser->previous.start;
    int len = parser->previous.length;

    /* INTERP_START token includes the opening " at the start */
    if (parser->previous.type == TOKEN_INTERP_START) {
        src++; len--;
    }
    /* INTERP_END token includes the closing " at the end */
    if (parser->previous.type == TOKEN_INTERP_END) {
        len--;
    }
    /* INTERP_MID has no quotes to strip */

    if (len > 0) {
        char* buffer = (char*)malloc(len + 1);
        int destLen;
        processEscapes(src, len, buffer, &destLen);
        buffer[destLen] = '\0';
        ObjString* str = copyString(parser->vm, buffer, destLen);
        free(buffer);
        emitConstant(parser, OBJ_VAL(str));
    } else {
        emitConstant(parser, OBJ_VAL(copyString(parser->vm, "", 0)));
    }
}

/* String interpolation: "text ${expr} more ${expr2} end" */
static void interpolation(Parser* parser, bool canAssign) {
    UNUSED(canAssign);

    /* Emit the prefix string segment */
    emitInterpSegment(parser);

    /* Parse the interpolated expression */
    expression(parser);
    /* Convert to string via shabd()-like concatenation: add to prefix */
    emitByte(parser, OP_ADD);

    /* Continue consuming middle segments and expressions */
    while (parser->current.type == TOKEN_INTERP_MID) {
        advanceParser(parser);
        emitInterpSegment(parser);
        emitByte(parser, OP_ADD);
        expression(parser);
        emitByte(parser, OP_ADD);
    }

    /* Final segment */
    consume(parser, TOKEN_INTERP_END, "String interpolation band nahi hui.");
    emitInterpSegment(parser);
    emitByte(parser, OP_ADD);
}

/* --- TERNARY: condition ? trueExpr : falseExpr --- */
static void ternary(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    /* condition already on stack, ? already consumed */
    int thenJump = emitJump(parser, OP_JUMP_IF_FALSE);
    emitByte(parser, OP_POP); /* pop condition */
    parsePrecedence(parser, PREC_ASSIGNMENT); /* true branch */
    int elseJump = emitJump(parser, OP_JUMP);
    patchJump(parser, thenJump);
    emitByte(parser, OP_POP); /* pop condition */
    consume(parser, TOKEN_COLON, "':' lagao ternary '?' ke baad.");
    parsePrecedence(parser, PREC_ASSIGNMENT); /* false branch */
    patchJump(parser, elseJump);
}

/* --- NULL COALESCING (??) --- */
static void nullCoalesce(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    /* Left side already on stack, ?? consumed */
    /* If left is NOT null, keep it; else use right side */
    emitByte(parser, OP_DUP);
    emitByte(parser, OP_NULL);
    emitByte(parser, OP_EQUAL);
    int skipJump = emitJump(parser, OP_JUMP_IF_FALSE);
    emitByte(parser, OP_POP); /* pop true (is null) */
    emitByte(parser, OP_POP); /* pop the null value */
    parsePrecedence(parser, PREC_OR); /* parse right side */
    int doneJump = emitJump(parser, OP_JUMP);
    patchJump(parser, skipJump);
    emitByte(parser, OP_POP); /* pop false (not null) */
    patchJump(parser, doneJump);
}

/* --- IN OPERATOR (mein) --- */
static void inOperator(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    /* Left side (needle) already on stack, 'mein' consumed */
    parsePrecedence(parser, PREC_COMPARISON + 1); /* parse collection */
    emitByte(parser, OP_IN);
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
        case TOKEN_TILDE: emitByte(parser, OP_BIT_NOT); break;
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
        case TOKEN_AMPERSAND:     emitByte(parser, OP_BIT_AND); break;
        case TOKEN_PIPE:          emitByte(parser, OP_BIT_OR); break;
        case TOKEN_CARET:         emitByte(parser, OP_BIT_XOR); break;
        case TOKEN_LESS_LESS:     emitByte(parser, OP_SHIFT_LEFT); break;
        case TOKEN_GREATER_GREATER: emitByte(parser, OP_SHIFT_RIGHT); break;
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
static void emitVarOp(Parser* parser, uint8_t shortOp, uint8_t longOp, int arg) {
    if (arg <= UINT8_MAX) {
        emitBytes(parser, shortOp, (uint8_t)arg);
    } else {
        emitByte(parser, longOp);
        emitByte(parser, (uint8_t)(arg & 0xff));
        emitByte(parser, (uint8_t)((arg >> 8) & 0xff));
    }
}

static void namedVariable(Parser* parser, Token name, bool canAssign) {
    uint8_t getOp, setOp;
    uint8_t getLongOp, setLongOp;
    int arg = resolveLocal(parser, currentCS(parser), &name);

    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
        getLongOp = OP_GET_LOCAL;
        setLongOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(parser, currentCS(parser), &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
        getLongOp = OP_GET_UPVALUE;
        setLongOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(parser, &name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
        getLongOp = OP_GET_GLOBAL_LONG;
        setLongOp = OP_SET_GLOBAL_LONG;
    }

    if (canAssign && matchToken(parser, TOKEN_EQUAL)) {
        expression(parser);
        emitVarOp(parser, setOp, setLongOp, arg);
    } else if (canAssign && (check(parser, TOKEN_PLUS_EQUAL) ||
               check(parser, TOKEN_MINUS_EQUAL) ||
               check(parser, TOKEN_STAR_EQUAL) ||
               check(parser, TOKEN_SLASH_EQUAL) ||
               check(parser, TOKEN_PERCENT_EQUAL) ||
               check(parser, TOKEN_STAR_STAR_EQUAL) ||
               check(parser, TOKEN_AMPERSAND_EQUAL) ||
               check(parser, TOKEN_PIPE_EQUAL) ||
               check(parser, TOKEN_CARET_EQUAL) ||
               check(parser, TOKEN_LESS_LESS_EQUAL) ||
               check(parser, TOKEN_GREATER_GREATER_EQUAL))) {
        RamTokenType opType = parser->current.type;
        advanceParser(parser);
        emitVarOp(parser, getOp, getLongOp, arg);
        expression(parser);
        switch (opType) {
            case TOKEN_PLUS_EQUAL:      emitByte(parser, OP_ADD); break;
            case TOKEN_MINUS_EQUAL:     emitByte(parser, OP_SUBTRACT); break;
            case TOKEN_STAR_EQUAL:      emitByte(parser, OP_MULTIPLY); break;
            case TOKEN_SLASH_EQUAL:     emitByte(parser, OP_DIVIDE); break;
            case TOKEN_PERCENT_EQUAL:   emitByte(parser, OP_MODULO); break;
            case TOKEN_STAR_STAR_EQUAL: emitByte(parser, OP_POWER); break;
            case TOKEN_AMPERSAND_EQUAL: emitByte(parser, OP_BIT_AND); break;
            case TOKEN_PIPE_EQUAL:      emitByte(parser, OP_BIT_OR); break;
            case TOKEN_CARET_EQUAL:     emitByte(parser, OP_BIT_XOR); break;
            case TOKEN_LESS_LESS_EQUAL: emitByte(parser, OP_SHIFT_LEFT); break;
            case TOKEN_GREATER_GREATER_EQUAL: emitByte(parser, OP_SHIFT_RIGHT); break;
            default: break;
        }
        emitVarOp(parser, setOp, setLongOp, arg);
    } else {
        emitVarOp(parser, getOp, getLongOp, arg);
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

    /* Check for list comprehension: [har x mein list : expr] */
    if (check(parser, TOKEN_HAR)) {
        /* List comprehension */
        advanceParser(parser); /* consume 'har' */

        beginScope(parser);

        /* Create empty result list */
        emitBytes(parser, OP_LIST_NEW, (uint8_t)0);
        /* Store result list as hidden local */
        Token resultToken = syntheticToken("__natija");
        addLocal(parser, resultToken);
        markInitialized(parser);
        int resultSlot = currentCS(parser)->localCount - 1;

        /* Parse: x mein iterable */
        consume(parser, TOKEN_IDENTIFIER, "Loop variable ka naam do.");
        Token varName = parser->previous;

        consume(parser, TOKEN_MEIN, "'mein' lagao list comprehension mein.");

        /* Evaluate iterable and store as hidden local */
        expression(parser);
        Token listTok = syntheticToken("__suchi");
        addLocal(parser, listTok);
        markInitialized(parser);
        int listSlot = currentCS(parser)->localCount - 1;

        /* Get length */
        emitBytes(parser, OP_GET_LOCAL, (uint8_t)listSlot);
        emitByte(parser, OP_LENGTH);
        Token lenTok = syntheticToken("__lambai");
        addLocal(parser, lenTok);
        markInitialized(parser);
        int lenSlot = currentCS(parser)->localCount - 1;

        /* Index = 0 */
        emitConstant(parser, NUMBER_VAL(0));
        Token idxTok = syntheticToken("__ganana");
        addLocal(parser, idxTok);
        markInitialized(parser);
        int idxSlot = currentCS(parser)->localCount - 1;

        /* Declare loop variable */
        addLocal(parser, varName);
        emitByte(parser, OP_NULL);
        markInitialized(parser);
        int varSlot = currentCS(parser)->localCount - 1;

        /* Loop start */
        int loopStart = currentChunk(parser)->count;

        /* Condition: idx < len */
        emitBytes(parser, OP_GET_LOCAL, (uint8_t)idxSlot);
        emitBytes(parser, OP_GET_LOCAL, (uint8_t)lenSlot);
        emitByte(parser, OP_LESS);

        int exitJump = emitJump(parser, OP_JUMP_IF_FALSE);
        emitByte(parser, OP_POP);

        /* Set x = list[idx] */
        emitBytes(parser, OP_GET_LOCAL, (uint8_t)listSlot);
        emitBytes(parser, OP_GET_LOCAL, (uint8_t)idxSlot);
        emitByte(parser, OP_LIST_GET);
        emitBytes(parser, OP_SET_LOCAL, (uint8_t)varSlot);
        emitByte(parser, OP_POP);

        /* Check for filter: agar condition */
        int filterJump = -1;
        bool hasFilter = false;
        if (matchToken(parser, TOKEN_AGAR)) {
            hasFilter = true;
            expression(parser);
            filterJump = emitJump(parser, OP_JUMP_IF_FALSE);
            emitByte(parser, OP_POP);
        }

        /* Parse: : expr */
        consume(parser, TOKEN_COLON, "':' lagao expression se pehle list comprehension mein.");
        skipNewlines(parser);

        /* Evaluate expression and append to result */
        emitBytes(parser, OP_GET_LOCAL, (uint8_t)resultSlot);
        expression(parser);
        emitByte(parser, OP_LIST_APPEND);
        emitByte(parser, OP_POP); /* pop list ref from OP_LIST_APPEND (it stays on stack via local) */

        if (hasFilter) {
            int skipJump = emitJump(parser, OP_JUMP);
            patchJump(parser, filterJump);
            emitByte(parser, OP_POP); /* pop false from filter */
            patchJump(parser, skipJump);
        }

        /* Increment index */
        emitBytes(parser, OP_GET_LOCAL, (uint8_t)idxSlot);
        emitConstant(parser, NUMBER_VAL(1));
        emitByte(parser, OP_ADD);
        emitBytes(parser, OP_SET_LOCAL, (uint8_t)idxSlot);
        emitByte(parser, OP_POP);

        emitLoop(parser, loopStart);

        patchJump(parser, exitJump);
        emitByte(parser, OP_POP); /* pop false from condition */

        /* Get result list back on stack */
        emitBytes(parser, OP_GET_LOCAL, (uint8_t)resultSlot);

        skipNewlines(parser);
        consume(parser, TOKEN_RIGHT_BRACKET, "']' lagao list comprehension band karne ke liye.");

        endScope(parser);
        return;
    }

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

/* --- MAP/DICT LITERAL --- */
static void mapLiteral(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    int pairCount = 0;
    skipNewlines(parser);
    if (!check(parser, TOKEN_RIGHT_BRACE)) {
        do {
            skipNewlines(parser);
            expression(parser);        /* key */
            consume(parser, TOKEN_COLON, "':' lagao key ke baad map mein.");
            skipNewlines(parser);
            expression(parser);        /* value */
            pairCount++;
            skipNewlines(parser);
        } while (matchToken(parser, TOKEN_COMMA));
    }
    skipNewlines(parser);
    consume(parser, TOKEN_RIGHT_BRACE, "'}' lagao map band karne ke liye.");
    emitBytes(parser, OP_MAP_NEW, (uint8_t)pairCount);
}

/* --- INDEX ACCESS --- */
static void index_(Parser* parser, bool canAssign) {
    /* Check for slice: obj[start:end] or obj[:end] or obj[start:] */
    if (check(parser, TOKEN_COLON)) {
        /* obj[:end] — start defaults to 0 */
        emitConstant(parser, NUMBER_VAL(0));
        advanceParser(parser); /* consume ':' */
        if (check(parser, TOKEN_RIGHT_BRACKET)) {
            /* obj[:] — end defaults to length (use large sentinel) */
            emitConstant(parser, NUMBER_VAL(2147483647));
        } else {
            expression(parser);
        }
        consume(parser, TOKEN_RIGHT_BRACKET, "']' lagao slice ke baad.");
        emitByte(parser, OP_SLICE);
        return;
    }

    expression(parser);

    /* Check if this is a slice: obj[start:end] */
    if (matchToken(parser, TOKEN_COLON)) {
        if (check(parser, TOKEN_RIGHT_BRACKET)) {
            /* obj[start:] — end defaults to length (use large sentinel) */
            emitConstant(parser, NUMBER_VAL(2147483647));
        } else {
            expression(parser);
        }
        consume(parser, TOKEN_RIGHT_BRACKET, "']' lagao slice ke baad.");
        emitByte(parser, OP_SLICE);
        return;
    }

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

/* --- DOT (property access / method call) --- */
static void dot(Parser* parser, bool canAssign) {
    consume(parser, TOKEN_IDENTIFIER, "Property ka naam do '.' ke baad.");
    int name = identifierConstant(parser, &parser->previous);

    if (canAssign && matchToken(parser, TOKEN_EQUAL)) {
        expression(parser);
        emitConstantOp(parser, OP_SET_PROPERTY, OP_SET_PROPERTY_LONG, name);
    } else if (matchToken(parser, TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList(parser);
        emitInvokeOp(parser, OP_INVOKE, OP_INVOKE_LONG, name, argCount);
    } else {
        emitConstantOp(parser, OP_GET_PROPERTY, OP_GET_PROPERTY_LONG, name);
    }
}

/* --- YEH (this/self) --- */
static void yeh_(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    if (currentClass == NULL) {
        error(parser, "'yeh' sirf kaksha ke methods mein use kar sakte ho.");
        return;
    }
    variable(parser, false);
}

/* --- Synthetic Token helper --- */
static Token syntheticToken(const char* text) {
    Token token;
    token.start = text;
    token.length = (int)strlen(text);
    token.line = 0;
    token.column = 0;
    return token;
}

/* --- SUPER --- */
static void super_(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    if (currentClass == NULL) {
        error(parser, "'super' sirf kaksha ke andar use kar sakte ho.");
        return;
    }
    if (!currentClass->hasSuperclass) {
        error(parser, "'super' tab hi use kar sakte ho jab kaksha inherit kare.");
        return;
    }

    consume(parser, TOKEN_DOT, "'.' lagao 'super' ke baad.");
    consume(parser, TOKEN_IDENTIFIER, "Superclass method ka naam do.");
    int name = identifierConstant(parser, &parser->previous);

    namedVariable(parser, syntheticToken("yeh"), false);

    if (matchToken(parser, TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList(parser);
        namedVariable(parser, syntheticToken("super"), false);
        emitInvokeOp(parser, OP_SUPER_INVOKE, OP_SUPER_INVOKE_LONG, name, argCount);
    } else {
        namedVariable(parser, syntheticToken("super"), false);
        emitConstantOp(parser, OP_GET_SUPER, OP_GET_SUPER_LONG, name);
    }
}

/* --- LAMBDA (anonymous function expression) --- */
static void lambdaExpression(Parser* parser, bool canAssign) {
    UNUSED(canAssign);

    CompilerState cs;
    initCompilerState(parser, &cs, TYPE_FUNCTION);
    cs.function->name = NULL; /* anonymous */
    beginScope(parser);

    consume(parser, TOKEN_LEFT_PAREN, "'(' lagao lambda parameters ke liye.");
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            currentCS(parser)->function->arity++;
            if (currentCS(parser)->function->arity > 255) {
                errorAtCurrent(parser, "255 se zyada parameters nahi rakh sakte.");
            }
            int constant = parseVariable(parser, "Parameter ka naam do.");
            defineVariable(parser, constant);
        } while (matchToken(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RIGHT_PAREN, "')' lagao parameters ke baad.");

    skipNewlines(parser);

    if (matchToken(parser, TOKEN_ARROW)) {
        /* Arrow body: kaam (x) => expr */
        expression(parser);
        emitByte(parser, OP_RETURN);
    } else {
        consume(parser, TOKEN_LEFT_BRACE, "'{' ya '=>' lagao lambda body ke liye.");
        block(parser);
    }

    ObjFunction* fn = endCompiler(parser);
    int closureConstant = makeConstant(parser, OBJ_VAL(fn));
    if (closureConstant <= UINT8_MAX) {
        emitBytes(parser, OP_CLOSURE, (uint8_t)closureConstant);
    } else {
        emitByte(parser, OP_CLOSURE_LONG);
        emitByte(parser, (uint8_t)(closureConstant & 0xff));
        emitByte(parser, (uint8_t)((closureConstant >> 8) & 0xff));
    }

    for (int i = 0; i < fn->upvalueCount; i++) {
        emitByte(parser, cs.upvalues[i].isLocal ? 1 : 0);
        emitByte(parser, cs.upvalues[i].index);
    }
}

/* ============================================================================
 *  PARSE RULES TABLE (Pratt parser)
 * ============================================================================ */
static ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, call,    PREC_CALL},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,    PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {mapLiteral, NULL,  PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,    PREC_NONE},
    [TOKEN_LEFT_BRACKET]  = {listLiteral, index_, PREC_CALL},
    [TOKEN_RIGHT_BRACKET] = {NULL,     NULL,    PREC_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,    PREC_NONE},
    [TOKEN_DOT]           = {NULL,     dot,     PREC_CALL},
    [TOKEN_DOT_DOT_DOT]  = {NULL,     NULL,    PREC_NONE},
    [TOKEN_COLON]         = {NULL,     NULL,    PREC_NONE},
    [TOKEN_SEMICOLON]     = {NULL,     NULL,    PREC_NONE},
    [TOKEN_MINUS]         = {unary,    binary,  PREC_TERM},
    [TOKEN_PLUS]          = {NULL,     binary,  PREC_TERM},
    [TOKEN_SLASH]         = {NULL,     binary,  PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,     binary,  PREC_FACTOR},
    [TOKEN_PERCENT]       = {NULL,     binary,  PREC_FACTOR},
    [TOKEN_STAR_STAR]     = {NULL,     binary,  PREC_POWER},
    [TOKEN_AMPERSAND]     = {NULL,     binary,  PREC_BIT_AND},
    [TOKEN_AMPERSAND_EQUAL] = {NULL,   NULL,    PREC_NONE},
    [TOKEN_PIPE]          = {NULL,     binary,  PREC_BIT_OR},
    [TOKEN_PIPE_EQUAL]    = {NULL,     NULL,    PREC_NONE},
    [TOKEN_CARET]         = {NULL,     binary,  PREC_BIT_XOR},
    [TOKEN_CARET_EQUAL]   = {NULL,     NULL,    PREC_NONE},
    [TOKEN_TILDE]         = {unary,    NULL,    PREC_NONE},
    [TOKEN_LESS_LESS]     = {NULL,     binary,  PREC_SHIFT},
    [TOKEN_LESS_LESS_EQUAL] = {NULL,   NULL,    PREC_NONE},
    [TOKEN_GREATER_GREATER] = {NULL,   binary,  PREC_SHIFT},
    [TOKEN_GREATER_GREATER_EQUAL] = {NULL, NULL, PREC_NONE},
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
    [TOKEN_PERCENT_EQUAL] = {NULL,     NULL,    PREC_NONE},
    [TOKEN_STAR_STAR_EQUAL] = {NULL,     NULL,    PREC_NONE},
    [TOKEN_IDENTIFIER]    = {variable, NULL,    PREC_NONE},
    [TOKEN_STRING]        = {stringLiteral, NULL, PREC_NONE},
    [TOKEN_TRIPLE_STRING] = {tripleStringLiteral, NULL, PREC_NONE},
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
    [TOKEN_KAAM]          = {lambdaExpression, NULL, PREC_NONE},
    [TOKEN_WAPAS_DO]      = {NULL,     NULL,    PREC_NONE},
    [TOKEN_RUKO]          = {NULL,     NULL,    PREC_NONE},
    [TOKEN_AGLA]          = {NULL,     NULL,    PREC_NONE},
    [TOKEN_SACH]          = {literal,  NULL,    PREC_NONE},
    [TOKEN_JHOOTH]        = {literal,  NULL,    PREC_NONE},
    [TOKEN_KHALI]         = {literal,  NULL,    PREC_NONE},
    [TOKEN_AUR]           = {NULL,     and_,    PREC_AND},
    [TOKEN_YA]            = {NULL,     or_,     PREC_OR},
    [TOKEN_NAHI]          = {unary,    NULL,    PREC_NONE},

    /* New token entries */
    [TOKEN_KOSHISH]       = {NULL,     NULL,    PREC_NONE},
    [TOKEN_PAKDO]         = {NULL,     NULL,    PREC_NONE},
    [TOKEN_AAKHIR]        = {NULL,     NULL,    PREC_NONE},
    [TOKEN_PHENKO]        = {NULL,     NULL,    PREC_NONE},
    [TOKEN_KAKSHA]        = {NULL,     NULL,    PREC_NONE},
    [TOKEN_NAYA]          = {NULL,     NULL,    PREC_NONE},
    [TOKEN_YEH]           = {yeh_,     NULL,    PREC_NONE},
    [TOKEN_SUPER]         = {super_,   NULL,    PREC_NONE},
    [TOKEN_MEIN]          = {NULL,     inOperator, PREC_COMPARISON},
    [TOKEN_KARO]          = {NULL,     NULL,    PREC_NONE},
    [TOKEN_GANANA]        = {NULL,     NULL,    PREC_NONE},
    [TOKEN_PAKKA]         = {NULL,     NULL,    PREC_NONE},
    [TOKEN_SHAMIL_KARO]   = {NULL,     NULL,    PREC_NONE},
    [TOKEN_BAHAR_BHEJO]   = {NULL,     NULL,    PREC_NONE},
    [TOKEN_DEKHO]         = {NULL,     NULL,    PREC_NONE},
    [TOKEN_JAB]           = {NULL,     NULL,    PREC_NONE},
    [TOKEN_CHALAO]        = {NULL,     NULL,    PREC_NONE},
    [TOKEN_ARROW]         = {NULL,     NULL,    PREC_NONE},

    [TOKEN_INTERP_START]  = {interpolation, NULL, PREC_NONE},
    [TOKEN_INTERP_MID]    = {NULL,     NULL,    PREC_NONE},
    [TOKEN_INTERP_END]    = {NULL,     NULL,    PREC_NONE},

    [TOKEN_QUESTION]      = {NULL,     ternary, PREC_ASSIGNMENT},
    [TOKEN_QUESTION_QUESTION] = {NULL, nullCoalesce, PREC_OR},

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
    consume(parser, TOKEN_IDENTIFIER, "Loop variable ka naam do 'har' ke baad.");
    Token varName = parser->previous;

    /* Check for for-in: har x mein list { ... } */
    if (matchToken(parser, TOKEN_MEIN)) {
        forInLoop(parser, varName);
        return;
    }

    /* Range-based for: har i se start tak end { ... } */
    if (matchToken(parser, TOKEN_SE)) {
        beginScope(parser);

        /* Declare loop variable using the saved varName */
        expression(parser); /* start value — this goes on stack as the var's value */
        addLocal(parser, varName);
        markInitialized(parser);
        int varSlot = currentCS(parser)->localCount - 1;

        consume(parser, TOKEN_TAK, "'tak' lagao range ke liye.");
        /* Evaluate end value and store as hidden local */
        expression(parser);
        Token endToken = syntheticToken("__ant");
        addLocal(parser, endToken);
        markInitialized(parser);
        int endSlot = currentCS(parser)->localCount - 1;

        CompilerState* cs = currentCS(parser);
        int surroundingLoopStart = cs->loopStart;
        int surroundingLoopDepth = cs->loopScopeDepth;
        int surroundingBreakCount = cs->breakCount;

        int loopStart = currentChunk(parser)->count;
        cs->loopStart = loopStart;
        cs->loopScopeDepth = cs->scopeDepth;
        cs->breakCount = 0;

        /* Condition: i <= end */
        emitBytes(parser, OP_GET_LOCAL, (uint8_t)varSlot);
        emitBytes(parser, OP_GET_LOCAL, (uint8_t)endSlot);
        emitByte(parser, OP_LESS_EQUAL);

        int exitJump = emitJump(parser, OP_JUMP_IF_FALSE);
        emitByte(parser, OP_POP);

        skipNewlines(parser);
        consume(parser, TOKEN_LEFT_BRACE, "'{' lagao loop body ke liye.");
        beginScope(parser);
        block(parser);
        endScope(parser);
        skipNewlines(parser);

        /* Increment: i = i + 1 */
        emitBytes(parser, OP_GET_LOCAL, (uint8_t)varSlot);
        emitConstant(parser, NUMBER_VAL(1));
        emitByte(parser, OP_ADD);
        emitBytes(parser, OP_SET_LOCAL, (uint8_t)varSlot);
        emitByte(parser, OP_POP);

        emitLoop(parser, loopStart);
        patchJump(parser, exitJump);
        emitByte(parser, OP_POP);

        for (int i = 0; i < cs->breakCount; i++) {
            patchJump(parser, cs->breakJumps[i]);
        }

        cs->loopStart = surroundingLoopStart;
        cs->loopScopeDepth = surroundingLoopDepth;
        cs->breakCount = surroundingBreakCount;

        endScope(parser);
        return;
    }

    /* C-style for: har i = 1; i <= 10; i = i + 1 { ... } */
    beginScope(parser);
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

/* --- FOR-IN (har x mein list) --- */
static void forInLoop(Parser* parser, Token varName) {
    beginScope(parser);

    /* Evaluate list expression and store as hidden local */
    expression(parser);
    emitByte(parser, OP_ITER_PREP); /* maps → keys list */
    Token listToken = syntheticToken("__suchi");
    addLocal(parser, listToken);
    markInitialized(parser);
    int listSlot = currentCS(parser)->localCount - 1;

    /* Get length and store as hidden local */
    emitBytes(parser, OP_GET_LOCAL, (uint8_t)listSlot);
    emitByte(parser, OP_LENGTH);
    Token lenToken = syntheticToken("__lambai");
    addLocal(parser, lenToken);
    markInitialized(parser);
    int lenSlot = currentCS(parser)->localCount - 1;

    /* Push 0 as initial index */
    emitConstant(parser, NUMBER_VAL(0));
    Token idxToken = syntheticToken("__ganana");
    addLocal(parser, idxToken);
    markInitialized(parser);
    int idxSlot = currentCS(parser)->localCount - 1;

    /* Declare iteration variable */
    addLocal(parser, varName);
    emitByte(parser, OP_NULL);
    markInitialized(parser);
    int varSlot = currentCS(parser)->localCount - 1;

    /* Loop tracking */
    CompilerState* cs = currentCS(parser);
    int surroundingLoopStart = cs->loopStart;
    int surroundingLoopDepth = cs->loopScopeDepth;
    int surroundingBreakCount = cs->breakCount;

    int loopStart = currentChunk(parser)->count;
    cs->loopStart = loopStart;
    cs->loopScopeDepth = cs->scopeDepth;
    cs->breakCount = 0;

    /* Condition: __idx < __len */
    emitBytes(parser, OP_GET_LOCAL, (uint8_t)idxSlot);
    emitBytes(parser, OP_GET_LOCAL, (uint8_t)lenSlot);
    emitByte(parser, OP_LESS);

    int exitJump = emitJump(parser, OP_JUMP_IF_FALSE);
    emitByte(parser, OP_POP);

    /* Set x = __list[__idx] */
    emitBytes(parser, OP_GET_LOCAL, (uint8_t)listSlot);
    emitBytes(parser, OP_GET_LOCAL, (uint8_t)idxSlot);
    emitByte(parser, OP_LIST_GET);
    emitBytes(parser, OP_SET_LOCAL, (uint8_t)varSlot);
    emitByte(parser, OP_POP);

    /* Parse body */
    skipNewlines(parser);
    consume(parser, TOKEN_LEFT_BRACE, "'{' lagao loop body ke liye.");
    beginScope(parser);
    block(parser);
    endScope(parser);
    skipNewlines(parser);

    /* Increment: __idx = __idx + 1 */
    int incrementStart = currentChunk(parser)->count;
    cs->loopStart = incrementStart;
    emitBytes(parser, OP_GET_LOCAL, (uint8_t)idxSlot);
    emitConstant(parser, NUMBER_VAL(1));
    emitByte(parser, OP_ADD);
    emitBytes(parser, OP_SET_LOCAL, (uint8_t)idxSlot);
    emitByte(parser, OP_POP);

    emitLoop(parser, loopStart);

    patchJump(parser, exitJump);
    emitByte(parser, OP_POP);

    for (int i = 0; i < cs->breakCount; i++) {
        patchJump(parser, cs->breakJumps[i]);
    }

    cs->loopStart = surroundingLoopStart;
    cs->loopScopeDepth = surroundingLoopDepth;
    cs->breakCount = surroundingBreakCount;

    endScope(parser);
}

/* --- TRY-CATCH (koshish/pakdo/aakhir) --- */
static void tryCatchStatement(Parser* parser) {
    skipNewlines(parser);
    consume(parser, TOKEN_LEFT_BRACE, "'{' lagao 'koshish' ke baad.");

    /* OP_TRY with offset to catch block */
    int tryJump = emitJump(parser, OP_TRY);

    beginScope(parser);
    block(parser);
    endScope(parser);

    /* No error - pop handler */
    emitByte(parser, OP_TRY_END);
    int successJump = emitJump(parser, OP_JUMP);

    /* Catch block starts here */
    patchJump(parser, tryJump);

    skipNewlines(parser);

    if (matchToken(parser, TOKEN_PAKDO)) {
        /* Error value is on stack (pushed by OP_THROW in VM) */
        beginScope(parser);

        if (matchToken(parser, TOKEN_LEFT_PAREN)) {
            consume(parser, TOKEN_IDENTIFIER, "Error variable ka naam do.");
            addLocal(parser, parser->previous);
            markInitialized(parser);
            consume(parser, TOKEN_RIGHT_PAREN, "')' lagao.");
        }

        skipNewlines(parser);
        consume(parser, TOKEN_LEFT_BRACE, "'{' lagao 'pakdo' ke baad.");
        block(parser);

        endScope(parser);
    } else {
        /* No catch clause - pop the error */
        emitByte(parser, OP_POP);
    }

    patchJump(parser, successJump);

    /* Optional finally */
    skipNewlines(parser);
    if (matchToken(parser, TOKEN_AAKHIR)) {
        skipNewlines(parser);
        consume(parser, TOKEN_LEFT_BRACE, "'{' lagao 'aakhir' ke baad.");
        beginScope(parser);
        block(parser);
        endScope(parser);
    }
}

/* --- THROW (phenko) --- */
/* --- ASSERT (pakka) --- */
static void assertStatement(Parser* parser) {
    expression(parser);
    int skipJump = emitJump(parser, OP_JUMP_IF_FALSE);
    emitByte(parser, OP_POP); /* pop true */
    int doneJump = emitJump(parser, OP_JUMP);
    patchJump(parser, skipJump);
    emitByte(parser, OP_POP); /* pop false */

    /* Check for optional message */
    if (matchToken(parser, TOKEN_COMMA)) {
        expression(parser); /* push custom message */
    } else {
        int msgConst = makeConstant(parser, OBJ_VAL(
            copyString(parser->vm, "Assertion failed!", 17)));
        emitConstantOp(parser, OP_CONSTANT, OP_CONSTANT_LONG, msgConst);
    }
    emitByte(parser, OP_THROW);
    patchJump(parser, doneJump);
    consumeNewlineOrEnd(parser);
}

static void throwStatement(Parser* parser) {
    expression(parser);
    emitByte(parser, OP_THROW);
    consumeNewlineOrEnd(parser);
}

/* --- DO-WHILE (karo { ... } jab tak condition) --- */
static void doWhileStatement(Parser* parser) {
    CompilerState* cs = currentCS(parser);
    int surroundingLoopStart = cs->loopStart;
    int surroundingLoopDepth = cs->loopScopeDepth;
    int surroundingBreakCount = cs->breakCount;

    int loopStart = currentChunk(parser)->count;
    cs->loopStart = loopStart;
    cs->loopScopeDepth = cs->scopeDepth;
    cs->breakCount = 0;

    /* Body */
    skipNewlines(parser);
    consume(parser, TOKEN_LEFT_BRACE, "'{' lagao 'karo' ke baad.");
    beginScope(parser);
    block(parser);
    endScope(parser);
    skipNewlines(parser);

    /* Condition: jab tak */
    consume(parser, TOKEN_JAB_TAK, "'jab tak' lagao 'karo' loop ke baad.");
    expression(parser);

    int exitJump = emitJump(parser, OP_JUMP_IF_FALSE);
    emitByte(parser, OP_POP);
    emitLoop(parser, loopStart);

    patchJump(parser, exitJump);
    emitByte(parser, OP_POP);

    for (int i = 0; i < cs->breakCount; i++) {
        patchJump(parser, cs->breakJumps[i]);
    }

    cs->loopStart = surroundingLoopStart;
    cs->loopScopeDepth = surroundingLoopDepth;
    cs->breakCount = surroundingBreakCount;

    consumeNewlineOrEnd(parser);
}

/* --- SWITCH (dekho) --- */
static void switchStatement(Parser* parser) {
    expression(parser);
    skipNewlines(parser);
    consume(parser, TOKEN_LEFT_BRACE, "'{' lagao 'dekho' ke baad.");
    skipNewlines(parser);

    int endJumps[256];
    int endJumpCount = 0;

    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        skipNewlines(parser);
        if (check(parser, TOKEN_RIGHT_BRACE)) break;

        if (matchToken(parser, TOKEN_JAB)) {
            /* Case: jab value { body } */
            emitByte(parser, OP_DUP);
            expression(parser);
            emitByte(parser, OP_EQUAL);

            int skipJump = emitJump(parser, OP_JUMP_IF_FALSE);
            emitByte(parser, OP_POP);

            skipNewlines(parser);
            consume(parser, TOKEN_LEFT_BRACE, "'{' lagao 'jab' case ke baad.");
            beginScope(parser);
            block(parser);
            endScope(parser);
            skipNewlines(parser);

            if (endJumpCount < 256) {
                endJumps[endJumpCount++] = emitJump(parser, OP_JUMP);
            }

            patchJump(parser, skipJump);
            emitByte(parser, OP_POP);

        } else if (matchToken(parser, TOKEN_CHALAO)) {
            /* Default: chalao { body } */
            skipNewlines(parser);
            consume(parser, TOKEN_LEFT_BRACE, "'{' lagao 'chalao' ke baad.");
            beginScope(parser);
            block(parser);
            endScope(parser);
            skipNewlines(parser);

            if (endJumpCount < 256) {
                endJumps[endJumpCount++] = emitJump(parser, OP_JUMP);
            }
        } else {
            error(parser, "'jab' ya 'chalao' expected tha 'dekho' block mein.");
            break;
        }
    }

    consume(parser, TOKEN_RIGHT_BRACE, "'}' lagao 'dekho' band karne ke liye.");

    for (int i = 0; i < endJumpCount; i++) {
        patchJump(parser, endJumps[i]);
    }

    emitByte(parser, OP_POP); /* Pop switch value */
}

/* --- CLASS METHOD --- */
static void method(Parser* parser) {
    consume(parser, TOKEN_IDENTIFIER, "Method ka naam do.");
    int constant = identifierConstant(parser, &parser->previous);

    FunctionType type = TYPE_METHOD;
    if (parser->previous.length == 5 &&
        memcmp(parser->previous.start, "shuru", 5) == 0) {
        type = TYPE_INITIALIZER;
    }

    function(parser, type);
    emitConstantOp(parser, OP_METHOD, OP_METHOD_LONG, constant);
}

/* --- CLASS DECLARATION (kaksha) --- */
static void classDeclaration(Parser* parser) {
    consume(parser, TOKEN_IDENTIFIER, "Kaksha ka naam do.");
    Token className = parser->previous;
    int nameConstant = identifierConstant(parser, &className);

    declareVariable(parser);

    emitConstantOp(parser, OP_CLASS, OP_CLASS_LONG, nameConstant);
    defineVariable(parser, nameConstant);

    ClassCompiler classCompiler;
    classCompiler.hasSuperclass = false;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    /* Inheritance: kaksha Child : Parent */
    if (matchToken(parser, TOKEN_COLON)) {
        consume(parser, TOKEN_IDENTIFIER, "Parent kaksha ka naam do.");
        variable(parser, false);

        if (identifiersEqual(&className, &parser->previous)) {
            error(parser, "Ek kaksha apne aap se inherit nahi kar sakti.");
        }

        beginScope(parser);
        addLocal(parser, syntheticToken("super"));
        defineVariable(parser, 0);

        namedVariable(parser, className, false);
        emitByte(parser, OP_INHERIT);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(parser, className, false);
    skipNewlines(parser);
    consume(parser, TOKEN_LEFT_BRACE, "'{' lagao kaksha body ke liye.");
    skipNewlines(parser);

    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        skipNewlines(parser);
        if (check(parser, TOKEN_RIGHT_BRACE)) break;
        method(parser);
        skipNewlines(parser);
    }

    consume(parser, TOKEN_RIGHT_BRACE, "'}' lagao kaksha band karne ke liye.");
    emitByte(parser, OP_POP);

    if (classCompiler.hasSuperclass) {
        endScope(parser);
    }

    currentClass = currentClass->enclosing;
}

/* --- FUNCTION (kaam) --- */
static void function(Parser* parser, FunctionType type) {
    CompilerState cs;
    initCompilerState(parser, &cs, type);
    beginScope(parser);

    bool seenDefault = false;
    bool isVariadic = false;
    int requiredCount = 0;

    consume(parser, TOKEN_LEFT_PAREN, "'(' lagao function parameters ke liye.");
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            if (isVariadic) {
                error(parser, "Rest parameter (...) ke baad aur parameter nahi aa sakta.");
            }

            /* Check for rest parameter: ...name */
            if (matchToken(parser, TOKEN_DOT_DOT_DOT)) {
                isVariadic = true;
                currentCS(parser)->function->isVariadic = true;
            }

            currentCS(parser)->function->arity++;
            if (currentCS(parser)->function->arity > 255) {
                errorAtCurrent(parser, "255 se zyada parameters nahi rakh sakte.");
            }
            int constant = parseVariable(parser, "Parameter ka naam do.");
            defineVariable(parser, constant);

            if (!isVariadic && matchToken(parser, TOKEN_EQUAL)) {
                seenDefault = true;
                /* Emit: if param == khali then param = default_expr */
                int slot = currentCS(parser)->localCount - 1;
                emitBytes(parser, OP_GET_LOCAL, (uint8_t)slot);
                emitByte(parser, OP_NULL);
                emitByte(parser, OP_EQUAL);
                int jumpOver = emitJump(parser, OP_JUMP_IF_FALSE);
                emitByte(parser, OP_POP); /* pop true */
                expression(parser);       /* parse default value expression */
                emitBytes(parser, OP_SET_LOCAL, (uint8_t)slot);
                emitByte(parser, OP_POP); /* pop set result */
                int jumpDone = emitJump(parser, OP_JUMP);
                patchJump(parser, jumpOver);
                emitByte(parser, OP_POP); /* pop false */
                patchJump(parser, jumpDone);
            } else if (!isVariadic) {
                if (seenDefault) {
                    error(parser, "Default parameter ke baad non-default parameter nahi aa sakta.");
                }
                requiredCount++;
            }
        } while (matchToken(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RIGHT_PAREN, "')' lagao parameters ke baad.");

    if (isVariadic) {
        currentCS(parser)->function->minArity = requiredCount;
    } else if (!seenDefault) {
        currentCS(parser)->function->minArity = currentCS(parser)->function->arity;
    } else {
        currentCS(parser)->function->minArity = requiredCount;
    }

    skipNewlines(parser);
    consume(parser, TOKEN_LEFT_BRACE, "'{' lagao function body shuru karne ke liye.");
    block(parser);
    skipNewlines(parser);

    ObjFunction* fn = endCompiler(parser);
    int closureConstant = makeConstant(parser, OBJ_VAL(fn));
    if (closureConstant <= UINT8_MAX) {
        emitBytes(parser, OP_CLOSURE, (uint8_t)closureConstant);
    } else {
        emitByte(parser, OP_CLOSURE_LONG);
        emitByte(parser, (uint8_t)(closureConstant & 0xff));
        emitByte(parser, (uint8_t)((closureConstant >> 8) & 0xff));
    }

    for (int i = 0; i < fn->upvalueCount; i++) {
        emitByte(parser, cs.upvalues[i].isLocal ? 1 : 0);
        emitByte(parser, cs.upvalues[i].index);
    }
}

static void functionDeclaration(Parser* parser) {
    int global = parseVariable(parser, "Function ka naam do 'kaam' ke baad.");
    markInitialized(parser);
    function(parser, TYPE_FUNCTION);
    defineVariable(parser, global);
}

/* --- RETURN (wapas do) --- */
static void returnStatement(Parser* parser) {
    if (currentCS(parser)->type == TYPE_SCRIPT) {
        error(parser, "'wapas do' sirf function ke andar use kar sakte ho.");
    }

    if (currentCS(parser)->type == TYPE_INITIALIZER) {
        error(parser, "Constructor (shuru) mein value return nahi kar sakte.");
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
    /* List destructuring: maano [a, b, c] = expr */
    if (check(parser, TOKEN_LEFT_BRACKET)) {
        advanceParser(parser); /* consume '[' */

        /* Collect variable names */
        Token names[256];
        int nameCount = 0;

        if (!check(parser, TOKEN_RIGHT_BRACKET)) {
            do {
                skipNewlines(parser);
                consume(parser, TOKEN_IDENTIFIER, "Variable ka naam do destructuring mein.");
                if (nameCount >= 256) {
                    error(parser, "256 se zyada variables destructure nahi kar sakte.");
                }
                names[nameCount++] = parser->previous;
                skipNewlines(parser);
            } while (matchToken(parser, TOKEN_COMMA));
        }
        consume(parser, TOKEN_RIGHT_BRACKET, "']' lagao destructuring pattern ke baad.");

        consume(parser, TOKEN_EQUAL, "'=' lagao value dene ke liye.");
        expression(parser); /* evaluate RHS (list on stack) */

        if (currentCS(parser)->scopeDepth > 0) {
            /* Local: store list as hidden local, then extract elements */
            Token hiddenTok = syntheticToken("__ds_tmp");
            addLocal(parser, hiddenTok);
            markInitialized(parser);
            int listSlot = currentCS(parser)->localCount - 1;

            for (int i = 0; i < nameCount; i++) {
                emitBytes(parser, OP_GET_LOCAL, (uint8_t)listSlot);
                emitConstant(parser, NUMBER_VAL(i));
                emitByte(parser, OP_LIST_GET);
                addLocal(parser, names[i]);
                markInitialized(parser);
            }
        } else {
            /* Global: use DUP approach since globals don't live on stack */
            for (int i = 0; i < nameCount; i++) {
                if (i < nameCount - 1) {
                    emitByte(parser, OP_DUP);
                }
                emitConstant(parser, NUMBER_VAL(i));
                emitByte(parser, OP_LIST_GET);
                int global = identifierConstant(parser, &names[i]);
                emitConstantOp(parser, OP_DEFINE_GLOBAL, OP_DEFINE_GLOBAL_LONG, global);
            }
        }

        consumeNewlineOrEnd(parser);
        return;
    }

    int global = parseVariable(parser, "Variable ka naam do 'maano' ke baad.");

    /* Multi-assignment: maano a, b, c = 1, 2, 3 */
    if (check(parser, TOKEN_COMMA)) {
        Token firstVarName = parser->previous;
        Token names[256];
        int globals[256];
        int localIndices[256];
        names[0] = firstVarName;
        globals[0] = global;
        localIndices[0] = currentCS(parser)->localCount - 1;
        int nameCount = 1;
        bool isLocal = (currentCS(parser)->scopeDepth > 0);

        while (matchToken(parser, TOKEN_COMMA)) {
            skipNewlines(parser);
            globals[nameCount] = parseVariable(parser, "Variable ka naam do.");
            names[nameCount] = parser->previous;
            localIndices[nameCount] = currentCS(parser)->localCount - 1;
            nameCount++;
            if (nameCount >= 256) {
                error(parser, "256 se zyada variables nahi assign kar sakte.");
                break;
            }
        }

        consume(parser, TOKEN_EQUAL, "'=' lagao values dene ke liye.");

        /* Parse all the values */
        for (int i = 0; i < nameCount; i++) {
            if (i > 0) {
                consume(parser, TOKEN_COMMA, "',' lagao next value ke liye.");
            }
            expression(parser);
        }

        if (isLocal) {
            /* For locals: values are already on the stack at correct slots.
               Just mark each local as initialized. */
            for (int i = 0; i < nameCount; i++) {
                currentCS(parser)->locals[localIndices[i]].depth = currentCS(parser)->scopeDepth;
            }
        } else {
            /* For globals: define in reverse order (stack pops top first) */
            for (int i = nameCount - 1; i >= 0; i--) {
                defineVariable(parser, globals[i]);
            }
        }

        consumeNewlineOrEnd(parser);
        return;
    }

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
            case TOKEN_KAKSHA:
            case TOKEN_HAR:
            case TOKEN_AGAR:
            case TOKEN_JAB_TAK:
            case TOKEN_LIKHO:
            case TOKEN_WAPAS_DO:
            case TOKEN_RUKO:
            case TOKEN_AGLA:
            case TOKEN_KOSHISH:
            case TOKEN_PHENKO:
            case TOKEN_KARO:
            case TOKEN_DEKHO:
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
    } else if (matchToken(parser, TOKEN_KOSHISH)) {
        tryCatchStatement(parser);
    } else if (matchToken(parser, TOKEN_PHENKO)) {
        throwStatement(parser);
    } else if (matchToken(parser, TOKEN_PAKKA)) {
        assertStatement(parser);
    } else if (matchToken(parser, TOKEN_KARO)) {
        doWhileStatement(parser);
    } else if (matchToken(parser, TOKEN_DEKHO)) {
        switchStatement(parser);
    } else if (matchToken(parser, TOKEN_LEFT_BRACE)) {
        beginScope(parser);
        block(parser);
        endScope(parser);
    } else {
        expressionStatement(parser);
    }
}

/* --- IMPORT (shamil karo) --- */
static void importStatement(Parser* parser) {
    if (currentCS(parser)->scopeDepth > 0) {
        error(parser, "'shamil karo' sirf top-level par use kar sakte ho.");
        return;
    }
    consume(parser, TOKEN_STRING, "'shamil karo' ke baad file path string do.");
    int constant = makeConstant(parser, OBJ_VAL(
        copyString(parser->vm, parser->previous.start + 1,
                   parser->previous.length - 2)));
    emitConstantOp(parser, OP_IMPORT, OP_IMPORT_LONG, constant);
    consumeNewlineOrEnd(parser);
}

/* --- ENUM DECLARATION (ganana) --- */
static void enumDeclaration(Parser* parser) {
    int global = parseVariable(parser, "Enum ka naam do 'ganana' ke baad.");
    markInitialized(parser);

    consume(parser, TOKEN_LEFT_BRACE, "'{' lagao enum body shuru karne ke liye.");
    skipNewlines(parser);

    int memberCount = 0;
    int nextValue = 0;

    if (!check(parser, TOKEN_RIGHT_BRACE)) {
        do {
            skipNewlines(parser);
            consume(parser, TOKEN_IDENTIFIER, "Enum member ka naam do.");

            /* Push member name as string key */
            int nameConst = makeConstant(parser, OBJ_VAL(
                copyString(parser->vm, parser->previous.start,
                           parser->previous.length)));
            emitConstantOp(parser, OP_CONSTANT, OP_CONSTANT_LONG, nameConst);

            /* Check for explicit value assignment */
            if (matchToken(parser, TOKEN_EQUAL)) {
                /* If it's a number literal, track for auto-increment */
                if (check(parser, TOKEN_NUMBER)) {
                    double val = strtod(parser->current.start, NULL);
                    nextValue = (int)val + 1;
                    expression(parser);
                } else {
                    expression(parser);
                    nextValue++;
                }
            } else {
                /* Auto-increment integer value */
                emitConstant(parser, NUMBER_VAL(nextValue));
                nextValue++;
            }
            memberCount++;
            skipNewlines(parser);
        } while (matchToken(parser, TOKEN_COMMA));
    }

    skipNewlines(parser);
    consume(parser, TOKEN_RIGHT_BRACE, "'}' lagao enum band karne ke liye.");
    consumeNewlineOrEnd(parser);

    emitBytes(parser, OP_MAP_NEW, (uint8_t)memberCount);
    defineVariable(parser, global);
}

static void declaration(Parser* parser) {
    skipNewlines(parser);
    if (check(parser, TOKEN_EOF)) return;

    if (matchToken(parser, TOKEN_MAANO)) {
        varDeclaration(parser);
    } else if (matchToken(parser, TOKEN_KAAM)) {
        functionDeclaration(parser);
    } else if (matchToken(parser, TOKEN_KAKSHA)) {
        classDeclaration(parser);
    } else if (matchToken(parser, TOKEN_GANANA)) {
        enumDeclaration(parser);
    } else if (matchToken(parser, TOKEN_SHAMIL_KARO)) {
        importStatement(parser);
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
