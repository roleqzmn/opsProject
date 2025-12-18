#include <add_lib.h>




void add(int argc, char** argv){
    if(argc < 2){
        LOG_ERR("argc");
        return;
    }

    const char* src_dir = argv[1];
    struct stat st;
    if(stat(src_dir, &st) == -1 || (stat(src_dir, &st) == 0 && !S_ISDIR(st.st_mode))){
        LOG_ERR("src_dir");
        return;
    }
    const char* dest_dirs[argc-2];
    for(int i = 2; i < argc; i++){
        dest_dirs[i-2] = argv[i];
        if(strcmp(dest_dirs[i-2], src_dir) == 0){
            if(argc <=3) return;
            LOG_ERR("add: destination directory cannot be the same as source directory");
            dest_dirs[i-2] = NULL;
            continue;
        }
        if(ensure_dir_exists(dest_dirs[i-2], st) == -1){
            LOG_ERR("ensure_dir_exists");
            dest_dirs[i-2] = NULL;
        }
    }
    backup(src_dir, dest_dirs, argc-2);
}