#include <add_lib.h>
#include <string_management.h>


void ensure_dir_exists(const char* path){
    struct stat st;
    if(stat(path, &st) == -1){
        if(mkdir(path, 0700) == -1){
            ERR("mkdir");
        }
    } 
}

void copy(const char* src_dir, const char* dest_dirs[], int dest_count){
    DIR* dir = opendir(src_dir);
    if(dir == NULL){
        ERR("opendir");
    }
    struct dirent* entry;
    while((entry = readdir(dir)) != NULL){
        char* src_path = file_path(src_dir, entry->d_name);
        if (src_path == NULL) continue;

        struct stat st;
        if(stat(src_path, &st) == -1){
            free(src_path);
            ERR("stat");
        }
        if (S_ISDIR(st.st_mode)) { //if a directory
            char* dir_path = file_path(src_dir, entry->d_name);
            if (dir_path == NULL) {
                free(dir_path);
                continue;
            }
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { 
                const char* new_dest_dirs[dest_count];
                for(int i = 0; i < dest_count; i++){
                    new_dest_dirs[i] = file_path(dest_dirs[i], entry->d_name);
                    if (new_dest_dirs[i] == NULL) continue;
                    ensure_dir_exists(new_dest_dirs[i]);
                } //create corresponding directories in each destination
                copy(dir_path, new_dest_dirs, dest_count);
                for(int i = 0; i < dest_count; i++){
                    free(new_dest_dirs[i]);
                }
            }
            free(dir_path);
        }
        else if (S_ISREG(st.st_mode)) {
            for(int i = 0; i < dest_count; i++){
                char* dest_path = file_path(dest_dirs[i], entry->d_name);
                if (dest_path == NULL) continue;
                cp(src_path, dest_path);
                free(dest_path);
            }
        } 
        else if (S_ISLNK(st.st_mode)) {
            printf("To jest dowiązanie symboliczne (symbolic link).\n");
        }
        free(src_path);
        
    }
    closedir(dir);
}

void add(int argc, char** argv){
    if(argc < 2){
        ERR("argc");
    }

    const char* src_dir = argv[1];
    const char* dest_dirs[argc-2];
    for(int i = 2; i < argc; i++){
        dest_dirs[i-2] = argv[i];
        ensure_dir_exists(dest_dirs[i-2]);
    }
    copy(src_dir, dest_dirs, argc-2);
}