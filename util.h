/*
    util.h: utility functions for the media-sort app
    Author: John R. Smith
    Date: December 18, 2023
*/

#define ROOT_DIRNAME_LEN 12
#define MAX_MONTH_LEN 9

char* get_friendly_month(int month);
void read_n_bytes(unsigned char* buf, int n, FILE *stream);
void strcpy_range(int start, int stop, char* src, char* dest);
