#include "FvwmConsole.h"

int  infd, outfd;    /* socket handle */
int  errorfd;
FILE *inf, *errorf;
char *name;  /* name of this program at executing time */
char *fvwm_getline();


/******************************************/
/*  close socket and exit */
/******************************************/
void dofclose () {
  fclose(inf);
  close(outfd);
  exit(0);
}

/************************************/
/* print error message on stderr */
/************************************/
void ErrMsg( char *msg ) {
  fprintf( stderr, "%s error in %s:%s\n", name , msg , strerror(errno));
  if (errorf != NULL )
    fprintf( errorf, "%s error in %s:%s\n", name , msg , strerror(errno));
  sleep(30);
  fclose(inf);
  close(outfd);
  exit(1);
}


/*******************************************************/
/* setup socket.                                       */
/* send command to and receive message from the server */
/*******************************************************/
void main ( int argc, char *argv[]) {
  char *cmd;
  unsigned char data[BUFSIZE];
  int  len;  /* length of socket address */
  int  clen; /* command length */
  int  pid;  /* child process id */
  
  signal (SIGCHLD, dofclose);  
  signal (SIGINT, dofclose);  
  signal (SIGQUIT, dofclose);  
  signal (SIGPIPE, dofclose);  

  if (argc == 0) {exit(1);}
  if (argc != FCARGS) {
    fprintf(stderr, "%s intended to be invoked by FvwmConsole only.\n", 
            argv[0]);
    sleep(30);
    exit(1);
  }
  name=strrchr(argv[0], '/');
  if (name != NULL) {
    name++;
  } else {
    name = argv[0];
  }

  infd = atoi(argv[1]);
  outfd = atoi(argv[2]);
  errorfd = atoi(argv[3]);
  errorf = fdopen(errorfd, "w");

  inf = fdopen( infd, "r" );
  if (inf == NULL) { ErrMsg("fdopen"); }

  pid = fork();
  if( pid == -1 ) {
	ErrMsg( "fork");
  }
  if( pid == 0 ) { /* child */
	fclose(inf);
	/* loop of get user's command and send it to server */
	while( 1 ) {

	  cmd = fvwm_getline();
	  if( cmd == NULL  ) {
		break;
	  }
	
	  clen = strlen(cmd);
	  if( clen <= 1 ) {
		continue;    /* empty line */
	  }

	  /* send the command to the server */
	  if (write( outfd, cmd, strlen(cmd) ) == -1) { ErrMsg( "write" ); }

	}
	kill( getppid(), SIGQUIT );
	dofclose();
  } else { /* parent */
    close(outfd);
    while( fgets( data, BUFSIZE, inf )  ) {
	/* get the response */
	if( *data == '\0' ) {
	  break;
	}
	printf( "%s",data );
    }
    kill( pid, SIGQUIT );
    dofclose();
  }
}

