#include "FvwmConsole.h"

#define PROMPT "Fvwm95> "

#ifndef HAVE_READLINE

static   char cmd[256];

/* no readline - starts here */
char *fvwm_getline() {
  fputs(PROMPT,stdout);
  fflush(stdout);
  if( fgets(cmd,256,stdin) == NULL  ) {
	return(NULL);
  }
  return(cmd);
}

#else 
/* readline - starts here */
#include <readline/readline.h>
#include <readline/history.h>

char *fvwm_getline() {
  static char *line;

  /* If the buffer has already been allocated, return the memory to the free pool. */
  if (line != (char *)NULL) {
	free (line);
	line = (char *)NULL;
  }
                             
  /* Get a line from the user. */
  line  = readline (PROMPT);
     
  if( line != NULL ) {

	/* If the line has any text in it, save it on the history. */
	if (*line != '\0')
	  add_history (line);
	
	/* add cr at the end*/
        if ((line = realloc(line,strlen(line)+2)) != NULL)
          strcat(line, NEWLINE ); 
  }

  return (line);

}
/* readline - end here */
#endif


