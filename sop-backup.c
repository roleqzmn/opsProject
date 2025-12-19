#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <add_lib.h>
#include <string.h>
#include <add_lib.h>


#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_LINE 4096
#define MAX_ARGS 100



int main()
{
    char line[MAX_LINE];
    char *args[MAX_ARGS];
    char *token;
    struct backup_record* head = NULL;
    printf("> ");
    while(fgets(line, MAX_LINE, stdin)){
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
            args[i]=token;
            token=strtok(NULL, " ");
            i++;
        }
        args[i]=NULL;
        if(strcmp(token, "exit")==0){
            printf("Exiting...\n");
            exit_backup(head);
            break;
        }
        else if(strcmp(token, "add")==0){
            struct backup_record* new_record = malloc(sizeof(struct backup_record));
            if(new_record == NULL){
                LOG_ERR("malloc");
                printf("failed to add backup\n> ");
                continue;
            }
            new_record->next = head;
            head = new_record;
            new_record->pid = fork();
            if(new_record->pid == -1){
                LOG_ERR("fork");
                printf("failed to add backup\n> ");
                head = new_record->next;
                free(new_record);
                continue;
            }
            if(new_record->pid == 0){ 
                add(i, args);
            }
        }
        else if(strcmp(token, "help")==0){
            printf("Available commands:\n");
            printf("add <source_directory> <destination_directory1> <destination_directory2> ... - Adds a directory to backup\n");
            printf("exit - Exits the program, terminating all backup processes\n");
            printf("help - Displays this help message\n");
        }
        else{
            printf("Unknown command: %s\n", command);
        }
        printf("\n> ");
    }
    return EXIT_SUCCESS;
}