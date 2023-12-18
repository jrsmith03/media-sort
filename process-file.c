#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>

#include "util.h"
#include "exif-util.h"
bool move_media(char* old_location, char* ext, int log_fd) {
    // Get a tm struct that identifies when the media was modified
    bool is_jpg = strncmp(ext, "JPG", 3) == 0 || strncmp(ext, "jpg", 3) == 0;
    struct stat my_stat;
    struct jpg_time *my_jpg_time;
    char new_name[100];
    char dir[160];
    char subdir[160];
    char file_rename[150];

    if (is_jpg && (my_jpg_time = get_jpg_time(old_location))) {        
        create_dir(atoi(my_jpg_time->year_s), atoi(my_jpg_time->month_s), dir, subdir);
        get_file_rename(true, file_rename, subdir, NULL, my_jpg_time);

    } else {
        stat(old_location, &my_stat);
        struct tm *x;
        x = localtime(&my_stat.st_mtime);
        if (x) {
            create_dir(x->tm_year + 1900, x->tm_mon + 1, dir, subdir);

        }
        get_file_rename(false, file_rename, subdir, x, NULL);

    }
    // todo: this is god awful

    strncpy(new_name, dir, strlen(dir) + 1);
    strcat(new_name, "/");
    strncat(new_name, file_rename, strlen(file_rename) + 1);
    strcat(new_name, ".");
    strncat(new_name, ext, 3);

    printf("NEW FILE NAME: %s\n", new_name);
    
    // // We are using the at2 variant of rename so that we can handle duplicate metadata
    // // The worst thing we can do is delete data!
    add_new_file(file_rename, dir, ext, old_location, new_name);
}


void create_dir(int year, int month, char* friendly_dirname, char* friendly_subdir) {
    char* ROOT_DIRNAME = (char*) malloc(ROOT_DIRNAME_LEN); 
    ROOT_DIRNAME = "photo-sort\0";
    char *friendly_month = (char*) malloc(MAX_MONTH_LEN); 
    friendly_month = get_friendly_month(month);
    char *friendly_year = (char*) malloc(4);
    itoa(year, friendly_year, 4);
    strcpy(friendly_dirname, ROOT_DIRNAME);
    strcat(friendly_dirname, "/");
    strcpy(friendly_subdir, friendly_year);
    strcat(friendly_subdir, "-");
    strcat(friendly_subdir, friendly_month);
    strcat(friendly_dirname, friendly_subdir);

    // create the root directory
    DIR* canary = opendir(ROOT_DIRNAME);
    if (!canary) {
        mkdir(ROOT_DIRNAME, 0700);
        printf("mkDIR ERROR: %s\n", strerror(errno));
        
    } else {
        closedir(canary);
    }
    
    DIR* canary2 = opendir(friendly_dirname);
    // create the new directory
    // printf("openDIR ERROR: %s\n", strerror(errno));

    if (!canary2) {
        mkdir(friendly_dirname, 0700);
        // printf("mkDIR ERROR: %s\n", strerror(errno));

    } else {
        closedir(canary);
    }
}

void get_file_rename(bool is_jpg, char* file_rename, char* subdir, struct tm *time, 
                        struct jpg_time *time_jpg) {
    strcpy(file_rename, subdir);
    strcat(file_rename, "_");

    if (!is_jpg) {
        char day[50];
        itoa(time->tm_mday, day, 2);
        char hour[50];
        itoa(time->tm_hour, hour, 2);
        char min[50];
        itoa(time->tm_min, min, 2);
        char sec[50];
        itoa(time->tm_sec, sec, 2);
        
        strcat(file_rename, day);
        strcat(file_rename, "_");
        strcat(file_rename, hour);
        strcat(file_rename, ":");
        strcat(file_rename, min);
        strcat(file_rename, ":");
        strcat(file_rename, sec);
    } else {
        strncat(file_rename, time_jpg->day_s, 2);
        strcat(file_rename, "_");
        strncat(file_rename, time_jpg->time_s, 8);
    }

}

void add_new_file(char* file_rename, char* dir, char* ext, char* old_location, 
                    char* file_name) {
     // Given that we have duplicates, we need to read the log to see how many already exist.
        int fd;
        if (fd = open(file_name, O_RDWR) > 0) {
            char dup_name[150];
            strcpy(dup_name, dir);

            strcat(dup_name, "/");
            strcat(dup_name, file_rename);
            strcat(dup_name, "$");
            // Find the ID of the last duped file, since this unfortunately happens a lot.
            char* test = (char*) malloc(100);

            test = strtok(file_name, "$");
            if (test) {
                test = strtok(NULL, "$");
            }
            // test should be the strings after the last occurance of test
            int last_dup = test ? atoi(test) : 0;
            // todo: support the longer numbers
            char dup_char[4];
            itoa(++last_dup, dup_char, 3);
            strcat(dup_name, dup_char);
            strcat(dup_name, ".");
            strncat(dup_name, ext, 3);
            // Now that we've generated a new directory name,
            // we recursively run the function again to see if that duplicate name exists.
            // If so, we are going to increment the duplicate number, rerun, and write.
            close(fd);
            // free(test);
            add_new_file(file_rename, dir, ext, old_location, dup_name);
            // rename(old_location, dup_name);
            // char log_entry[150];
            // strcpy(log_entry, "MOVE $ ");
            // strcat(log_entry, old_location);
            // strcat(log_entry, " $ ");
            // strcat(log_entry, dup_name);
            // strcat(log_entry, "\n");
            // write(log_fd, log_entry, strlen(log_entry));
        } else {
            // char log_entry[150];
            // strcpy(log_entry, "MOVE $ ");
            // strcat(log_entry, old_location);
            // strcat(log_entry, " $ ");
            // strcat(log_entry, new_name);
            // strcat(log_entry, "\n");
            // write(log_fd, log_entry, strlen(log_entry));
            rename(old_location, file_name);
            close(fd);
        }
    
}

/* 
    Helper to break a given file name into the file extension and determine
    if it is a media (i.e image, video) file. Return true if so.
*/
bool get_media_extension(char* name, char* ext) {
    char *tok = (char*) malloc(strlen(name) + 1);
    char cpy[strlen(name) + 1];

    for (int i = 0; i < strlen(name); i++) {
        cpy[i] = name[i];
    }

    tok = strtok(cpy, ".");
    if (!tok) return false;
    tok = strtok(NULL, ".");

    for (int i = 0; i < 3; i++) {
        ext[i] = tok[i];
    }
    ext[3] = '\0';
    if (!tok) return false;
    // todo: refine this logic 
    if (strncmp(ext, "jpg", 3) == 0 || strncmp(ext, "png", 3) == 0 || strncmp(ext, "JPG", 3) == 0 || 
        strncmp(ext, "bmp", 3) == 0 || strncmp(ext, "mov", 3) == 0 || strncmp(ext, "mp4", 3) == 0 ||
        strncmp(ext, "mpg", 3) == 0 || strncmp(ext, "MOV", 3) == 0 || strncmp(ext, "AVI", 3) == 0) {
        // free(tok);
        return true;
    }
    // free(tok);
    return false;
}