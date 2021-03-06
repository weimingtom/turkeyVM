#ifndef EXE_H
#define EXE_H
#include "../main/turkey.h"
#include "../heapManager/alloc.h"

#define C Class_t
#define O Object_t

/*execute*/
extern void executeMethod(MethodBlock_t * mb, va_list jargs);

extern void executeStaticMain(MethodBlock_t * mb, O args);

extern void executeMethodArgs(C class, MethodBlock_t * mb, ...);

extern void invokeConstructNative(MethodBlock_t * mb, O args, O this);

#undef C
#undef O

#endif
