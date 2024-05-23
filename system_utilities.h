#ifndef SYSTEM_UTILITIES_H
#define SYSTEM_UTILITIES_H

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

void panicf [[noreturn]] (const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    std::exit(EXIT_FAILURE);
}

#endif