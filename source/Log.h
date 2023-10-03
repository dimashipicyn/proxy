#pragma once

#include <cstdio>

#ifdef NDEBUG
#define LOG_DEBUG(fmt, ...) \
    do                      \
    {                       \
    } while (0)
#else
#define LOG_DEBUG(fmt, ...)                \
    do                                     \
    {                                      \
        fprintf(stderr, fmt, ## __VA_ARGS__); \
    } while (0)
#endif

#define LOG_ERROR(fmt, ...)                \
    do                                     \
    {                                      \
        fprintf(stderr, fmt,  __VA_ARGS__); \
    } while (0)
