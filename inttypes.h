
#ifndef PS2_VUTEST_INTTYPES_H
#define PS2_VUTEST_INTTYPES_H

#ifdef _EE

#include <tamtypes.h>

typedef u32 uint32_t;
typedef u64 uint64_t;
typedef u16 uint16_t;

#else

#include <stdint.h>

#endif

#define QWORD_SIZE (2*sizeof(uint64_t))

#endif

