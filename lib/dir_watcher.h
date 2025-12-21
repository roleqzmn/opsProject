#pragma once

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string_management.h>
#include <linux/limits.h>
#include <utime.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/inotify.h>
#include <copy_lib.h>
#include <string_management.h>

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

void add_to_map(struct WatchMap *map, int wd, const char *path); //adds a watch descriptor and its associated path to the map

struct Watch *find_watch(struct WatchMap *map, int wd); //finds the watch associated with a given watch descriptor

void remove_from_map(struct WatchMap *map, int wd); //removes a watch descriptor and its associated path from the map

void add_watcher_recursive(int notify_fd,struct WatchMap *watch_map, const char* src_dir); //adds watchers to a directory and its subdirectories

void watch_directory(const char* src_dir, const char* dest_dir); //watches a directory for changes and mirrors them to the destination directory