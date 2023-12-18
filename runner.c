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

#include <stdio.h>
#include <fcntl.h>

#include "util.h"
#include "exif-util.h"

// Parse user arguments, display usage messages given improper input.
int main(int argc, char** argv) {    
    char *DIR_PATH = (char*) malloc(sizeof(char) * 200);
    // The user will specify which directory they wish to put the sorted files in, but for now
    // we will assume it's on working directoy. 
    int log_fd = creat( "log.ps", 0);

    // Scan the command-line arguments for desired flags. If none of hte arguments match flags, 
    // print the usage message.
    for (int i = 1; i < argc; i++) {
        if (i % 2 != 0) {
            if (argc > 2 && strcmp(argv[i], "-d") == 0 && argv[2]) {
                strncpy(DIR_PATH, argv[i+1], strlen(argv[i+1]));
                DIR_PATH[strlen(argv[i+1])+1] = '\0';
            } else if (argc > 2 && strcmp(argv[1], "-u") == 0 && argv[i+1]) {
                int undo_fd = open(argv[i+1], O_RDWR);
                
                if (undo_fd > 0 && undo_logged_changes(undo_fd)) {
                    printf("Changes successfully undone.\n");
                    exit(1);
                } else {
                    printf("Invalid undo file argument, please verify the file name and try again.\n");
                    exit(-1);
                }
            } else {
                printf("usage: photo-sort -d <relative directory path>\n");
                exit(-1);
            }
        }
                // DIR_PATH = "test-files/big\0";   
    }
    // size_t written = write(log_fd, "start photo-sort\0\n", 19);
    traverse_directory(DIR_PATH, log_fd);
}













