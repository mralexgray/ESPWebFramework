

#ifndef _FDVDEBUG_H_
#define _FDVDEBUG_H_

#include "fdv.h"

extern "C" {
#include <stdarg.h>
}

void debug(char const *fmt, ...);
void debugstrn(char const *str, uint32_t len);
void debugstr(char const *str);
void debug(char c);

#endif