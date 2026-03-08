/*
 * ============================================================================
 *  RAMPYAARYAN - Native (Built-in) Functions
 *  All standard library functions available without importing
 *  60+ built-in functions for strings, math, lists, I/O, types, system
 * ============================================================================
 */

#include "native.h"
#include "vm.h"
#include "object.h"
#include "memory.h"

#include <time.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

/* ============================================================================
 *  I/O FUNCTIONS
 * ============================================================================ */

/* likho_line(values...) - Print with newline (alias for convenience) */
static Value nativeLikhoLine(VM* vm, int argCount, Value* args) {
    for (int i = 0; i < argCount; i++) {
        if (i > 0) printf(" ");
        printValue(args[i]);
    }
    printf("\n");
    return NULL_VAL;
}

/* ============================================================================
 *  TYPE FUNCTIONS
 * ============================================================================ */

/* prakar(value) - Return type name as string */
static Value nativePrakar(VM* vm, int argCount, Value* args) {
    (void)argCount;
    const char* typeName = valueTypeName(args[0]);
    return OBJ_VAL(copyString(vm, typeName, (int)strlen(typeName)));
}

/* sankhya(value) - Convert to number */
static Value nativeSankhya(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (IS_NUMBER(args[0])) return args[0];
    if (IS_BOOL(args[0])) return NUMBER_VAL(AS_BOOL(args[0]) ? 1.0 : 0.0);
    if (IS_NULL(args[0])) return NUMBER_VAL(0.0);
    if (IS_STRING(args[0])) {
        char* end;
        double val = strtod(AS_CSTRING(args[0]), &end);
        if (*end != '\0') {
            runtimeError(vm, "'%s' ko number mein convert nahi kar sakte.", AS_CSTRING(args[0]));
            return NULL_VAL;
        }
        return NUMBER_VAL(val);
    }
    runtimeError(vm, "Is value ko number mein convert nahi kar sakte.");
    return NULL_VAL;
}

/* dashmlav(value) - Convert to float/decimal */
static Value nativeDashmlav(VM* vm, int argCount, Value* args) {
    return nativeSankhya(vm, argCount, args);
}

/* shabd(value) - Convert to string */
static Value nativeShabd(VM* vm, int argCount, Value* args) {
    (void)argCount;
    char* str = valueToString(args[0]);
    if (str) {
        ObjString* result = copyString(vm, str, (int)strlen(str));
        free(str);
        return OBJ_VAL(result);
    }
    return OBJ_VAL(copyString(vm, "", 0));
}

/* purn(value) - Convert to integer (floor) */
static Value nativePurn(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (IS_NUMBER(args[0])) {
        return NUMBER_VAL((double)(long long)AS_NUMBER(args[0]));
    }
    if (IS_STRING(args[0])) {
        char* end;
        long long val = strtoll(AS_CSTRING(args[0]), &end, 10);
        if (*end != '\0') {
            runtimeError(vm, "'%s' ko integer mein convert nahi kar sakte.", AS_CSTRING(args[0]));
            return NULL_VAL;
        }
        return NUMBER_VAL((double)val);
    }
    runtimeError(vm, "Is value ko integer mein convert nahi kar sakte.");
    return NULL_VAL;
}

/* ============================================================================
 *  STRING FUNCTIONS
 * ============================================================================ */

/* lambai(value) - Get length of string or list */
static Value nativeLambai(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (IS_STRING(args[0])) {
        return NUMBER_VAL((double)AS_STRING(args[0])->length);
    }
    if (IS_LIST(args[0])) {
        return NUMBER_VAL((double)AS_LIST(args[0])->items.count);
    }
    runtimeError(vm, "lambai() sirf string ya list ke liye kaam karta hai.");
    return NULL_VAL;
}

/* bade_akshar(str) - Uppercase */
static Value nativeBadeAkshar(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0])) {
        runtimeError(vm, "bade_akshar() ko string chahiye.");
        return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    char* upper = ALLOCATE(char, str->length + 1);
    for (int i = 0; i < str->length; i++) {
        upper[i] = (char)toupper((unsigned char)str->chars[i]);
    }
    upper[str->length] = '\0';
    return OBJ_VAL(takeString(vm, upper, str->length));
}

/* chhote_akshar(str) - Lowercase */
static Value nativeChhoteAkshar(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0])) {
        runtimeError(vm, "chhote_akshar() ko string chahiye.");
        return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    char* lower = ALLOCATE(char, str->length + 1);
    for (int i = 0; i < str->length; i++) {
        lower[i] = (char)tolower((unsigned char)str->chars[i]);
    }
    lower[str->length] = '\0';
    return OBJ_VAL(takeString(vm, lower, str->length));
}

/* kato(str, start, end) - Substring */
static Value nativeKato(VM* vm, int argCount, Value* args) {
    if (!IS_STRING(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "kato() ko string aur number(s) chahiye.");
        return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    int start = (int)AS_NUMBER(args[1]);
    int end = (argCount >= 3 && IS_NUMBER(args[2])) ? (int)AS_NUMBER(args[2]) : str->length;

    if (start < 0) start = str->length + start;
    if (end < 0) end = str->length + end;
    if (start < 0) start = 0;
    if (end > str->length) end = str->length;
    if (start >= end) return OBJ_VAL(copyString(vm, "", 0));

    return OBJ_VAL(copyString(vm, str->chars + start, end - start));
}

/* dhundho(str, substr) - Find substring, returns index or -1 */
static Value nativeDhundho(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0]) || !IS_STRING(args[1])) {
        runtimeError(vm, "dhundho() ko do strings chahiye.");
        return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    ObjString* substr = AS_STRING(args[1]);
    char* found = strstr(str->chars, substr->chars);
    if (found == NULL) return NUMBER_VAL(-1);
    return NUMBER_VAL((double)(found - str->chars));
}

/* badlo(str, old, new) - Replace in string */
static Value nativeBadlo(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0]) || !IS_STRING(args[1]) || !IS_STRING(args[2])) {
        runtimeError(vm, "badlo() ko teen strings chahiye.");
        return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    ObjString* old = AS_STRING(args[1]);
    ObjString* rep = AS_STRING(args[2]);

    if (old->length == 0) return args[0];

    /* Count occurrences */
    int count = 0;
    const char* search = str->chars;
    while ((search = strstr(search, old->chars)) != NULL) {
        count++;
        search += old->length;
    }
    if (count == 0) return args[0];

    int newLen = str->length + count * (rep->length - old->length);
    char* result = ALLOCATE(char, newLen + 1);
    char* dst = result;
    search = str->chars;
    const char* prev = search;

    while ((search = strstr(search, old->chars)) != NULL) {
        int copyLen = (int)(search - prev);
        memcpy(dst, prev, copyLen);
        dst += copyLen;
        memcpy(dst, rep->chars, rep->length);
        dst += rep->length;
        search += old->length;
        prev = search;
    }
    strcpy(dst, prev);

    return OBJ_VAL(takeString(vm, result, newLen));
}

/* todo(str, sep) - Split string */
static Value nativeTodo(VM* vm, int argCount, Value* args) {
    if (!IS_STRING(args[0])) {
        runtimeError(vm, "todo() ko string chahiye.");
        return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    const char* sep = " ";
    if (argCount >= 2 && IS_STRING(args[1])) {
        sep = AS_CSTRING(args[1]);
    }

    ObjList* list = newList(vm);
    push(vm, OBJ_VAL(list));

    int sepLen = (int)strlen(sep);
    if (sepLen == 0) {
        /* Split each character */
        for (int i = 0; i < str->length; i++) {
            ObjString* ch = copyString(vm, &str->chars[i], 1);
            listAppend(vm, list, OBJ_VAL(ch));
        }
    } else {
        const char* start = str->chars;
        const char* end;
        while ((end = strstr(start, sep)) != NULL) {
            ObjString* part = copyString(vm, start, (int)(end - start));
            listAppend(vm, list, OBJ_VAL(part));
            start = end + sepLen;
        }
        ObjString* part = copyString(vm, start, (int)strlen(start));
        listAppend(vm, list, OBJ_VAL(part));
    }

    pop(vm);
    return OBJ_VAL(list);
}

/* ============================================================================
 *  LIST FUNCTIONS
 * ============================================================================ */

/* joodo(list, item) - Append to list */
static Value nativeJoodo(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) {
        runtimeError(vm, "joodo() ko list chahiye.");
        return NULL_VAL;
    }
    listAppend(vm, AS_LIST(args[0]), args[1]);
    return args[0];
}

/* nikalo(list, index) - Remove from list at index */
static Value nativeNikalo(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) {
        runtimeError(vm, "nikalo() ko list chahiye.");
        return NULL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    int index = (argCount >= 2 && IS_NUMBER(args[1])) ? (int)AS_NUMBER(args[1]) : list->items.count - 1;
    if (index < 0 || index >= list->items.count) {
        runtimeError(vm, "List index %d seema se bahar hai.", index);
        return NULL_VAL;
    }
    Value removed = list->items.values[index];
    /* Shift elements */
    for (int i = index; i < list->items.count - 1; i++) {
        list->items.values[i] = list->items.values[i + 1];
    }
    list->items.count--;
    return removed;
}

/* ulta(list_or_str) - Reverse */
static Value nativeUlta(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (IS_LIST(args[0])) {
        ObjList* list = AS_LIST(args[0]);
        ObjList* reversed = newList(vm);
        push(vm, OBJ_VAL(reversed));
        for (int i = list->items.count - 1; i >= 0; i--) {
            listAppend(vm, reversed, list->items.values[i]);
        }
        pop(vm);
        return OBJ_VAL(reversed);
    }
    if (IS_STRING(args[0])) {
        ObjString* str = AS_STRING(args[0]);
        char* rev = ALLOCATE(char, str->length + 1);
        for (int i = 0; i < str->length; i++) {
            rev[i] = str->chars[str->length - 1 - i];
        }
        rev[str->length] = '\0';
        return OBJ_VAL(takeString(vm, rev, str->length));
    }
    runtimeError(vm, "ulta() sirf list ya string ke liye kaam karta hai.");
    return NULL_VAL;
}

/* kram(list) - Sort list */
static Value nativeKram(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) {
        runtimeError(vm, "kram() ko list chahiye.");
        return NULL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    /* Simple bubble sort - works for numbers and strings */
    for (int i = 0; i < list->items.count - 1; i++) {
        for (int j = 0; j < list->items.count - i - 1; j++) {
            bool shouldSwap = false;
            Value a = list->items.values[j];
            Value b = list->items.values[j + 1];
            if (IS_NUMBER(a) && IS_NUMBER(b)) {
                shouldSwap = AS_NUMBER(a) > AS_NUMBER(b);
            } else if (IS_STRING(a) && IS_STRING(b)) {
                shouldSwap = strcmp(AS_CSTRING(a), AS_CSTRING(b)) > 0;
            }
            if (shouldSwap) {
                list->items.values[j] = b;
                list->items.values[j + 1] = a;
            }
        }
    }
    return args[0];
}

/* suchi(args...) - Create list from arguments */
static Value nativeSuchi(VM* vm, int argCount, Value* args) {
    ObjList* list = newList(vm);
    push(vm, OBJ_VAL(list));
    for (int i = 0; i < argCount; i++) {
        listAppend(vm, list, args[i]);
    }
    pop(vm);
    return OBJ_VAL(list);
}

/* range_(start_or_end, end, step) - Create range list */
static Value nativeRange(VM* vm, int argCount, Value* args) {
    double start = 0, end, step = 1;

    if (argCount == 1) {
        if (!IS_NUMBER(args[0])) {
            runtimeError(vm, "range_ ko numbers chahiye.");
            return NULL_VAL;
        }
        end = AS_NUMBER(args[0]);
    } else if (argCount >= 2) {
        if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
            runtimeError(vm, "range_ ko numbers chahiye.");
            return NULL_VAL;
        }
        start = AS_NUMBER(args[0]);
        end = AS_NUMBER(args[1]);
        if (argCount >= 3 && IS_NUMBER(args[2])) {
            step = AS_NUMBER(args[2]);
        }
    } else {
        runtimeError(vm, "range_ ko kam se kam 1 argument chahiye.");
        return NULL_VAL;
    }

    if (step == 0) {
        runtimeError(vm, "range_ mein step zero nahi ho sakta.");
        return NULL_VAL;
    }

    ObjList* list = newList(vm);
    push(vm, OBJ_VAL(list));

    if (step > 0) {
        for (double i = start; i < end; i += step) {
            listAppend(vm, list, NUMBER_VAL(i));
        }
    } else {
        for (double i = start; i > end; i += step) {
            listAppend(vm, list, NUMBER_VAL(i));
        }
    }

    pop(vm);
    return OBJ_VAL(list);
}

/* shamil(list, value) - Check if value is in list/string */
static Value nativeShamil(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (IS_LIST(args[0])) {
        ObjList* list = AS_LIST(args[0]);
        for (int i = 0; i < list->items.count; i++) {
            if (valuesEqual(list->items.values[i], args[1])) {
                return BOOL_VAL(true);
            }
        }
        return BOOL_VAL(false);
    }
    if (IS_STRING(args[0]) && IS_STRING(args[1])) {
        return BOOL_VAL(strstr(AS_CSTRING(args[0]), AS_CSTRING(args[1])) != NULL);
    }
    runtimeError(vm, "shamil() ko list ya string chahiye.");
    return NULL_VAL;
}

/* ============================================================================
 *  MATH FUNCTIONS
 * ============================================================================ */

/* abs_val(x) - Absolute value */
static Value nativeAbsVal(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) {
        runtimeError(vm, "abs_val() ko number chahiye.");
        return NULL_VAL;
    }
    return NUMBER_VAL(fabs(AS_NUMBER(args[0])));
}

/* gol(x) - Round */
static Value nativeGol(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) {
        runtimeError(vm, "gol() ko number chahiye.");
        return NULL_VAL;
    }
    return NUMBER_VAL(round(AS_NUMBER(args[0])));
}

/* upar(x) - Ceiling */
static Value nativeUpar(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) {
        runtimeError(vm, "upar() ko number chahiye.");
        return NULL_VAL;
    }
    return NUMBER_VAL(ceil(AS_NUMBER(args[0])));
}

/* neeche(x) - Floor */
static Value nativeNeeche(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) {
        runtimeError(vm, "neeche() ko number chahiye.");
        return NULL_VAL;
    }
    return NUMBER_VAL(floor(AS_NUMBER(args[0])));
}

/* sqrt_val(x) - Square root */
static Value nativeSqrtVal(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) {
        runtimeError(vm, "sqrt_val() ko number chahiye.");
        return NULL_VAL;
    }
    double val = AS_NUMBER(args[0]);
    if (val < 0) {
        runtimeError(vm, "Negative number ka square root nahi le sakte.");
        return NULL_VAL;
    }
    return NUMBER_VAL(sqrt(val));
}

/* bada(a, b) - Max of two */
static Value nativeBada(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "bada() ko do numbers chahiye.");
        return NULL_VAL;
    }
    double a = AS_NUMBER(args[0]);
    double b = AS_NUMBER(args[1]);
    return NUMBER_VAL(a > b ? a : b);
}

/* chhota(a, b) - Min of two */
static Value nativeChhota(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "chhota() ko do numbers chahiye.");
        return NULL_VAL;
    }
    double a = AS_NUMBER(args[0]);
    double b = AS_NUMBER(args[1]);
    return NUMBER_VAL(a < b ? a : b);
}

/* ============================================================================
 *  UTILITY FUNCTIONS
 * ============================================================================ */

/* samay() - Get current time in seconds */
static Value nativeSamay(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount; (void)args;
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

/* ruko_samay(ms) - Sleep for milliseconds */
static Value nativeRukoSamay(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) {
        runtimeError(vm, "ruko_samay() ko number chahiye (milliseconds).");
        return NULL_VAL;
    }
    int ms = (int)AS_NUMBER(args[0]);
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
    return NULL_VAL;
}

/* yaadrchik(min, max) - Random number between min and max */
static Value nativeYaadrchik(VM* vm, int argCount, Value* args) {
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }
    if (argCount == 0) {
        return NUMBER_VAL((double)rand() / RAND_MAX);
    }
    if (argCount >= 2 && IS_NUMBER(args[0]) && IS_NUMBER(args[1])) {
        int min = (int)AS_NUMBER(args[0]);
        int max = (int)AS_NUMBER(args[1]);
        if (min > max) { int tmp = min; min = max; max = tmp; }
        return NUMBER_VAL((double)(rand() % (max - min + 1) + min));
    }
    runtimeError(vm, "yaadrchik() ko do numbers chahiye ya koi argument nahi.");
    return NULL_VAL;
}

/* bahar(code) - Exit program */
static Value nativeBahar(VM* vm, int argCount, Value* args) {
    (void)vm;
    int code = (argCount > 0 && IS_NUMBER(args[0])) ? (int)AS_NUMBER(args[0]) : 0;
    exit(code);
    return NULL_VAL;
}

/* ============================================================================
 *  ADVANCED STRING FUNCTIONS
 * ============================================================================ */

/* saaf(str) - Trim whitespace from both ends */
static Value nativeSaaf(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0])) {
        runtimeError(vm, "saaf() ko string chahiye.");
        return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    int start = 0, end = str->length - 1;
    while (start <= end && isspace((unsigned char)str->chars[start])) start++;
    while (end >= start && isspace((unsigned char)str->chars[end])) end--;
    return OBJ_VAL(copyString(vm, str->chars + start, end - start + 1));
}

/* shuru_se(str, prefix) - Check if string starts with prefix */
static Value nativeShuru(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0]) || !IS_STRING(args[1])) {
        runtimeError(vm, "shuru_se() ko do strings chahiye.");
        return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    ObjString* prefix = AS_STRING(args[1]);
    if (prefix->length > str->length) return BOOL_VAL(false);
    return BOOL_VAL(memcmp(str->chars, prefix->chars, prefix->length) == 0);
}

/* ant_se(str, suffix) - Check if string ends with suffix */
static Value nativeAnt(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0]) || !IS_STRING(args[1])) {
        runtimeError(vm, "ant_se() ko do strings chahiye.");
        return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    ObjString* suffix = AS_STRING(args[1]);
    if (suffix->length > str->length) return BOOL_VAL(false);
    return BOOL_VAL(memcmp(str->chars + str->length - suffix->length,
                           suffix->chars, suffix->length) == 0);
}

/* dohrao(str, count) - Repeat string n times */
static Value nativeDohrao(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "dohrao() ko string aur number chahiye.");
        return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    int count = (int)AS_NUMBER(args[1]);
    if (count <= 0) return OBJ_VAL(copyString(vm, "", 0));
    int newLen = str->length * count;
    char* result = ALLOCATE(char, newLen + 1);
    for (int i = 0; i < count; i++) {
        memcpy(result + i * str->length, str->chars, str->length);
    }
    result[newLen] = '\0';
    return OBJ_VAL(takeString(vm, result, newLen));
}

/* akshar(str, index) - Get character at index */
static Value nativeAkshar(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "akshar() ko string aur index chahiye.");
        return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    int index = (int)AS_NUMBER(args[1]);
    if (index < 0) index = str->length + index;
    if (index < 0 || index >= str->length) {
        runtimeError(vm, "Index %d seema se bahar hai (lambai: %d).", index, str->length);
        return NULL_VAL;
    }
    return OBJ_VAL(copyString(vm, &str->chars[index], 1));
}

/* ascii_code(char) - Get ASCII code of character */
static Value nativeAsciiCode(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0])) {
        runtimeError(vm, "ascii_code() ko string chahiye.");
        return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    if (str->length == 0) return NUMBER_VAL(0);
    return NUMBER_VAL((double)(unsigned char)str->chars[0]);
}

/* ascii_se(code) - Get character from ASCII code */
static Value nativeAsciiSe(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) {
        runtimeError(vm, "ascii_se() ko number chahiye.");
        return NULL_VAL;
    }
    char c = (char)(int)AS_NUMBER(args[0]);
    return OBJ_VAL(copyString(vm, &c, 1));
}

/* jodo_shabd(list, separator) - Join list elements with separator */
static Value nativeJodoShabd(VM* vm, int argCount, Value* args) {
    if (!IS_LIST(args[0])) {
        runtimeError(vm, "jodo_shabd() ko list chahiye.");
        return NULL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    const char* sep = (argCount >= 2 && IS_STRING(args[1])) ? AS_CSTRING(args[1]) : "";
    int sepLen = (int)strlen(sep);

    /* Calculate total length */
    int totalLen = 0;
    char** parts = malloc(sizeof(char*) * list->items.count);
    int* partLens = malloc(sizeof(int) * list->items.count);
    for (int i = 0; i < list->items.count; i++) {
        parts[i] = valueToString(list->items.values[i]);
        partLens[i] = (int)strlen(parts[i]);
        totalLen += partLens[i];
        if (i > 0) totalLen += sepLen;
    }

    char* result = ALLOCATE(char, totalLen + 1);
    int pos = 0;
    for (int i = 0; i < list->items.count; i++) {
        if (i > 0) { memcpy(result + pos, sep, sepLen); pos += sepLen; }
        memcpy(result + pos, parts[i], partLens[i]);
        pos += partLens[i];
        free(parts[i]);
    }
    result[totalLen] = '\0';
    free(parts);
    free(partLens);
    return OBJ_VAL(takeString(vm, result, totalLen));
}

/* format(template, args...) - String formatting with {} placeholders */
static Value nativeFormat(VM* vm, int argCount, Value* args) {
    if (argCount < 1 || !IS_STRING(args[0])) {
        runtimeError(vm, "format() ko template string chahiye.");
        return NULL_VAL;
    }
    ObjString* tmpl = AS_STRING(args[0]);

    /* First pass: calculate length */
    char** subs = malloc(sizeof(char*) * (argCount - 1));
    int* subLens = malloc(sizeof(int) * (argCount - 1));
    for (int i = 1; i < argCount; i++) {
        subs[i-1] = valueToString(args[i]);
        subLens[i-1] = (int)strlen(subs[i-1]);
    }

    int totalLen = 0, subIdx = 0;
    for (int i = 0; i < tmpl->length; i++) {
        if (tmpl->chars[i] == '{' && i + 1 < tmpl->length && tmpl->chars[i+1] == '}') {
            if (subIdx < argCount - 1) totalLen += subLens[subIdx++];
            i++; /* skip } */
        } else {
            totalLen++;
        }
    }

    char* result = ALLOCATE(char, totalLen + 1);
    int pos = 0;
    subIdx = 0;
    for (int i = 0; i < tmpl->length; i++) {
        if (tmpl->chars[i] == '{' && i + 1 < tmpl->length && tmpl->chars[i+1] == '}') {
            if (subIdx < argCount - 1) {
                memcpy(result + pos, subs[subIdx], subLens[subIdx]);
                pos += subLens[subIdx];
                subIdx++;
            }
            i++;
        } else {
            result[pos++] = tmpl->chars[i];
        }
    }
    result[totalLen] = '\0';

    for (int i = 0; i < argCount - 1; i++) free(subs[i]);
    free(subs);
    free(subLens);
    return OBJ_VAL(takeString(vm, result, totalLen));
}

/* gino(str, substr) - Count occurrences of substr in str */
static Value nativeGino(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0]) || !IS_STRING(args[1])) {
        runtimeError(vm, "gino() ko do strings chahiye.");
        return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    ObjString* sub = AS_STRING(args[1]);
    if (sub->length == 0) return NUMBER_VAL(0);
    int count = 0;
    const char* p = str->chars;
    while ((p = strstr(p, sub->chars)) != NULL) { count++; p += sub->length; }
    return NUMBER_VAL((double)count);
}

/* ============================================================================
 *  ADVANCED MATH FUNCTIONS
 * ============================================================================ */

/* sin_val(x) - Sine (radians) */
static Value nativeSin(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "sin_val() ko number chahiye."); return NULL_VAL; }
    return NUMBER_VAL(sin(AS_NUMBER(args[0])));
}

/* cos_val(x) - Cosine (radians) */
static Value nativeCos(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "cos_val() ko number chahiye."); return NULL_VAL; }
    return NUMBER_VAL(cos(AS_NUMBER(args[0])));
}

/* tan_val(x) - Tangent (radians) */
static Value nativeTan(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "tan_val() ko number chahiye."); return NULL_VAL; }
    return NUMBER_VAL(tan(AS_NUMBER(args[0])));
}

/* asin_val(x) - Arc sine */
static Value nativeAsin(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "asin_val() ko number chahiye."); return NULL_VAL; }
    return NUMBER_VAL(asin(AS_NUMBER(args[0])));
}

/* acos_val(x) - Arc cosine */
static Value nativeAcos(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "acos_val() ko number chahiye."); return NULL_VAL; }
    return NUMBER_VAL(acos(AS_NUMBER(args[0])));
}

/* atan_val(x) - Arc tangent */
static Value nativeAtan(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "atan_val() ko number chahiye."); return NULL_VAL; }
    return NUMBER_VAL(atan(AS_NUMBER(args[0])));
}

/* log_val(x) - Natural logarithm */
static Value nativeLog(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "log_val() ko number chahiye."); return NULL_VAL; }
    double val = AS_NUMBER(args[0]);
    if (val <= 0) { runtimeError(vm, "Negative/zero ka log nahi le sakte."); return NULL_VAL; }
    return NUMBER_VAL(log(val));
}

/* log10_val(x) - Log base 10 */
static Value nativeLog10(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "log10_val() ko number chahiye."); return NULL_VAL; }
    double val = AS_NUMBER(args[0]);
    if (val <= 0) { runtimeError(vm, "Negative/zero ka log nahi le sakte."); return NULL_VAL; }
    return NUMBER_VAL(log10(val));
}

/* exp_val(x) - e^x */
static Value nativeExp(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "exp_val() ko number chahiye."); return NULL_VAL; }
    return NUMBER_VAL(exp(AS_NUMBER(args[0])));
}

/* power_val(base, exp) - Power function */
static Value nativePower(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "power_val() ko do numbers chahiye.");
        return NULL_VAL;
    }
    return NUMBER_VAL(pow(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
}

/* PI() - Returns PI constant */
static Value nativePI(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount; (void)args;
    return NUMBER_VAL(3.14159265358979323846);
}

/* E() - Returns Euler's number */
static Value nativeE(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount; (void)args;
    return NUMBER_VAL(2.71828182845904523536);
}

/* gcd(a, b) - Greatest Common Divisor */
static Value nativeGCD(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "gcd() ko do numbers chahiye.");
        return NULL_VAL;
    }
    long long a = (long long)fabs(AS_NUMBER(args[0]));
    long long b = (long long)fabs(AS_NUMBER(args[1]));
    while (b) { long long t = b; b = a % b; a = t; }
    return NUMBER_VAL((double)a);
}

/* lcm(a, b) - Least Common Multiple */
static Value nativeLCM(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "lcm() ko do numbers chahiye.");
        return NULL_VAL;
    }
    long long a = (long long)fabs(AS_NUMBER(args[0]));
    long long b = (long long)fabs(AS_NUMBER(args[1]));
    if (a == 0 || b == 0) return NUMBER_VAL(0);
    long long g = a, h = b;
    while (h) { long long t = h; h = g % h; g = t; }
    return NUMBER_VAL((double)(a / g * b));
}

/* sign(x) - Returns -1, 0, or 1 */
static Value nativeSign(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "sign() ko number chahiye."); return NULL_VAL; }
    double x = AS_NUMBER(args[0]);
    return NUMBER_VAL(x > 0 ? 1 : (x < 0 ? -1 : 0));
}

/* clamp(value, min, max) - Clamp value between min and max */
static Value nativeClamp(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1]) || !IS_NUMBER(args[2])) {
        runtimeError(vm, "clamp() ko teen numbers chahiye.");
        return NULL_VAL;
    }
    double val = AS_NUMBER(args[0]);
    double lo = AS_NUMBER(args[1]);
    double hi = AS_NUMBER(args[2]);
    if (val < lo) return NUMBER_VAL(lo);
    if (val > hi) return NUMBER_VAL(hi);
    return NUMBER_VAL(val);
}

/* degrees(radians) - Convert radians to degrees */
static Value nativeDegrees(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "degrees() ko number chahiye."); return NULL_VAL; }
    return NUMBER_VAL(AS_NUMBER(args[0]) * 180.0 / 3.14159265358979323846);
}

/* radians(degrees) - Convert degrees to radians */
static Value nativeRadians(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "radians() ko number chahiye."); return NULL_VAL; }
    return NUMBER_VAL(AS_NUMBER(args[0]) * 3.14159265358979323846 / 180.0);
}

/* ============================================================================
 *  TYPE CHECKING FUNCTIONS
 * ============================================================================ */

/* kya_sankhya(value) - Is it a number? */
static Value nativeKyaSankhya(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    return BOOL_VAL(IS_NUMBER(args[0]));
}

/* kya_shabd(value) - Is it a string? */
static Value nativeKyaShabd(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    return BOOL_VAL(IS_STRING(args[0]));
}

/* kya_suchi(value) - Is it a list? */
static Value nativeKyaSuchi(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    return BOOL_VAL(IS_LIST(args[0]));
}

/* kya_kaam(value) - Is it a function? */
static Value nativeKyaKaam(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    if (!IS_OBJ(args[0])) return BOOL_VAL(false);
    ObjType type = OBJ_TYPE(args[0]);
    return BOOL_VAL(type == OBJ_FUNCTION || type == OBJ_CLOSURE || type == OBJ_NATIVE);
}

/* kya_bool(value) - Is it a boolean? */
static Value nativeKyaBool(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    return BOOL_VAL(IS_BOOL(args[0]));
}

/* kya_khali(value) - Is it null? */
static Value nativeKyaKhali(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    return BOOL_VAL(IS_NULL(args[0]));
}

/* kya_purn(value) - Is it an integer? */
static Value nativeKyaPurn(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    if (!IS_NUMBER(args[0])) return BOOL_VAL(false);
    double val = AS_NUMBER(args[0]);
    return BOOL_VAL(val == (double)(long long)val);
}

/* ============================================================================
 *  ADVANCED LIST FUNCTIONS
 * ============================================================================ */

/* katao(list, start, end) - Slice a list */
static Value nativeKatao(VM* vm, int argCount, Value* args) {
    if (!IS_LIST(args[0])) {
        runtimeError(vm, "katao() ko list chahiye.");
        return NULL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    int start = (argCount >= 2 && IS_NUMBER(args[1])) ? (int)AS_NUMBER(args[1]) : 0;
    int end = (argCount >= 3 && IS_NUMBER(args[2])) ? (int)AS_NUMBER(args[2]) : list->items.count;
    if (start < 0) start = list->items.count + start;
    if (end < 0) end = list->items.count + end;
    if (start < 0) start = 0;
    if (end > list->items.count) end = list->items.count;

    ObjList* result = newList(vm);
    push(vm, OBJ_VAL(result));
    for (int i = start; i < end; i++) {
        listAppend(vm, result, list->items.values[i]);
    }
    pop(vm);
    return OBJ_VAL(result);
}

/* pahla(list) - Get first element */
static Value nativePahla(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) {
        runtimeError(vm, "pahla() ko list chahiye.");
        return NULL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->items.count == 0) return NULL_VAL;
    return list->items.values[0];
}

/* aakhri(list) - Get last element */
static Value nativeAakhri(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) {
        runtimeError(vm, "aakhri() ko list chahiye.");
        return NULL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->items.count == 0) return NULL_VAL;
    return list->items.values[list->items.count - 1];
}

/* milao(list1, list2) - Concatenate two lists */
static Value nativeMilao(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0]) || !IS_LIST(args[1])) {
        runtimeError(vm, "milao() ko do lists chahiye.");
        return NULL_VAL;
    }
    ObjList* a = AS_LIST(args[0]);
    ObjList* b = AS_LIST(args[1]);
    ObjList* result = newList(vm);
    push(vm, OBJ_VAL(result));
    for (int i = 0; i < a->items.count; i++) listAppend(vm, result, a->items.values[i]);
    for (int i = 0; i < b->items.count; i++) listAppend(vm, result, b->items.values[i]);
    pop(vm);
    return OBJ_VAL(result);
}

/* index_of(list, value) - Find index of value in list, -1 if not found */
static Value nativeIndexOf(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) {
        runtimeError(vm, "index_of() ko list chahiye.");
        return NULL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    for (int i = 0; i < list->items.count; i++) {
        if (valuesEqual(list->items.values[i], args[1])) return NUMBER_VAL((double)i);
    }
    return NUMBER_VAL(-1);
}

/* anokha(list) - Return list with unique elements */
static Value nativeAnokha(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) {
        runtimeError(vm, "anokha() ko list chahiye.");
        return NULL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    ObjList* result = newList(vm);
    push(vm, OBJ_VAL(result));
    for (int i = 0; i < list->items.count; i++) {
        bool found = false;
        for (int j = 0; j < result->items.count; j++) {
            if (valuesEqual(result->items.values[j], list->items.values[i])) {
                found = true; break;
            }
        }
        if (!found) listAppend(vm, result, list->items.values[i]);
    }
    pop(vm);
    return OBJ_VAL(result);
}

/* daalo(list, index, value) - Insert at index */
static Value nativeDaalo(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "daalo() ko list, index, aur value chahiye.");
        return NULL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    int index = (int)AS_NUMBER(args[1]);
    if (index < 0) index = 0;
    if (index > list->items.count) index = list->items.count;
    /* Make room */
    listAppend(vm, list, NULL_VAL);
    for (int i = list->items.count - 1; i > index; i--) {
        list->items.values[i] = list->items.values[i - 1];
    }
    list->items.values[index] = args[2];
    return args[0];
}

/* jod(list) - Sum all numbers in list */
static Value nativeJod(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) {
        runtimeError(vm, "jod() ko list chahiye.");
        return NULL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    double sum = 0;
    for (int i = 0; i < list->items.count; i++) {
        if (IS_NUMBER(list->items.values[i])) sum += AS_NUMBER(list->items.values[i]);
    }
    return NUMBER_VAL(sum);
}

/* sabse_bada(list) - Maximum value in list */
static Value nativeSabseBada(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) {
        runtimeError(vm, "sabse_bada() ko list chahiye.");
        return NULL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->items.count == 0) return NULL_VAL;
    Value best = list->items.values[0];
    for (int i = 1; i < list->items.count; i++) {
        if (IS_NUMBER(best) && IS_NUMBER(list->items.values[i])) {
            if (AS_NUMBER(list->items.values[i]) > AS_NUMBER(best)) best = list->items.values[i];
        }
    }
    return best;
}

/* sabse_chhota(list) - Minimum value in list */
static Value nativeSabseChhota(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) {
        runtimeError(vm, "sabse_chhota() ko list chahiye.");
        return NULL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->items.count == 0) return NULL_VAL;
    Value best = list->items.values[0];
    for (int i = 1; i < list->items.count; i++) {
        if (IS_NUMBER(best) && IS_NUMBER(list->items.values[i])) {
            if (AS_NUMBER(list->items.values[i]) < AS_NUMBER(best)) best = list->items.values[i];
        }
    }
    return best;
}

/* ausat(list) - Average of numbers in list */
static Value nativeAusat(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) {
        runtimeError(vm, "ausat() ko list chahiye.");
        return NULL_VAL;
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->items.count == 0) return NUMBER_VAL(0);
    double sum = 0; int count = 0;
    for (int i = 0; i < list->items.count; i++) {
        if (IS_NUMBER(list->items.values[i])) { sum += AS_NUMBER(list->items.values[i]); count++; }
    }
    return NUMBER_VAL(count > 0 ? sum / count : 0);
}

/* ============================================================================
 *  FILE I/O FUNCTIONS
 * ============================================================================ */

/* padho_file(path) - Read entire file as string */
static Value nativePadhoFile(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0])) {
        runtimeError(vm, "padho_file() ko file path (string) chahiye.");
        return NULL_VAL;
    }
    const char* path = AS_CSTRING(args[0]);
    FILE* file = fopen(path, "rb");
    if (!file) {
        runtimeError(vm, "File '%s' padh nahi sakti: %s", path, strerror(errno));
        return NULL_VAL;
    }
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    rewind(file);
    char* content = ALLOCATE(char, (int)fsize + 1);
    size_t bytesRead = fread(content, 1, fsize, file);
    content[bytesRead] = '\0';
    fclose(file);
    return OBJ_VAL(takeString(vm, content, (int)bytesRead));
}

/* likho_file(path, content) - Write string to file */
static Value nativeLikhoFile(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0]) || !IS_STRING(args[1])) {
        runtimeError(vm, "likho_file() ko path aur content (strings) chahiye.");
        return NULL_VAL;
    }
    const char* path = AS_CSTRING(args[0]);
    ObjString* content = AS_STRING(args[1]);
    FILE* file = fopen(path, "wb");
    if (!file) {
        runtimeError(vm, "File '%s' likh nahi sakti: %s", path, strerror(errno));
        return NULL_VAL;
    }
    fwrite(content->chars, 1, content->length, file);
    fclose(file);
    return BOOL_VAL(true);
}

/* joodo_file(path, content) - Append string to file */
static Value nativeJoodFile(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0]) || !IS_STRING(args[1])) {
        runtimeError(vm, "joodo_file() ko path aur content (strings) chahiye.");
        return NULL_VAL;
    }
    const char* path = AS_CSTRING(args[0]);
    ObjString* content = AS_STRING(args[1]);
    FILE* file = fopen(path, "ab");
    if (!file) {
        runtimeError(vm, "File '%s' append nahi kar sakti: %s", path, strerror(errno));
        return NULL_VAL;
    }
    fwrite(content->chars, 1, content->length, file);
    fclose(file);
    return BOOL_VAL(true);
}

/* file_hai(path) - Check if file exists */
static Value nativeFileHai(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0])) {
        runtimeError(vm, "file_hai() ko path chahiye.");
        return NULL_VAL;
    }
    FILE* file = fopen(AS_CSTRING(args[0]), "r");
    if (file) { fclose(file); return BOOL_VAL(true); }
    return BOOL_VAL(false);
}

/* ============================================================================
 *  SYSTEM / UTILITY FUNCTIONS
 * ============================================================================ */

/* ghadi() - High resolution clock in seconds (for benchmarking) */
static Value nativeGhadi(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount; (void)args;
#ifdef _WIN32
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return NUMBER_VAL((double)counter.QuadPart / (double)freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return NUMBER_VAL((double)ts.tv_sec + (double)ts.tv_nsec / 1e9);
#endif
}

/* platform() - Return OS name */
static Value nativePlatform(VM* vm, int argCount, Value* args) {
    (void)argCount; (void)args;
#ifdef _WIN32
    return OBJ_VAL(copyString(vm, "windows", 7));
#elif defined(__APPLE__)
    return OBJ_VAL(copyString(vm, "macos", 5));
#elif defined(__linux__)
    return OBJ_VAL(copyString(vm, "linux", 5));
#else
    return OBJ_VAL(copyString(vm, "unknown", 7));
#endif
}

/* env_var(name) - Get environment variable */
static Value nativeEnvVar(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0])) {
        runtimeError(vm, "env_var() ko variable name chahiye.");
        return NULL_VAL;
    }
    const char* val = getenv(AS_CSTRING(args[0]));
    if (!val) return NULL_VAL;
    return OBJ_VAL(copyString(vm, val, (int)strlen(val)));
}

/* taareekh() - Get current date as "YYYY-MM-DD" string */
static Value nativeTaareekh(VM* vm, int argCount, Value* args) {
    (void)argCount; (void)args;
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    return OBJ_VAL(copyString(vm, buf, len));
}

/* waqt() - Get current time as "HH:MM:SS" string */
static Value nativeWaqt(VM* vm, int argCount, Value* args) {
    (void)argCount; (void)args;
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
        tm->tm_hour, tm->tm_min, tm->tm_sec);
    return OBJ_VAL(copyString(vm, buf, len));
}

/* timestamp() - Get Unix timestamp */
static Value nativeTimestamp(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount; (void)args;
    return NUMBER_VAL((double)time(NULL));
}

/* hash_val(value) - Get hash of a value */
static Value nativeHashVal(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (IS_STRING(args[0])) {
        return NUMBER_VAL((double)AS_STRING(args[0])->hash);
    }
    if (IS_NUMBER(args[0])) {
        uint32_t h = 2166136261u;
        double n = AS_NUMBER(args[0]);
        unsigned char* bytes = (unsigned char*)&n;
        for (size_t i = 0; i < sizeof(double); i++) {
            h ^= bytes[i]; h *= 16777619;
        }
        return NUMBER_VAL((double)h);
    }
    runtimeError(vm, "hash_val() sirf string aur number ke liye kaam karta hai.");
    return NULL_VAL;
}

/* ============================================================================
 *  MAP/DICTIONARY FUNCTIONS
 * ============================================================================ */

/* shabdkosh() – Create empty map, or from list of [key, value] pairs */
static Value nativeShabdkosh(VM* vm, int argCount, Value* args) {
    ObjMap* map = newMap(vm);
    if (argCount == 0) return OBJ_VAL(map);
    if (argCount == 1 && IS_LIST(args[0])) {
        ObjList* list = AS_LIST(args[0]);
        push(vm, OBJ_VAL(map));
        for (int i = 0; i < list->items.count; i++) {
            if (!IS_LIST(list->items.values[i])) {
                pop(vm);
                runtimeError(vm, "shabdkosh() ke liye har item ek [key, value] list honi chahiye.");
                return NULL_VAL;
            }
            ObjList* pair = AS_LIST(list->items.values[i]);
            if (pair->items.count != 2) {
                pop(vm);
                runtimeError(vm, "shabdkosh() ke liye har item mein exactly 2 elements hone chahiye.");
                return NULL_VAL;
            }
            mapSet(vm, map, pair->items.values[0], pair->items.values[1]);
        }
        pop(vm);
        return OBJ_VAL(map);
    }
    runtimeError(vm, "shabdkosh() ko 0 ya 1 (list of pairs) argument chahiye.");
    return NULL_VAL;
}

/* chabi(map) – Return list of keys */
static Value nativeChabi(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_MAP(args[0])) {
        runtimeError(vm, "chabi() sirf map ke liye hai.");
        return NULL_VAL;
    }
    ObjMap* map = AS_MAP(args[0]);
    ObjList* list = newList(vm);
    push(vm, OBJ_VAL(list));
    for (int i = 0; i < map->capacity; i++) {
        if (map->entries[i].isOccupied)
            listAppend(vm, list, map->entries[i].key);
    }
    pop(vm);
    return OBJ_VAL(list);
}

/* mulya(map) – Return list of values */
static Value nativeMulya(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_MAP(args[0])) {
        runtimeError(vm, "mulya() sirf map ke liye hai.");
        return NULL_VAL;
    }
    ObjMap* map = AS_MAP(args[0]);
    ObjList* list = newList(vm);
    push(vm, OBJ_VAL(list));
    for (int i = 0; i < map->capacity; i++) {
        if (map->entries[i].isOccupied)
            listAppend(vm, list, map->entries[i].value);
    }
    pop(vm);
    return OBJ_VAL(list);
}

/* jodi(map) – Return list of [key, value] pairs */
static Value nativeJodi(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_MAP(args[0])) {
        runtimeError(vm, "jodi() sirf map ke liye hai.");
        return NULL_VAL;
    }
    ObjMap* map = AS_MAP(args[0]);
    ObjList* result = newList(vm);
    push(vm, OBJ_VAL(result));
    for (int i = 0; i < map->capacity; i++) {
        if (!map->entries[i].isOccupied) continue;
        ObjList* pair = newList(vm);
        push(vm, OBJ_VAL(pair));
        listAppend(vm, pair, map->entries[i].key);
        listAppend(vm, pair, map->entries[i].value);
        pop(vm);
        listAppend(vm, result, OBJ_VAL(pair));
    }
    pop(vm);
    return OBJ_VAL(result);
}

/* map_hatao(map, key) – Delete key from map */
static Value nativeMapHatao(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_MAP(args[0])) {
        runtimeError(vm, "map_hatao() sirf map ke liye hai.");
        return NULL_VAL;
    }
    bool deleted = mapDelete(AS_MAP(args[0]), args[1]);
    return BOOL_VAL(deleted);
}

/* map_hai(map, key) – Check if key exists */
static Value nativeMapHai(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_MAP(args[0])) {
        runtimeError(vm, "map_hai() sirf map ke liye hai.");
        return NULL_VAL;
    }
    return BOOL_VAL(mapHasKey(AS_MAP(args[0]), args[1]));
}

/* map_milao(map1, map2) – Merge two maps */
static Value nativeMapMilao(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_MAP(args[0]) || !IS_MAP(args[1])) {
        runtimeError(vm, "map_milao() ko dono arguments map hone chahiye.");
        return NULL_VAL;
    }
    ObjMap* a = AS_MAP(args[0]);
    ObjMap* b = AS_MAP(args[1]);
    ObjMap* result = newMap(vm);
    push(vm, OBJ_VAL(result));
    for (int i = 0; i < a->capacity; i++)
        if (a->entries[i].isOccupied) mapSet(vm, result, a->entries[i].key, a->entries[i].value);
    for (int i = 0; i < b->capacity; i++)
        if (b->entries[i].isOccupied) mapSet(vm, result, b->entries[i].key, b->entries[i].value);
    pop(vm);
    return OBJ_VAL(result);
}

/* map_lambai(map) – Return number of entries */
static Value nativeMapLambai(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_MAP(args[0])) {
        runtimeError(vm, "map_lambai() sirf map ke liye hai.");
        return NULL_VAL;
    }
    return NUMBER_VAL((double)AS_MAP(args[0])->count);
}

/* kya_map(v) – Is value a map? */
static Value nativeKyaMap(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    return BOOL_VAL(IS_MAP(args[0]));
}

/* map_get(map, key, default) – Get with default value */
static Value nativeMapGet(VM* vm, int argCount, Value* args) {
    if (!IS_MAP(args[0])) {
        runtimeError(vm, "map_get() sirf map ke liye hai.");
        return NULL_VAL;
    }
    Value result;
    if (mapGet(AS_MAP(args[0]), args[1], &result)) return result;
    return (argCount >= 3) ? args[2] : NULL_VAL;
}

/* ============================================================================
 *  HIGHER-ORDER FUNCTIONS
 * ============================================================================ */

/* naksha(list, fn) – map() – applies native function to each element */
static Value nativeNaksha(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) { runtimeError(vm, "naksha() ka pehla argument list hona chahiye."); return NULL_VAL; }
    if (!IS_NATIVE(args[1])) { runtimeError(vm, "naksha() ka doosra argument built-in function hona chahiye."); return NULL_VAL; }
    ObjList* src = AS_LIST(args[0]);
    ObjNative* fn = AS_NATIVE(args[1]);
    ObjList* result = newList(vm);
    push(vm, OBJ_VAL(result));
    for (int i = 0; i < src->items.count; i++) {
        Value mapped = fn->function(vm, 1, &src->items.values[i]);
        if (vm->hadError) { pop(vm); return NULL_VAL; }
        listAppend(vm, result, mapped);
    }
    pop(vm);
    return OBJ_VAL(result);
}

/* chhaano(list, fn) – filter() */
static Value nativeChhaano(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) { runtimeError(vm, "chhaano() ka pehla argument list hona chahiye."); return NULL_VAL; }
    if (!IS_NATIVE(args[1])) { runtimeError(vm, "chhaano() ka doosra argument built-in function hona chahiye."); return NULL_VAL; }
    ObjList* src = AS_LIST(args[0]);
    ObjList* result = newList(vm);
    push(vm, OBJ_VAL(result));
    ObjNative* fn = AS_NATIVE(args[1]);
    for (int i = 0; i < src->items.count; i++) {
        Value testResult = fn->function(vm, 1, &src->items.values[i]);
        if (vm->hadError) { pop(vm); return NULL_VAL; }
        if (isTruthy(testResult)) {
            listAppend(vm, result, src->items.values[i]);
        }
    }
    pop(vm);
    return OBJ_VAL(result);
}

/* ikkatha(list, fn, initial) – reduce() */
static Value nativeIkkatha(VM* vm, int argCount, Value* args) {
    if (!IS_LIST(args[0])) { runtimeError(vm, "ikkatha() ka pehla argument list hona chahiye."); return NULL_VAL; }
    if (!IS_NATIVE(args[1])) { runtimeError(vm, "ikkatha() ka doosra argument built-in function hona chahiye."); return NULL_VAL; }
    ObjList* src = AS_LIST(args[0]);
    if (src->items.count == 0) {
        return (argCount >= 3) ? args[2] : NULL_VAL;
    }
    ObjNative* fn = AS_NATIVE(args[1]);
    Value accumulator = (argCount >= 3) ? args[2] : src->items.values[0];
    int start = (argCount >= 3) ? 0 : 1;
    for (int i = start; i < src->items.count; i++) {
        Value fnArgs[2] = { accumulator, src->items.values[i] };
        accumulator = fn->function(vm, 2, fnArgs);
        if (vm->hadError) return NULL_VAL;
    }
    return accumulator;
}

/* sab(list) – all() - check if all values are truthy */
static Value nativeSab(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    if (!IS_LIST(args[0])) { runtimeError(vm, "sab() sirf list ke liye hai."); return NULL_VAL; }
    ObjList* list = AS_LIST(args[0]);
    for (int i = 0; i < list->items.count; i++)
        if (!isTruthy(list->items.values[i])) return BOOL_VAL(false);
    return BOOL_VAL(true);
}

/* koi(list) – any() */
static Value nativeKoi(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    if (!IS_LIST(args[0])) { runtimeError(vm, "koi() sirf list ke liye hai."); return NULL_VAL; }
    ObjList* list = AS_LIST(args[0]);
    for (int i = 0; i < list->items.count; i++)
        if (isTruthy(list->items.values[i])) return BOOL_VAL(true);
    return BOOL_VAL(false);
}

/* jodi_banao(l1, l2) – zip() */
static Value nativeJodiBanao(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0]) || !IS_LIST(args[1])) {
        runtimeError(vm, "jodi_banao() ko dono arguments list hone chahiye.");
        return NULL_VAL;
    }
    ObjList* a = AS_LIST(args[0]);
    ObjList* b = AS_LIST(args[1]);
    int len = a->items.count < b->items.count ? a->items.count : b->items.count;
    ObjList* result = newList(vm);
    push(vm, OBJ_VAL(result));
    for (int i = 0; i < len; i++) {
        ObjList* pair = newList(vm);
        push(vm, OBJ_VAL(pair));
        listAppend(vm, pair, a->items.values[i]);
        listAppend(vm, pair, b->items.values[i]);
        pop(vm);
        listAppend(vm, result, OBJ_VAL(pair));
    }
    pop(vm);
    return OBJ_VAL(result);
}

/* ginati_banao(list) – enumerate() → list of [index, value] */
static Value nativeGinatiBanao(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) {
        runtimeError(vm, "ginati_banao() sirf list ke liye hai.");
        return NULL_VAL;
    }
    ObjList* src = AS_LIST(args[0]);
    ObjList* result = newList(vm);
    push(vm, OBJ_VAL(result));
    for (int i = 0; i < src->items.count; i++) {
        ObjList* pair = newList(vm);
        push(vm, OBJ_VAL(pair));
        listAppend(vm, pair, NUMBER_VAL(i));
        listAppend(vm, pair, src->items.values[i]);
        pop(vm);
        listAppend(vm, result, OBJ_VAL(pair));
    }
    pop(vm);
    return OBJ_VAL(result);
}

/* ============================================================================
 *  ADDITIONAL STRING FUNCTIONS
 * ============================================================================ */

/* title_case(str) – capitalize each word */
static Value nativeTitleCase(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0])) { runtimeError(vm, "title_case() sirf string ke liye hai."); return NULL_VAL; }
    ObjString* str = AS_STRING(args[0]);
    char* buf = ALLOCATE(char, str->length + 1);
    bool newWord = true;
    for (int i = 0; i < str->length; i++) {
        if (str->chars[i] == ' ' || str->chars[i] == '\t' || str->chars[i] == '\n') {
            buf[i] = str->chars[i];
            newWord = true;
        } else if (newWord) {
            buf[i] = toupper((unsigned char)str->chars[i]);
            newWord = false;
        } else {
            buf[i] = tolower((unsigned char)str->chars[i]);
        }
    }
    buf[str->length] = '\0';
    return OBJ_VAL(takeString(vm, buf, str->length));
}

/* capitalize(str) – capitalize first char only */
static Value nativeCapitalize(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0])) { runtimeError(vm, "capitalize() sirf string ke liye hai."); return NULL_VAL; }
    ObjString* str = AS_STRING(args[0]);
    if (str->length == 0) return args[0];
    char* buf = ALLOCATE(char, str->length + 1);
    memcpy(buf, str->chars, str->length);
    buf[0] = toupper((unsigned char)buf[0]);
    for (int i = 1; i < str->length; i++) buf[i] = tolower((unsigned char)buf[i]);
    buf[str->length] = '\0';
    return OBJ_VAL(takeString(vm, buf, str->length));
}

/* swapcase(str) – swap upper/lower */
static Value nativeSwapcase(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_STRING(args[0])) { runtimeError(vm, "swapcase() sirf string ke liye hai."); return NULL_VAL; }
    ObjString* str = AS_STRING(args[0]);
    char* buf = ALLOCATE(char, str->length + 1);
    for (int i = 0; i < str->length; i++) {
        unsigned char c = str->chars[i];
        buf[i] = isupper(c) ? tolower(c) : toupper(c);
    }
    buf[str->length] = '\0';
    return OBJ_VAL(takeString(vm, buf, str->length));
}

/* center(str, width, fillchar) */
static Value nativeCenter(VM* vm, int argCount, Value* args) {
    if (!IS_STRING(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "center() ko string aur number chahiye.");
        return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    int width = (int)AS_NUMBER(args[1]);
    char fill = ' ';
    if (argCount >= 3 && IS_STRING(args[2]) && AS_STRING(args[2])->length > 0)
        fill = AS_STRING(args[2])->chars[0];
    if (width <= str->length) return args[0];
    int total = width - str->length;
    int left = total / 2;
    int right = total - left;
    char* buf = ALLOCATE(char, width + 1);
    memset(buf, fill, left);
    memcpy(buf + left, str->chars, str->length);
    memset(buf + left + str->length, fill, right);
    buf[width] = '\0';
    return OBJ_VAL(takeString(vm, buf, width));
}

/* kya_ank(str) – is_digit */
static Value nativeKyaAnk(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    if (!IS_STRING(args[0])) return BOOL_VAL(false);
    ObjString* str = AS_STRING(args[0]);
    if (str->length == 0) return BOOL_VAL(false);
    for (int i = 0; i < str->length; i++)
        if (!isdigit((unsigned char)str->chars[i])) return BOOL_VAL(false);
    return BOOL_VAL(true);
}

/* kya_akshar(str) – is_alpha */
static Value nativeKyaAkshar(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    if (!IS_STRING(args[0])) return BOOL_VAL(false);
    ObjString* str = AS_STRING(args[0]);
    if (str->length == 0) return BOOL_VAL(false);
    for (int i = 0; i < str->length; i++)
        if (!isalpha((unsigned char)str->chars[i])) return BOOL_VAL(false);
    return BOOL_VAL(true);
}

/* kya_alnum(str) – is_alphanumeric */
static Value nativeKyaAlnum(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    if (!IS_STRING(args[0])) return BOOL_VAL(false);
    ObjString* str = AS_STRING(args[0]);
    if (str->length == 0) return BOOL_VAL(false);
    for (int i = 0; i < str->length; i++)
        if (!isalnum((unsigned char)str->chars[i])) return BOOL_VAL(false);
    return BOOL_VAL(true);
}

/* kya_space(str) – is_space */
static Value nativeKyaSpace(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    if (!IS_STRING(args[0])) return BOOL_VAL(false);
    ObjString* str = AS_STRING(args[0]);
    if (str->length == 0) return BOOL_VAL(false);
    for (int i = 0; i < str->length; i++)
        if (!isspace((unsigned char)str->chars[i])) return BOOL_VAL(false);
    return BOOL_VAL(true);
}

/* pad_left(str, width, fillchar) */
static Value nativePadLeft(VM* vm, int argCount, Value* args) {
    if (!IS_STRING(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "pad_left() ko string aur number chahiye."); return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    int width = (int)AS_NUMBER(args[1]);
    char fill = ' ';
    if (argCount >= 3 && IS_STRING(args[2]) && AS_STRING(args[2])->length > 0)
        fill = AS_STRING(args[2])->chars[0];
    if (width <= str->length) return args[0];
    int padLen = width - str->length;
    char* buf = ALLOCATE(char, width + 1);
    memset(buf, fill, padLen);
    memcpy(buf + padLen, str->chars, str->length);
    buf[width] = '\0';
    return OBJ_VAL(takeString(vm, buf, width));
}

/* pad_right(str, width, fillchar) */
static Value nativePadRight(VM* vm, int argCount, Value* args) {
    if (!IS_STRING(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "pad_right() ko string aur number chahiye."); return NULL_VAL;
    }
    ObjString* str = AS_STRING(args[0]);
    int width = (int)AS_NUMBER(args[1]);
    char fill = ' ';
    if (argCount >= 3 && IS_STRING(args[2]) && AS_STRING(args[2])->length > 0)
        fill = AS_STRING(args[2])->chars[0];
    if (width <= str->length) return args[0];
    int padLen = width - str->length;
    char* buf = ALLOCATE(char, width + 1);
    memcpy(buf, str->chars, str->length);
    memset(buf + str->length, fill, padLen);
    buf[width] = '\0';
    return OBJ_VAL(takeString(vm, buf, width));
}

/* ============================================================================
 *  ADDITIONAL MATH FUNCTIONS
 * ============================================================================ */

/* factorial(n) */
static Value nativeFactorial(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "factorial() ko number chahiye."); return NULL_VAL; }
    int n = (int)AS_NUMBER(args[0]);
    if (n < 0) { runtimeError(vm, "factorial() negative number ke liye nahi hai."); return NULL_VAL; }
    double result = 1;
    for (int i = 2; i <= n; i++) result *= i;
    return NUMBER_VAL(result);
}

/* kya_prime(n) – is_prime */
static Value nativeKyaPrime(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "kya_prime() ko number chahiye."); return NULL_VAL; }
    long long n = (long long)AS_NUMBER(args[0]);
    if (n < 2) return BOOL_VAL(false);
    if (n < 4) return BOOL_VAL(true);
    if (n % 2 == 0 || n % 3 == 0) return BOOL_VAL(false);
    for (long long i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return BOOL_VAL(false);
    }
    return BOOL_VAL(true);
}

/* random_choice(list) */
static Value nativeRandomChoice(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) { runtimeError(vm, "random_choice() sirf list ke liye hai."); return NULL_VAL; }
    ObjList* list = AS_LIST(args[0]);
    if (list->items.count == 0) { runtimeError(vm, "random_choice() khali list pe nahi chalta."); return NULL_VAL; }
    int idx = rand() % list->items.count;
    return list->items.values[idx];
}

/* random_shuffle(list) – returns new shuffled list */
static Value nativeRandomShuffle(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) { runtimeError(vm, "random_shuffle() sirf list ke liye hai."); return NULL_VAL; }
    ObjList* src = AS_LIST(args[0]);
    ObjList* result = newList(vm);
    push(vm, OBJ_VAL(result));
    for (int i = 0; i < src->items.count; i++)
        listAppend(vm, result, src->items.values[i]);
    /* Fisher-Yates shuffle */
    for (int i = result->items.count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Value temp = result->items.values[i];
        result->items.values[i] = result->items.values[j];
        result->items.values[j] = temp;
    }
    pop(vm);
    return OBJ_VAL(result);
}

/* random_int(min, max) */
static Value nativeRandomInt(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "random_int() ko dono numbers chahiye."); return NULL_VAL;
    }
    int mn = (int)AS_NUMBER(args[0]);
    int mx = (int)AS_NUMBER(args[1]);
    if (mn > mx) { int t = mn; mn = mx; mx = t; }
    return NUMBER_VAL((double)(mn + rand() % (mx - mn + 1)));
}

/* ============================================================================
 *  ADDITIONAL LIST FUNCTIONS
 * ============================================================================ */

/* flatten(list) – flatten nested lists one level */
static Value nativeFlatten(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) { runtimeError(vm, "flatten() sirf list ke liye hai."); return NULL_VAL; }
    ObjList* src = AS_LIST(args[0]);
    ObjList* result = newList(vm);
    push(vm, OBJ_VAL(result));
    for (int i = 0; i < src->items.count; i++) {
        if (IS_LIST(src->items.values[i])) {
            ObjList* inner = AS_LIST(src->items.values[i]);
            for (int j = 0; j < inner->items.count; j++)
                listAppend(vm, result, inner->items.values[j]);
        } else {
            listAppend(vm, result, src->items.values[i]);
        }
    }
    pop(vm);
    return OBJ_VAL(result);
}

/* tukda(list, size) – chunk list into sublists of given size */
static Value nativeTukda(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "tukda() ko list aur number chahiye."); return NULL_VAL;
    }
    ObjList* src = AS_LIST(args[0]);
    int size = (int)AS_NUMBER(args[1]);
    if (size <= 0) { runtimeError(vm, "tukda() ka size 0 se bada hona chahiye."); return NULL_VAL; }
    ObjList* result = newList(vm);
    push(vm, OBJ_VAL(result));
    for (int i = 0; i < src->items.count; i += size) {
        ObjList* chunk = newList(vm);
        push(vm, OBJ_VAL(chunk));
        for (int j = i; j < i + size && j < src->items.count; j++)
            listAppend(vm, chunk, src->items.values[j]);
        pop(vm);
        listAppend(vm, result, OBJ_VAL(chunk));
    }
    pop(vm);
    return OBJ_VAL(result);
}

/* ghuma(list, n) – rotate list by n positions */
static Value nativeGhuma(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "ghuma() ko list aur number chahiye."); return NULL_VAL;
    }
    ObjList* src = AS_LIST(args[0]);
    int n = (int)AS_NUMBER(args[1]);
    int count = src->items.count;
    if (count == 0) return args[0];
    n = ((n % count) + count) % count;
    ObjList* result = newList(vm);
    push(vm, OBJ_VAL(result));
    for (int i = n; i < count; i++)
        listAppend(vm, result, src->items.values[i]);
    for (int i = 0; i < n; i++)
        listAppend(vm, result, src->items.values[i]);
    pop(vm);
    return OBJ_VAL(result);
}

/* copy_suchi(list) – shallow copy of a list */
static Value nativeCopySuchi(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_LIST(args[0])) { runtimeError(vm, "copy_suchi() sirf list ke liye hai."); return NULL_VAL; }
    ObjList* src = AS_LIST(args[0]);
    ObjList* result = newList(vm);
    push(vm, OBJ_VAL(result));
    for (int i = 0; i < src->items.count; i++)
        listAppend(vm, result, src->items.values[i]);
    pop(vm);
    return OBJ_VAL(result);
}

/* khali_karo(list) – clear list in place */
static Value nativeKhaliKaro(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    if (!IS_LIST(args[0])) { runtimeError(vm, "khali_karo() sirf list ke liye hai."); return NULL_VAL; }
    ObjList* list = AS_LIST(args[0]);
    list->items.count = 0;
    return NULL_VAL;
}

/* ============================================================================
 *  CONVERSION FUNCTIONS
 * ============================================================================ */

/* hex_shabd(n) – number to hex string */
static Value nativeHexShabd(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "hex_shabd() ko number chahiye."); return NULL_VAL; }
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%llx", (long long)AS_NUMBER(args[0]));
    return OBJ_VAL(copyString(vm, buf, (int)strlen(buf)));
}

/* oct_shabd(n) – number to octal string */
static Value nativeOctShabd(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "oct_shabd() ko number chahiye."); return NULL_VAL; }
    char buf[32];
    snprintf(buf, sizeof(buf), "0o%llo", (long long)AS_NUMBER(args[0]));
    return OBJ_VAL(copyString(vm, buf, (int)strlen(buf)));
}

/* bin_shabd(n) – number to binary string */
static Value nativeBinShabd(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "bin_shabd() ko number chahiye."); return NULL_VAL; }
    long long n = (long long)AS_NUMBER(args[0]);
    char buf[68];
    int pos = 0;
    buf[pos++] = '0'; buf[pos++] = 'b';
    if (n == 0) { buf[pos++] = '0'; buf[pos] = '\0'; }
    else {
        unsigned long long un = (n < 0) ? (unsigned long long)(-n) : (unsigned long long)n;
        char tmp[65]; int tpos = 0;
        while (un > 0) { tmp[tpos++] = (un & 1) ? '1' : '0'; un >>= 1; }
        if (n < 0) { buf[0] = '-'; buf[1] = '0'; buf[2] = 'b'; pos = 3; }
        for (int i = tpos - 1; i >= 0; i--) buf[pos++] = tmp[i];
        buf[pos] = '\0';
    }
    return OBJ_VAL(copyString(vm, buf, pos));
}

/* ============================================================================
 *  DATE/TIME FUNCTIONS
 * ============================================================================ */

/* din() – day of month */
static Value nativeDin(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount; (void)args;
    time_t t = time(NULL); struct tm* tm = localtime(&t);
    return NUMBER_VAL((double)tm->tm_mday);
}

/* mahina() – month (1-12) */
static Value nativeMahina(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount; (void)args;
    time_t t = time(NULL); struct tm* tm = localtime(&t);
    return NUMBER_VAL((double)(tm->tm_mon + 1));
}

/* saal() – year */
static Value nativeSaal(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount; (void)args;
    time_t t = time(NULL); struct tm* tm = localtime(&t);
    return NUMBER_VAL((double)(tm->tm_year + 1900));
}

/* ghanta() – hour */
static Value nativeGhanta(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount; (void)args;
    time_t t = time(NULL); struct tm* tm = localtime(&t);
    return NUMBER_VAL((double)tm->tm_hour);
}

/* minute() – minute */
static Value nativeMinute(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount; (void)args;
    time_t t = time(NULL); struct tm* tm = localtime(&t);
    return NUMBER_VAL((double)tm->tm_min);
}

/* second() – second */
static Value nativeSecond(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount; (void)args;
    time_t t = time(NULL); struct tm* tm = localtime(&t);
    return NUMBER_VAL((double)tm->tm_sec);
}

/* hafta_din() – day of week (0=Sun, 6=Sat) */
static Value nativeHaftaDin(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount; (void)args;
    time_t t = time(NULL); struct tm* tm = localtime(&t);
    return NUMBER_VAL((double)tm->tm_wday);
}

/* ============================================================================
 *  ADDITIONAL UTILITY FUNCTIONS
 * ============================================================================ */

/* typeof_val(v) – stronger type check returning string */
static Value nativeTypeofVal(VM* vm, int argCount, Value* args) {
    (void)argCount;
    const char* name;
    if (IS_MAP(args[0])) name = "shabdkosh";
    else name = valueTypeName(args[0]);
    return OBJ_VAL(copyString(vm, name, (int)strlen(name)));
}

/* bool_val(v) – convert to boolean */
static Value nativeBoolVal(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    return BOOL_VAL(isTruthy(args[0]));
}

/* print_type(v) – print type and value (debug helper) */
static Value nativePrintType(VM* vm, int argCount, Value* args) {
    (void)argCount;
    const char* name;
    if (IS_MAP(args[0])) name = "shabdkosh";
    else name = valueTypeName(args[0]);
    printf("[%s] ", name);
    printValue(args[0]);
    printf("\n");
    return NULL_VAL;
}

/* ============================================================================
 *  ADDITIONAL MATH
 * ============================================================================ */

/* fib(n) – nth fibonacci number */
static Value nativeFib(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "fib() ko number chahiye."); return NULL_VAL; }
    int n = (int)AS_NUMBER(args[0]);
    if (n < 0) { runtimeError(vm, "fib() negative number ke liye nahi hai."); return NULL_VAL; }
    if (n <= 1) return NUMBER_VAL((double)n);
    double a = 0, b = 1;
    for (int i = 2; i <= n; i++) { double c = a + b; a = b; b = c; }
    return NUMBER_VAL(b);
}

/* hypot_val(a, b) – hypotenuse */
static Value nativeHypot(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        runtimeError(vm, "hypot_val() ko dono numbers chahiye."); return NULL_VAL;
    }
    return NUMBER_VAL(hypot(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
}

/* log2_val(n) */
static Value nativeLog2(VM* vm, int argCount, Value* args) {
    (void)argCount;
    if (!IS_NUMBER(args[0])) { runtimeError(vm, "log2_val() ko number chahiye."); return NULL_VAL; }
    return NUMBER_VAL(log2(AS_NUMBER(args[0])));
}

/* is_nan(n) */
static Value nativeIsNan(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    if (!IS_NUMBER(args[0])) return BOOL_VAL(false);
    return BOOL_VAL(isnan(AS_NUMBER(args[0])));
}

/* is_inf(n) */
static Value nativeIsInf(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount;
    if (!IS_NUMBER(args[0])) return BOOL_VAL(false);
    return BOOL_VAL(isinf(AS_NUMBER(args[0])));
}

/* INF constant */
static Value nativeINF(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount; (void)args;
    return NUMBER_VAL(INFINITY);
}

/* NAN constant */
static Value nativeNAN(VM* vm, int argCount, Value* args) {
    (void)vm; (void)argCount; (void)args;
    return NUMBER_VAL(NAN);
}

/* ============================================================================
 *  REGISTRATION — 110+ Built-in Functions
 * ============================================================================ */
void registerNatives(VM* vm) {
    /* ── I/O ── */
    defineNative(vm, "likho_line", nativeLikhoLine, -1);

    /* ── Type Conversion ── */
    defineNative(vm, "prakar",    nativePrakar,    1);
    defineNative(vm, "sankhya",   nativeSankhya,   1);
    defineNative(vm, "dashmlav",  nativeDashmlav,  1);
    defineNative(vm, "shabd",     nativeShabd,     1);
    defineNative(vm, "purn",      nativePurn,      1);
    defineNative(vm, "bool_val",  nativeBoolVal,   1);

    /* ── Type Checking ── */
    defineNative(vm, "kya_sankhya", nativeKyaSankhya, 1);
    defineNative(vm, "kya_shabd",   nativeKyaShabd,   1);
    defineNative(vm, "kya_suchi",   nativeKyaSuchi,   1);
    defineNative(vm, "kya_kaam",    nativeKyaKaam,    1);
    defineNative(vm, "kya_bool",    nativeKyaBool,    1);
    defineNative(vm, "kya_khali",   nativeKyaKhali,   1);
    defineNative(vm, "kya_purn",    nativeKyaPurn,    1);
    defineNative(vm, "kya_map",     nativeKyaMap,     1);
    defineNative(vm, "typeof_val",  nativeTypeofVal,  1);
    defineNative(vm, "print_type",  nativePrintType,  1);

    /* ── String Operations ── */
    defineNative(vm, "lambai",       nativeLambai,       1);
    defineNative(vm, "bade_akshar",  nativeBadeAkshar,   1);
    defineNative(vm, "chhote_akshar", nativeChhoteAkshar, 1);
    defineNative(vm, "kato",         nativeKato,         -1);
    defineNative(vm, "dhundho",      nativeDhundho,      2);
    defineNative(vm, "badlo",        nativeBadlo,        3);
    defineNative(vm, "todo",         nativeTodo,         -1);
    defineNative(vm, "saaf",         nativeSaaf,         1);
    defineNative(vm, "shuru_se",     nativeShuru,        2);
    defineNative(vm, "ant_se",       nativeAnt,          2);
    defineNative(vm, "dohrao",       nativeDohrao,       2);
    defineNative(vm, "akshar",       nativeAkshar,       2);
    defineNative(vm, "ascii_code",   nativeAsciiCode,    1);
    defineNative(vm, "ascii_se",     nativeAsciiSe,      1);
    defineNative(vm, "jodo_shabd",   nativeJodoShabd,    -1);
    defineNative(vm, "format",       nativeFormat,       -1);
    defineNative(vm, "gino",         nativeGino,         2);
    defineNative(vm, "title_case",   nativeTitleCase,    1);
    defineNative(vm, "capitalize",   nativeCapitalize,   1);
    defineNative(vm, "swapcase",     nativeSwapcase,     1);
    defineNative(vm, "center",       nativeCenter,       -1);
    defineNative(vm, "kya_ank",      nativeKyaAnk,       1);
    defineNative(vm, "kya_akshar",   nativeKyaAkshar,    1);
    defineNative(vm, "kya_alnum",    nativeKyaAlnum,     1);
    defineNative(vm, "kya_space",    nativeKyaSpace,     1);
    defineNative(vm, "pad_left",     nativePadLeft,      -1);
    defineNative(vm, "pad_right",    nativePadRight,     -1);

    /* ── List Operations ── */
    defineNative(vm, "joodo",       nativeJoodo,       2);
    defineNative(vm, "nikalo",      nativeNikalo,      -1);
    defineNative(vm, "ulta",        nativeUlta,        1);
    defineNative(vm, "kram",        nativeKram,        1);
    defineNative(vm, "suchi",       nativeSuchi,       -1);
    defineNative(vm, "range_",      nativeRange,       -1);
    defineNative(vm, "shamil",      nativeShamil,      2);
    defineNative(vm, "katao",       nativeKatao,       -1);
    defineNative(vm, "pahla",       nativePahla,       1);
    defineNative(vm, "aakhri",      nativeAakhri,      1);
    defineNative(vm, "milao",       nativeMilao,       2);
    defineNative(vm, "index_of",    nativeIndexOf,     2);
    defineNative(vm, "anokha",      nativeAnokha,      1);
    defineNative(vm, "daalo",       nativeDaalo,       3);
    defineNative(vm, "jod",         nativeJod,         1);
    defineNative(vm, "sabse_bada",  nativeSabseBada,   1);
    defineNative(vm, "sabse_chhota", nativeSabseChhota, 1);
    defineNative(vm, "ausat",       nativeAusat,       1);
    defineNative(vm, "flatten",     nativeFlatten,     1);
    defineNative(vm, "tukda",       nativeTukda,       2);
    defineNative(vm, "ghuma",       nativeGhuma,       2);
    defineNative(vm, "copy_suchi",  nativeCopySuchi,   1);
    defineNative(vm, "khali_karo",  nativeKhaliKaro,   1);

    /* ── Higher-Order Functions ── */
    defineNative(vm, "naksha",       nativeNaksha,      2);
    defineNative(vm, "chhaano",      nativeChhaano,     2);
    defineNative(vm, "ikkatha",      nativeIkkatha,     -1);
    defineNative(vm, "sab",          nativeSab,         1);
    defineNative(vm, "koi",          nativeKoi,         1);
    defineNative(vm, "jodi_banao",   nativeJodiBanao,   2);
    defineNative(vm, "ginati_banao", nativeGinatiBanao, 1);

    /* ── Map/Dictionary ── */
    defineNative(vm, "shabdkosh",   nativeShabdkosh,   -1);
    defineNative(vm, "chabi",       nativeChabi,       1);
    defineNative(vm, "mulya",       nativeMulya,       1);
    defineNative(vm, "jodi",        nativeJodi,        1);
    defineNative(vm, "map_hatao",   nativeMapHatao,    2);
    defineNative(vm, "map_hai",     nativeMapHai,      2);
    defineNative(vm, "map_milao",   nativeMapMilao,    2);
    defineNative(vm, "map_lambai",  nativeMapLambai,   1);
    defineNative(vm, "map_get",     nativeMapGet,      -1);

    /* ── Math ── */
    defineNative(vm, "abs_val",  nativeAbsVal,   1);
    defineNative(vm, "gol",      nativeGol,      1);
    defineNative(vm, "upar",     nativeUpar,     1);
    defineNative(vm, "neeche",   nativeNeeche,   1);
    defineNative(vm, "sqrt_val", nativeSqrtVal,  1);
    defineNative(vm, "bada",     nativeBada,     2);
    defineNative(vm, "chhota",   nativeChhota,   2);
    defineNative(vm, "sin_val",  nativeSin,      1);
    defineNative(vm, "cos_val",  nativeCos,      1);
    defineNative(vm, "tan_val",  nativeTan,      1);
    defineNative(vm, "asin_val", nativeAsin,     1);
    defineNative(vm, "acos_val", nativeAcos,     1);
    defineNative(vm, "atan_val", nativeAtan,     1);
    defineNative(vm, "log_val",  nativeLog,      1);
    defineNative(vm, "log10_val", nativeLog10,   1);
    defineNative(vm, "log2_val", nativeLog2,     1);
    defineNative(vm, "exp_val",  nativeExp,      1);
    defineNative(vm, "power_val", nativePower,   2);
    defineNative(vm, "PI",       nativePI,       0);
    defineNative(vm, "E",        nativeE,        0);
    defineNative(vm, "INF",      nativeINF,      0);
    defineNative(vm, "NAN_VAL",  nativeNAN,      0);
    defineNative(vm, "gcd",      nativeGCD,      2);
    defineNative(vm, "lcm",      nativeLCM,      2);
    defineNative(vm, "sign",     nativeSign,      1);
    defineNative(vm, "clamp",    nativeClamp,     3);
    defineNative(vm, "degrees",  nativeDegrees,   1);
    defineNative(vm, "radians",  nativeRadians,   1);
    defineNative(vm, "factorial", nativeFactorial, 1);
    defineNative(vm, "kya_prime", nativeKyaPrime, 1);
    defineNative(vm, "fib",      nativeFib,       1);
    defineNative(vm, "hypot_val", nativeHypot,    2);
    defineNative(vm, "is_nan",   nativeIsNan,     1);
    defineNative(vm, "is_inf",   nativeIsInf,     1);
    defineNative(vm, "random_choice",   nativeRandomChoice,   1);
    defineNative(vm, "random_shuffle",  nativeRandomShuffle,  1);
    defineNative(vm, "random_int",      nativeRandomInt,      2);

    /* ── Conversion ── */
    defineNative(vm, "hex_shabd", nativeHexShabd, 1);
    defineNative(vm, "oct_shabd", nativeOctShabd, 1);
    defineNative(vm, "bin_shabd", nativeBinShabd, 1);

    /* ── File I/O ── */
    defineNative(vm, "padho_file",  nativePadhoFile,  1);
    defineNative(vm, "likho_file",  nativeLikhoFile,  2);
    defineNative(vm, "joodo_file",  nativeJoodFile,   2);
    defineNative(vm, "file_hai",    nativeFileHai,    1);

    /* ── System / Utility ── */
    defineNative(vm, "samay",      nativeSamay,      0);
    defineNative(vm, "ruko_samay", nativeRukoSamay,  1);
    defineNative(vm, "yaadrchik",  nativeYaadrchik,  -1);
    defineNative(vm, "bahar",      nativeBahar,      -1);
    defineNative(vm, "ghadi",      nativeGhadi,      0);
    defineNative(vm, "platform",   nativePlatform,   0);
    defineNative(vm, "env_var",    nativeEnvVar,     1);
    defineNative(vm, "taareekh",   nativeTaareekh,   0);
    defineNative(vm, "waqt",       nativeWaqt,       0);
    defineNative(vm, "timestamp",  nativeTimestamp,   0);
    defineNative(vm, "hash_val",   nativeHashVal,    1);

    /* ── Date/Time ── */
    defineNative(vm, "din",       nativeDin,       0);
    defineNative(vm, "mahina",    nativeMahina,    0);
    defineNative(vm, "saal",      nativeSaal,      0);
    defineNative(vm, "ghanta",    nativeGhanta,    0);
    defineNative(vm, "minute",    nativeMinute,    0);
    defineNative(vm, "second",    nativeSecond,    0);
    defineNative(vm, "hafta_din", nativeHaftaDin,  0);
}
