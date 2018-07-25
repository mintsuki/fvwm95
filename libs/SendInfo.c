#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/***********************************************************************
 *
 *  Procedure:
 *	SendInfo - send a command back to fvwm 
 *
 ***********************************************************************/
void SendInfo(int *fd,char *message,unsigned long window)
{
  int w;

  if(message != NULL)
    {
      write(fd[0],&window, sizeof(unsigned long));
      w=strlen(message);
      write(fd[0],&w,sizeof(int));
      write(fd[0],message,w);

      /* keep going */
      w=1;
      write(fd[0],&w,sizeof(int));
    }
}

