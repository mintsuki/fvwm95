/* This module, and the entire FvwmAuto program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1994, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#define TRUE 1
#define FALSE 

#include <FVWMconfig.h>
#ifdef ISC
#include <sys/bsdtypes.h> /* Saul */
#endif 

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include "../../fvwm/module.h"
#include <fvwm/fvwmlib.h>     
#include <fvwm/version.h>

int fd_width;
int fd[2];
int timeout, t_secs, t_usecs;

/*************************************************************************
 *
 * Subroutine Prototypes
 * 
 *************************************************************************/
void Loop(int *fd);
void DeadPipe(int nonsense);

#ifdef BROKEN_SUN_HEADERS
#include "../../fvwm/sun_headers.h"
#endif

#ifdef NEEDS_ALPHA_HEADER
#include "../../fvwm/alpha_header.h"
#endif /* NEEDS_ALPHA_HEADER */

/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
void main(int argc, char **argv)
{
  FILE *file;
  char mask_mesg[80];

  if(argc != 7)
    {
      fprintf(stderr,"FvwmAuto requires one argument.\n");
      exit(1);
    }

  /* Dead pipes mean fvwm died */
  signal (SIGPIPE, DeadPipe);  
  
  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  timeout = atoi(argv[6]);
  t_secs = timeout/1000;
  t_usecs = (timeout - t_secs*1000)*1000;

  fd_width = GetFdWidth();
  sprintf(mask_mesg,"SET_MASK %lu\n",(unsigned long)(M_FOCUS_CHANGE));
  SendInfo(fd,mask_mesg,0);
  Loop(fd);
}


/***********************************************************************
 *
 *  Procedure:
 *	Loop - wait for data to process
 *
 ***********************************************************************/
unsigned long focus_win = 0;

void Loop(int *fd)
{
  unsigned long header[HEADER_SIZE], *body;
  fd_set in_fdset;
  struct itimerval value;
  int retval;
  int Raised = 0;

  while(1)
    {
      FD_ZERO(&in_fdset);
      FD_SET(fd[1],&in_fdset);

      /* set up a time-out, in case no packets arrive before we have to
       * iconify something */
      value.it_value.tv_usec = t_usecs;
      value.it_value.tv_sec = t_secs;

#ifdef __hpux
      if((timeout > 0)&&(Raised == 0))
	retval=select(fd_width,(int *)&in_fdset, 0, 0, &value.it_value);
      else
	retval=select(fd_width,(int *)&in_fdset, 0, 0, NULL);  
#else      
      if((timeout > 0)&&(Raised == 0))
	retval=select(fd_width,&in_fdset, 0, 0, &value.it_value);
      else
	retval=select(fd_width,&in_fdset, 0, 0, NULL);  
#endif

      if(FD_ISSET(fd[1], &in_fdset))
	{
	  /* read a packet */
	  if(ReadFvwmPacket(fd[1],header, &body) > 0)
	    {
	      if(header[1] == M_FOCUS_CHANGE)
		{
		  focus_win = body[0];
		  if(focus_win != 0)
		    Raised = 0;
		  if(timeout == 0)
		    {
		      if(focus_win != 0)
			SendInfo(fd,"Raise",focus_win);		    
		      Raised = 1;
		    }
		}
	      free(body);
	    }
	}
      else
	{
	  /* Raise the current focus window */
	  Raised = 1;
	  if(focus_win != 0)
	    {
	      SendInfo(fd,"Raise",focus_win);
	    }
	}
    }
}



/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 ***********************************************************************/
void DeadPipe(int nonsense)
{
  exit(0);
}

