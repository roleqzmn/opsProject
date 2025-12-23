#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "lib/add_lib.h"
#include "lib/copy_lib.h"
#include "lib/dir_watcher.h"

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_LINE 4096
#define MAX_ARGS 100

volatile sig_atomic_t should_exit = 0;

static void exit_handler(int sig) { should_exit = 1; }

int ensure_new_backup(char *src_dir, char *dest_dir, struct backup_record **head)
{
    while (*head != NULL)
    {
        if (strcmp((*head)->src_path, src_dir) == 0 && strcmp((*head)->dest_path, dest_dir) == 0)
        {
            if ((*head)->ifworking)
            {
                return -1;
            }
            else
            {
                (*head)->ifworking = true;
                return 1;
            }
        }
        head = &((*head)->next);
    }
    return 0;
}

struct backup_record *find_backup(char *src_dir, char *dest_dir, struct backup_record *head)
{
    while (head != NULL)
    {
        if (strcmp(head->src_path, src_dir) == 0 && strcmp(head->dest_path, dest_dir) == 0)
        {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

int main()
{
    help();
    struct sigaction sa;
    sa.sa_handler = exit_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1)
    {
        ERR("sigaction");
    }

    char line[MAX_LINE];
    char *args[MAX_ARGS];
    struct backup_record *head = NULL;
    printf("> ");
    fflush(stdout);
    while (fgets(line, MAX_LINE, stdin) && !should_exit)
    {
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0)
        {
            printf("\n> ");
            continue;
        }

        char command[PATH_MAX];
        int arg_count = parse_command_line(line, command, args);

        if (arg_count == 0 && strlen(command) == 0)
        {
            printf("\n> ");
            continue;
        }

        if (strcmp(command, "exit") == 0)
        {
            printf("Exiting...\n");
            exit_backup(head);
            break;
        }
        else if (strcmp(command, "add") == 0)
        {
            char *src_dir = args[0];
            for (int j = 1; j < arg_count; j++)
            {
                char *dest_dir=args[j];
                int check = ensure_new_backup(src_dir, dest_dir, &head);
                if (check == -1)
                {
                    printf("Backup from %s to %s already exists\n> ", src_dir, dest_dir);
                    goto print_prompt;
                }
                if (strstr(dest_dir, src_dir) == dest_dir &&
                    (dest_dir[strlen(src_dir)] == '/' || dest_dir[strlen(src_dir)] == '\0'))
                {
                    printf(
                        "Destination directory %s cannot be a subdirectory of source "
                        "directory %s\n> ",
                        dest_dir, src_dir);
                    goto print_prompt;
                }

                struct backup_record *new_record = NULL;

                if (check == 0)
                {  // if doesnt exist creating new
                    new_record = malloc(sizeof(struct backup_record));
                    if (new_record == NULL)
                    {
                        LOG_ERR("malloc");
                        printf("failed to add backup\n> ");
                        goto print_prompt;
                    }
                    new_record->next = head;
                    if (head != NULL)
                        head->prev = new_record;
                    new_record->prev = NULL;
                    new_record->last_backup = 0;
                    head = new_record;
                    new_record->ifworking = true;
                    strncpy(new_record->src_path, src_dir, PATH_MAX);
                    strncpy(new_record->dest_path, dest_dir, PATH_MAX);
                }
                if (check == 1)
                {  // if exists reusing existing
                    new_record = find_backup(src_dir, dest_dir, head);
                    if (new_record == NULL)
                    {
                        LOG_ERR("find_backup");
                        printf("failed to add backup\n> ");
                        goto print_prompt;
                    }
                }
                new_record->pid = fork();

                if (new_record->pid == -1)
                {
                    LOG_ERR("fork");
                    printf("failed to add backup\n> ");
                    if (check == 0)
                    {
                        head = new_record->next;
                        if (head != NULL)
                            head->prev = NULL;
                        free(new_record);
                    }
                    goto print_prompt;
                }
                if (new_record->pid == 0)
                {
                    if (add(src_dir, dest_dir, head) == -1)
                    {
                        exit(EXIT_FAILURE);
                    }
                    watch_directory(src_dir, dest_dir, head);
                    exit(EXIT_SUCCESS);
                }
                else
                {
                    printf("Started backup from %s to %s with PID %d\n", src_dir, dest_dir, new_record->pid);
                    fflush(stdout);
                }
            }
        }
        else if (strcmp(command, "end") == 0)
        {
            char *src_dir = args[0];
            for (int j = 1; j < arg_count; j++)
            {
                char *dest_dir = args[j];
                end_backup(src_dir, dest_dir, &head);
            }
        }
        else if (strcmp(command, "restore") == 0)
        {
            if (arg_count != 2)
            {
                printf("Invalid amount of arguments\n");
                goto print_prompt;
            }
            struct backup_record *current = head;
            char *src_dir = args[0];
            char *dest_dir = args[1];
            while (current != NULL)
            {
                if (strcmp(current->src_path, src_dir) == 0 && strcmp(current->dest_path, dest_dir) == 0)
                {
                    restore_copy(dest_dir, src_dir);
                    break;
                }
                current = current->next;
            }
        }
        else if (strcmp(command, "help") == 0)
        {
            help();
        }
        else if (strcmp(command, "list") == 0)
        {
            list_backups(head);
        }
        else
        {
            printf("Unknown command: %s\n", command);
        }
    print_prompt:
        for (int k = 0; k < arg_count; k++)
            free(args[k]);
        printf("\n> ");
    }
    if (should_exit)
    {
        printf("\nExiting...\n");
        exit_backup(head);
    }
    return EXIT_SUCCESS;
}
