#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

char* file_path(const char* dir, const char* file_name); //constructs a full file path from directory and file/directory name

char* replace_prefix(const char* path, const char* old_prefix, const char* new_prefix); //replaces old_prefix with new_prefix in path