#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

/************************************************************************
 *
 * Sends arbitrary text to fvwm
 *
 ***********************************************************************/
void SendText(int *fd,char *message,unsigned long window)
{
  int w;
  
  if(message != NULL)
    {
      write(fd[0],&window, sizeof(unsigned long));
      
      w=strlen(message);
      write(fd[0],&w,sizeof(int));
      write(fd[0],message,w);
      
      /* keep going */
      w = 1;
      write(fd[0],&w,sizeof(int));
    }
}
