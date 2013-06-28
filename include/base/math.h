#ifndef _BASE_MATH_
#define _BASE_MATH_

/** general math header - goes in base directory... */

#include <math.h>

typedef unsigned long long  uint64;
typedef unsigned int        uint,   uint32;
typedef unsigned short      ushort, uint16;
typedef unsigned char       ubyte,  uint8;

#define PI     3.14159265359
#define TWOPI  6.28318530717
#define HALFPI 1.57079632679


#include "vec.h"
#include "matrix.h"
#include "point.h"
#include "quaternion.h"
#include "bounds.h"


#endif

