/*
 * ============================================================================
 *  RAMPYAARYAN - Value System Implementation
 * ============================================================================
 */

#include "value.h"
#include "memory.h"
#include "object.h"

/* ============================================================================
 *  VALUE ARRAY (Dynamic array used as constant pool)
 * ============================================================================ */
void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

/* ============================================================================
 *  VALUE EQUALITY
 * ============================================================================ */
bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NULL:   return true;
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ: {
            /* Strings are interned, so pointer comparison works */
            return AS_OBJ(a) == AS_OBJ(b);
        }
        default: return false;
    }
}

/* ============================================================================
 *  TRUTHINESS
 * ============================================================================ */
bool isTruthy(Value value) {
    switch (value.type) {
        case VAL_BOOL:   return AS_BOOL(value);
        case VAL_NULL:   return false;
        case VAL_NUMBER: return AS_NUMBER(value) != 0;
        case VAL_OBJ: {
            if (IS_STRING(value)) {
                return AS_STRING(value)->length > 0;
            }
            if (IS_LIST(value)) {
                return AS_LIST(value)->items.count > 0;
            }
            if (IS_MAP(value)) {
                return AS_MAP(value)->count > 0;
            }
            return true;
        }
        default: return false;
    }
}

/* ============================================================================
 *  PRINT VALUE  (Hinglish style output)
 * ============================================================================ */
void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf("%s", AS_BOOL(value) ? "sach" : "jhooth");
            break;
        case VAL_NULL:
            printf("khali");
            break;
        case VAL_NUMBER: {
            double num = AS_NUMBER(value);
            if (num == (long long)num && num >= -1e15 && num <= 1e15) {
                printf("%lld", (long long)num);
            } else {
                printf("%g", num);
            }
            break;
        }
        case VAL_OBJ:
            printObject(value);
            break;
    }
}

void printValueRepr(Value value) {
    if (IS_STRING(value)) {
        printf("\"%s\"", AS_CSTRING(value));
    } else {
        printValue(value);
    }
}

/* ============================================================================
 *  VALUE TO STRING (caller must free!)
 * ============================================================================ */
char* valueToString(Value value) {
    char* buffer = (char*)malloc(256);
    if (buffer == NULL) return NULL;

    switch (value.type) {
        case VAL_BOOL:
            snprintf(buffer, 256, "%s", AS_BOOL(value) ? "sach" : "jhooth");
            break;
        case VAL_NULL:
            snprintf(buffer, 256, "khali");
            break;
        case VAL_NUMBER: {
            double num = AS_NUMBER(value);
            if (num == (long long)num && num >= -1e15 && num <= 1e15) {
                snprintf(buffer, 256, "%lld", (long long)num);
            } else {
                snprintf(buffer, 256, "%g", num);
            }
            break;
        }
        case VAL_OBJ: {
            if (IS_STRING(value)) {
                ObjString* str = AS_STRING(value);
                free(buffer);
                buffer = (char*)malloc(str->length + 1);
                if (buffer == NULL) return NULL;
                memcpy(buffer, str->chars, str->length);
                buffer[str->length] = '\0';
            } else if (IS_FUNCTION(value)) {
                ObjFunction* fn = AS_FUNCTION(value);
                if (fn->name != NULL) {
                    snprintf(buffer, 256, "<kaam %s>", fn->name->chars);
                } else {
                    snprintf(buffer, 256, "<script>");
                }
            } else if (IS_NATIVE(value)) {
                ObjNative* native = AS_NATIVE(value);
                snprintf(buffer, 256, "<built-in %s>", native->name);
            } else if (IS_CLOSURE(value)) {
                ObjClosure* closure = AS_CLOSURE(value);
                if (closure->function->name != NULL) {
                    snprintf(buffer, 256, "<kaam %s>", closure->function->name->chars);
                } else {
                    snprintf(buffer, 256, "<script>");
                }
            } else if (IS_LIST(value)) {
                ObjList* list = AS_LIST(value);
                /* Build list string representation */
                free(buffer);
                size_t bufSize = 1024;
                buffer = (char*)malloc(bufSize);
                if (buffer == NULL) return NULL;
                int pos = 0;
                pos += snprintf(buffer + pos, bufSize - pos, "[");
                for (int i = 0; i < list->items.count; i++) {
                    if (i > 0) pos += snprintf(buffer + pos, bufSize - pos, ", ");
                    char* itemStr = valueToString(list->items.values[i]);
                    if (itemStr) {
                        if (IS_STRING(list->items.values[i])) {
                            pos += snprintf(buffer + pos, bufSize - pos, "\"%s\"", itemStr);
                        } else {
                            pos += snprintf(buffer + pos, bufSize - pos, "%s", itemStr);
                        }
                        free(itemStr);
                    }
                    if ((size_t)pos >= bufSize - 64) {
                        bufSize *= 2;
                        buffer = (char*)realloc(buffer, bufSize);
                        if (buffer == NULL) return NULL;
                    }
                }
                snprintf(buffer + pos, bufSize - pos, "]");
            } else if (IS_MAP(value)) {
                ObjMap* map = AS_MAP(value);
                free(buffer);
                size_t bufSize = 1024;
                buffer = (char*)malloc(bufSize);
                if (buffer == NULL) return NULL;
                int pos = 0;
                pos += snprintf(buffer + pos, bufSize - pos, "{");
                bool first = true;
                for (int i = 0; i < map->capacity; i++) {
                    if (!map->entries[i].isOccupied) continue;
                    if (!first) pos += snprintf(buffer + pos, bufSize - pos, ", ");
                    first = false;
                    char* ks = valueToString(map->entries[i].key);
                    char* vs = valueToString(map->entries[i].value);
                    if (ks && vs) {
                        if (IS_STRING(map->entries[i].key))
                            pos += snprintf(buffer + pos, bufSize - pos, "\"%s\"", ks);
                        else
                            pos += snprintf(buffer + pos, bufSize - pos, "%s", ks);
                        pos += snprintf(buffer + pos, bufSize - pos, ": ");
                        if (IS_STRING(map->entries[i].value))
                            pos += snprintf(buffer + pos, bufSize - pos, "\"%s\"", vs);
                        else
                            pos += snprintf(buffer + pos, bufSize - pos, "%s", vs);
                    }
                    if (ks) free(ks);
                    if (vs) free(vs);
                    if ((size_t)pos >= bufSize - 128) {
                        bufSize *= 2;
                        buffer = (char*)realloc(buffer, bufSize);
                        if (buffer == NULL) return NULL;
                    }
                }
                snprintf(buffer + pos, bufSize - pos, "}");
            } else {
                snprintf(buffer, 256, "<object>");
            }
            break;
        }
        default:
            snprintf(buffer, 256, "<unknown>");
            break;
    }
    return buffer;
}

/* ============================================================================
 *  VALUE TYPE NAME  (Hinglish)
 * ============================================================================ */
const char* valueTypeName(Value value) {
    switch (value.type) {
        case VAL_BOOL:   return "boolean";
        case VAL_NULL:   return "khali";
        case VAL_NUMBER: return "sankhya";
        case VAL_OBJ: {
            switch (OBJ_TYPE(value)) {
                case OBJ_STRING:   return "shabd";
                case OBJ_FUNCTION: return "kaam";
                case OBJ_NATIVE:   return "built-in kaam";
                case OBJ_CLOSURE:  return "kaam";
                case OBJ_LIST:     return "suchi";
                case OBJ_MAP:      return "shabdkosh";
                case OBJ_UPVALUE:  return "upvalue";
            }
        }
        default: return "anjaana";
    }
}
