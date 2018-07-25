#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <fvwm/fvwmlib.h>
#include "../fvwm/module.h"

/***************************************************************************
 *
 * Gets a module configuration line from fvwm. Returns NULL if there are 
 * no more lines to be had. "line" is a pointer to a char *.
 *
 **************************************************************************/
void *GetConfigLine(int *fd, char **tline)
{
  static int first_pass = 1;
  int count,done = 0;
  static char *line = NULL;
  unsigned long header[HEADER_SIZE];

  if(line != NULL)
    free(line);

  if(first_pass)
    {
      SendInfo(fd,"Send_ConfigInfo",0);
      first_pass = 0;
    }
  
  while(!done)
    {
      count = ReadFvwmPacket(fd[1],header,(unsigned long **)&line);
      if(count > 0)
	*tline = &line[3*sizeof(long)];
      else 
	*tline = NULL;
      if(*tline != NULL)
	{
	  while(isspace((unsigned char)**tline))(*tline)++;
	}

/*      fprintf(stderr,"%x %x\n",header[1],M_END_CONFIG_INFO);*/
      if(header[1] == M_CONFIG_INFO)
	done = 1;
      else if(header[1] == M_END_CONFIG_INFO)
	{
	  done = 1;
	  if(line != NULL)
	    free(line);
	  line = NULL;
	  *tline = NULL;
	}
      
    }
  return line;
}

