emcc -o index.html main.c -Os -Wall deps/libs/libraylib.a \
    -I. -I../raylib/src -L. -L../raylib/src -s USE_GLFW=3 \
    -DPLATFORM_WEB --embed-file roms/morse_demo.ch8 --embed-file beep-02.wav \
    -s TOTAL_MEMORY=67108864 \
    -s FORCE_FILESYSTEM=1 -s ASSERTIONS=1 --profiling \
    -s ALLOW_MEMORY_GROWTH=1 \
    --shell-file shell.html -s NO_EXIT_RUNTIME=1 -s "EXPORTED_FUNCTIONS=['_malloc', '_main']" -s "EXPORTED_RUNTIME_METHODS=ccall"

    #  -s MODULARIZE=1 -s 'EXPORT_NAME="createModule"' \
