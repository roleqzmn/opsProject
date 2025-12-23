#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "string_management.h"

char *file_path(const char *dir, const char *file_name)
{
    size_t dir_len = strlen(dir);
    size_t file_len = strlen(file_name);
    char *path = malloc(dir_len + file_len + 2);
    if (path == NULL)
    {
        return NULL;
    }
    strcpy(path, dir);
    strcat(path, "/");
    strcat(path, file_name);
    return path;
}

char *replace_prefix(const char *path, const char *old_prefix, const char *new_prefix)
{
    size_t old_len = strlen(old_prefix);
    if (strncmp(path, old_prefix, old_len) != 0)
    {
        return NULL;
    }
    size_t new_len = strlen(new_prefix);
    size_t rest_len = strlen(path) - old_len;
    char *result = malloc(new_len + rest_len + 1);
    if (result == NULL)
    {
        return NULL;
    }
    strcpy(result, new_prefix);
    strcat(result, path + old_len);
    return result;
}

char *expand_tilde(const char *path)
{
    if (!path || path[0] != '~')
    {
        return strdup(path);
    }

    const char *home = getenv("HOME");
    if (!home)
    {
        fprintf(stderr, "Brak zmiennej HOME!\n");
        return NULL;
    }

    size_t home_len = strlen(home);
    size_t path_len = strlen(path);

    char *result = malloc(home_len + path_len);
    if (!result)
        return NULL;

    strcpy(result, home);
    strcat(result, path + 1);

    return result;
}

int parse_command_line(char *line, char *command, char **args)
{
    size_t pos = 0;
    size_t line_len = strlen(line);

    while (pos < line_len && line[pos] == ' ')
        pos++;
    size_t cmd_start = pos;
    while (pos < line_len && line[pos] != ' ')
        pos++;

    strncpy(command, line + cmd_start, pos - cmd_start);
    command[pos - cmd_start] = '\0';

    if (strlen(command) == 0)
    {
        return 0;
    }

    int arg_count = 0;
    while (pos < line_len && arg_count < MAX_ARGS)
    {
        while (pos < line_len && line[pos] == ' ')
            pos++;
        if (pos >= line_len)
            break;

        char arg_buf[PATH_MAX];
        int arg_idx = 0;

        if (line[pos] == '"')
        {
            pos++;
            while (pos < line_len && line[pos] != '"')
            {
                arg_buf[arg_idx++] = line[pos];
                pos++;
            }
            if (pos < line_len)
                pos++;
        }
        else
        {
            while (pos < line_len && line[pos] != ' ')
            {
                arg_buf[arg_idx++] = line[pos];
                pos++;
            }
        }
        arg_buf[arg_idx] = '\0';
        args[arg_count] = strdup(arg_buf);

        arg_count++;
    }

    return arg_count;
}
