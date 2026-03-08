/*
 * ============================================================================
 *  RAMPYAARYAN - Lexer Implementation
 *  Tokenizes Hinglish source code with multi-word keyword support
 * ============================================================================
 */

#include "lexer.h"

void initLexer(Lexer* lexer, const char* source, const char* filename) {
    lexer->start = source;
    lexer->current = source;
    lexer->source = source;
    lexer->line = 1;
    lexer->column = 1;
    lexer->filename = filename ? filename : "<stdin>";
    lexer->interpDepth = 0;
}

/* ============================================================================
 *  HELPERS
 * ============================================================================ */
static bool isAtEnd(Lexer* lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer* lexer) {
    lexer->current++;
    lexer->column++;
    return lexer->current[-1];
}

static char peekChar(Lexer* lexer) {
    return *lexer->current;
}

static char peekNext(Lexer* lexer) {
    if (isAtEnd(lexer)) return '\0';
    return lexer->current[1];
}

static bool match(Lexer* lexer, char expected) {
    if (isAtEnd(lexer)) return false;
    if (*lexer->current != expected) return false;
    lexer->current++;
    lexer->column++;
    return true;
}

static Token makeToken(Lexer* lexer, RamTokenType type) {
    Token token;
    token.type = type;
    token.start = lexer->start;
    token.length = (int)(lexer->current - lexer->start);
    token.line = lexer->line;
    token.column = lexer->column - token.length;
    return token;
}

static Token errorToken(Lexer* lexer, const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = lexer->line;
    token.column = lexer->column;
    return token;
}

/* ============================================================================
 *  SKIP WHITESPACE & COMMENTS
 * ============================================================================ */
static void skipWhitespace(Lexer* lexer) {
    for (;;) {
        char c = peekChar(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '#':
                /* Single-line comment */
                while (peekChar(lexer) != '\n' && !isAtEnd(lexer)) advance(lexer);
                break;
            case '/':
                if (peekNext(lexer) == '/') {
                    /* // comment */
                    while (peekChar(lexer) != '\n' && !isAtEnd(lexer)) advance(lexer);
                } else if (peekNext(lexer) == '*') {
                    /* block comment */
                    advance(lexer); /* skip / */
                    advance(lexer); /* skip * */
                    while (!isAtEnd(lexer)) {
                        if (peekChar(lexer) == '\n') lexer->line++;
                        if (peekChar(lexer) == '*' && peekNext(lexer) == '/') {
                            advance(lexer); advance(lexer);
                            break;
                        }
                        advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

/* ============================================================================
 *  STRING LITERAL
 * ============================================================================ */
static Token string(Lexer* lexer, char quote) {
    while (peekChar(lexer) != quote && !isAtEnd(lexer)) {
        if (peekChar(lexer) == '\n') {
            return errorToken(lexer, "String mein naye line allowed nahi hai. String band karo pehle.");
        }
        if (peekChar(lexer) == '\\') {
            advance(lexer); /* Skip backslash */
            if (!isAtEnd(lexer)) advance(lexer); /* Skip escaped char */
        } else if (quote == '"' && peekChar(lexer) == '$' && peekNext(lexer) == '{') {
            /* String interpolation: emit the prefix as INTERP_START */
            Token t = makeToken(lexer, TOKEN_INTERP_START);
            advance(lexer); /* skip $ */
            advance(lexer); /* skip { */
            lexer->interpDepth++;
            return t;
        } else {
            advance(lexer);
        }
    }
    if (isAtEnd(lexer)) {
        return errorToken(lexer, "String khatam nahi hui! Closing quote lagao.");
    }
    advance(lexer); /* closing quote */
    return makeToken(lexer, TOKEN_STRING);
}

/* Triple-quoted multi-line string: """...""" or '''...''' */
static Token tripleString(Lexer* lexer, char quote) {
    while (!isAtEnd(lexer)) {
        if (peekChar(lexer) == quote && peekNext(lexer) == quote) {
            /* Check for third quote */
            const char* saved = lexer->current;
            advance(lexer); advance(lexer);
            if (!isAtEnd(lexer) && peekChar(lexer) == quote) {
                advance(lexer); /* consume third closing quote */
                return makeToken(lexer, TOKEN_TRIPLE_STRING);
            }
            /* Not triple-close, continue scanning */
            (void)saved;
        } else if (peekChar(lexer) == '\\') {
            advance(lexer);
            if (!isAtEnd(lexer)) advance(lexer);
        } else {
            if (peekChar(lexer) == '\n') lexer->line++;
            advance(lexer);
        }
    }
    return errorToken(lexer, "Multi-line string khatam nahi hui! Closing triple-quote lagao.");
}

/* Resume scanning the rest of an interpolated string after '}' */
static Token interpString(Lexer* lexer) {
    /* lexer->current is just past the '}', start scanning string content */
    lexer->start = lexer->current;
    while (peekChar(lexer) != '"' && !isAtEnd(lexer)) {
        if (peekChar(lexer) == '\n') {
            return errorToken(lexer, "String mein naye line allowed nahi hai.");
        }
        if (peekChar(lexer) == '\\') {
            advance(lexer);
            if (!isAtEnd(lexer)) advance(lexer);
        } else if (peekChar(lexer) == '$' && peekNext(lexer) == '{') {
            /* Another interpolation segment */
            Token t = makeToken(lexer, TOKEN_INTERP_MID);
            advance(lexer); /* skip $ */
            advance(lexer); /* skip { */
            lexer->interpDepth++;
            return t;
        } else {
            advance(lexer);
        }
    }
    if (isAtEnd(lexer)) {
        return errorToken(lexer, "String khatam nahi hui! Closing quote lagao.");
    }
    advance(lexer); /* closing quote */
    return makeToken(lexer, TOKEN_INTERP_END);
}

/* ============================================================================
 *  NUMBER LITERAL
 * ============================================================================ */
static Token number(Lexer* lexer) {
    while (!isAtEnd(lexer) && isdigit(peekChar(lexer))) advance(lexer);
    /* Look for decimal part */
    if (peekChar(lexer) == '.' && isdigit(peekNext(lexer))) {
        advance(lexer); /* consume '.' */
        while (!isAtEnd(lexer) && isdigit(peekChar(lexer))) advance(lexer);
    }
    return makeToken(lexer, TOKEN_NUMBER);
}

/* ============================================================================
 *  KEYWORD MATCHING
 * ============================================================================ */
static RamTokenType checkKeyword(Lexer* lexer, int start, int length,
                               const char* rest, RamTokenType type) {
    if (lexer->current - lexer->start == start + length &&
        memcmp(lexer->start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

/*
 * Trie-based keyword matching for fast Hinglish keyword lookup
 */
static RamTokenType identifierType(Lexer* lexer) {
    int len = (int)(lexer->current - lexer->start);
    char first = lexer->start[0];

    /* Convert first char to lowercase for case-insensitive matching */
    char fl = (first >= 'A' && first <= 'Z') ? first + 32 : first;

    switch (fl) {
        case 'a':
            if (len == 4) {
                if (lexer->start[1] == 'g' || lexer->start[1] == 'G') {
                    if (checkKeyword(lexer, 0, 4, "agar", TOKEN_AGAR) != TOKEN_IDENTIFIER)
                        return TOKEN_AGAR;
                    if (checkKeyword(lexer, 0, 4, "agla", TOKEN_AGLA) != TOKEN_IDENTIFIER)
                        return TOKEN_AGLA;
                }
                if (checkKeyword(lexer, 0, 4, "Agar", TOKEN_AGAR) != TOKEN_IDENTIFIER)
                    return TOKEN_AGAR;
                if (checkKeyword(lexer, 0, 4, "Agla", TOKEN_AGLA) != TOKEN_IDENTIFIER)
                    return TOKEN_AGLA;
            }
            if (len == 6 && checkKeyword(lexer, 0, 6, "aakhir", TOKEN_AAKHIR) != TOKEN_IDENTIFIER)
                return TOKEN_AAKHIR;
            if (len == 3 && checkKeyword(lexer, 0, 3, "aur", TOKEN_AUR) != TOKEN_IDENTIFIER)
                return TOKEN_AUR;
            break;

        case 'b':
            break;

        case 'c':
            if (len == 6 && checkKeyword(lexer, 0, 6, "chalao", TOKEN_CHALAO) != TOKEN_IDENTIFIER)
                return TOKEN_CHALAO;
            break;

        case 'd':
            if (len == 5 && checkKeyword(lexer, 0, 5, "dekho", TOKEN_DEKHO) != TOKEN_IDENTIFIER)
                return TOKEN_DEKHO;
            break;

        case 'g':
            if (len == 6 && checkKeyword(lexer, 0, 6, "ganana", TOKEN_GANANA) != TOKEN_IDENTIFIER)
                return TOKEN_GANANA;
            break;

        case 'h':
            if (len == 3 && checkKeyword(lexer, 0, 3, "har", TOKEN_HAR) != TOKEN_IDENTIFIER)
                return TOKEN_HAR;
            break;

        case 'j':
            if (len == 6 && checkKeyword(lexer, 0, 6, "jhooth", TOKEN_JHOOTH) != TOKEN_IDENTIFIER)
                return TOKEN_JHOOTH;
            if (len == 3 && checkKeyword(lexer, 0, 3, "jab", TOKEN_JAB) != TOKEN_IDENTIFIER)
                return TOKEN_JAB;
            break;

        case 'k':
            if (len == 4) {
                if (checkKeyword(lexer, 0, 4, "kaam", TOKEN_KAAM) != TOKEN_IDENTIFIER)
                    return TOKEN_KAAM;
                if (checkKeyword(lexer, 0, 4, "karo", TOKEN_KARO) != TOKEN_IDENTIFIER)
                    return TOKEN_KARO;
            }
            if (len == 5) {
                if (checkKeyword(lexer, 0, 5, "khali", TOKEN_KHALI) != TOKEN_IDENTIFIER)
                    return TOKEN_KHALI;
            }
            if (len == 6 && checkKeyword(lexer, 0, 6, "kaksha", TOKEN_KAKSHA) != TOKEN_IDENTIFIER)
                return TOKEN_KAKSHA;
            if (len == 7 && checkKeyword(lexer, 0, 7, "koshish", TOKEN_KOSHISH) != TOKEN_IDENTIFIER)
                return TOKEN_KOSHISH;
            break;

        case 'l':
            if (len == 5 && checkKeyword(lexer, 0, 5, "likho", TOKEN_LIKHO) != TOKEN_IDENTIFIER)
                return TOKEN_LIKHO;
            break;

        case 'm':
            if (len == 5 && checkKeyword(lexer, 0, 5, "maano", TOKEN_MAANO) != TOKEN_IDENTIFIER)
                return TOKEN_MAANO;
            if (len == 4 && checkKeyword(lexer, 0, 4, "mein", TOKEN_MEIN) != TOKEN_IDENTIFIER)
                return TOKEN_MEIN;
            break;

        case 'n':
            if (len == 4) {
                if (checkKeyword(lexer, 0, 4, "nahi", TOKEN_NAHI) != TOKEN_IDENTIFIER)
                    return TOKEN_NAHI;
                if (checkKeyword(lexer, 0, 4, "naya", TOKEN_NAYA) != TOKEN_IDENTIFIER)
                    return TOKEN_NAYA;
            }
            break;

        case 'p':
            if (len == 5) {
                if (checkKeyword(lexer, 0, 5, "pucho", TOKEN_PUCHO) != TOKEN_IDENTIFIER)
                    return TOKEN_PUCHO;
                if (checkKeyword(lexer, 0, 5, "pakdo", TOKEN_PAKDO) != TOKEN_IDENTIFIER)
                    return TOKEN_PAKDO;
                if (checkKeyword(lexer, 0, 5, "pakka", TOKEN_PAKKA) != TOKEN_IDENTIFIER)
                    return TOKEN_PAKKA;
            }
            if (len == 6 && checkKeyword(lexer, 0, 6, "phenko", TOKEN_PHENKO) != TOKEN_IDENTIFIER)
                return TOKEN_PHENKO;
            break;

        case 'r':
            if (len == 4 && checkKeyword(lexer, 0, 4, "ruko", TOKEN_RUKO) != TOKEN_IDENTIFIER)
                return TOKEN_RUKO;
            break;

        case 's':
            if (len == 4 && checkKeyword(lexer, 0, 4, "sach", TOKEN_SACH) != TOKEN_IDENTIFIER)
                return TOKEN_SACH;
            if (len == 5 && checkKeyword(lexer, 0, 5, "super", TOKEN_SUPER) != TOKEN_IDENTIFIER)
                return TOKEN_SUPER;
            if (len == 2 && checkKeyword(lexer, 0, 2, "se", TOKEN_SE) != TOKEN_IDENTIFIER)
                return TOKEN_SE;
            break;

        case 't':
            if (len == 3 && checkKeyword(lexer, 0, 3, "tak", TOKEN_TAK) != TOKEN_IDENTIFIER)
                return TOKEN_TAK;
            break;

        case 'w':
            if (len == 5 && checkKeyword(lexer, 0, 5, "warna", TOKEN_WARNA) != TOKEN_IDENTIFIER)
                return TOKEN_WARNA;
            break;

        case 'y':
            if (len == 2 && checkKeyword(lexer, 0, 2, "ya", TOKEN_YA) != TOKEN_IDENTIFIER)
                return TOKEN_YA;
            if (len == 3 && checkKeyword(lexer, 0, 3, "yeh", TOKEN_YEH) != TOKEN_IDENTIFIER)
                return TOKEN_YEH;
            break;
    }

    return TOKEN_IDENTIFIER;
}

/* ============================================================================
 *  IDENTIFIER & MULTI-WORD KEYWORD HANDLING
 * ============================================================================ */
static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

static bool isAlphaNumeric(char c) {
    return isAlpha(c) || isdigit(c);
}

/* Check for multi-word keywords by looking ahead */
static bool matchMultiWordAhead(Lexer* lexer, const char* word) {
    const char* p = lexer->current;
    /* Skip whitespace */
    while (*p == ' ' || *p == '\t') p++;
    /* Match next word */
    int len = (int)strlen(word);
    if (strncmp(p, word, len) == 0 && !isAlphaNumeric(p[len])) {
        return true;
    }
    return false;
}

static void consumeMultiWord(Lexer* lexer, const char* word) {
    /* Skip whitespace */
    while (*lexer->current == ' ' || *lexer->current == '\t') {
        lexer->current++;
        lexer->column++;
    }
    /* Skip word */
    int len = (int)strlen(word);
    lexer->current += len;
    lexer->column += len;
}

static Token identifier(Lexer* lexer) {
    while (!isAtEnd(lexer) && isAlphaNumeric(peekChar(lexer))) advance(lexer);

    RamTokenType type = identifierType(lexer);

    /* Check for multi-word keywords */
    if (type == TOKEN_IDENTIFIER || type == TOKEN_JAB) {
        int len = (int)(lexer->current - lexer->start);
        /* "jab" -> check for "tak" to form "jab tak" */
        if (len == 3 && strncmp(lexer->start, "jab", 3) == 0) {
            if (matchMultiWordAhead(lexer, "tak")) {
                consumeMultiWord(lexer, "tak");
                return makeToken(lexer, TOKEN_JAB_TAK);
            }
        }
        /* "wapas" -> check for "do" to form "wapas do" */
        if (len == 5 && strncmp(lexer->start, "wapas", 5) == 0) {
            if (matchMultiWordAhead(lexer, "do")) {
                consumeMultiWord(lexer, "do");
                return makeToken(lexer, TOKEN_WAPAS_DO);
            }
        }
        /* "shamil" -> check for "karo" to form "shamil karo" */
        if (len == 6 && strncmp(lexer->start, "shamil", 6) == 0) {
            if (matchMultiWordAhead(lexer, "karo")) {
                consumeMultiWord(lexer, "karo");
                return makeToken(lexer, TOKEN_SHAMIL_KARO);
            }
        }
        /* "bahar" -> check for "bhejo" to form "bahar bhejo" */
        if (len == 5 && strncmp(lexer->start, "bahar", 5) == 0) {
            if (matchMultiWordAhead(lexer, "bhejo")) {
                consumeMultiWord(lexer, "bhejo");
                return makeToken(lexer, TOKEN_BAHAR_BHEJO);
            }
        }
    }

    /* Check "warna" -> "warna agar" */
    if (type == TOKEN_WARNA) {
        if (matchMultiWordAhead(lexer, "agar")) {
            consumeMultiWord(lexer, "agar");
            return makeToken(lexer, TOKEN_WARNA_AGAR);
        }
    }

    return makeToken(lexer, type);
}

/* ============================================================================
 *  MAIN SCAN FUNCTION
 * ============================================================================ */
Token scanToken(Lexer* lexer) {
    skipWhitespace(lexer);
    lexer->start = lexer->current;

    if (isAtEnd(lexer)) return makeToken(lexer, TOKEN_EOF);

    char c = advance(lexer);

    /* Newline */
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
        return makeToken(lexer, TOKEN_NEWLINE);
    }

    /* Identifiers & keywords */
    if (isAlpha(c)) return identifier(lexer);

    /* Numbers */
    if (isdigit(c)) return number(lexer);

    /* Operators & delimiters */
    switch (c) {
        case '(': return makeToken(lexer, TOKEN_LEFT_PAREN);
        case ')': return makeToken(lexer, TOKEN_RIGHT_PAREN);
        case '{': return makeToken(lexer, TOKEN_LEFT_BRACE);
        case '}':
            if (lexer->interpDepth > 0) {
                lexer->interpDepth--;
                return interpString(lexer);
            }
            return makeToken(lexer, TOKEN_RIGHT_BRACE);
        case '[': return makeToken(lexer, TOKEN_LEFT_BRACKET);
        case ']': return makeToken(lexer, TOKEN_RIGHT_BRACKET);
        case ',': return makeToken(lexer, TOKEN_COMMA);
        case '.':
            if (peekChar(lexer) == '.' && peekNext(lexer) == '.') {
                advance(lexer); advance(lexer);
                return makeToken(lexer, TOKEN_DOT_DOT_DOT);
            }
            return makeToken(lexer, TOKEN_DOT);
        case ':': return makeToken(lexer, TOKEN_COLON);
        case ';': return makeToken(lexer, TOKEN_SEMICOLON);
        case '%': return makeToken(lexer, match(lexer, '=') ? TOKEN_PERCENT_EQUAL : TOKEN_PERCENT);
        case '?': return makeToken(lexer, match(lexer, '?') ? TOKEN_QUESTION_QUESTION : TOKEN_QUESTION);

        case '&': return makeToken(lexer, match(lexer, '=') ? TOKEN_AMPERSAND_EQUAL : TOKEN_AMPERSAND);
        case '|': return makeToken(lexer, match(lexer, '=') ? TOKEN_PIPE_EQUAL : TOKEN_PIPE);
        case '^': return makeToken(lexer, match(lexer, '=') ? TOKEN_CARET_EQUAL : TOKEN_CARET);
        case '~': return makeToken(lexer, TOKEN_TILDE);

        case '+':
            return makeToken(lexer, match(lexer, '=') ? TOKEN_PLUS_EQUAL : TOKEN_PLUS);
        case '-':
            return makeToken(lexer, match(lexer, '=') ? TOKEN_MINUS_EQUAL : TOKEN_MINUS);
        case '*':
            if (match(lexer, '*')) {
                return makeToken(lexer, match(lexer, '=') ? TOKEN_STAR_STAR_EQUAL : TOKEN_STAR_STAR);
            }
            return makeToken(lexer, match(lexer, '=') ? TOKEN_STAR_EQUAL : TOKEN_STAR);
        case '/':
            return makeToken(lexer, match(lexer, '=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);
        case '!':
            return makeToken(lexer, match(lexer, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            if (match(lexer, '>')) return makeToken(lexer, TOKEN_ARROW);
            return makeToken(lexer, match(lexer, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            if (match(lexer, '<')) {
                return makeToken(lexer, match(lexer, '=') ? TOKEN_LESS_LESS_EQUAL : TOKEN_LESS_LESS);
            }
            return makeToken(lexer, match(lexer, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            if (match(lexer, '>')) {
                return makeToken(lexer, match(lexer, '=') ? TOKEN_GREATER_GREATER_EQUAL : TOKEN_GREATER_GREATER);
            }
            return makeToken(lexer, match(lexer, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

        case '"':
            if (peekChar(lexer) == '"' && peekNext(lexer) == '"') {
                advance(lexer); advance(lexer); /* skip two more quotes */
                return tripleString(lexer, '"');
            }
            return string(lexer, '"');
        case '\'':
            if (peekChar(lexer) == '\'' && peekNext(lexer) == '\'') {
                advance(lexer); advance(lexer);
                return tripleString(lexer, '\'');
            }
            return string(lexer, '\'');
    }

    return errorToken(lexer, "Ye character samajh nahi aaya.");
}

/* ============================================================================
 *  TOKEN TYPE NAME (for debugging)
 * ============================================================================ */
const char* tokenTypeName(RamTokenType type) {
    switch (type) {
        case TOKEN_LEFT_PAREN:    return "LEFT_PAREN";
        case TOKEN_RIGHT_PAREN:   return "RIGHT_PAREN";
        case TOKEN_LEFT_BRACE:    return "LEFT_BRACE";
        case TOKEN_RIGHT_BRACE:   return "RIGHT_BRACE";
        case TOKEN_LEFT_BRACKET:  return "LEFT_BRACKET";
        case TOKEN_RIGHT_BRACKET: return "RIGHT_BRACKET";
        case TOKEN_COMMA:         return "COMMA";
        case TOKEN_DOT:           return "DOT";
        case TOKEN_DOT_DOT_DOT:   return "DOT_DOT_DOT";
        case TOKEN_COLON:         return "COLON";
        case TOKEN_SEMICOLON:     return "SEMICOLON";
        case TOKEN_PLUS:          return "PLUS";
        case TOKEN_MINUS:         return "MINUS";
        case TOKEN_STAR:          return "STAR";
        case TOKEN_SLASH:         return "SLASH";
        case TOKEN_PERCENT:       return "PERCENT";
        case TOKEN_PERCENT_EQUAL: return "PERCENT_EQUAL";
        case TOKEN_BANG:          return "BANG";
        case TOKEN_BANG_EQUAL:    return "BANG_EQUAL";
        case TOKEN_EQUAL:         return "EQUAL";
        case TOKEN_EQUAL_EQUAL:   return "EQUAL_EQUAL";
        case TOKEN_GREATER:       return "GREATER";
        case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
        case TOKEN_LESS:          return "LESS";
        case TOKEN_LESS_EQUAL:    return "LESS_EQUAL";
        case TOKEN_PLUS_EQUAL:    return "PLUS_EQUAL";
        case TOKEN_MINUS_EQUAL:   return "MINUS_EQUAL";
        case TOKEN_STAR_EQUAL:    return "STAR_EQUAL";
        case TOKEN_SLASH_EQUAL:   return "SLASH_EQUAL";
        case TOKEN_STAR_STAR:     return "STAR_STAR";
        case TOKEN_STAR_STAR_EQUAL: return "STAR_STAR_EQUAL";
        case TOKEN_AMPERSAND:    return "AMPERSAND";
        case TOKEN_AMPERSAND_EQUAL: return "AMPERSAND_EQUAL";
        case TOKEN_PIPE:         return "PIPE";
        case TOKEN_PIPE_EQUAL:   return "PIPE_EQUAL";
        case TOKEN_CARET:        return "CARET";
        case TOKEN_CARET_EQUAL:  return "CARET_EQUAL";
        case TOKEN_TILDE:        return "TILDE";
        case TOKEN_LESS_LESS:    return "LESS_LESS";
        case TOKEN_LESS_LESS_EQUAL: return "LESS_LESS_EQUAL";
        case TOKEN_GREATER_GREATER: return "GREATER_GREATER";
        case TOKEN_GREATER_GREATER_EQUAL: return "GREATER_GREATER_EQUAL";
        case TOKEN_IDENTIFIER:    return "IDENTIFIER";
        case TOKEN_STRING:        return "STRING";
        case TOKEN_TRIPLE_STRING: return "TRIPLE_STRING";
        case TOKEN_NUMBER:        return "NUMBER";
        case TOKEN_MAANO:         return "MAANO";
        case TOKEN_LIKHO:         return "LIKHO";
        case TOKEN_PUCHO:         return "PUCHO";
        case TOKEN_AGAR:          return "AGAR";
        case TOKEN_WARNA:         return "WARNA";
        case TOKEN_WARNA_AGAR:    return "WARNA_AGAR";
        case TOKEN_JAB_TAK:       return "JAB_TAK";
        case TOKEN_HAR:           return "HAR";
        case TOKEN_SE:            return "SE";
        case TOKEN_TAK:           return "TAK";
        case TOKEN_KAAM:          return "KAAM";
        case TOKEN_WAPAS_DO:      return "WAPAS_DO";
        case TOKEN_RUKO:          return "RUKO";
        case TOKEN_AGLA:          return "AGLA";
        case TOKEN_SACH:          return "SACH";
        case TOKEN_JHOOTH:        return "JHOOTH";
        case TOKEN_KHALI:         return "KHALI";
        case TOKEN_AUR:           return "AUR";
        case TOKEN_YA:            return "YA";
        case TOKEN_NAHI:          return "NAHI";
        case TOKEN_KOSHISH:       return "KOSHISH";
        case TOKEN_PAKDO:         return "PAKDO";
        case TOKEN_AAKHIR:        return "AAKHIR";
        case TOKEN_PHENKO:        return "PHENKO";
        case TOKEN_KAKSHA:        return "KAKSHA";
        case TOKEN_NAYA:          return "NAYA";
        case TOKEN_YEH:           return "YEH";
        case TOKEN_SUPER:         return "SUPER";
        case TOKEN_MEIN:          return "MEIN";
        case TOKEN_KARO:          return "KARO";
        case TOKEN_GANANA:        return "GANANA";
        case TOKEN_PAKKA:         return "PAKKA";
        case TOKEN_SHAMIL_KARO:   return "SHAMIL_KARO";
        case TOKEN_BAHAR_BHEJO:   return "BAHAR_BHEJO";
        case TOKEN_DEKHO:         return "DEKHO";
        case TOKEN_JAB:           return "JAB";
        case TOKEN_CHALAO:        return "CHALAO";
        case TOKEN_ARROW:         return "ARROW";
        case TOKEN_INTERP_START:  return "INTERP_START";
        case TOKEN_INTERP_MID:    return "INTERP_MID";
        case TOKEN_INTERP_END:    return "INTERP_END";
        case TOKEN_QUESTION:      return "QUESTION";
        case TOKEN_QUESTION_QUESTION: return "QUESTION_QUESTION";
        case TOKEN_NEWLINE:       return "NEWLINE";
        case TOKEN_ERROR:         return "ERROR";
        case TOKEN_EOF:           return "EOF";
        default:                  return "UNKNOWN";
    }
}
