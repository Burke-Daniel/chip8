#include <stdbool.h>

#define DEBUG
#define BUILD_DIR "build"

const char *target = "chip8_emulator";

const char *files[] = {
    "main.c",
    NULL,
};

const char *libs[] = {
    "raylib",
    "GL",
    "m",
    "pthread",
    "dl",
    "rt",
    "X11",
    NULL,
};
