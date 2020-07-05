#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

int time_printf(const char* format, ...);
int file_time_printf(FILE *file, const char* format, ...);

#endif // UTILS_H