/* File: zmetal.h */

#ifndef ZMETAL_H
#define ZMETAL_H

#if defined(__cplusplus) && defined(__MVS__)
extern "OS"
{
#elif defined(__cplusplus)
extern "C"
{
#endif

int HELLO(const char* name, char* output, int* outsize);

#if defined(__cplusplus)
}
#endif

#endif
