#ifndef DIR_WATCHER_H
#define DIR_WATCHER_H

#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

#include "add_lib.h"
#include "copy_lib.h"
#include "string_management.h"

#define MAX_WATCHES 8192
#define EVENT_BUF_LEN (64 * (sizeof(struct inotify_event) + NAME_MAX + 1))

struct Watch {
  int wd;
  char *path;
};

struct WatchMap {
  struct Watch watch_map[MAX_WATCHES];
  int watch_count;
};

void add_to_map(struct WatchMap *map, int wd,
                const char *path); // adds a watch descriptor and its associated
                                   // path to the map

struct Watch *
find_watch(struct WatchMap *map,
           int wd); // finds the watch associated with a given watch descriptor

void remove_from_map(
    struct WatchMap *map,
    int wd); // removes a watch descriptor and its associated path from the map

void add_watcher_recursive(
    int notify_fd, struct WatchMap *watch_map,
    const char *src_dir); // adds watchers to a directory and its subdirectories

void watch_directory(
    const char *src_dir, const char *dest_dir,
    struct backup_record *head); // watches a directory for changes and mirrors
                                 // them to the destination directory
#endif                           // DIR_WATCHER_H