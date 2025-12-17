#include <add_lib.h>

void ensure_dir_exists(const char* path){
    struct stat st;
    if(stat(path, &st) == -1){
        if(mkdir(path, 0700) == -1){
            ERR("mkdir");
        }
    } 
}

void copy_file(const char* src_path, const char* dest_path){
    FILE* src = fopen(src_path, "rb");
    if(src == NULL){
        LOG_ERR("fopen src");
        return;
    }
    FILE* dest = fopen(dest_path, "wb");
    if(dest == NULL){
        LOG_ERR("fopen dest");
        fclose(src);
        return;
    }

    char buffer[4096];
    size_t bytes;
    while((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0){
        fwrite(buffer, 1, bytes, dest);
    }

    fclose(src);
    fclose(dest);
}

void copy_symlink(const char* src_path, const char* dest_path){
    char link_target[PATH_MAX];
    ssize_t len = readlink(src_path, link_target, sizeof(link_target) - 1);
    if (len == -1) {
        LOG_ERR("readlink");
        return;
    }
    link_target[len] = '\0'; 

    unlink(dest_path); 

    if (symlink(link_target, dest_path) == -1) {
        LOG_ERR("symlink");
    }
}

void copy(const char* src_dir, const char* dest_dirs[], int dest_count){
    DIR* dir = opendir(src_dir);
    if(dir == NULL){
        LOG_ERR("opendir");
        return;
    }
    struct dirent* entry;
    while((entry = readdir(dir)) != NULL){
        char* src_path = file_path(src_dir, entry->d_name);
        if (src_path == NULL) {
            free(src_path);
            continue;
        }


        struct stat st;
        if(stat(src_path, &st) == -1){
            free(src_path);
            LOG_ERR("stat");
        }
        if (S_ISDIR(st.st_mode)) { //if a directory
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { 
                const char* new_dest_dirs[dest_count];

                for(int i = 0; i < dest_count; i++){
                    new_dest_dirs[i] = file_path(dest_dirs[i], entry->d_name);

                    if (new_dest_dirs[i] == NULL) continue;

                    ensure_dir_exists(new_dest_dirs[i]);
                } //create corresponding directories in each destination

                copy(src_path, new_dest_dirs, dest_count);

                for(int i = 0; i < dest_count; i++){
                    free(new_dest_dirs[i]);
                }
            }
        }
        else if (S_ISREG(st.st_mode)) {
            for(int i = 0; i < dest_count; i++){
                char* dest_path = file_path(dest_dirs[i], entry->d_name);
                if (dest_path == NULL) {
                    free(dest_path);
                    continue;
                }
                copy_file(src_path, dest_path);
                free(dest_path);
            }
        } 
        else if (S_ISLNK(st.st_mode)) {
            for(int i = 0; i < dest_count; i++){
                char* dest_path = file_path(dest_dirs[i], entry->d_name);
                if (dest_path == NULL) {
                    free(dest_path);
                    continue;
                }
                copy_symlink(src_path, dest_path);
                free(dest_path);
            }
        }
        free(src_path);
        
    }
    closedir(dir);
}

void add(int argc, char** argv){
    if(argc < 2){
        LOG_ERR("argc");
        return;
    }

    const char* src_dir = argv[1];
    const char* dest_dirs[argc-2];
    for(int i = 2; i < argc; i++){
        dest_dirs[i-2] = argv[i];
        ensure_dir_exists(dest_dirs[i-2]);
    }
    copy(src_dir, dest_dirs, argc-2);
}