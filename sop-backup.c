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

char* expand_tilde(const char* path) {
    if (path && path[0] == '~' && path[1] == '/') {
        const char* home = getenv("HOME");
        if (home) {
            size_t home_len = strlen(home);
            size_t path_len = strlen(path);
            char* expanded = malloc(home_len + path_len); // +1 for null, but path has /, so ok
            if (expanded) {
                strcpy(expanded, home);
                strcpy(expanded + home_len, path + 1);
                return expanded;
            }
        }
    }
    return path ? strdup(path) : NULL;
}



int main()
{
    char line[MAX_LINE];
    char *args[MAX_ARGS];
    char *token;
    struct backup_record* head = NULL;
    printf("> ");
    while(fgets(line, MAX_LINE, stdin)){
        fflush(stdout);
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
            token=strtok(NULL, " ");
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
            // Expand ~ in paths
            for(int j = 0; j < i; j++){
                if(args[j]){
                    char* expanded = expand_tilde(args[j]);
                    if(expanded != args[j]){ // if expanded, free old and set new
                        free(args[j]);
                        args[j] = expanded;
                    }
                }
            }
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
        else if(strcmp(command, "help")==0){
            printf("Available commands:\n");
            printf("add <source_directory> <destination_directory1> <destination_directory2> ... - Adds a directory to backup\n");
            printf("exit - Exits the program, terminating all backup processes\n");
            printf("list - Lists all current backups\n");
            printf("help - U can see rn on screen\n");       
        }
        else if(strcmp(command, "list")==0){
            list_backups(head);
        }
        else{
            printf("Unknown command: %s\n", command);
        }
        printf("\n> ");
    }
    return EXIT_SUCCESS;
}