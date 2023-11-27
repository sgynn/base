#pragma once

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

// Some more handy math functions
inline float randf() { return (float)rand() / (float)RAND_MAX; }
inline float fsign(float v) { return v<0? -1: v>0? 1: 0; }
inline float fsign(float v, float epsilon) { return v<0-epsilon? -1: v>epsilon? 1: 0; }
inline float fclamp(float v, float low=0, float high=1) { return fmin(fmax(v, low), high); }
inline float flerp(float a, float b, float t) { return a * (1-t) + b * t; }
inline float fstep(float v, float target, float amount) {
	if(target > v + amount) return v + amount;
	if(target < v - amount) return v - amount;
	return target;
}


