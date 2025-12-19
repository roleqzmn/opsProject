#pragma once


#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define LOG_ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__))


int ensure_dir_exists(const char* path, mode_t mode); //ensures that a directory exists, creates it if it doesn't

void copy_file(const char* src_path, const char* dest_path); //copies a file from src_path to dest_path

void copy_symlink(const char* src_dir, const char* src_path, const char* dest_path); //copies a symbolic link from src_path to dest_path

void backup_copy(const char* src_dir, const char* dest_dirs[], int dest_count); //copies files from src_dir to each directory in dest_dirs


