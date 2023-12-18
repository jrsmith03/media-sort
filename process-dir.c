#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>

#include "util.h"

void traverse_directory(char* path_to_start, int log_fd) {
    DIR *dir_open;

    if (dir_open = opendir(path_to_start)) {
        struct dirent *main_dir;
    
        while(main_dir = readdir(dir_open)) {
            DIR *child_dir;
            bool is_dir = false;
            bool not_implicit = (strcmp(main_dir->d_name, ".") != 0) && (strcmp(main_dir->d_name, "..") != 0);
            char *my_path = (char*) malloc(1000);
            strcpy(my_path, path_to_start);
            strcat(my_path, "/\0");
            strcat(my_path, main_dir->d_name);
            if (not_implicit && (child_dir = opendir(my_path))) {
                is_dir = true;
                printf("(parent %s) file %s is a directory\n", path_to_start, my_path);
                traverse_directory(my_path, log_fd);
                // closedir(my_path);
            }
            // printf("DIR ERROR: %s\n", strerror(errno));
            printf("(parent %s) child file name: %s\n", path_to_start, main_dir->d_name);
            // Create a new file descriptor for this file entry without any special arguments.
            // Filter such that we are only reading valid image files
            char *tok = (char*) malloc(3);
            
            if (not_implicit && !is_dir && get_media_extension(my_path, tok)) {
                move_media(my_path, tok, log_fd);
            } 
        } 
    }
}

bool undo_logged_changes(int undo_log_fd) {
    char line_buf_read[300];
    size_t res = 1;
    char *buf[1];
    int i = 0;
    while (res = read(undo_log_fd, buf, 1) == 1) {
        if (*buf != '\n') {
            line_buf_read[i++] = buf;
        } else {
            i = 0;
            char *return_dir = (char*) malloc(300);
            char *undo_dir = (char*) malloc(300);

            // first tok is a deliminator
            strtok(line_buf_read, "$");
            // second tok is the original directory
            return_dir = strtok(line_buf_read, "$");
            // third tok is the new directory
            undo_dir = strtok(line_buf_read, "$");
            // end of line should appear here.

            // rename will move the file back and restore the name
            rename(undo_dir, return_dir);
            // free(return_dir);
            // free(undo_dir);
            
            return true;
            
        }
    }
    return false;
}