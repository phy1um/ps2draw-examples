
#ifndef PS2_VUTEST_INTTYPES_H
#define PS2_VUTEST_INTTYPES_H

#ifdef _EE

#include <stdint.h>
#include <tamtypes.h>

#else

#include <stdint.h>

#endif

#define QWORD_SIZE (2*sizeof(uint64_t))

#endif

