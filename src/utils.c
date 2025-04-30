#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

char *strndup_impl(const char *src, size_t length)
{
    char *result = malloc(length + 1);

    if (!result)
        return NULL;

    memcpy(result, src, length);

    result[length] = '\0';

    return result;
}

char *read_file(const char *filename)
{
    FILE *file = fopen(filename, "r");

    if (!file)
        return NULL;

    fseek(file, 0, SEEK_END);

    long length = ftell(file);

    fseek(file, 0, SEEK_SET);

    char *content = malloc(length + 1);

    if (!content)
    {
        fclose(file);
        return NULL;
    }

    fread(content, 1, length, file);

    content[length] = '\0';

    fclose(file);

    return content;
}

void print_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);

#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_ERROR_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD saved_attributes;

    GetConsoleScreenBufferInfo(console, &consoleInfo);
    saved_attributes = consoleInfo.wAttributes;

    SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_INTENSITY);

    vfprintf(stderr, format, args);

    SetConsoleTextAttribute(console, saved_attributes);
#else
    fprintf(stderr, "\x1b[31m%s\x1b[0m\n", format);
#endif
}