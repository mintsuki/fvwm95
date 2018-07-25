#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fvwm/fvwmlib.h>
#include "../fvwm/module.h"

/************************************************************************
 * 
 * Reads a single packet of info from fvwm. Prototype is:
 * unsigned long header[HEADER_SIZE];
 * unsigned long *body;
 * int fd[2];
 * void DeadPipe(int nonsense);  * Called if the pipe is no longer open  *
 *
 * ReadFvwmPacket(fd[1],header, &body);
 *
 * Returns:
 *   > 0 everything is OK.
 *   = 0 invalid packet.
 *   < 0 pipe is dead. (Should never occur)
 *   body is a malloc'ed space which needs to be freed 
 *
 **************************************************************************/
int ReadFvwmPacket(int fd, unsigned long *header, unsigned long **body)
{
  int count,total,count2,body_length;
  char *cbody;
  extern void DeadPipe(int);

  if((count = read(fd,header,HEADER_SIZE*sizeof(unsigned long))) >0)
    {
      if(header[0] == START_FLAG)
	{
	  body_length = header[2]-HEADER_SIZE;
	  *body = (unsigned long *)
	    safemalloc(body_length * sizeof(unsigned long));
	  cbody = (char *)(*body);
	  total = 0;
	  while(total < body_length*sizeof(unsigned long))
	    {
	      if((count2=
		  read(fd,&cbody[total],
		       body_length*sizeof(unsigned long)-total)) >0)
		{
		  total += count2;
		}
	      else if(count2 < 0)
		{
		  DeadPipe(1);
		}
	    }
	}
      else
	count = 0;
    }
  if(count <= 0)
    DeadPipe(1);
  return count;
}

