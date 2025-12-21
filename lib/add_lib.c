    #define _GNU_SOURCE

    #include <add_lib.h>
    #include <signal.h>     
    #include <stdlib.h>

    int add(char* src_dir, char* dest_dir){
    char* src_dir_real = realpath(src_dir, NULL);
    if (src_dir_real == NULL) {
        perror("realpath src");
        return -1;
    }
    struct stat st;
    if(stat(src_dir_real, &st) == -1){
        perror("stat src");
        free(src_dir_real);  
        return -1;
    }
    if(!S_ISDIR(st.st_mode)){
        fprintf(stderr, "src is not a directory\n");
        free(src_dir_real);
        return -1;
    }
    if(strcmp(dest_dir, src_dir_real) == 0){
        fprintf(stderr, "destination directory cannot be the same as source directory\n");
            free(src_dir_real); 
            return -1;
        }
        if(ensure_dir_exists(dest_dir, st.st_mode) == -1){
            LOG_ERR("ensure_dir_exists");
            free(src_dir_real);
            return -1;
        } else {
            clear_directory(dest_dir);
        }
        backup_copy(src_dir_real, dest_dir);
        free(src_dir_real);
        return 0;
    }

    void exit_backup(struct backup_record* head){
        struct backup_record* current = head;
                while(current != NULL){
                    struct backup_record* temp = current;
                    kill(current->pid, SIGTERM);
                    current = current->next;
                    free(temp);
                }
    }

    void list_backups(struct backup_record* head){
        struct backup_record* current = head;
        while(current != NULL){
            printf("Backup PID: %d\n", current->pid);
            printf("Source Path: %s\n", current->src_path);
            printf("Destination Path: %s\n", current->dest_path);
            printf("\n");
            current = current->next;
        }
    }

    void end_backup(char* src_dir, char* dest_dir, struct backup_record** head){
        struct backup_record* current = *head;
        while(current != NULL){
            if(strcmp(current->src_path, src_dir) == 0 && strcmp(current->dest_path, dest_dir) == 0){
                kill(current->pid, SIGTERM);
                if(current->prev != NULL){
                    current->prev->next = current->next;
                } else {
                    *head = current->next;
                }
                if(current->next != NULL){
                    current->next->prev = current->prev;
                }
                if(*head != NULL){
                    (*head)->prev = NULL;
                }
                free(current);
                return;
            }
            current = current->next;
        }
    }