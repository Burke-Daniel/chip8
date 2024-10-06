#ifndef __CMAKEPP_C_CODE
gcc -D__CMAKEPP_C_CODE cmakepp.c -o cmakepp && ./cmakepp && rm ./cmakepp
exit $?
#endif

#ifndef __CMAKEPP_DONT_COMPILE_MYSELF

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/limits.h>
#include <sys/stat.h>

#include "config.h"

extern const char *target;
extern const char *files[];
extern const char *libs[];

int main(int argc, char **argv)
{
    char compile_command[65536];
    size_t offset = 0;

    char working_directory[PATH_MAX];
    if (getcwd(working_directory, PATH_MAX) == NULL)
    {
        printf("Could not get current working directory: %s\n", strerror(errno));
        return errno;
    }
    
#ifdef BUILD_DIR
    int result = mkdir(BUILD_DIR, S_IRWXU|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (result != 0 && errno != EEXIST)
    {
        fprintf(stderr, "Error: could not create build directory: %s\n", strerror(errno));
        return errno;
    }
#endif
    
    const char *compiler_command = "cd " BUILD_DIR " && cc -D__CMAKEPP_C_CODE -D__CMAKEPP_DONT_COMPILE_MYSELF ";
    sprintf(compile_command + offset, "%s", compiler_command);
    offset += strlen(compiler_command);

    size_t i = 0;
    const char *file_name = files[i];
    while (file_name != NULL)
    {
        // working_directory + / + file_name + space
        size_t length = strlen(working_directory) + 1 + strlen(file_name) + 1;
        sprintf(compile_command + offset, "%s/%s ", working_directory, file_name);
        offset += length;
        i++;
        file_name = files[i];
    }

    i = 0;
    const char *lib_name = libs[i];
    while (lib_name != NULL)
    {
        size_t length = strlen("-l") + strlen(lib_name) + strlen(" ");
        sprintf(compile_command + offset, "-l%s ", lib_name);
        offset += length;
        i++;
        lib_name = libs[i];
    }
    printf("\n");

    sprintf(compile_command + offset, "-o %s", target);
    offset += strlen("-o ") + strlen(target);

    printf("Compile command: %s\n", compile_command);

    // TODO properly handle return code
    int status = system(compile_command);

    if (status == -1)
    {
        printf("An error occurred when trying to compile: %s", strerror(errno));
        return 1;
    }

    return WEXITSTATUS(status);
}

#endif
