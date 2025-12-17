#include <string_management.h>
#include <stdlib.h>
#include <stdio.h>

char* file_path(const char* dir, const char* file_name){
    size_t dir_len = strlen(dir);
    size_t file_len = strlen(file_name);
    char* path = malloc(dir_len + file_len + 2); // +2 for '/' and '\0'
    if(path == NULL){
        return NULL;
    }
    strcpy(path, dir);
    strcat(path, "/");
    strcat(path, file_name);
    return path;
}