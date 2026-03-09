/*
 * ============================================================================
 *  RAMPYAARYAN - ASCII Art, Animations & Terminal Effects
 *  Enterprise-level terminal UI with colors and animations
 * ============================================================================
 */

#include "ascii_art.h"

/* ============================================================================
 *  TERMINAL COLOR SUPPORT
 * ============================================================================ */

#ifdef _WIN32
static HANDLE hConsole = NULL;
static WORD originalAttributes = 7;
static bool colorsInitialized = false;

static void initColors(void) {
    if (colorsInitialized) return;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(hConsole, &info)) {
        originalAttributes = info.wAttributes;
    }
    /* Enable ANSI escape codes on Windows 10+ */
    DWORD mode = 0;
    GetConsoleMode(hConsole, &mode);
    SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    colorsInitialized = true;
}
#endif

void setColor(TermColor color) {
#ifdef _WIN32
    initColors();
#endif
    const char* code;
    switch (color) {
        case COLOR_RESET:          code = "\033[0m"; break;
        case COLOR_RED:            code = "\033[31m"; break;
        case COLOR_GREEN:          code = "\033[32m"; break;
        case COLOR_YELLOW:         code = "\033[33m"; break;
        case COLOR_BLUE:           code = "\033[34m"; break;
        case COLOR_MAGENTA:        code = "\033[35m"; break;
        case COLOR_CYAN:           code = "\033[36m"; break;
        case COLOR_WHITE:          code = "\033[37m"; break;
        case COLOR_BOLD_RED:       code = "\033[1;31m"; break;
        case COLOR_BOLD_GREEN:     code = "\033[1;32m"; break;
        case COLOR_BOLD_YELLOW:    code = "\033[1;33m"; break;
        case COLOR_BOLD_BLUE:      code = "\033[1;34m"; break;
        case COLOR_BOLD_MAGENTA:   code = "\033[1;35m"; break;
        case COLOR_BOLD_CYAN:      code = "\033[1;36m"; break;
        case COLOR_BOLD_WHITE:     code = "\033[1;37m"; break;
        case COLOR_DIM:            code = "\033[2m"; break;
        case COLOR_UNDERLINE:      code = "\033[4m"; break;
        default:                   code = "\033[0m"; break;
    }
    printf("%s", code);
}

void sleepMs(int ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}

/* ============================================================================
 *  SPLASH SCREEN - The Big Logo
 * ============================================================================ */
void showSplashScreen(void) {
#ifdef _WIN32
    initColors();
#endif

    /* RAMPYAARYAN in bold pixel block font - '#' becomes full block */
    static const char* banner[] = {
        "####  ####  #   #  ####  #   #  ####  ####  ####  #   #  ####  #   #",
        "#  #  #  #  ## ##  #  #   # #   #  #  #  #  #  #   # #   #  #  ##  #",
        "####  ####  # # #  ####    #    ####  ####  ####    #    ####  # # #",
        "# #   #  #  #   #  #       #    #  #  #  #  # #     #    #  #  #  ##",
        "#  #  #  #  #   #  #       #    #  #  #  #  #  #    #    #  #  #   #"
    };

    printf("\n");
    for (int i = 0; i < 5; i++) {
        printf("   \033[1;38;5;208m");
        for (int j = 0; banner[i][j]; j++) {
            if (banner[i][j] == '#')
                printf("\xe2\x96\x88");
            else
                putchar(banner[i][j]);
        }
        printf("\033[0m\n");
        fflush(stdout);
        sleepMs(50);
    }
    printf("\n                    \033[2mHinglish Programming Language\033[0m\n\n");
    fflush(stdout);
}

/* ============================================================================
 *  STARTUP ANIMATION - Loading effect
 * ============================================================================ */
void showStartupAnimation(void) {
#ifdef _WIN32
    initColors();
#endif

    const char* steps[] = {
        "Lexer shuru ho raha hai",
        "Parser tayyar ho raha hai",
        "Bytecode compiler load ho raha hai",
        "Virtual Machine initialize ho rahi hai",
        "GC setup ho raha hai",
        "Native functions register ho rahe hain",
        "Rampyaaryan tayyar hai!"
    };
    int stepCount = sizeof(steps) / sizeof(steps[0]);

    printf("\n");
    for (int i = 0; i < stepCount; i++) {
        /* Spinner animation */
        const char* spinner = "|/-\\";
        for (int j = 0; j < 4; j++) {
            printf("  \033[1;36m%c\033[0m \033[2m%s\033[0m\r", spinner[j], steps[i]);
            fflush(stdout);
            sleepMs(60);
        }
        printf("  \033[1;32m\xe2\x9c\x93\033[0m %s\n", steps[i]);
        fflush(stdout);
    }
    printf("\n");
}

/* ============================================================================
 *  ERROR DISPLAY
 * ============================================================================ */
void showError(const char* title, const char* message, int line, const char* filename) {
    printf("\n");
    setColor(COLOR_BOLD_RED);
    printf("  \xe2\x95\x94\xe2\x95\x90\xe2\x95\x90 GALTI ");
    setColor(COLOR_RESET);
    setColor(COLOR_DIM);
    if (filename) printf(" [%s", filename);
    if (line > 0) printf(":%d", line);
    if (filename) printf("]");
    setColor(COLOR_RESET);
    printf("\n");

    setColor(COLOR_BOLD_RED);
    printf("  \xe2\x95\x91\033[0m  ");
    setColor(COLOR_BOLD_WHITE);
    printf("%s", title);
    setColor(COLOR_RESET);
    printf("\n");

    setColor(COLOR_BOLD_RED);
    printf("  \xe2\x95\x91\033[0m  ");
    setColor(COLOR_YELLOW);
    printf("%s", message);
    setColor(COLOR_RESET);
    printf("\n");

    setColor(COLOR_BOLD_RED);
    printf("  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90");
    setColor(COLOR_RESET);
    printf("\n\n");
}

/* ============================================================================
 *  SUCCESS DISPLAY
 * ============================================================================ */
void showSuccess(const char* message) {
    setColor(COLOR_BOLD_GREEN);
    printf("  \xe2\x9c\x93 ");
    setColor(COLOR_GREEN);
    printf("%s", message);
    setColor(COLOR_RESET);
    printf("\n");
}

/* ============================================================================
 *  PROGRESS BAR
 * ============================================================================ */
void showProgressBar(const char* label, int current, int total) {
    int width = 30;
    int filled = (int)((double)current / total * width);

    printf("  %s [", label);
    setColor(COLOR_BOLD_GREEN);
    for (int i = 0; i < filled; i++) printf("\xe2\x96\x88");
    setColor(COLOR_DIM);
    for (int i = filled; i < width; i++) printf("\xe2\x96\x91");
    setColor(COLOR_RESET);
    printf("] %d%%\r", (int)((double)current / total * 100));
    fflush(stdout);
}

/* ============================================================================
 *  REPL PROMPT
 * ============================================================================ */
void showPrompt(int lineNum) {
    setColor(COLOR_BOLD_CYAN);
    printf("  ram");
    setColor(COLOR_DIM);
    printf("[%d]", lineNum);
    setColor(COLOR_BOLD_CYAN);
    printf("> ");
    setColor(COLOR_RESET);
}

/* ============================================================================
 *  MEMORY STATS
 * ============================================================================ */
void showMemoryStats(size_t allocated, size_t nextGC, int objectCount) {
    printf("\n");
    setColor(COLOR_BOLD_CYAN);
    printf("  \xe2\x94\x80\xe2\x94\x80 Memory Stats \xe2\x94\x80\xe2\x94\x80\n");
    setColor(COLOR_RESET);

    setColor(COLOR_DIM);
    printf("  Allocated:  ");
    setColor(COLOR_WHITE);
    if (allocated < 1024) {
        printf("%zu bytes\n", allocated);
    } else if (allocated < 1024 * 1024) {
        printf("%.1f KB\n", (double)allocated / 1024);
    } else {
        printf("%.1f MB\n", (double)allocated / (1024 * 1024));
    }

    setColor(COLOR_DIM);
    printf("  Next GC:    ");
    setColor(COLOR_WHITE);
    if (nextGC < 1024) {
        printf("%zu bytes\n", nextGC);
    } else if (nextGC < 1024 * 1024) {
        printf("%.1f KB\n", (double)nextGC / 1024);
    } else {
        printf("%.1f MB\n", (double)nextGC / (1024 * 1024));
    }

    setColor(COLOR_DIM);
    printf("  Objects:    ");
    setColor(COLOR_WHITE);
    printf("%d\n", objectCount);
    setColor(COLOR_RESET);
    printf("\n");
}

/* ============================================================================
 *  HELP TEXT
 * ============================================================================ */
void showHelp(void) {
    printf("\n");
    setColor(COLOR_BOLD_CYAN);
    printf("  Rampyaaryan - Hinglish Programming Language\n");
    setColor(COLOR_RESET);
    printf("  Version %s\n\n", RAM_VERSION);

    setColor(COLOR_BOLD_WHITE);
    printf("  Usage:\n");
    setColor(COLOR_RESET);
    printf("    rampyaaryan                  REPL (interactive mode)\n");
    printf("    rampyaaryan <file.ram>       Run a .ram file\n");
    printf("    rampyaaryan --help           Show this help\n");
    printf("    rampyaaryan --version        Show version\n");
    printf("    rampyaaryan --debug <file>   Run with bytecode debug output\n");
    printf("\n");

    setColor(COLOR_BOLD_WHITE);
    printf("  REPL Commands:\n");
    setColor(COLOR_RESET);
    printf("    .help       Show help\n");
    printf("    .clear      Clear screen\n");
    printf("    .memory     Show memory stats\n");
    printf("    .exit       Exit REPL\n");
    printf("\n");

    setColor(COLOR_BOLD_WHITE);
    printf("  Language Keywords:\n");
    setColor(COLOR_RESET);
    setColor(COLOR_YELLOW);
    printf("    maano       ");
    setColor(COLOR_DIM);
    printf("- Variable declare karo (let)\n");
    setColor(COLOR_YELLOW);
    printf("    likho       ");
    setColor(COLOR_DIM);
    printf("- Print karo (print)\n");
    setColor(COLOR_YELLOW);
    printf("    pucho       ");
    setColor(COLOR_DIM);
    printf("- Input lo (input)\n");
    setColor(COLOR_YELLOW);
    printf("    agar        ");
    setColor(COLOR_DIM);
    printf("- Condition check (if)\n");
    setColor(COLOR_YELLOW);
    printf("    warna       ");
    setColor(COLOR_DIM);
    printf("- Else\n");
    setColor(COLOR_YELLOW);
    printf("    warna agar  ");
    setColor(COLOR_DIM);
    printf("- Else-if\n");
    setColor(COLOR_YELLOW);
    printf("    jab tak     ");
    setColor(COLOR_DIM);
    printf("- While loop\n");
    setColor(COLOR_YELLOW);
    printf("    har         ");
    setColor(COLOR_DIM);
    printf("- For loop\n");
    setColor(COLOR_YELLOW);
    printf("    kaam        ");
    setColor(COLOR_DIM);
    printf("- Function define (func)\n");
    setColor(COLOR_YELLOW);
    printf("    wapas do    ");
    setColor(COLOR_DIM);
    printf("- Return\n");
    setColor(COLOR_YELLOW);
    printf("    ruko        ");
    setColor(COLOR_DIM);
    printf("- Break\n");
    setColor(COLOR_YELLOW);
    printf("    agla        ");
    setColor(COLOR_DIM);
    printf("- Continue\n");
    setColor(COLOR_YELLOW);
    printf("    sach/jhooth ");
    setColor(COLOR_DIM);
    printf("- true/false\n");
    setColor(COLOR_YELLOW);
    printf("    khali       ");
    setColor(COLOR_DIM);
    printf("- null\n");
    setColor(COLOR_YELLOW);
    printf("    aur/ya/nahi ");
    setColor(COLOR_DIM);
    printf("- and/or/not\n");
    setColor(COLOR_RESET);
    printf("\n");

    setColor(COLOR_BOLD_WHITE);
    printf("  Built-in Functions:\n");
    setColor(COLOR_RESET);
    setColor(COLOR_GREEN);
    printf("    lambai(x)        ");
    setColor(COLOR_DIM);
    printf("- Length of string/list\n");
    setColor(COLOR_GREEN);
    printf("    prakar(x)        ");
    setColor(COLOR_DIM);
    printf("- Type of value\n");
    setColor(COLOR_GREEN);
    printf("    sankhya(x)       ");
    setColor(COLOR_DIM);
    printf("- Convert to number\n");
    setColor(COLOR_GREEN);
    printf("    shabd(x)         ");
    setColor(COLOR_DIM);
    printf("- Convert to string\n");
    setColor(COLOR_GREEN);
    printf("    joodo(list, x)   ");
    setColor(COLOR_DIM);
    printf("- Append to list\n");
    setColor(COLOR_GREEN);
    printf("    ulta(x)          ");
    setColor(COLOR_DIM);
    printf("- Reverse string/list\n");
    setColor(COLOR_GREEN);
    printf("    kram(list)       ");
    setColor(COLOR_DIM);
    printf("- Sort list\n");
    setColor(COLOR_GREEN);
    printf("    suchi(...)       ");
    setColor(COLOR_DIM);
    printf("- Create list\n");
    setColor(COLOR_GREEN);
    printf("    range_(a, b, s)  ");
    setColor(COLOR_DIM);
    printf("- Create number range\n");
    setColor(COLOR_GREEN);
    printf("    yaadrchik(a, b)  ");
    setColor(COLOR_DIM);
    printf("- Random number\n");
    setColor(COLOR_GREEN);
    printf("    samay()          ");
    setColor(COLOR_DIM);
    printf("- Current time\n");
    setColor(COLOR_RESET);
    printf("\n");

    setColor(COLOR_DIM);
    printf("  Website: https://github.com/rampyaaryan\n");
    printf("  License: Rampyaaryan License\n");
    setColor(COLOR_RESET);
    printf("\n");
}

/* ============================================================================
 *  VERSION DISPLAY
 * ============================================================================ */
void showVersion(void) {
    setColor(COLOR_BOLD_CYAN);
    printf("Rampyaaryan ");
    setColor(COLOR_BOLD_WHITE);
    printf("v%s", RAM_VERSION);
    setColor(COLOR_DIM);
    printf(" (bytecode VM, built %s)\n", __DATE__);
    setColor(COLOR_RESET);
}

/* ============================================================================
 *  GOODBYE MESSAGE
 * ============================================================================ */
void showGoodbye(void) {
    printf("\n");
    setColor(COLOR_BOLD_CYAN);
    printf("  \xf0\x9f\x99\x8f Alvida! ");
    setColor(COLOR_DIM);
    printf("Rampyaaryan use karne ke liye shukriya.\n");
    setColor(COLOR_RESET);
    printf("\n");
}
