#ifndef STRING_MANAGEMENT_H
#define STRING_MANAGEMENT_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *file_path(const char *dir,
                const char *file_name);  // constructs a full file path from
                                         // directory and file/directory name

char *replace_prefix(const char *path, const char *old_prefix,
                     const char *new_prefix);  // replaces old_prefix with new_prefix in path
#endif                                         // STRING_MANAGEMENT_H
