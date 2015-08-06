#ifndef STRING_H
#define STRING_H


#define T String_t

typedef char* String_t;



int String_equals(T, T);

long String_hashCode(T);

T String_new(T x);

#undef T

#endif