/*
    Issues/Observations
    1. Bad metadata = bad results
        a. some are miscategorized into the current date
    2. Duplicate media should either be ignored or moved to a temp directory
    3. Need to have a log to undo changes. Don't delete people's photos!

    Author: John R. Smith
    Date: December 08, 2023
    Version: 0.01

    This application provides a simple way to sort mass amounts of photo/video/audio files
    based on when the media was created. It also has the ability to mark duplicate files
    (i.e media with an identical timestamp) for deletion or review. 

    Although some applications provide user interfaces to display media in a sorted way,
    the search scope is often limited. This application re-organizes media at the
    directory level, making it easier to further organize/discover images.

    File system changes can be undone by re-executing the utility with the -u
    flag and specifying the 'media-sort.log' file that was generated when the unwanted
    changes were made.

    The general usage is to run 'media-sort' with the '-d (directory)' flag and provide
    the relative path to the directory you want to sort. Changes will be written
    to the 'media-sort' folder, and media will be organized according to the Month and 
    Year the media was last modified. Duplicate files will be located in the /duplicate
    directory within each Month-Year directory.

    Currently, this application is only compatible with Unix systems, but there are
    future plans to expand support to Windows systems.
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>

#define ROOT_DIRNAME_LEN 12
#define MAX_MONTH_LEN 9

// Program imports
bool get_media_extension(char* name, char* buf);
void traverse_directory(char* path_to_start, int log_fd);
char* get_friendly_month(int month);
bool move_media(int fd, char* old_location, char* ext, int log_fd);
bool undo_logged_changes(int undo_log_fd);


void itoa(int num, char* res, int len);


// Parse user arguments, display usage messages given improper input.
int main(int argc, char** argv) {    
    // attempt the open syscall
    // allocate on the stack for now   
    char *DIR_PATH = (char*) malloc(sizeof(char) * 200);
    // The user will specify which directory they wish to put the sorted files in, but for now
    // we will assume it's on working directoy. 
    int log_fd = creat( "log.ps", 0);

    // Scan the command-line arguments for desired flags. If none of hte arguments match flags, 
    // print the usage message.
    for (int i = 1; i < argc; i++) {
        if (i % 2 != 0) {
            if (argc > 2 && strcmp(argv[i], "-d") == 0 && argv[2]) {
                strcpy(DIR_PATH, argv[i+1]);
            } else if (argc > 2 && strcmp(argv[1], "-u") == 0 && argv[i+1]) {
                printf("the file: %s\n", argv[i+1]);
                int undo_fd = open(argv[i+1], O_RDWR);
                
                if (undo_fd > 0 && undo_logged_changes(undo_fd)) {
                    printf("Changes successfully undone.\n");
                    exit(1);
                } else {
                    printf("Invalid undo file argument, please verify the file name and try again.\n");
                    exit(-1);
                }
            } else {
                // DIR_PATH = "test\0";
                printf("usage: photo-sort -d <relative directory path>\n");
                exit(-1);
            }
        }
        
        printf("the first argv: %s\n", argv[i]);
    }
    size_t written = write(log_fd, "start photo-sort\0\n", 19);
    traverse_directory(DIR_PATH, log_fd);
    
}

void traverse_directory(char* path_to_start, int log_fd) {
    DIR *dir_open;

    if (dir_open = opendir(path_to_start)) {
        struct dirent *main_dir;
    
        while(main_dir = readdir(dir_open)) {
            DIR *child_dir;
            bool is_dir = false;
            bool not_implicit = (strcmp(main_dir->d_name, ".") != 0) && (strcmp(main_dir->d_name, "..") != 0);
            char my_path[1000];
            strcpy(my_path, path_to_start);
            strcat(my_path, "/\0");
            strcat(my_path, main_dir->d_name);
            if (not_implicit && (child_dir = opendir(my_path))) {
                is_dir = true;
                printf("(parent %s) file %s is directory\n", path_to_start, my_path);
                traverse_directory(my_path, log_fd);
            }
            printf("(parent %s) child file name: %s\n", path_to_start, main_dir->d_name);
            // Create a new file descriptor for this file entry without any special arguments.
            int fd = open(my_path, 0);
            // Filter such that we are only reading valid image files
            char tok[strlen(my_path)];
            char why[strlen(my_path)];
            strcpy(why, my_path);
            
            if (not_implicit && !is_dir && tok && get_media_extension(why, tok)) {
                move_media(fd, my_path, tok, log_fd);
            } 
        } 
    }
}

/* 
    Helper to break a given file name into the file extension and determine
    if it is a media (i.e image, video) file. Return true if so.
*/
bool get_media_extension(char* name, char* ext) {
    char *tok = (char*) malloc(strlen(name));
    printf("name: %s\n", name);
    tok = strtok(name, ".");
    if (!tok) return false;
    tok = strtok(NULL, ".");
    if (!tok) return false;
    // todo: refine this logic 
    if (strcmp(tok, "jpg") == 0 || strcmp(tok, "png") == 0 || strcmp(tok, "JPG") == 0 || 
        strcmp(tok, "bmp") == 0 || strcmp(tok, "mov") == 0 || strcmp(tok, "mp4") == 0 ||
        strcmp(tok, "mpg") == 0) {
        strcpy(ext, tok);
        return true;
    }
    return false;
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
    if (!opendir(ROOT_DIRNAME)) mkdir(ROOT_DIRNAME, 0700);
    
    // create the new directory
    if (!opendir(friendly_dirname)) mkdir(friendly_dirname, 0700);
}

bool move_media(int fd, char* old_location, char* ext, int log_fd) {
    // Get a tm struct that identifies when the media was modified
    struct stat my_stat;
    fstat(fd, &my_stat);
    struct tm *x;
    x = localtime(&my_stat.st_mtime);
    char new_name[100];
    int dup_num = 0;
    
    // todo: this is god awful
    char dir[160];
    char subdir[160];
    create_dir(x->tm_year + 1900, x->tm_mon + 1, dir, subdir);

    strcpy(new_name, dir);
    strcat(new_name, "/");
    char file_rename[150];
    get_file_rename(file_rename, subdir, x);
    strcat(new_name, file_rename);
    strcat(new_name, ".");
    strcat(new_name, ext);

    printf("NEW FILE NAME: %s\n", new_name);
    
    // We are using the at2 variant of rename so that we can handle duplicate metadata
    // The worst thing we can do is delete data!

 
    if (open(new_name, O_RDWR) > 0) {
        // Given that we have duplicates, we need to read the log to see how many already exist.

        char dup_name[150];
        printf("Duplicate file detected!\n");
        strcpy(dup_name, dir);
        strcat(dup_name, "/duplicate");
        if (!opendir(dup_name)) mkdir(dup_name, 0700);
        int dup_log_fd = open("dup_log.ps", O_RDWR);



        
        strcat(dup_name, "/");
        strcat(dup_name, file_rename);
        strcat(dup_name, "_");
        char dup_char[dup_num];
        itoa(dup_num++, dup_char, 3);
        strcat(dup_name, dup_char);
        strcat(dup_name, ".");
        strcat(dup_name, ext);
        rename(old_location, dup_name);
        char log_entry[150];
        strcpy(log_entry, "MOVE $ ");
        strcat(log_entry, old_location);
        strcat(log_entry, " $ ");
        strcat(log_entry, dup_name);
        strcat(log_entry, "\n");
        write(log_fd, log_entry, strlen(log_entry));
    } else {
        char log_entry[150];
        strcpy(log_entry, "MOVE $ ");
        strcat(log_entry, old_location);
        strcat(log_entry, " $ ");
        strcat(log_entry, new_name);
        strcat(log_entry, "\n");
        write(log_fd, log_entry, strlen(log_entry));
        rename(old_location, new_name);
    }
}

void get_file_rename(char* file_rename, char* subdir, struct tm *time) {
    char day[50];
    itoa(time->tm_mday, day, 2);
    char hour[50];
    itoa(time->tm_hour, hour, 2);
    char min[50];
    itoa(time->tm_min, min, 2);
    char sec[50];
    itoa(time->tm_sec, sec, 2);
    strcpy(file_rename, subdir);
    strcat(file_rename, "_");
    strcat(file_rename, day);
    strcat(file_rename, "_");
    strcat(file_rename, hour);
    strcat(file_rename, ":");
    strcat(file_rename, min);
    strcat(file_rename, ":");
    strcat(file_rename, sec);

}

bool undo_logged_changes(int undo_log_fd) {
    char line_buf_read[300];
    size_t res = 1;
    char c;
    int i = 0;
    while (res = read(undo_log_fd, c, 1) == 1) {
        if (c != '\n') {
            line_buf_read[i++] = c;
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
            return (rename(undo_dir, return_dir) > 0);
            
        }
    }
    return false;
}
char* get_friendly_month(int month) {
    switch (month) {
        case 1 : 
            return "January";
        case 2 : 
            return "February";
        case 3 : 
            return "March";
        case 4 : 
            return "April";
        case 5 : 
            return "May";
        case 6 : 
            return "June";
        case 7 : 
            return "July";
        case 8 : 
            return "August";
        case 9 : 
            return "September";
        case 10 : 
            return "October";
        case 11 : 
            return "November";
        case 12 : 
            return "December";
        default :
            return "Month";
    }
}


void itoa(int num, char* res, int len) {
    int i = 0;
    res[len] = '\0';
    res[--len] = num % 10 + '0';
    num /= 10;
    while (len > 0) {
        res[--len] = num % 10 + '0';
        num /= 10;
    }
}

