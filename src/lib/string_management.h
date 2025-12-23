#ifndef STRING_MANAGEMENT_H
#define STRING_MANAGEMENT_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 4096
#define MAX_ARGS 100

char *file_path(const char *dir,
                const char *file_name);  // constructs a full file path from
                                         // directory and file/directory name

char *replace_prefix(const char *path, const char *old_prefix,
                     const char *new_prefix);  // replaces old_prefix with new_prefix in path

char *expand_tilde(const char *path);  // expands ~ to the user's home directory

int parse_command_line(char *line, char *command,
                       char **args);  // parses a command line into command and arguments
#endif                                // STRING_MANAGEMENT_H
