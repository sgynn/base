//USAGE: call BINDATA(name, file.txt) and access the char array &name.

#pragma once

#ifdef MSC_VER
//#error The VS ASM compiler won't work with this, but you can get external ones to do the trick
#define BINDATA(n,s)
#error BINDATA requires nasm
#elif defined(EMSCRIPTEN)
#warning BINDATA requires nasm
#define BINDATA(n,s) \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wunused\"") \
	static char n = 0; \
	static int n##_len = 0; \
	static const char* n##_file = 0; \
	_Pragma("clang diagnostic pop")
#else

asm(
".altmacro\n" \
".macro binfile p q\n" \
"   .global \\p\n" \
"\\p:\n" \
"   .incbin \\q\n" \
"\\p&_end:\n" \
"   .byte 0\n" \
"   .global \\p&_len\n" \
"\\p&_len:\n" \
"   .int(\\p&_end - \\p)\n" \
".endm\n\t"
);

extern "C" {
#define BINDATA(name, source) \
	asm("\n\n.data\n\tbinfile " #name " \"" source "\"\n"); \
	extern char name; \
	extern int name##_len; \
	constexpr const char* name##_file = source;
}

#endif

