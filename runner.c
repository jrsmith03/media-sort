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

// #define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#define ROOT_DIRNAME_LEN 12
#define MAX_MONTH_LEN 9

struct jpg_time {
    int year;
    char year_s[4];
    int month;
    char month_s[2];
    int day;
    char day_s[2];
    char time_s[8];
};

// Program imports
bool get_media_extension(char* name, char* buf);
void traverse_directory(char* path_to_start, int log_fd);
char* get_friendly_month(int month);
bool move_media(char* old_location, char* ext, int log_fd);
bool undo_logged_changes(int undo_log_fd);

void add_new_file(char* file_rename, char* dir, char* ext, char* old_location, 
                    char* file_name);
void get_file_rename(bool is_jpg, char* file_rename, char* subdir, struct tm *time, 
    struct jpg_time *time_jpg);
struct jpg_time *get_jpg_time(char* old_location);



// Various utility functions
void strcpy_range(int start, int stop, char* src, char* dest);
void read_n_bytes(unsigned char* buf, int n, FILE *stream);
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
    // free(DIR_PATH);
    
}

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
            // free(my_path);
            // free(tok);
        } 
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

struct jpg_time *get_jpg_time(char* old_location) {
        return NULL;
        struct jpg_time *res = (struct jpg_time*) malloc(sizeof(struct jpg_time));
        // struct jpg_time *res;

        // This code is non-trivial. I did not want to include an Exif library just to read a simple field.
        FILE *stream = fopen(old_location, "r+");
        // First thing, cheick if it is an Exif formatted JPG. If not, abandon ship.
        // We already know that the file is JPG, so the first 2 bytes are fluff.
        fseek(stream, 0x02, SEEK_SET);
        // I am going to walk an arbitrary number of bytes looking for the Exif marker.
        // It would probably be safe to assume if we walk over a hundred bytes into the file,
        // it is not encoded with EXIF.
        bool is_exif = false;
        unsigned char encoding_type;
        for (int i = 0; i < 100; i++) {
            // read_n_bytes(encoding_type, 1,stream);
	    encoding_type = fgetc(stream);
            // printf("bytes: %x\n", encoding_type);
            if (encoding_type == 0xE1) {
                is_exif = true;
                break;
            }
        }
        if (is_exif) printf("IMAGE IS EXIF FORMAT!\n");
        if (!is_exif) return NULL;
        
        // Might be an error if 0xE1 was not the end of the sequence.
        // next 8 bytes unneeded
        fseek(stream, 0x08, SEEK_CUR);
        uint32_t main_ofs = ftell(stream);
        bool big_endian = false;
        // Thank you Johnathan Swift
        unsigned char endianness[1];
        read_n_bytes(endianness, 2, stream);
        
        if (endianness[0] == 0x4D && endianness[1] == 0x4D) {
            big_endian = true;
        }
        // If we are using Intel encoding, we need to read in the buffer backwards.
        fseek(stream, 0x02, SEEK_CUR);
        // Following 2 bytes unneeded, then the next 4 determine the offset into IFD directory
        // structure relative to the position of the endianness marker.
        unsigned char ifd_offset_bytes[4];
        read_n_bytes(ifd_offset_bytes, 4, stream);
        uint32_t ifs_offs = 0;
        if (!big_endian) {
            for (int i = 0; i <= 3; i++) {
                ifs_offs += (uint32_t) ifd_offset_bytes[i];
            }
        } else {
            for (int i = 3; i >= 0; i--) {
                ifs_offs += (uint32_t) ifd_offset_bytes[i];
            }
        }
        // Now we want to seek to the position of the tags!
        // Note that the first two bytes indicate the number of tags, but we're going 
        // to disregard that because the algorithm is simply searching for the 
        // DateTime tag. We may want to improve the logic by making sure we are within
        // the bounds of the Exif tag, but for now we'll just walk an arbitrary length.
        fseek(stream, (main_ofs + ifs_offs + 2) - ftell(stream), SEEK_CUR);
        int walked = 0;
        unsigned char tag_id[1];
        // unsigned char datetime_tag = 0x0132;
        unsigned char datetime_1, datetime_2;
        if (big_endian) {
            datetime_1 = 0x01;
            datetime_2 = 0x32;
        } else {
            datetime_1 = 0x32;
            datetime_2 = 0x01;
        }

        // todo: replace by looking at how many IFD entries there are.
        while (walked < 6000) {
            read_n_bytes(tag_id, 2, stream);
            if (tag_id[0] == datetime_1 && tag_id[1] == datetime_2) {
                printf("found match!\n");
                break;
            }
            fseek(stream, 10, SEEK_CUR);
            walked += 12;
            // I already accounted for endinaess when I prepared the tag reference.
        }
        if (walked > 5900) return NULL;
        // It looks like we found the DateTime field.
        // Now, we walk 6 bytes into that field to read the location of the date.
        fseek(stream, 6, SEEK_CUR);
        unsigned char date_ptr[3];
        read_n_bytes(date_ptr, 4, stream);
        uint32_t datefield_ofs = 0x00000000;
        if (!big_endian) {
            for (int i = 0; i <= 3; i++) {
                // math
                datefield_ofs |= (uint32_t) date_ptr[i] << (i * 2) * 4;
            }
        } else {
            for (int i = 3; i >= 0; i--) {
                datefield_ofs += (uint32_t) date_ptr[i];
            }
        }
        fseek(stream, (datefield_ofs + main_ofs) - ftell(stream), SEEK_CUR);
        // The Exif encoding of the date should be as follows:
        // <year>:<month, 2 digits>:<day, 2 digits> <ascii space> <time>
        // It is far easier to just read from disk instead of dealing with the data in memory.
        // Plus, the file system reads 4k chunks from each file, and the metadata is typically
        // less than that. So realistically, all of this should require only 1 disk read on average.

        // char year_buf[4];
        // char month_buf[2];
        // char day_buf[2];
        // char time_buf[8];
        read_n_bytes(res->year_s, 4, stream);
        res->year_s[4] = '\0';
        fseek(stream, 1, SEEK_CUR);
        read_n_bytes(res->month_s, 2, stream);
        res->month_s[2] = '\0';
        fseek(stream, 1, SEEK_CUR);

        read_n_bytes(res->day_s, 2, stream);
        res->day_s[2] = '\0';
        fseek(stream, 1, SEEK_CUR);

        read_n_bytes(res->time_s, 8, stream);
        res->time_s[8] = '\0';
        if (!res->year_s) return NULL;
        // res->year = atoi(year_buf);
        // strncpy(res->year_s, year_buf, 4);
        // res->year_s[4] = "\0";
        // res->month = atoi(month_buf);
        // printf("mon: %d\n", res->month);
        // char *friendly_mon = malloc(9);
        // friendly_mon = get_friendly_month(res->month);
        // strncpy(res->month_s, friendly_mon, 8);

        // // res->day = atoi(day_buf);
        // strncpy(res->day_s, day_buf, 2);
        // res->day_s[2] = "\0";
    
        // strncpy(res->time_s, time_buf, 8);

        printf("year: %s\n", res->year_s);
        printf("month: %s\n", res->month_s);
        printf("day: %s\n", res->day_s);
        printf("time: %s\n", res->time_s);
        fclose(stream);
        
        // char date[19];
        // read_n_bytes(date, 19, stream);
        // date[19] = "\0";
    
        

        // printf("THE EXIFII DATE: %s\n", date);
        // return NULL;
        // if (!date) return NULL;
        // char mon_char_buf[2];
        // char *mon_chars_buf = malloc(10);

        // strcpy_range(0, 3, date, res->year_s);
        // res->year_s[4] = "\0";
        // res->year = atoi(res->year_s);

        // printf("the year: %d\n", res->year);
        // strcpy_range(5, 6, date, mon_char_buf);
        // mon_char_buf[2] = "\0";
        // printf("the month int: %s\n", mon_char_buf);
        // res->month = atoi(mon_char_buf);

        // mon_chars_buf = get_friendly_month(res->month);
        // printf("the month: %s\n", res->month_s);
        // strncpy(res->month_s, mon_chars_buf, 10);
        // strcpy_range(8, 9, date, res->day_s);
        // res->day = atoi(res->day_s);
        // printf("the day: %d\n", res->day);
        // strcpy_range(11, 18, date, res->time_s);
        // res->time_s[9] = "\0"; 
        // printf("the time: %s\n", res->time_s);

        return res;
}

void read_n_bytes(unsigned char* buf, int n, FILE *stream) {
    for (int i = 0; i < n; i++) {
        buf[i] = fgetc(stream);
        // printf("current value of buf[%d]: %x", i, buf[i]);
        // fseek(stream, 1, SEEK_SET);
        // printf("current stream offs: %d\n", stream->_offset);
    }
}
// note: stop is inclusive
void strcpy_range(int start, int stop, char* src, char* dest) {
    int idx = 0;
    for (int i = start; i <= stop; i++) {
        dest[idx++] = src[i];
        printf("the dst %d: %s\n", idx, dest);
    }
    dest[idx] = '\0';
    printf("final copy: %s\n", dest);

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
            // printf("Duplicate file detected!\n");
            strcpy(dup_name, dir);
            // strcat(dup_name, "/duplicate");
            // if (!opendir(dup_name)) mkdir(dup_name, 0700);
        
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
            // printf("rename ERROR: %s\n", strerror(errno));

            close(fd);
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

