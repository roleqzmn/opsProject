#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include "lib/add_lib.h"
#include <string.h>
#include "lib/dir_watcher.h"
#include "lib/copy_lib.h"
#include <signal.h>
#include <ctype.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_LINE 4096
#define MAX_ARGS 100

volatile sig_atomic_t should_exit = 0;

static void exit_handler(int sig) {
    should_exit = 1;
}

int ensure_new_backup(char* src_dir, char* dest_dir, struct backup_record** head){
    while(*head != NULL){
        if(strcmp((*head)->src_path, src_dir) == 0 && strcmp((*head)->dest_path, dest_dir) == 0){
            if((*head)->ifworking){
                return -1; 
            }else{
                (*head)->ifworking=true;
                return 1;
            }
        }
        head = &((*head)->next);
    }
    return 0;
}

struct backup_record* find_backup(char* src_dir, char* dest_dir, struct backup_record* head){
    while(head != NULL){
        if(strcmp(head->src_path, src_dir) == 0 && strcmp(head->dest_path, dest_dir) == 0){
            return head;
        }
        head = head->next;
    }
    return NULL;
}

int main()
{
    struct sigaction sa;
    sa.sa_handler = exit_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1 ||
        sigaction(SIGTERM, &sa, NULL) == -1) {
        ERR("sigaction");
    }

    char line[MAX_LINE];
    char *args[MAX_ARGS];
    struct backup_record* head = NULL;
    printf("> ");
    fflush(stdout);
    while(fgets(line, MAX_LINE, stdin) && !should_exit){
        
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) {
            printf("\n> ");
            continue;
        }

        size_t pos = 0;
        size_t line_len = strlen(line);
        while (pos < line_len && line[pos] == ' ') pos++;
        size_t cmd_start = pos;
        while (pos < line_len && line[pos] != ' ') pos++;
        char command[PATH_MAX];
        strncpy(command, line + cmd_start, pos - cmd_start);
        command[pos - cmd_start] = '\0';
        
        if (strlen(command) == 0) {
            printf("\n> ");
            continue;
        }
        
        int arg_count = 0;
        while (pos < line_len && arg_count < MAX_ARGS) {
            while (pos < line_len && line[pos] == ' ') pos++;
            if (pos >= line_len) break;
            
            char arg_buf[PATH_MAX];
            int arg_idx = 0;
            
            if (line[pos] == '"') {
                pos++;
                while (pos < line_len && line[pos] != '"') {
                    arg_buf[arg_idx++] = line[pos];
                    pos++;
                }
                if (pos < line_len) pos++;
            } else {
                while (pos < line_len && line[pos] != ' ') {
                    arg_buf[arg_idx++] = line[pos];
                    pos++;
                }
            }
            arg_buf[arg_idx] = '\0';
            args[arg_count] = strdup(arg_buf);
            arg_count++;
        }
        
        if(strcmp(command, "exit")==0){
            printf("Exiting...\n");
            exit_backup(head);
            break;
        }
        else if(strcmp(command, "add")==0){
            char* src_dir = args[0];
            for(int j=1; j<arg_count; j++){
                int check = ensure_new_backup(src_dir, args[j], &head);
                if(check == -1){
                    printf("Backup from %s to %s already exists\n> ", src_dir, args[j]);
                    continue;
                }
                if(strstr(args[j], src_dir) == args[j]){
                    printf("Destination directory %s cannot be a subdirectory of source directory %s\n> ", args[j], src_dir);
                    continue;
                }
                struct backup_record* new_record = malloc(sizeof(struct backup_record));
                if(new_record == NULL){
                    LOG_ERR("malloc");
                    printf("failed to add backup\n> ");
                    continue;
                }

                if(check==0){
                    new_record->next = head;
                    if(head != NULL)
                        head->prev = new_record;
                    new_record->prev = NULL;
                    new_record->last_backup = 0;
                    head = new_record;
                    new_record->ifworking = true;
                    strncpy(new_record->src_path, src_dir, PATH_MAX);
                    strncpy(new_record->dest_path, args[j], PATH_MAX);
                }
                if(check==1){
                    struct backup_record* existing = find_backup(src_dir, args[j], head);
                    if(existing == NULL){
                        LOG_ERR("find_backup");
                        printf("failed to add backup\n> ");
                        continue;
                    }
                    new_record = existing;
                }
                new_record->pid = fork();

                if(new_record->pid == -1){
                    LOG_ERR("fork");
                    printf("failed to add backup\n> ");
                    head = new_record->next;
                    free(new_record);
                    continue;
                }
                if(new_record->pid == 0){ 
                    if(add(src_dir, args[j], head)== -1){
                        exit(EXIT_FAILURE);
                    }
                    watch_directory(src_dir, args[j], head);
                    exit(EXIT_SUCCESS);
                }else {
                    printf("Started backup from %s to %s with PID %d\n", src_dir, args[j], new_record->pid);
                    fflush(stdout);
                }
            }
        }
        else if(strcmp(command, "end")==0){
            char* src_dir = args[0];
            for(int j=1; j<arg_count; j++){
                char* dest_dir = args[j];
                end_backup(src_dir, dest_dir, &head);
            }
        }
        else if(strcmp(command, "restore")==0){
            if(arg_count != 2){
                printf("Invalid amount of arguments\n");
                goto print_prompt;
            }
            struct backup_record* current = head;
            char* src_dir = args[0];
            char* dest_dir = args[1];
            while(current != NULL){
                if(strcmp(current->src_path, src_dir) == 0 && strcmp(current->dest_path, dest_dir) == 0){
                    restore_copy(dest_dir, src_dir);
                    break;
                }
                current = current->next;
            }
        }
        else if(strcmp(command, "help")==0){
            printf("Available commands:\n");
            printf("add \"<source_directory>\" \"<destination_directory1>\" \"<destination_directory2>\" ... - Adds directories to backup\n");
            printf("end \"<source_directory>\" \"<destination_directory1>\" \"<destination_directory2>\" ... - Stops backup processes\n");
            printf("restore \"<source_directory>\" \"<destination_directory>\" - Restores files from destination to source directory (only if there exists a backup from source to destination)\n");
            printf("exit - Exits the program, terminating all backup processes\n");
            printf("list - Lists all backups, active and restore-only\n");
            printf("help - You can see right now on the screen\n");       
        }
        else if(strcmp(command, "list")==0){
            list_backups(head);
        }
        else{
            printf("Unknown command: %s\n", command);
        }
        print_prompt:
        for(int k=0; k<arg_count; k++) free(args[k]);
        printf("\n> ");
    }
    if(should_exit){
        printf("\nExiting...\n");
        exit_backup(head);
    }
    return EXIT_SUCCESS;
}
