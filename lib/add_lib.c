    #define _POSIX_C_SOURCE 200809L

    #include <add_lib.h>
    #include <signal.h>     
    #include <sys/types.h>
    #include <copy_lib.h>


    void add(char* src_dir, char* dest_dir){
        struct stat st;
        if(stat(src_dir, &st) == -1 || (stat(src_dir, &st) == 0 && !S_ISDIR(st.st_mode))){
            LOG_ERR("src_dir");
            exit(EXIT_FAILURE);
        }
        if(strcmp(dest_dir, src_dir) == 0){
            LOG_ERR("add: destination directory cannot be the same as source directory");
            exit(EXIT_FAILURE);
        }
        if(ensure_dir_exists(dest_dir, st.st_mode) == -1){
            LOG_ERR("ensure_dir_exists");
            exit(EXIT_FAILURE);
        } else {
            clear_directory(dest_dir);
        }
        backup_copy(src_dir, dest_dir);
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