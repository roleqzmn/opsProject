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
            char* src_dir = args[0];
            for(int j=1; j<i-1; j++){
                struct backup_record* new_record = malloc(sizeof(struct backup_record));
                if(new_record == NULL){
                    LOG_ERR("malloc");
                    printf("failed to add backup\n> ");
                    continue;
                }
                new_record->next = head;
                head = new_record;
                strncpy(new_record->src_path, src_dir, PATH_MAX);
                strncpy(new_record->dest_path, args[j], PATH_MAX);
                new_record->pid = fork();
                if(new_record->pid == -1){
                    LOG_ERR("fork");
                    printf("failed to add backup\n> ");
                    head = new_record->next;
                    free(new_record);
                    continue;
                }
                if(new_record->pid == 0){ 
                    add(src_dir, args[j]);
                }
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