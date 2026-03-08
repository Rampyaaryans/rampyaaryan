/*
 * ============================================================================
 *  RAMPYAARYAN - Lexer/Tokenizer Header
 *  Converts Hinglish source code into tokens
 * ============================================================================
 */

#ifndef RAMPYAARYAN_LEXER_H
#define RAMPYAARYAN_LEXER_H

#include "common.h"

/* ============================================================================
 *  TOKEN TYPES
 * ============================================================================ */
typedef enum {
    /* Single-character tokens */
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_DOT_DOT_DOT, TOKEN_COLON, TOKEN_SEMICOLON,
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH,
    TOKEN_PERCENT,

    /* One or two character tokens */
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL,
    TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL, TOKEN_PERCENT_EQUAL,
    TOKEN_STAR_STAR,  /* ** power */
    TOKEN_STAR_STAR_EQUAL, /* **= power assign */
    TOKEN_AMPERSAND,         /* & bitwise AND */
    TOKEN_AMPERSAND_EQUAL,   /* &= bitwise AND assign */
    TOKEN_PIPE,              /* | bitwise OR */
    TOKEN_PIPE_EQUAL,        /* |= bitwise OR assign */
    TOKEN_CARET,             /* ^ bitwise XOR */
    TOKEN_CARET_EQUAL,       /* ^= bitwise XOR assign */
    TOKEN_TILDE,             /* ~ bitwise NOT */
    TOKEN_LESS_LESS,         /* << left shift */
    TOKEN_LESS_LESS_EQUAL,   /* <<= left shift assign */
    TOKEN_GREATER_GREATER,   /* >> right shift */
    TOKEN_GREATER_GREATER_EQUAL, /* >>= right shift assign */

    /* Literals */
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_TRIPLE_STRING, TOKEN_NUMBER,

    /* Hinglish Keywords */
    TOKEN_MAANO,        /* let / var */
    TOKEN_LIKHO,        /* print */
    TOKEN_PUCHO,        /* input */
    TOKEN_AGAR,         /* if */
    TOKEN_WARNA,        /* else */
    TOKEN_WARNA_AGAR,   /* else if */
    TOKEN_JAB_TAK,      /* while */
    TOKEN_HAR,          /* for (har x se 1 tak 10) */
    TOKEN_SE,           /* from */
    TOKEN_TAK,          /* to/until */
    TOKEN_KAAM,         /* function */
    TOKEN_WAPAS_DO,     /* return */
    TOKEN_RUKO,         /* break */
    TOKEN_AGLA,         /* continue */
    TOKEN_SACH,         /* true */
    TOKEN_JHOOTH,       /* false */
    TOKEN_KHALI,        /* null/none */
    TOKEN_AUR,          /* and */
    TOKEN_YA,           /* or */
    TOKEN_NAHI,         /* not */

    /* Try-catch */
    TOKEN_KOSHISH,      /* try */
    TOKEN_PAKDO,        /* catch */
    TOKEN_AAKHIR,       /* finally */
    TOKEN_PHENKO,       /* throw */

    /* OOP */
    TOKEN_KAKSHA,       /* class */
    TOKEN_NAYA,         /* new */
    TOKEN_YEH,          /* this/self */
    TOKEN_SUPER,        /* super (parent class) */

    /* Enhanced loops */
    TOKEN_MEIN,         /* in (for-in) */
    TOKEN_KARO,         /* do (do-while) */

    /* Enum */
    TOKEN_GANANA,       /* enum */
    TOKEN_PAKKA,        /* assert */

    /* Module system */
    TOKEN_SHAMIL_KARO,  /* import */
    TOKEN_BAHAR_BHEJO,  /* export */

    /* Switch/Match */
    TOKEN_DEKHO,        /* switch */
    TOKEN_JAB,          /* case (when) */
    TOKEN_CHALAO,       /* default */

    /* Lambda */
    TOKEN_ARROW,        /* => */

    /* String interpolation */
    TOKEN_INTERP_START, /* opening part of interpolated string "text${ */
    TOKEN_INTERP_MID,   /* middle segment }text${ */
    TOKEN_INTERP_END,   /* closing segment }text" */

    /* Ternary */
    TOKEN_QUESTION,          /* ? ternary operator */
    TOKEN_QUESTION_QUESTION, /* ?? null coalescing */

    /* Special */
    TOKEN_NEWLINE,
    TOKEN_ERROR,
    TOKEN_EOF,
} RamTokenType;

/* ============================================================================
 *  TOKEN
 * ============================================================================ */
typedef struct {
    RamTokenType type;
    const char* start;
    int length;
    int line;
    int column;
} Token;

/* ============================================================================
 *  LEXER
 * ============================================================================ */
typedef struct {
    const char* start;
    const char* current;
    const char* source;   /* Original source for error reporting */
    int line;
    int column;
    const char* filename;
    int interpDepth;      /* >0 when inside ${...} interpolation */
} Lexer;

void initLexer(Lexer* lexer, const char* source, const char* filename);
Token scanToken(Lexer* lexer);
const char* tokenTypeName(RamTokenType type);

#endif /* RAMPYAARYAN_LEXER_H */
