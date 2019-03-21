#pragma once
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#undef __STDC_FORMAT_MACROS


#define STDERR(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#define STDOUT(fmt, ...) fprintf(stdout, fmt "\n", ##__VA_ARGS__)
#define STDOUT_R(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#define LOG(fmt, ...) STDERR(fmt, ##__VA_ARGS__)


#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
// #define unless(x) if (unlikely(!(x)))
#define unless(x) if ((!(x)))


#define MAX(a, b) (((a)>(b))?(a):(b));
#define MIN(a, b) (((a)<(b))?(a):(b));


typedef enum {
	WIDTH_8=8, WIDTH_16=16, WIDTH_32=32, WIDTH_64=64 // need 16?
} width_t;
