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
    char *token;
    struct backup_record* head = NULL;
    printf("> ");
    fflush(stdout);
    while(fgets(line, MAX_LINE, stdin) && !should_exit){
        
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) {
            printf("\n> ");
            continue;
        }

        token=strtok(line, " ");
        if(token==NULL){
            printf("\n> ");
            continue;
        }
        
        char* command = token;
        int i=0;
        while(token!=NULL && i < MAX_ARGS-1){
            strtok(NULL, "\"");
            token=strtok(NULL, "\"");
            args[i]=token;
            i++;
            
        }
        args[i]=NULL;
        if(strcmp(command, "exit")==0){
            printf("Exiting...\n");
            exit_backup(head);
            break;
        }
        else if(strcmp(command, "add")==0){
            char* src_dir = args[0];
            for(int j=1; j<i-1; j++){
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

                int pipefd[2];
                if (pipe(pipefd) == -1) {
                    ERR("pipe");
                }
                if(check==0){
                    new_record->next = head;
                    if(head != NULL)
                        head->prev = new_record;
                    new_record->prev = NULL;
                    new_record->last_backup = 0;
                    head = new_record;
                    new_record->pipe_fd = pipefd[0];
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
                    new_record->pipe_fd = pipefd[0];
                }
                new_record->pid = fork();

                if(new_record->pid == -1){
                    LOG_ERR("fork");
                    printf("failed to add backup\n> ");
                    head = new_record->next;
                    free(new_record);
                    close(pipefd[1]);
                    continue;
                }
                if(new_record->pid == 0){ 
                    close(pipefd[0]);
                    if(add(src_dir, args[j], head)== -1){
                        exit(EXIT_FAILURE);
                    }
                    watch_directory(src_dir, args[j], head, pipefd[1]);
                    close(pipefd[1]);
                    exit(EXIT_SUCCESS);
                }else {
                    close(pipefd[1]);
                }
            }
        }
        else if(strcmp(command, "end")==0){
            char* src_dir = args[0];
            for(int j=1; j<i-1; j++){
                char* dest_dir = args[j];
                end_backup(src_dir, dest_dir, &head);
            }
        }
        else if(strcmp(command, "restore")==0){
            printf("%d", i);
            fflush(stdout);
            if(i != 3){
                printf("Invalid ammount of arguments\n> ");
                continue;
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
        printf("\n> ");
    }
    if(should_exit){
        printf("\nExiting...\n");
        exit_backup(head);
    }
    return EXIT_SUCCESS;
}
