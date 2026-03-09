/*
 * ============================================================================
 *  RAMPYAARYAN PROGRAMMING LANGUAGE
 *  A Hinglish Programming Language - Made in India, for the World!
 * ============================================================================
 *  File: common.h
 *  Description: Common includes, macros, and configuration
 * ============================================================================
 */

#ifndef RAMPYAARYAN_COMMON_H
#define RAMPYAARYAN_COMMON_H

/* Enable POSIX APIs (nanosleep, clock_gettime, etc.) on glibc/Linux */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 199309L
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

/* ============================================================================
 *  VERSION
 * ============================================================================ */
#define RAMPYAARYAN_VERSION_MAJOR 1
#define RAMPYAARYAN_VERSION_MINOR 0
#define RAMPYAARYAN_VERSION_PATCH 0
#define RAMPYAARYAN_VERSION "1.0.0"
#define RAM_VERSION RAMPYAARYAN_VERSION
#define RAMPYAARYAN_FILE_EXTENSION ".ram"

/* ============================================================================
 *  DEBUG FLAGS (comment out for release builds)
 * ============================================================================ */
/* #define DEBUG_PRINT_CODE */
/* #define DEBUG_TRACE_EXECUTION */
/* #define DEBUG_STRESS_GC */
/* #define DEBUG_LOG_GC */

/* ============================================================================
 *  LIMITS
 * ============================================================================ */
#define UINT8_COUNT         (UINT8_MAX + 1)
#define MAX_CALL_FRAMES     256
#define STACK_MAX           (MAX_CALL_FRAMES * UINT8_COUNT)
#define MAX_STRING_LENGTH   1048576  /* 1MB */
#define MAX_LIST_SIZE       1048576
#define MAX_LOOP_ITERATIONS 10000000
#define GC_HEAP_GROW_FACTOR 2

/* ============================================================================
 *  PLATFORM DETECTION
 * ============================================================================ */
#ifdef _WIN32
    #define PLATFORM_WINDOWS 1
    #include <windows.h>
    #include <io.h>
    #define isatty _isatty
    #define fileno _fileno
#else
    #define PLATFORM_WINDOWS 0
    #include <unistd.h>
    #include <sys/ioctl.h>
#endif

/* ============================================================================
 *  UTILITY MACROS
 * ============================================================================ */
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define UNUSED(x) (void)(x)

#endif /* RAMPYAARYAN_COMMON_H */
