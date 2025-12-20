#include <dir_watcher.h>

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
    printf("new watch: '%s' @wd=%d\n", path, wd);
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
            printf("removing watch: '%s' @wd=%d\n", map->watch_map[i].path, wd);
            free(map->watch_map[i].path);
            map->watch_map[i] = map->watch_map[map->watch_count - 1];
            map->watch_count--;
            return;
        }
    }
}


void add_watcher_recursive(struct WatchMap *watch_map, const char* src_dir){
    // Implementation would go here
}

void watch_directory(struct WatchMap *watch_map, const char* src_dir, const char* dest_dir){
    while(1) {
        sleep(1);
    }
    // Implementation would go here
}