#include <stdio.h>
#include "STDIO.H"
/*
** Get command line argument. 
** Entry: n    = Number of the argument.
**        s    = Destination string pointer.
**        size = Size of destination string.
**        argc = Argument count from main().
**        argv = Argument vector(s) from main().
** Returns number of characters moved on success,
** else EOF.
*/

int getarg (int n, char *s, int size, int argc, char * argv[]) {
  char *str;
  int i;
  if(n < 0 | n >= argc) {
    *s = NUL;
    return EOF;
    }
  i = 0;
  str=argv[n];
  while(i<size) {
    if((s[i]=str[i])==NUL) break;
    ++i;
    }
  s[i]=NUL;
  return i;
  }

