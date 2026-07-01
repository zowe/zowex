/* File: zmetal.c */

#include "zmetal.h"
#include <stdio.h>

#pragma prolog(HELLO, "&CCN_MAIN SETB 1 \n MYPROLOG")

int HELLO(const char* name, char* output, int* outsize) {
    snprintf(output, *outsize, "Hello, %s!", name);
    return 0;
}
