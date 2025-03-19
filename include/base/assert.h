#pragma once

#ifndef EMSCRIPTEN
#define assert(x) { if(!(x)) asm("int $3\nnop"); }
#else
#define assert(x)
#endif

