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
#include <time.h>
  
#define MAX_ARGS 100
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define LOG_ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__))
typedef struct backup_record{
    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];
    pid_t pid;
    struct backup_record* next;
    struct backup_record* prev;
    time_t last_backup;
<<<<<<< HEAD
    int pipe_fd;
=======
>>>>>>> a06b5222593d92bea93f5ee01704ee46a2ad3d1c
} backup_record;

int add(char* src_dir, char* dest_dir, struct backup_record* process); //adds a directory to backup

void exit_backup(struct backup_record* head); //exits the program, terminating all backup processes

void list_backups(struct backup_record* head); //lists all current backups

void end_backup(char* src_dir, char* dest_dir, struct backup_record** head); //stops a backup process and removes it from the list




