#include <stdio.h>

#include "util.h"


// Convert the number of a month into its English name.
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

// Convert n characters of given number into a string
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


void read_n_bytes(unsigned char* buf, int n, FILE *stream) {
    for (int i = 0; i < n; i++) {
        buf[i] = fgetc(stream);
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