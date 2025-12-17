#pragma once
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string_management.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void ensure_dir_exists(const char* path); //ensures that a directory exists, creates it if it doesn't

void copy(const char* src_dir, const char* dest_dirs[], int dest_count); //copies files from src_dir to each directory in dest_dirs

void add(int argc, char** argv); //adds a directory to backup





