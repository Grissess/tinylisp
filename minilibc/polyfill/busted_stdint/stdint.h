#ifndef MINILIB_STDINT_H
#define MINILIB_STDINT_H

#ifdef __UINT8_TYPE__
typedef __UINT8_TYPE__ uint8_t;
#else
#warning No __UINT8_TYPE__ for this platform/compiler
typedef unsigned char uint8_t;
#endif
#ifdef __UINT16_TYPE__
typedef __UINT16_TYPE__ uint16_t;
#else
typedef unsigned short uint16_t;
#warning No __UINT16_TYPE__ for this platform/compiler
#endif
#ifdef __UINT32_TYPE__
typedef __UINT32_TYPE__ uint32_t;
#else
typedef unsigned int uint32_t;
#warning No __UINT32_TYPE__ for this platform/compiler
#endif
#ifdef __UINT64_TYPE__
typedef __UINT64_TYPE__ uint64_t;
#else
typedef unsigned long uint64_t;
#warning No __UINT64_TYPE__ for this platform/compiler
#endif

#ifdef __INT8_TYPE__
typedef __INT8_TYPE__ int8_t;
#else
typedef char int8_t;
#warning No __INT8_TYPE__ for this platform/compiler
#endif
#ifdef __INT16_TYPE__
typedef __INT16_TYPE__ int16_t;
#else
typedef short int16_t;
#warning No __INT16_TYPE__ for this platform/compiler
#endif
#ifdef __INT32_TYPE__
typedef __INT32_TYPE__ int32_t;
#else
typedef int int32_t;
#warning No __INT32_TYPE__ for this platform/compiler
#endif
#ifdef __INT64_TYPE__
typedef __INT64_TYPE__ int64_t;
#else
typedef long int64_t;
#warning No __INT64_TYPE__ for this platform/compiler
#endif

#endif
