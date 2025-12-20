#pragma once
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string_management.h>
#include <linux/limits.h>
#include <utime.h>
#include <copy_lib.h>
#include <signal.h>
  
#define MAX_ARGS 100
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define LOG_ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__))
typedef struct backup_record{
    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];
    pid_t pid;
    struct backup_record* next;
} backup_record;

void add(char* src_dir, char* dest_dir); //adds a directory to backup

void exit_backup(struct backup_record* head); //exits the program, terminating all backup processes

void list_backups(struct backup_record* head); //lists all current backups




