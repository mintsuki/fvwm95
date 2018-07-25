#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <FVWMconfig.h>
#include <fvwm/fvwmlib.h>


#ifndef HAVE_STRCASECMP

int strcasecmp(const char *s1, const char *s2)
{
  register int c1, c2, len1, len2;

  len1 = strlen(s1);
  len2 = strlen(s2);

  if (len1 != len2) return 1;

  for (;;) {
    c1 = *s1; 
    c2 = *s2;

    if (!c1 || !c2) return (c1 - c2);

    c1 = tolower(c1);
    c2 = tolower(c2);

    if (c1 != c2) return (c1 - c2);

    len1--, s1++, s2++;
  }

  return 0;
}

#endif


#ifndef HAVE_STRNCASECMP

int strncasecmp(const char *s1, const char *s2, size_t n)
{
  register int c1, c2;
  
  while (n--) {

    c1 = *s1;
    c2 = *s2;

    if (!c1 || !c2) return (c1 - c2);

    c1 = tolower(c1);
    c2 = tolower(c2);

    if (c1 != c2) return (c1 - c2);

    s1++, s2++;
    }

  return 0;
}

#endif


#ifndef HAVE_STRERROR

char *strerror(int num)
{
  extern int sys_nerr;
  extern char *sys_errlist[];

  if (num >= 0 && num < sys_nerr)
    return(sys_errlist[num]);
  else
    return "Unknown error number";
}

#endif


/***************************************************************************
 * A simple routine to copy a string, stripping spaces and mallocing
 * space for the new string 
 ***************************************************************************/

void CopyString(char **dest, char *source)
{
  int len;
  char *start;
  
  while(((isspace((unsigned char)*source))&&(*source != '\n'))&&(*source != 0)) source++;

  len = 0;
  start = source;
  while((*source != '\n')&&(*source != 0)) len++, source++;
  
  source--;
  while((isspace((unsigned char)*source))&&(*source != 0)&&(len >0)) len--, source--;

  *dest = safemalloc(len+1);
  strncpy(*dest,start,len);
  (*dest)[len]=0;	  
}

/************************************************************************
 *
 * Concatentates 3 strings
 *
 *************************************************************************/

char *CatString3(char *a, char *b, char *c)
{
  static char CatS[256];
  int len = 0;

  if(a != NULL)
    len += strlen(a);
  if(b != NULL)
    len += strlen(b);
  if(c != NULL)
    len += strlen(c);

  if (len > 255)
    return NULL;

  if(a == NULL)
    CatS[0] = 0;
  else
    strcpy(CatS, a);
  if(b != NULL)
  strcat(CatS, b);
  if(c != NULL)
    strcat(CatS, c);
  return CatS;
}

int StrEquals(char *s1,char *s2)
{
  if (!s1 || !s2)
    return 0;
  return (strcasecmp(s1,s2)==0);
}

