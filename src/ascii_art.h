/*
 * ============================================================================
 *  RAMPYAARYAN - ASCII Art & Animation Header
 * ============================================================================
 */

#ifndef RAMPYAARYAN_ASCII_ART_H
#define RAMPYAARYAN_ASCII_ART_H

#include "common.h"

/* Terminal colors */
typedef enum {
    COLOR_RESET = 0,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE,
    COLOR_BOLD_RED,
    COLOR_BOLD_GREEN,
    COLOR_BOLD_YELLOW,
    COLOR_BOLD_BLUE,
    COLOR_BOLD_MAGENTA,
    COLOR_BOLD_CYAN,
    COLOR_BOLD_WHITE,
    COLOR_DIM,
    COLOR_UNDERLINE,
} TermColor;

/* Initialize terminal for color support */
void initTerminal(void);

/* Color output */
void setColor(TermColor color);
void colorPrintf(TermColor color, const char* fmt, ...);

/* ASCII art displays */
void showSplashScreen(void);
void showStartupAnimation(void);
void showBanner(void);
void showMiniBanner(void);
void showGoodbye(void);
void showHelp(void);
void showVersion(void);
void showCompileSuccess(const char* filename);
void showCompileError(void);
void showLoadingAnimation(const char* message);
void showProgressBar(const char* label, int current, int total);

/* REPL prompt */
void showPrompt(int lineNum);
void showMemoryStats(size_t allocated, size_t nextGC, int objectCount);

/* Error display */
void showError(const char* type, const char* message, int line, const char* filename);
void showErrorWithSource(const char* type, const char* message, int line,
                         const char* filename, const char* source);

/* Clear screen */
void clearScreen(void);

/* Sleep milliseconds */
void sleepMs(int ms);

/* Get terminal width */
int getTerminalWidth(void);

#endif /* RAMPYAARYAN_ASCII_ART_H */
