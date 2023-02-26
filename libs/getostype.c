#include <string.h>
#include <FVWMconfig.h>

#if HAVE_UNAME
#include <sys/utsname.h>

/* return a string indicating the OS type (i.e. "Linux", "SINIX-D", ... ) */
int getostype(char *buf, int max)
{
  struct utsname sysname;
  int ret;
  
  if ((ret = uname(&sysname)) == -1)
    strcpy (buf,"");
  else
    strncat (strcpy(buf,""), sysname.sysname, max);
  return ret;
}
#else
int getostype(char *buf, int max)
{
  strcpy (buf,"");
  return -1;
}
#endif

