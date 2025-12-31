 Interactive Backup Management System

An automated system designed to manage directory backups in a Linux environment. The system allows for real-time monitoring of source directories and mirrors changes across multiple target locations using parallel subprocesses.

Features:


Interactive Command Line: After starting, the program prints available commands and waits for user instructions.

Parallel Execution: Each target directory is managed by an independent subprocess, providing parallel execution and high responsiveness.

Real-Time Monitoring: Uses the inotify API to immediately mirror changes in files, symbolic links, and subfolders.

Smart Symlink Handling: If a symbolic link contains an absolute path leading to the source directory, the backup creates a symlink pointing to the relevant file in the target directory. In all other cases, the path in the link is copied unchanged.

Safety Checks: Prevents creating a backup of a directory inside itself to avoid infinite recursion. Prevents creating multiple backups with the same source and target paths.

Differential Restoration: The restore command copies the directory structure back to the source, copying only files that have changed and deleting files not present in the backup.


Commands:
```
add <src> <target1> <target2>...
```
Starts a backup of the source folder to one or more target locations.
```
end <src> <target1>...
```
Stops backup in the given target directories; contents remain intact.
```
list
```
Outputs a summary showing which folders have backups and where they are saved.
```
restore <src> <target>
```
Restores the selected backup to the source in blocking mode.
```
exit
```
Terminates the program, frees resources, and terminates all child processes.
```
help
```
Displays the list of available commands and their usage.


Technical Details:


Platform: Linux.

Key APIs: inotify for monitoring, fork for parallelism, and realpath for path comparison.

Signal Handling: Correctly exits in response to SIGINT and SIGTERM while ignoring other signals.

Path Management: Supports directory names with spaces through quote handling.


Compilation and Execution:
The project includes a Makefile for easy compilation.
To compile the project:
```
make
```
To run the program:
```
./sop-backup
```
To clean build files:
```
make clean
```


Project Structure:

sop-backup.c: Main entry point and command loop handler.

add_lib.c/h: Logic for adding, listing, and ending backup processes.

copy_lib.c/h: Core logic for file copying, symlink handling, and directory clearing.

dir_watcher.c/h: Implementation of recursive directory watching using inotify.

string_management.c/h: Utilities for command line parsing and path string manipulation.

