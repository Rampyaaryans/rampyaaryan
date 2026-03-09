/*
 * ============================================================================
 *  RAMPYAARYAN - Main Entry Point
 *  CLI, REPL, and file execution
 * ============================================================================
 */

#include "vm.h"
#include "ascii_art.h"

#include <errno.h>

/* ============================================================================
 *  FILE READING
 * ============================================================================ */
static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "\n");
        showError("File Error", path, 0, NULL);
        fprintf(stderr, "  File nahi khul sakti: %s\n", strerror(errno));
        fprintf(stderr, "  Path: %s\n\n", path);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = (size_t)ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "File padhne ke liye memory nahi hai: %s\n", path);
        fclose(file);
        return NULL;
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "File poori nahi padh sakti: %s\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[bytesRead] = '\0';
    fclose(file);
    return buffer;
}

/* ============================================================================
 *  RUN FILE
 * ============================================================================ */
static int runFile(const char* path, bool debugMode) {
    /* Check file extension */
    const char* ext = strrchr(path, '.');
    if (ext == NULL || (strcmp(ext, ".ram") != 0 && strcmp(ext, ".rpy") != 0)) {
        showError("File Error", "Sirf .ram files chala sakte ho!", 0, NULL);
        return 74;
    }

    char* source = readFile(path);
    if (source == NULL) return 74;

    VM vm;
    initVM(&vm);

    if (debugMode) {
        printf("\n");
        setColor(COLOR_BOLD_CYAN);
        printf("  Debug Mode: ");
        setColor(COLOR_DIM);
        printf("%s\n", path);
        setColor(COLOR_RESET);
        printf("\n");
    }

    InterpretResult result = interpret(&vm, source, path);

    free(source);
    freeVM(&vm);

    switch (result) {
        case INTERPRET_OK:
            return 0;
        case INTERPRET_COMPILE_ERROR:
            return 65;
        case INTERPRET_RUNTIME_ERROR:
            return 70;
    }
    return 0;
}

/* ============================================================================
 *  REPL - Interactive Mode
 * ============================================================================ */
static void repl(void) {
    showSplashScreen();
    showStartupAnimation();

    setColor(COLOR_DIM);
    printf("  Type '.help' for help, '.exit' to quit.\n");
    setColor(COLOR_RESET);
    printf("\n");

    VM vm;
    initVM(&vm);

    char line[8192];
    int lineNum = 1;
    char multiline[65536];
    int braceDepth = 0;
    bool inMultiline = false;

    for (;;) {
        if (!inMultiline) {
            showPrompt(lineNum);
        } else {
            setColor(COLOR_DIM);
            printf("  .... ");
            setColor(COLOR_RESET);
        }

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        /* Remove trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
        if (len > 0 && line[len - 1] == '\r') line[--len] = '\0';

        /* REPL commands (only when not in multiline mode) */
        if (!inMultiline) {
            if (strcmp(line, ".exit") == 0 || strcmp(line, ".quit") == 0) {
                showGoodbye();
                break;
            }
            if (strcmp(line, ".help") == 0) {
                showHelp();
                continue;
            }
            if (strcmp(line, ".clear") == 0) {
#ifdef _WIN32
                system("cls");
#else
                system("clear");
#endif
                continue;
            }
            if (strcmp(line, ".memory") == 0) {
                int objCount = 0;
                Obj* obj = vm.objects;
                while (obj) { objCount++; obj = obj->next; }
                showMemoryStats(vm.bytesAllocated, vm.nextGC, objCount);
                continue;
            }
            if (strcmp(line, ".version") == 0) {
                showVersion();
                continue;
            }
            if (len == 0) continue;
        }

        /* Count braces for multiline support */
        for (size_t i = 0; i < len; i++) {
            if (line[i] == '{') braceDepth++;
            else if (line[i] == '}') braceDepth--;
        }

        if (!inMultiline) {
            strcpy(multiline, line);
        } else {
            strcat(multiline, "\n");
            strcat(multiline, line);
        }

        if (braceDepth > 0) {
            inMultiline = true;
            continue;
        }

        /* Execute */
        inMultiline = false;
        braceDepth = 0;

        interpret(&vm, multiline, "<repl>");
        vm.hadError = false;
        lineNum++;
    }

    freeVM(&vm);
}

/* ============================================================================
 *  MAIN
 * ============================================================================ */
int main(int argc, char* argv[]) {
#ifdef _WIN32
    /* Enable UTF-8 on Windows */
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    /* Enable ANSI colors */
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
#endif

    if (argc == 1) {
        /* Interactive REPL */
        repl();
        return 0;
    }

    if (argc == 2) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            showHelp();
            return 0;
        }
        if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
            showVersion();
            return 0;
        }
        if (strcmp(argv[1], "hindi") == 0) {
#ifdef _WIN32
            printf("\n");
            setColor(COLOR_BOLD_YELLOW);
            printf("  \xe0\xa4\xa8\xe0\xa4\xae\xe0\xa4\xb8\xe0\xa5\x8d\xe0\xa4\xa4\xe0\xa5\x87! ");
            setColor(COLOR_RESET);
            printf("Terminal ko Hindi mein set kar rahe hain...\n\n");
            system("chcp 65001 > nul 2>&1");

            /* Get real $PROFILE path from PowerShell (handles OneDrive redirection) */
            char profilePath[512];
            {
                FILE* pp = _popen("powershell -NoProfile -Command \"echo $PROFILE\"", "r");
                profilePath[0] = '\0';
                if (pp) {
                    if (fgets(profilePath, sizeof(profilePath), pp)) {
                        /* trim newline */
                        size_t len = strlen(profilePath);
                        while (len > 0 && (profilePath[len-1] == '\n' || profilePath[len-1] == '\r'))
                            profilePath[--len] = '\0';
                    }
                    _pclose(pp);
                }
                if (profilePath[0] == '\0') {
                    snprintf(profilePath, sizeof(profilePath), "%s\\Documents\\WindowsPowerShell\\Microsoft.PowerShell_profile.ps1", getenv("USERPROFILE"));
                }
            }

            /* Create dir if needed */
            {
                char profileDir[512];
                strncpy(profileDir, profilePath, sizeof(profileDir));
                profileDir[sizeof(profileDir)-1] = '\0';
                char* lastSlash = strrchr(profileDir, '\\');
                if (lastSlash) *lastSlash = '\0';
                CreateDirectoryA(profileDir, NULL);
            }

            /* Check if already has Hindi prompt */
            FILE* checkFile = fopen(profilePath, "r");
            bool alreadySet = false;
            if (checkFile) {
                char buf[4096];
                while (fgets(buf, sizeof(buf), checkFile)) {
                    if (strstr(buf, "RAMPYAARYAN-HINDI-PROMPT")) {
                        alreadySet = true;
                        break;
                    }
                }
                fclose(checkFile);
            }

            if (!alreadySet) {
                FILE* pf = fopen(profilePath, "a");
                if (pf) {
                    fprintf(pf, "\n# RAMPYAARYAN-HINDI-PROMPT-START\n");
                    fprintf(pf, "function prompt {\n");
                    fprintf(pf, "    $loc = (Get-Location).Path\n");
                    fprintf(pf, "    Write-Host \"\xe0\xa4\xb0\xe0\xa4\xbe\xe0\xa4\xae\xe0\xa4\xaa\xe0\xa5\x8d\xe0\xa4\xaf\xe0\xa4\xbe\xe0\xa4\xb0\xe0\xa4\xaf\xe0\xa4\xa8 \" -NoNewline -ForegroundColor Yellow\n");
                    fprintf(pf, "    Write-Host \"$loc\" -NoNewline -ForegroundColor Cyan\n");
                    fprintf(pf, "    Write-Host \" \xe0\xa4\x86\xe0\xa4\xa6\xe0\xa5\x87\xe0\xa4\xb6>\" -NoNewline -ForegroundColor Green\n");
                    fprintf(pf, "    return \" \"\n");
                    fprintf(pf, "}\n");
                    fprintf(pf, "# RAMPYAARYAN-HINDI-PROMPT-END\n");
                    fclose(pf);
                }
            }

            setColor(COLOR_BOLD_GREEN);
            printf("  \xe2\x9c\x85 ");
            setColor(COLOR_RESET);
            printf("Terminal Hindi mein set ho gaya!\n");
            printf("  \xe2\x9e\x95 UTF-8 encoding enabled (chcp 65001)\n");
            printf("  \xe2\x9e\x95 Hindi prompt set for PowerShell\n\n");
            setColor(COLOR_DIM);
            printf("  Naya PowerShell kholein dekhne ke liye.\n");
            printf("  Wapas English: rampyaaryan hindi --off\n\n");
            setColor(COLOR_RESET);
#else
            printf("\n");
            setColor(COLOR_BOLD_YELLOW);
            printf("  \xe0\xa4\xa8\xe0\xa4\xae\xe0\xa4\xb8\xe0\xa5\x8d\xe0\xa4\xa4\xe0\xa5\x87! ");
            setColor(COLOR_RESET);
            printf("Terminal ko Hindi mein set kar rahe hain...\n\n");

            const char* home = getenv("HOME");
            if (home) {
                /* Detect shell config file */
                char rcPath[512];
#ifdef __APPLE__
                snprintf(rcPath, sizeof(rcPath), "%s/.zshrc", home);
#else
                snprintf(rcPath, sizeof(rcPath), "%s/.bashrc", home);
#endif
                /* Check if already set */
                FILE* checkFile = fopen(rcPath, "r");
                bool alreadySet = false;
                if (checkFile) {
                    char buf[4096];
                    while (fgets(buf, sizeof(buf), checkFile)) {
                        if (strstr(buf, "RAMPYAARYAN-HINDI-PROMPT")) {
                            alreadySet = true;
                            break;
                        }
                    }
                    fclose(checkFile);
                }

                if (!alreadySet) {
                    FILE* pf = fopen(rcPath, "a");
                    if (pf) {
                        fprintf(pf, "\n# RAMPYAARYAN-HINDI-PROMPT-START\n");
                        fprintf(pf, "export LANG=hi_IN.UTF-8\n");
                        fprintf(pf, "export PS1=\"\\[\\033[1;33m\\]\xe0\xa4\xb0\xe0\xa4\xbe\xe0\xa4\xae\xe0\xa4\xaa\xe0\xa5\x8d\xe0\xa4\xaf\xe0\xa4\xbe\xe0\xa4\xb0\xe0\xa4\xaf\xe0\xa4\xa8 \\[\\033[1;36m\\]\\w \\[\\033[1;32m\\]\xe0\xa4\x86\xe0\xa4\xa6\xe0\xa5\x87\xe0\xa4\xb6>\\[\\033[0m\\] \"\n");
                        fprintf(pf, "# RAMPYAARYAN-HINDI-PROMPT-END\n");
                        fclose(pf);
                    }
                }

                setColor(COLOR_BOLD_GREEN);
                printf("  \xe2\x9c\x85 ");
                setColor(COLOR_RESET);
                printf("Hindi prompt & UTF-8 set!\n");
                printf("  Naya terminal kholein. Wapas: rampyaaryan hindi --off\n\n");
            }
#endif
            return 0;
        }
        /* Run file */
        return runFile(argv[1], false);
    }

    if (argc == 3) {
        if (strcmp(argv[1], "--debug") == 0 || strcmp(argv[1], "-d") == 0) {
            return runFile(argv[2], true);
        }
        if (strcmp(argv[1], "hindi") == 0 && (strcmp(argv[2], "--off") == 0 || strcmp(argv[2], "--english") == 0)) {
            printf("\n  Switching terminal back to English...\n\n");
#ifdef _WIN32
            system("chcp 437 > nul 2>&1");
            /* Get real $PROFILE path from PowerShell */
            char profilePath[512];
            {
                FILE* pp = _popen("powershell -NoProfile -Command \"echo $PROFILE\"", "r");
                profilePath[0] = '\0';
                if (pp) {
                    if (fgets(profilePath, sizeof(profilePath), pp)) {
                        size_t len = strlen(profilePath);
                        while (len > 0 && (profilePath[len-1] == '\n' || profilePath[len-1] == '\r'))
                            profilePath[--len] = '\0';
                    }
                    _pclose(pp);
                }
                if (profilePath[0] == '\0') {
                    snprintf(profilePath, sizeof(profilePath), "%s\\Documents\\WindowsPowerShell\\Microsoft.PowerShell_profile.ps1", getenv("USERPROFILE"));
                }
            }
            /* Remove Hindi prompt from PowerShell profile */
            FILE* rf = fopen(profilePath, "r");
            if (rf) {
                char content[65536];
                size_t totalLen = 0;
                char line[4096];
                bool skip = false;
                while (fgets(line, sizeof(line), rf)) {
                    if (strstr(line, "RAMPYAARYAN-HINDI-PROMPT-START")) { skip = true; continue; }
                    if (strstr(line, "RAMPYAARYAN-HINDI-PROMPT-END")) { skip = false; continue; }
                    if (!skip) {
                        size_t ll = strlen(line);
                        if (totalLen + ll < sizeof(content) - 1) {
                            memcpy(content + totalLen, line, ll);
                            totalLen += ll;
                        }
                    }
                }
                content[totalLen] = '\0';
                fclose(rf);
                FILE* wf = fopen(profilePath, "w");
                if (wf) {
                    fwrite(content, 1, totalLen, wf);
                    fclose(wf);
                }
            }
#else
            const char* home = getenv("HOME");
            if (home) {
                char rcPath[512];
#ifdef __APPLE__
                snprintf(rcPath, sizeof(rcPath), "%s/.zshrc", home);
#else
                snprintf(rcPath, sizeof(rcPath), "%s/.bashrc", home);
#endif
                FILE* rf = fopen(rcPath, "r");
                if (rf) {
                    char content[65536];
                    size_t totalLen = 0;
                    char line[4096];
                    bool skip = false;
                    while (fgets(line, sizeof(line), rf)) {
                        if (strstr(line, "RAMPYAARYAN-HINDI-PROMPT-START")) { skip = true; continue; }
                        if (strstr(line, "RAMPYAARYAN-HINDI-PROMPT-END")) { skip = false; continue; }
                        if (!skip) {
                            size_t ll = strlen(line);
                            if (totalLen + ll < sizeof(content) - 1) {
                                memcpy(content + totalLen, line, ll);
                                totalLen += ll;
                            }
                        }
                    }
                    content[totalLen] = '\0';
                    fclose(rf);
                    FILE* wf = fopen(rcPath, "w");
                    if (wf) {
                        fwrite(content, 1, totalLen, wf);
                        fclose(wf);
                    }
                }
            }
#endif
            setColor(COLOR_BOLD_GREEN);
            printf("  \xe2\x9c\x85 ");
            setColor(COLOR_RESET);
            printf("Terminal English mein wapas aa gaya!\n");
            printf("  Naya terminal kholein.\n\n");
            return 0;
        }
    }

    fprintf(stderr, "Usage: rampyaaryan [path.ram]\n");
    fprintf(stderr, "       rampyaaryan --help\n");
    return 64;
}
