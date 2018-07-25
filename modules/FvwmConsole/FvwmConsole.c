/*
  Fvwm command input interface.
 
  Copyright 1996, Toshi Isogai. No guarantees or warantees or anything
  are provided. Use this program at your own risk. Permission to use 
  this program for any purpose is given,
  as long as the copyright is kept intact. 
*/

#include "FvwmConsole.h"

char *MyName;

int fd[2];  /* pipe to fvwm */
FILE *sp;
int tocfd[2], fromcfd[2]; /* to/from client pipes */
char name[32]; /* name of this program in executable format */
int  pid;      /* server routine child process id */

void server(int *fd, int to, int from);
void GetResponse(int toc); 
void DeadPipe();
void CloseSocket();
void ErrMsg( char *msg );
void SigHandler();

#define XARGS (sizeof(xterm_a)/sizeof(char *))

void main(int argc, char **argv){
  char *tmp, *s;
  char *client = NULL;
  char **eargv;
  char tocfdrname[10];
  char fromcfdwname[10];
  char errordupfd[10];
  int i,j;
  char *xterm_a[] = {"-title", name,"-name",name, "-e",client,NULL };
#define client_loc 5

  /* initially no child */
  pid = 0;

  /* Save the program name - its used for error messages and option parsing */
  tmp = argv[0];

  s=strrchr(argv[0], '/');
  if (s != NULL)
    tmp = s + 1;

  strncpy( name, tmp , 32);
  name[31] = '\0';

  MyName = safemalloc(strlen(tmp)+2);
  strcpy(MyName,"*");
  strcat(MyName, tmp);

  /* construct client's name */
  client = safemalloc(strlen(argv[0])+2);
  strcpy( client, argv[0] );
  strcat( client, "C" );
  xterm_a[client_loc] = client;

  if(argc < FARGS)    {
	fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",
                       MyName, VERSION);
	exit(1);
  }

  if( ( eargv =(char **)safemalloc((argc-FARGS+XARGS+FCARGS-1)*sizeof(char *)) ) == NULL ) {
	ErrMsg( "allocation" );
  }

  /* copy arguments */
  eargv[0] = XTERM;
  j= 1;
  for ( i=FARGS ; i<argc; i++,j++ ) {
	eargv[j] = argv[i];
  }

  for ( i=0 ; xterm_a[i] != NULL ; j++, i++ ) {
	eargv[j] = xterm_a[i];
  }
  /* Make pipes to/from client */
  if (pipe(tocfd) == -1) { ErrMsg("pipe"); }
  if (pipe(fromcfd) == -1) { ErrMsg("pipe"); }
  /* Add client arguments */
  sprintf(tocfdrname, "%d", tocfd[0]); 
  eargv[j] = tocfdrname; j++;
  sprintf(fromcfdwname, "%d", fromcfd[1]); 
  eargv[j] = fromcfdwname; j++;
  eargv[j] = errordupfd; j++;
  eargv[j] = NULL;

  /* Dead pipes mean fvwm or client died */
  signal (SIGPIPE, DeadPipe);  
  signal (SIGINT, SigHandler);  
  signal (SIGQUIT, SigHandler);  
  signal (SIGCHLD, SigHandler);  

  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  /* launch xterm with client */
  if( fork() == 0 ) {
	close(tocfd[1]);
	close(fromcfd[0]);
	close(fd[0]);
	close(fd[1]);
	sprintf(errordupfd, "%d", dup(2));
	execvp( *eargv, eargv );
	ErrMsg("exec");
  }
  close(tocfd[0]);
  close(fromcfd[1]);
  
  server(fd, tocfd[1], fromcfd[0]);
}

/***********************************************************************
 *	signal handler
 ***********************************************************************/
void DeadPipe() {
  fprintf(stderr,"%s: dead pipe\n", name);
  CloseSocket();
  exit(0);
}

void SigHandler() {
  CloseSocket();
  exit(1);
}

/*********************************************************/
/* close sockets and spawned process                     */
/*********************************************************/
void CloseSocket() {
  if( pid ) {
	kill( pid, SIGKILL );
  }
  close(tocfd[1]);
  close(fromcfd[0]);
}

/*********************************************************/
/* setup server and communicate with fvwm and the client */
/*********************************************************/
void server (int *fd, int toclient, int fromclient) {
  char buf[BUFSIZE];      /*  command line buffer */
  FILE *inf;

  /* get command from client and return result */
  pid = fork();
  if( pid == -1 ) {
	ErrMsg(  "fork");
	exit(1);
  }
  if( pid == 0 ) {
	inf = fdopen(fromclient , "r" );
	close(toclient);
	while(fgets( buf, BUFSIZE, inf )) {
          /* strip trailing newline */
          if (buf[strlen(buf)-1] == '\n') buf[strlen(buf)-1] = '\0';
	  SendText(fd,buf,0); /* send command */
	}
	CloseSocket();
	kill( getppid(), SIGQUIT);
	exit(0);
  } else {
    close(fromclient);
    while(1) {
	GetResponse(toclient);
    }
  }
}

/**********************************************/
/* read fvwm packet and pass it to the client */
/**********************************************/
void GetResponse(int toclient) {
  fd_set in_fdset;
  unsigned long *body;
  unsigned long header[HEADER_SIZE];

  FD_ZERO(&in_fdset);
  FD_SET(fd[1],&in_fdset);

  /* ignore anything but error message */
  if( ReadFvwmPacket(fd[1],header,&body) > 0)	 {
	if(header[1] == M_PASS)	     { 
	  write( toclient, (char *)&body[3], strlen((char *)&body[3])); 
	} 
	free(body);
  }
}

/************************************/
/* print error message on stderr */
/************************************/
void ErrMsg( char *msg ) {
  fprintf( stderr, "%s server error in %s\n", name, msg );
  CloseSocket();
  exit(1);
}
