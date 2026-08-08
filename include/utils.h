
#ifndef UTILS_H
#define UTILS_H

#include<stdlib.h>
#include<errno.h>
#include<stdbool.h>
#include<string.h>
#include<SDL3/SDL.h>

// This file contains declarations of functions that doesn't take
// AppState pointer as argument and have different purposes.

bool parse_double(const char *str, double *value);
bool parse_long(const char *str, long int *value, long int base);
bool parse_color(const char *str, Uint32 *value, int base);
int parse_command(char *buffer, char *argv[], int max_args);
char *join_strings(const char *str1, const char *str2);
void calculate_colors(int size, Uint32 *base, int range, Uint32 *dest);


#endif
