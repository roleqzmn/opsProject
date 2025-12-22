#pragma once
#include <dirent.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <utime.h>

#include "copy_lib.h"
#include "string_management.h"

#define MAX_ARGS 100
#define ERR(source)                                                            \
  (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),             \
   exit(EXIT_FAILURE))
#define LOG_ERR(source)                                                        \
  (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__))
typedef struct backup_record {
  char src_path[PATH_MAX];
  char dest_path[PATH_MAX];
  pid_t pid;
  struct backup_record *next;
  struct backup_record *prev;
  time_t last_backup;
  bool ifworking;
} backup_record;

int add(char *src_dir, char *dest_dir,
        struct backup_record *process); // adds a directory to backup

void exit_backup(struct backup_record *head); // exits the program, terminating
                                              // all backup processes

void list_backups(struct backup_record *head); // lists all current backups

void end_backup(char *src_dir, char *dest_dir,
                struct backup_record **head); // stops a backup process and
                                              // removes it from the list

void help(); // prints help message
