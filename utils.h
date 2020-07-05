#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

/**
 * Print to stdout with formated text
 * */
int time_printf(const char* format, ...);

/**
 * Print to file with formated text
 * */
int file_time_printf(FILE *file, const char* format, ...);

#endif // UTILS_H