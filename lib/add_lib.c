#include <add_lib.h>


void copy(const char* src_dir, const char* dest_dirs[], int dest_count){
    
}

void add(int argc, char** argv){
    if(argc < 2){
        ERR(argc);
    }

    const char* src_dir = argv[1];
    const char* dest_dirs[argc-2];
    for(int i = 2; i < argc; i++){
        dest_dirs[i-2] = argv[i];
        printf("Adding directory %s to backup location %s\n", src_dir, dest_dirs[i-2]);
    }
    copy(src_dir, dest_dirs, argc-2);
}