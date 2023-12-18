/*
    EXIF-util.h
    Functions for generating a structure representing the DateTime field
    of a given JPG file.

    Author: John R. Smith
    Date: December 18, 2023
*/

struct jpg_time {
    int year;
    char year_s[4];
    int month;
    char month_s[2];
    int day;
    char day_s[2];
    char time_s[8];
};

struct jpg_time *get_jpg_time(char* old_location);