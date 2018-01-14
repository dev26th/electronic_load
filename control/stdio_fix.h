#ifndef STDIO_FIX_H
#define STDIO_FIX_H

#include <stdio.h>

#ifdef _WIN32

#define snprintf sprintf_s
#define FMT_SIZE_T ""

#else

#define FMT_SIZE_T "z"

#endif

#endif // STDIO_FIX_H
