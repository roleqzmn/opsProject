#define _GNU_SOURCE

#include <dirent.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <utime.h>

#include "add_lib.h"
#include "copy_lib.h"
#include "string_management.h"

int add(char *src_dir, char *dest_dir, struct backup_record *process)
{
    char *src_dir_real = realpath(src_dir, NULL);
    if (src_dir_real == NULL)
    {
        LOG_ERR("realpath src");
        return -1;
    }
    struct stat st;
    if (stat(src_dir_real, &st) == -1)
    {
        LOG_ERR("stat src");
        free(src_dir_real);
        return -1;
    }
    if (!S_ISDIR(st.st_mode))
    {
        LOG_ERR("source is not a directory");
        free(src_dir_real);
        return -1;
    }
    if (strcmp(dest_dir, src_dir_real) == 0)
    {
        LOG_ERR("destination directory cannot be the same as source directory");
        free(src_dir_real);
        return -1;
    }

    struct stat dest_st;
    int dest_exists = (stat(dest_dir, &dest_st) == 0);

    if (dest_exists)
    {
        if (!S_ISDIR(dest_st.st_mode))
        {
            LOG_ERR("destination exists but is not a directory");
            free(src_dir_real);
            return -1;
        }

        DIR *check_dir = opendir(dest_dir);
        if (check_dir == NULL)
        {
            LOG_ERR("opendir dest_dir");
            free(src_dir_real);
            return -1;
        }
        struct dirent *entry;
        int count = 0;
        while ((entry = readdir(check_dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                count++;
                break;
            }
        }
        closedir(check_dir);

        if (count > 0)
        {
            LOG_ERR("destination directory is not empty");
            free(src_dir_real);
            return -1;
        }
    }
    else
    {
        if (ensure_dir_exists(dest_dir, st.st_mode) == -1)
        {
            LOG_ERR("ensure_dir_exists");
            free(src_dir_real);
            return -1;
        }
    }

    backup_copy(src_dir_real, dest_dir);
    free(src_dir_real);
    process->last_backup = time(NULL);
    return 0;
}

void exit_backup(struct backup_record *head)
{
    struct backup_record *current = head;
    while (current != NULL)
    {
        struct backup_record *temp = current;
        kill(current->pid, SIGTERM);
        usleep(500000);
        if (waitpid(current->pid, NULL, WNOHANG) == 0)
        {
            kill(current->pid, SIGKILL);
            waitpid(current->pid, NULL, 0);
        }
        current = current->next;
        free(temp);
    }
}

void list_backups(struct backup_record *head)
{
    struct backup_record *current = head;
    while (current != NULL)
    {
        printf("Backup PID: %d\n", current->pid);
        printf("Source Path: %s\n", current->src_path);
        printf("Destination Path: %s\n", current->dest_path);
        if (current->ifworking)
        {
            printf("Status: Working\n");
        }
        else
        {
            printf("Status: Not Working. Restore mode only.\n");
        }
        printf("\n");
        current = current->next;
    }
}

void end_backup(char *src_dir, char *dest_dir, struct backup_record **head)
{
    struct backup_record *current = *head;
    while (current != NULL)
    {
        if (strcmp(current->src_path, src_dir) == 0 && strcmp(current->dest_path, dest_dir) == 0)
        {
            kill(current->pid, SIGTERM);
            current->ifworking = false;
            usleep(500000);
            if (waitpid(current->pid, NULL, WNOHANG) == 0)
            {
                kill(current->pid, SIGKILL);
                waitpid(current->pid, NULL, 0);
            }
            printf("Backup process from %s to %s has been terminated.\n", src_dir, dest_dir);
            fflush(stdout);
            return;
        }
        current = current->next;
    }
    printf("No active backup process found from %s to %s.\n", src_dir, dest_dir);
    fflush(stdout);
}

void help()
{
    printf("Available commands:\n");
    printf(
        "add \"<source_directory>\" \"<destination_directory1>\" "
        "\"<destination_directory2>\" ... - Adds directories to backup\n");
    printf(
        "end \"<source_directory>\" \"<destination_directory1>\" "
        "\"<destination_directory2>\" ... - Stops backup processes\n");
    printf(
        "restore \"<source_directory>\" \"<destination_directory>\" - "
        "Restores files from destination to source directory (only if "
        "there exists a backup from source to destination)\n");
    printf("exit - Exits the program, terminating all backup processes\n");
    printf("list - Lists all backups, active and restore-only\n");
    printf("help - You can see right now on the screen\n");
}
