#define _GNU_SOURCE
#include "dir_watcher.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <linux/limits.h>
#include <utime.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/inotify.h>
#include "copy_lib.h"
#include "string_management.h"
#include "add_lib.h"

#define MAX_WATCHES 8192
#define EVENT_BUF_LEN (64 * (sizeof(struct inotify_event) + NAME_MAX + 1))



void add_to_map(struct WatchMap *map, int wd, const char *path) {
    if (map->watch_count >= MAX_WATCHES) {
        fprintf(stderr, "Exceeded max watches!\n");
        return;
    }
    map->watch_map[map->watch_count].wd = wd;
    map->watch_map[map->watch_count].path = strdup(path);
    map->watch_count++;
}

struct Watch *find_watch(struct WatchMap *map, int wd) {
    for (int i = 0; i < map->watch_count; i++) {
        if (map->watch_map[i].wd == wd) {
            return &map->watch_map[i];
        }
    }
    return NULL;
}

void remove_from_map(struct WatchMap *map, int wd) {
    for (int i = 0; i < map->watch_count; i++) {
        if (map->watch_map[i].wd == wd) {
            free(map->watch_map[i].path);
            map->watch_map[i] = map->watch_map[map->watch_count - 1];
            map->watch_count--;
            return;
        }
    }
}


void add_watcher_recursive(int notify_fd, struct WatchMap *watch_map, const char* src_dir){
    uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_CLOSE_WRITE;

    int wd = inotify_add_watch(notify_fd, src_dir, mask);
    if (wd < 0) {
        perror("inotify_add_watch");
        return;
    }
    add_to_map(watch_map, wd, src_dir);

    DIR *dir = opendir(src_dir);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", src_dir, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            add_watcher_recursive(notify_fd, watch_map, full_path);
        }
    }

    closedir(dir);

}

void watch_directory(const char* src_dir, const char* dest_dir, struct backup_record* head) {
    struct WatchMap watch_map = {0};
    int notify_fd = inotify_init();
    if (notify_fd < 0) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }
    watch_map.watch_count = 0;
    add_watcher_recursive(notify_fd, &watch_map, src_dir);
    while (watch_map.watch_count > 0) {
        char buffer[EVENT_BUF_LEN];
        ssize_t len = read(notify_fd, buffer, EVENT_BUF_LEN);
        if (len < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("read");
            exit(EXIT_FAILURE);
        }

        ssize_t i = 0;
        while (i < len) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];

            struct Watch *watch = find_watch(&watch_map, event->wd);

            char event_path[PATH_MAX] = "";
            if (watch && event->len > 0) {
                snprintf(event_path, sizeof(event_path), "%s/%s", watch->path, event->name);
            } else if (watch) {
                strncpy(event_path, watch->path, sizeof(event_path));
            }

            if (event->mask & IN_IGNORED) {
                remove_from_map(&watch_map, event->wd);
            } else if (event->mask & IN_ISDIR) {
                if (event->mask & IN_CREATE) {
                    add_watcher_recursive(notify_fd, &watch_map, event_path);
                    char* dest_path = replace_prefix(event_path, src_dir, dest_dir);
                    if (dest_path) {
                        ensure_dir_exists(dest_path, 0755);
                        free(dest_path);
                    }
                }
                if (event->mask & IN_DELETE) {
                    char* dest_path = replace_prefix(event_path, src_dir, dest_dir);
                    if (dest_path) {
                        clear_directory(dest_path);
                        if (rmdir(dest_path) == -1) {
                            perror("rmdir");
                        }
                        free(dest_path);
                    }
                }
            } else {
                if (event->mask & IN_MODIFY || event->mask & IN_CLOSE_WRITE) {
                    char* dest_path = replace_prefix(event_path, src_dir, dest_dir);
                    if (dest_path) {
                        struct stat st;
                        if (lstat(event_path, &st) == 0 && S_ISLNK(st.st_mode)) {
                            copy_symlink(src_dir, event_path, dest_path);
                        } else {
                            copy_file(event_path, dest_path);
                        }
                        free(dest_path);
                    }
                }
                if (event->mask & IN_CREATE && !(event->mask & IN_ISDIR)) {
                    char* dest_path = replace_prefix(event_path, src_dir, dest_dir);
                    if (dest_path) {
                        struct stat st;
                        if (lstat(event_path, &st) == 0 && S_ISLNK(st.st_mode)) {
                            copy_symlink(src_dir, event_path, dest_path);
                        } else {
                            copy_file(event_path, dest_path);
                        }
                        free(dest_path);
                    }
                }
                if (event->mask & IN_DELETE && !(event->mask & IN_ISDIR)) {
                    char* dest_path = replace_prefix(event_path, src_dir, dest_dir);
                    if (dest_path) {
                        if (remove(dest_path) == -1) {
                            perror("remove");
                        }
                        free(dest_path);
                    }
                }
            }

            i += sizeof(struct inotify_event) + event->len;
        }
        head->last_backup = time(NULL);

    }
    close(notify_fd);
}