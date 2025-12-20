    #define _POSIX_C_SOURCE 200809L

    #include <add_lib.h>
    #include <signal.h>     
    #include <sys/types.h>
    #include <copy_lib.h>


    void add(int argc, char** argv){
        if(argc < 2){
            LOG_ERR("argc");
            return;
        }

        if(argv[0] == NULL){
            LOG_ERR("argv[0] is NULL");
            return;
        }

        const char* src_dir = argv[0];
        struct stat st;
        if(stat(src_dir, &st) == -1 || (stat(src_dir, &st) == 0 && !S_ISDIR(st.st_mode))){
            LOG_ERR("src_dir");
            return;
        }
        char* dest_dirs[argc-1];
        for(int i = 1; i < argc; i++){
            dest_dirs[i-1] = argv[i];
            if(dest_dirs[i-1] == NULL) continue;
            if(strcmp(dest_dirs[i-1], src_dir) == 0){
                if(argc <=3) return;
                LOG_ERR("add: destination directory cannot be the same as source directory");
                dest_dirs[i-1] = NULL;
                continue;
            }
            if(ensure_dir_exists(dest_dirs[i-1], st.st_mode) == -1){
                LOG_ERR("ensure_dir_exists");
                dest_dirs[i-1] = NULL;
            } else {
                clear_directory(dest_dirs[i-1]);
            }
        }
        backup_copy(src_dir, dest_dirs, argc-1);
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
            printf("Destination Paths:");
            for(int i = 0; i < MAX_ARGS && current->dest_paths[i] != NULL; i++){
                printf("  %s", current->dest_paths[i]);
            }
            printf("\n");
            current = current->next;
        }
    }