#include <copy_lib.h>


int ensure_dir_exists(const char* path, mode_t mode) {
    struct stat _st;
    if(mode=0){
        mode=0755;
    }
    if(stat(path, &_st) == 0){
        if(!S_ISDIR(_st.st_mode)){
            LOG_ERR("ensure_dir_exists: path exists but is not a directory");
            return -1;
        }
        return 0;
    } 
    if(errno != ENOENT) {
        LOG_ERR("ensure_dir_exists stat");
        return -1;
    }

    char* path_cp = strdup(path);
    if (path_cp == NULL) {
        LOG_ERR("ensure_dir_exists strdup");
        return -1;
    }
    char* parent_dir = dirname(path_cp);
    if (strcmp(parent_dir, ".") != 0 && strcmp(parent_dir, "/") != 0) {
        if (ensure_dir_exists(parent_dir, mode) == -1) {
            free(path_cp);
            return -1;
        }
    }

    free(path_cp);
    
    if (mkdir(path, mode) == -1 && errno != EEXIST) {
        LOG_ERR("ensure_dir_exists mkdir");
        return -1;
    }
    return 0;
}

void copy_file(const char* src_path, const char* dest_path){   
    struct stat st;
    if(stat(src_path, &st) == -1){
        LOG_ERR("stat copy_file");
        return;
    }

    FILE* src = fopen(src_path, "rb");
    if(src == NULL){
        LOG_ERR("fopen src copy_file");
        return;
    }

    FILE* dest = fopen(dest_path, "wb"); 
    if(dest == NULL){
        LOG_ERR("fopen dest copy_file");
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

    if(chmod(dest_path, st.st_mode) == -1){
        LOG_ERR("chmod copy_file");
    }

    struct utimbuf times = {
        .actime  = st.st_atime,   
        .modtime = st.st_mtime    
    };
    if (utime(dest_path, &times) == -1) {
        LOG_ERR("utime copy_file");
    }
}

void copy_symlink(const char* src_dir, const char* src_path, const char* dest_path){
    char link_target[PATH_MAX];
    ssize_t len = readlink(src_path, link_target, sizeof(link_target) - 1);
    if (len == -1) {
        LOG_ERR("readlink");
        return;
    }
    link_target[len] = '\0'; 
    char * same_folder_target = strstr(link_target, src_dir);
    if(same_folder_target != NULL){
        char new_target[PATH_MAX];
        snprintf(new_target, sizeof(new_target), "%s%s", dest_path, same_folder_target + strlen(src_dir));
        strcpy(link_target, new_target);
    }
    unlink(dest_path); 

    
    if (symlink(link_target, dest_path) == -1) { 
        LOG_ERR("symlink");
    }
}

void backup(const char* src_dir, const char* dest_dirs[], int dest_count){
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

                    if(ensure_dir_exists(new_dest_dirs[i], st.st_mode)==-1){
                        free((void*)new_dest_dirs[i]);
                        continue;
                    };
                } //create corresponding directories in each destination

                backup(src_path, new_dest_dirs, dest_count);

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
                copy_symlink(src_dir, src_path, dest_path);
                free(dest_path);
            }
        }
        free(src_path);
        
    }
    closedir(dir);
}
