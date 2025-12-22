#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

#include "add_lib.h"
#include "copy_lib.h"
#include "string_management.h"

int ensure_dir_exists(const char *path, mode_t mode) {
  struct stat _st;
  if (mode == 0) {
    mode = 0755;
  }
  if (stat(path, &_st) == 0) {
    if (!S_ISDIR(_st.st_mode)) {
      LOG_ERR("ensure_dir_exists: path exists but is not a directory");
      return -1;
    }
    return 0;
  }
  if (errno != ENOENT) {
    LOG_ERR("ensure_dir_exists stat");
    return -1;
  }

  char *path_cp = strdup(path);
  if (path_cp == NULL) {
    LOG_ERR("ensure_dir_exists strdup");
    return -1;
  }
  char *parent_dir = dirname(path_cp);
  if (strcmp(parent_dir, ".") != 0 && strcmp(parent_dir, "/") != 0) {
    if (ensure_dir_exists(parent_dir, mode) == -1) {
      free(path_cp);
      return -1;
    }
  }

  free(path_cp);

  if (mkdir(path, mode) == -1 && errno != EEXIST) {
    LOG_ERR("ensure_dir_exists mkdir");
    return -1;
  }
  return 0;
}

void clear_directory(const char *path) {
  DIR *dir = opendir(path);
  if (dir == NULL) {
    LOG_ERR("clear_directory opendir");
    return;
  }
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    char *src_path = file_path(path, entry->d_name);
    if (src_path == NULL) {
      continue;
    }

    struct stat st;
    if (lstat(src_path, &st) == -1) {
      free(src_path);
      continue;
    }
    if (S_ISDIR(st.st_mode)) {
      clear_directory(src_path);
      if (rmdir(src_path) == -1) {
        LOG_ERR("clear_directory rmdir");
      }
    } else if (S_ISREG(st.st_mode)) {
      if (remove(src_path) == -1) {
        LOG_ERR("clear_directory remove file");
      }
    } else if (S_ISLNK(st.st_mode)) {
      if (unlink(src_path) == -1) {
        LOG_ERR("clear_directory unlink symlink");
      }
    }
    free(src_path);
  }
  closedir(dir);
}

void copy_file(const char *src_path, const char *dest_path) {
  struct stat st;
  if (stat(src_path, &st) == -1) {
    LOG_ERR("stat copy_file");
    return;
  }

  FILE *src = fopen(src_path, "rb");
  if (src == NULL) {
    LOG_ERR("fopen src copy_file");
    return;
  }

  FILE *dest = fopen(dest_path, "wb");
  if (dest == NULL) {
    LOG_ERR("fopen dest copy_file");
    fclose(src);
    return;
  }

  char buffer[4096];
  size_t bytes;
  while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
    fwrite(buffer, 1, bytes, dest);
  }

  fclose(src);
  fclose(dest);

  if (chmod(dest_path, st.st_mode) == -1) {
    LOG_ERR("chmod copy_file");
  }

  struct utimbuf times = {.actime = st.st_atime, .modtime = st.st_mtime};
  if (utime(dest_path, &times) == -1) {
    LOG_ERR("utime copy_file");
  }
}

void copy_symlink(const char *src_dir, const char *src_path,
                  const char *dest_path) {
  char link_target[PATH_MAX];
  ssize_t len = readlink(src_path, link_target, sizeof(link_target) - 1);
  if (len == -1) {
    LOG_ERR("readlink");
    return;
  }
  link_target[len] = '\0';
  char *same_folder_target = strstr(link_target, src_dir);
  if (same_folder_target != NULL) {
    char new_target[PATH_MAX];
    snprintf(new_target, sizeof(new_target), "%s%s", dest_path,
             same_folder_target + strlen(src_dir));
    strcpy(link_target, new_target);
  }
  unlink(dest_path);

  if (symlink(link_target, dest_path) == -1) {
    LOG_ERR("symlink");
  }
}

void backup_copy(const char *src_dir, char *dest_dir) {
  DIR *dir = opendir(src_dir);
  if (dir == NULL) {
    LOG_ERR("opendir");
    return;
  }
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    char *src_path = file_path(src_dir, entry->d_name);
    if (src_path == NULL) {
      continue;
    }
    char *dest_path = file_path(dest_dir, entry->d_name);
    if (dest_path == NULL) {
      continue;
    }

    struct stat st;
    if (lstat(src_path, &st) == -1) {
      free(src_path);
      LOG_ERR("lstat(may be faulty link in src)");
      continue;
    }
    if (S_ISDIR(st.st_mode)) {
      if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
        if (ensure_dir_exists(dest_path, st.st_mode) == -1) {
          free((void *)dest_path);
          continue;
        }
        backup_copy(src_path, dest_path);
      }
    } else if (S_ISREG(st.st_mode)) {
      copy_file(src_path, dest_path);
    } else if (S_ISLNK(st.st_mode)) {
      copy_symlink(src_dir, src_path, dest_path);
    }
    free(dest_path);
    free(src_path);
  }
  closedir(dir);
}

void restore_copy(const char *src_dir, char *dest_dir) {
  DIR *dir = opendir(src_dir);
  if (dir == NULL) {
    LOG_ERR("opendir");
    return;
  }
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    char *backup_path = file_path(src_dir, entry->d_name);
    if (backup_path == NULL) {
      continue;
    }
    char *dest_path = file_path(dest_dir, entry->d_name);

    struct stat backup_st;
    if (lstat(backup_path, &backup_st) == -1) {
      free(backup_path);
      if (dest_path)
        free(dest_path);
      LOG_ERR("lstat(may be faulty link in src)");
      continue;
    }
    struct stat dest_st;
    bool nofile;
    if (dest_path == NULL) {
      nofile = true;
    } else {
      nofile = (lstat(dest_path, &dest_st) == -1);
      if (nofile && errno != ENOENT) {
        free(backup_path);
        free(dest_path);
        LOG_ERR("lstat dest");
        continue;
      }
    }
    if (dest_path == NULL) {
      free(backup_path);
      continue;
    }
    if (S_ISDIR(backup_st.st_mode) &&
        (nofile || (!nofile && dest_st.st_mtime > backup_st.st_mtime))) {
      if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
        if (ensure_dir_exists(dest_path, backup_st.st_mode) == -1) {
          free(backup_path);
          free((void *)dest_path);
          continue;
        }
        restore_copy(backup_path, dest_path);
      }
    } else if (S_ISREG(backup_st.st_mode) &&
               (nofile || (!nofile && dest_st.st_mtime > backup_st.st_mtime))) {
      copy_file(backup_path, dest_path);
    } else if (S_ISLNK(backup_st.st_mode) &&
               (nofile || (!nofile && dest_st.st_mtime > backup_st.st_mtime))) {
      copy_symlink(src_dir, backup_path, dest_path);
    }
    free(dest_path);
    free(backup_path);
  }
  closedir(dir);
  DIR *org_dir = opendir(dest_dir);
  if (org_dir == NULL) {
    LOG_ERR("opendir");
    return;
  }
  struct dirent *org_entry;
  while ((org_entry = readdir(org_dir)) != NULL) {
    char *backup_path = file_path(src_dir, org_entry->d_name);
    if (backup_path == NULL) {
      continue;
    }
    char *dest_path = file_path(dest_dir, org_entry->d_name);
    if (dest_path == NULL) {
      free(backup_path);
      continue;
    }

    struct stat backup_st;
    if (lstat(backup_path, &backup_st) == -1) {
      struct stat dest_st;
      if (lstat(dest_path, &dest_st) == -1) {
        free(backup_path);
        free(dest_path);
        continue;
      }
      if (S_ISDIR(dest_st.st_mode)) {
        clear_directory(dest_path);
        if (rmdir(dest_path) == -1) {
          LOG_ERR("rmdir during restore");
        }
      } else {
        if (remove(dest_path) == -1) {
          LOG_ERR("remove during restore");
        }
      }
      free(backup_path);
      free(dest_path);
      continue;
    }

    free(backup_path);
    free(dest_path);
  }
  closedir(org_dir);
}
