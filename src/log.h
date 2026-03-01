#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#define LOG_ERROR(fmt, ...) fprintf(stderr, "error: " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  fprintf(stderr, "warn: " fmt "\n", ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  fprintf(stderr, "info: " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) fprintf(stderr, "debug: " fmt "\n", ##__VA_ARGS__)

#endif
