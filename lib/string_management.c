#include "string_management.h"


char* file_path(const char* dir, const char* file_name){
    size_t dir_len = strlen(dir);
    size_t file_len = strlen(file_name);
    char* path = malloc(dir_len + file_len + 2); 
    if(path == NULL){
        return NULL;
    }
    strcpy(path, dir);
    strcat(path, "/");
    strcat(path, file_name);
    return path;
}

char* replace_prefix(const char* path, const char* old_prefix, const char* new_prefix) {
    size_t old_len = strlen(old_prefix);
    if (strncmp(path, old_prefix, old_len) != 0) {
        return NULL;
    }
    size_t new_len = strlen(new_prefix);
    size_t rest_len = strlen(path) - old_len;
    char* result = malloc(new_len + rest_len + 1);
    if (result == NULL) {
        return NULL;
    }
    strcpy(result, new_prefix);
    strcat(result, path + old_len);
    return result;
}