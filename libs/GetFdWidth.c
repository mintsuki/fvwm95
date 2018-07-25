#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <FVWMconfig.h>

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

/*
** just in case...
*/
#ifndef FD_SETSIZE
#define FD_SETSIZE 2048
#endif

int GetFdWidth(void)
{
#ifdef HAVE_SYSCONF
  return min(sysconf(_SC_OPEN_MAX),FD_SETSIZE);
#else
  return min(getdtablesize(),FD_SETSIZE);
#endif
}
