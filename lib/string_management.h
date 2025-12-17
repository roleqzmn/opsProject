#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

char* file_path(const char* dir, const char* file_name); //constructs a full file path from directory and file name