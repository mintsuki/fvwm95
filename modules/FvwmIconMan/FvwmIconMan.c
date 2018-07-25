#include "FvwmIconMan.h"
#include "../../fvwm/module.h"
#include <fvwm/fvwmlib.h>

static int fd_width;
static FILE *console = NULL;
static char *VERSION = "0.4";

#if !defined (__GNUC__) || defined (PRINT_DEBUG)
char *actionNames[] = {
  "click1",
  "click2",
  "click3",
  "select"
};
#endif

typedef enum {
  READ_LINE = 1,
  READ_OPTION = 2,
  READ_ARG = 4,
  READ_REST_OF_LINE = 12
} ReadOption;

void Free (void *p)
{
  if (p != NULL)
    free (p);
}

static int iswhite (char c)
{
  if (c == ' ' || c == '\t' || c == '\0')
    return 1;
  return 0;
}

static void skip_space (char **p)
{
  while (**p == ' ' || **p == '\t')
    (*p)++;
}

#if FVWM_VERSION == 1
static FILE *config_fp = NULL;
#endif

static int init_config_file (char *file)
{
#if FVWM_VERSION == 1
  if ((config_fp = fopen (file, "r")) == NULL)  {
    ConsoleMessage ("Couldn't open file: %s\n", file);
    return 0;
  }
#endif
  return 1;
}

static void close_config_file (void)
{
#if FVWM_VERSION == 1
  if (config_fp)
    fclose (config_fp);
#endif
}

static int GetConfigLineWrapper (int *fd, char **tline)
{
#if FVWM_VERSION == 1

  static char buffer[1024];
  char *temp;

  if (fgets (buffer, 1024, config_fp)) {
    *tline = buffer;
    temp = strchr (*tline, '\n');
    if (temp) {
      *temp = '\0';
    }
    else {
      fprintf (stderr, "line too long\n");
      exit (1);
    }
    return 1;
  }

#else

  char *temp;

  GetConfigLine (fd, tline);
  if (*tline) {
    temp = strchr (*tline, '\n');
    if (temp) {
      *temp = '\0';
    }
    return 1;
  }

#endif

  return 0;
}

static char *read_next_cmd (ReadOption flag)
{
  static ReadOption status;
  static char *buffer;
  static char *retstring, displaced, *cur_pos;

  retstring = NULL;
  if (flag != READ_LINE && !(flag & status))
    return NULL;

  switch (flag) {
  case READ_LINE:
    while (GetConfigLineWrapper (Fvwm_fd, &buffer)) {
      cur_pos = buffer;
      skip_space (&cur_pos);
      if (!strncasecmp (Module, cur_pos, ModuleLen)) {
        retstring = cur_pos;
        cur_pos += ModuleLen;
        displaced = *cur_pos;
        if (displaced == '*') 
          status = READ_OPTION;
        else if (displaced == '\0')
          status = READ_LINE;
        else if (iswhite (displaced))
          status = READ_ARG;
        else 
          status = READ_LINE;
        break;
      }
    }
    break;

  case READ_OPTION:
    *cur_pos = displaced;
    retstring = ++cur_pos;
    while (*cur_pos != '*' && !iswhite (*cur_pos)) 
      cur_pos++;
    displaced = *cur_pos;
    *cur_pos = '\0';
    if (displaced == '*')
      status = READ_OPTION;
    else if (displaced == '\0')
      status = READ_LINE;
    else if (iswhite (displaced))
      status = READ_ARG;
    else 
      status = READ_LINE;
    break;

  case READ_ARG:
    *cur_pos = displaced;
    skip_space (&cur_pos);
    retstring = cur_pos;
    while (!iswhite (*cur_pos))
      cur_pos++;
    displaced = *cur_pos;
    *cur_pos = '\0';
    if (displaced == '\0')
      status = READ_LINE;
    else if (iswhite (displaced))
      status = READ_ARG;
    else 
      status = READ_LINE;
    break;

  case READ_REST_OF_LINE:
    status = READ_LINE;
    *cur_pos = displaced;
    skip_space (&cur_pos);
    retstring = cur_pos;
    break;
  }

  if (retstring && retstring[0] == '\0')
    retstring = NULL;

  return retstring;
}

int OpenConsole()
{
#ifdef PRINT_CONSOLE
  if ((console=fopen("/dev/console","w"))==NULL) {
    fprintf(stderr,"%s: cannot open console\n",Module);
    return 0;
  }
#endif
  return 1;
}

void ShutMeDown (int flag)
{
  ConsoleDebug ("Bye Bye\n");
  exit (flag);
}

void ConsoleMessage(char *fmt, ...)
{
#ifdef PRINT_CONSOLE
  va_list args;
  FILE *filep;
  if (console==NULL) filep=stderr;
  else filep=console;
  va_start(args,fmt);
  vfprintf(filep,fmt,args);
  va_end(args);
#endif
}

#if defined(PRINT_DEBUG) || !defined(__GNUC__)
void ConsoleDebug (char *fmt, ...) {
#ifdef PRINT_CONSOLE
#ifdef PRINT_DEBUG
  va_list args;
  FILE *filep;
  if (console==NULL) filep=stderr;
  else filep=console;
  va_start(args,fmt);
  vfprintf(filep,fmt,args);
  va_end(args);
#endif
#endif
}
#endif

void DeadPipe (int nothing)
{
  ConsoleDebug ("Bye Bye\n");
  ShutMeDown (0);
}

void SendFvwmPipe (char *message,unsigned long window)
{
  char *hold,*temp,*temp_msg;
  hold=message;

  while(1) {
    temp=strchr(hold,',');
    if (temp!=NULL) {
      temp_msg= (char *)safemalloc(temp-hold+1);
      strncpy(temp_msg,hold,(temp-hold));
      temp_msg[(temp-hold)]='\0';
      hold=temp+1;
    } else temp_msg=hold;

    SendText(Fvwm_fd, temp_msg, window);

    if(temp_msg!=hold) free(temp_msg);
    else break;
  }
}

static void main_loop (void)
{
  fd_set readset;
  struct timeval tv;

  while(1) {
    FD_ZERO( &readset);
    FD_SET (Fvwm_fd[1],&readset);
    FD_SET (x_fd,&readset);
    tv.tv_sec=0;
    tv.tv_usec=0;
    if (!select(fd_width,&readset,NULL,NULL,&tv)) {
      FD_ZERO (&readset);
      FD_SET (Fvwm_fd[1],&readset);
      FD_SET (x_fd, &readset);
      select(fd_width,&readset,NULL,NULL,NULL);
    }

    if (FD_ISSET (x_fd, &readset) || XPending (theDisplay)) {
      xevent_loop();
    }
    if (FD_ISSET(Fvwm_fd[1],&readset)) {
      ReadFvwmPipe();
    }
  }
}

static int extract_int (char *p, int *n)
{
  char *s;

  for (s = p; *s; s++) {
    if (*s < '0' || *s > '9') {
      return 0;
    }
  }

  *n = atoi (p);

  return 1;
}      

char *copy_string (char **target, char *src)
{
  int len = strlen (src);

  if (*target)
    Free (*target);
  
  *target = (char *)safemalloc ((len + 1) * sizeof (char));
  strcpy (*target, src);
  return *target;
}

static char *conditional_copy_string (char **s1, char *s2)
{
  if (*s1)
    return *s1;
  else
    return copy_string (s1, s2);
}

#define SET_MANAGER(manager,field,value)                           \
   do {                                                            \
     int id = manager;                                             \
     if (id == -1) {                                               \
       for (id = 0; id < globals.num_managers; id++) {             \
	 globals.managers[id]. field = value;                     \
       }                                                           \
     }                                                             \
     else if (id < globals.num_managers) {                         \
       globals.managers[id]. field = value;                       \
     }                                                             \
     else {                                                        \
       ConsoleMessage ("Internal error in SET_MANAGER: %d\n", id); \
     }                                                             \
   } while (0)

static void read_in_resources (char *file)
{
  char *p, *q;
  int i, n, manager;
  char *option1;
  Resolution r = SHOW_GLOBAL;
  ButtonState state;

  if (!init_config_file (file))
    return;

  while ((p = read_next_cmd (READ_LINE))) {
    ConsoleDebug ("line: %s\n", p);
    option1 = read_next_cmd (READ_OPTION);
    if (option1 == NULL)
      continue;

    ConsoleDebug ("option1: %s\n", option1);
    if (!strcasecmp (option1, "nummanagers")) {
      p = read_next_cmd (READ_ARG);
      if (!option1) {
	ConsoleMessage ("Error in input file\n");
	continue;
      }
      if (extract_int (p, &n) == 0) {
	ConsoleMessage ("This is not a number: %s\n", p);
	continue;
      }
      if (n > 0) {
	allocate_managers (n);
	ConsoleDebug ("num managers: %d\n", n);
      }
      else {
	ConsoleMessage ("You can't have zero managers. I'll give you one.\n");
	allocate_managers (1);
      }
    }
    else {
      /* these all can specify a specific manager */

      if (globals.managers == NULL) {
	ConsoleDebug ("I'm assuming you only want one manager\n");
	allocate_managers (1);
      }

      manager = 0;

      if (option1[0] >= '0' && option1[0] <= '9') {
	if (extract_int (option1, &manager) == 0 || 
	    manager <= 0 || manager > globals.num_managers) {
	  ConsoleMessage ("This is not a valid manager: %s.\n", option1);
	  manager = 0;
	}
	option1 = read_next_cmd (READ_OPTION);
	if (!option1) {
	  ConsoleMessage ("Error in input file\n");
	  continue;
	}
      }

      manager--; /* -1 means global */

      ConsoleDebug ("Applying %s to manager %d\n", option1, manager);

      if (!strcasecmp (option1, "resolution")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Error in input file\n");
	  continue;
	}
	ConsoleDebug ("resolution: %s\n", p);
	if (!strcasecmp (p, "global"))
	  r = SHOW_GLOBAL;
	else if (!strcasecmp (p, "desk"))
	  r = SHOW_DESKTOP;
	else if (!strcasecmp (p, "page"))
	  r = SHOW_PAGE;

	SET_MANAGER (manager, res, r);
      }
      else if (!strcasecmp (option1, "font")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Error in input file\n");
	  continue;
	}
	ConsoleDebug ("font: %s\n", p);

	SET_MANAGER (manager, fontname, 
		     copy_string (&globals.managers[id].fontname, p));
      }
      else if (!strcasecmp (option1, "geometry")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Error in input file\n");
	  continue;
	}

	SET_MANAGER (manager, geometry, 
		     copy_string (&globals.managers[id].geometry, p));
      }
      else if (!strcasecmp (option1, "dontshow")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Error in input_file\n");
	  continue;
	}
	do {
	  ConsoleDebug ("dont show: %s\n", p);
	  if (manager == -1) {
	    int i;
	    for (i = 0; i < globals.num_managers; i++)
	      add_to_stringlist (&globals.managers[i].dontshow, p);
	  }
	  else {
	    add_to_stringlist (&globals.managers[manager].dontshow, p);
	  }
	  p = read_next_cmd (READ_ARG);
	} while (p);
      }
      else if (!strcasecmp (option1, "show")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Error in input_file\n");
	  continue;
	}
	do {
	  ConsoleDebug ("show: %s\n", p);
	  if (manager == -1) {
	    int i;
	    for (i = 0; i < globals.num_managers; i++)
	      add_to_stringlist (&globals.managers[i].show, p);
	  }
	  else {
	    add_to_stringlist (&globals.managers[manager].show, p);
	  }
	  p = read_next_cmd (READ_ARG);
	} while (p);
      }
      else if (!strcasecmp (option1, "background")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Error in input file\n");
	  continue;
	}
	ConsoleDebug ("default background: %s\n", p);

        for ( i = 0; i < NUM_CONTEXTS; i++ )
	  SET_MANAGER (manager, backColorName[i], 
	    conditional_copy_string (&globals.managers[id].backColorName[i], 
				     p));
      }
      else if (!strcasecmp (option1, "foreground")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Error in input file\n");
	  continue;
	}
	ConsoleDebug ("default foreground: %s\n", p);

        for ( i = 0; i < NUM_CONTEXTS; i++ )
	SET_MANAGER (manager, foreColorName[i], 
           conditional_copy_string (&globals.managers[id].foreColorName[i], 
				    p));
      }
      else if (!strcasecmp (option1, "action")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Error in input file: need arguments to action\n");
	  continue;
	}
	if (!strcasecmp (p, "click1")) {
	  i = CLICK1;
	}
	else if (!strcasecmp (p, "click2")) {
	  i = CLICK2;
	}
	else if (!strcasecmp (p, "click3")) {
	  i = CLICK3;
	}
        else if (!strcasecmp (p, "select")) {
           i = SELECT;
        }
	else {
	  ConsoleMessage ("Error in input file: this isn't a valid action name: %s\n", p);
	  continue;
	}

	q = read_next_cmd (READ_REST_OF_LINE);
	if (!q) {
	  ConsoleMessage ("Error in input file: need an action\n");
	  continue;
	}
	ConsoleDebug ("Setting action for %s: %s\n", actionNames[i], q);
	SET_MANAGER (manager, actions[i], 
		     copy_string (&globals.managers[id].actions[i], q));
      }
      else if (!strcasecmp (option1, "showtitle")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Error in input file: need argument to showtitle\n");
	  continue;
	}
	if (!strcasecmp (p, "true")) {
	  i = 1;
	}
	else if (!strcasecmp (p, "false")) {
	  i = 0;
	}
	else {
	  ConsoleMessage ("Error in input file. What is this: %s?\n", p);
	  continue;
	}
	ConsoleDebug ("Setting showtitle to: %d\n", i);
	SET_MANAGER (manager, use_titlename, i);
      }
      else if (!strcasecmp (option1, "plainButton")  ||
               !strcasecmp (option1, "selectButton")  ||
               !strcasecmp (option1, "focusButton")   ||
	       !strcasecmp (option1, "focusandselectButton")) {
         if ( !strcasecmp (option1, "plainButton") )
            i = PLAIN_CONTEXT;
         else if ( !strcasecmp (option1, "selectButton") )
            i = SELECT_CONTEXT;
         else if (!strcasecmp (option1, "focusandselectButton"))
	   i = FOCUS_SELECT_CONTEXT;
	 else
            i = FOCUS_CONTEXT;
         p = read_next_cmd (READ_ARG);
         if (!p) {
            ConsoleMessage ("Error in input file: need argument to %s\n",
			    option1);
            continue;
         }
         else if (!strcasecmp (p, "flat")) {
            state = BUTTON_FLAT;
         }
         else if (!strcasecmp (p, "up")) {
            state = BUTTON_UP;
         }
         else if (!strcasecmp (p, "down")) {
            state = BUTTON_DOWN;
         }
         else {
           ConsoleMessage ("Error in input file: this isn't a valid button state: %s\n", p);
           continue;
         }
	 ConsoleDebug ("Setting buttonState[%s] to %s\n", 
		       contextDefaults[i].name, p);
         SET_MANAGER (manager, buttonState[i], state);

            /* check for optional fore color */
         p = read_next_cmd (READ_ARG);
         if ( !p )
            continue;
         
         ConsoleDebug ("Setting foreColorName[%s] to %s\n", 
		       contextDefaults[i].name, p);
	 SET_MANAGER (manager, foreColorName[i], 
		      copy_string (&globals.managers[id].foreColorName[i], p));

            /* check for optional back color */
         p = read_next_cmd (READ_ARG);
         if ( !p )
            continue;
         
         ConsoleDebug ("Setting backColorName[%s] to %s\n",
		       contextDefaults[i].name, p);
	 SET_MANAGER (manager, backColorName[i], 
		      copy_string (&globals.managers[id].backColorName[i], p));
      }
      else if (!strcasecmp (option1, "followfocus")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Error in input file: need argument to followfocus\n");
	  continue;
	}
	if (!strcasecmp (p, "true")) {
	  i = 1;
	}
	else if (!strcasecmp (p, "false")) {
	  i = 0;
	}
	else {
	  ConsoleMessage ("Error in input file. What is this: %s?\n", p);
	  continue;
	}
	ConsoleDebug ("Setting followfocus to: %d\n", i);
	SET_MANAGER (manager, followFocus, i);
      }
      else if (!strcasecmp (option1, "sort")) {
	p = read_next_cmd (READ_ARG);
	if (!p) {
	  ConsoleMessage ("Error in input file: need argument to sort\n");
	  continue;
	}
	if (!strcasecmp (p, "true")) {
	  i = 1;
	}
	else if (!strcasecmp (p, "false")) {
	  i = 0;
	}
	else {
	  ConsoleMessage ("Error in input file. What is this: %s?\n", p);
	  continue;
	}
	ConsoleDebug ("Setting sort to: %d\n", i);
	SET_MANAGER (manager, sort, i);
      }
      else {
	ConsoleMessage ("Unknown option: %s\n", p);
      }
    }
  }

  if (globals.managers == NULL) {
    ConsoleDebug ("I'm assuming you only want one manager\n");
    allocate_managers (1);
  }
  print_managers();
  close_config_file();
}

int main (int argc, char **argv)
{
  char *temp, *s;
  FvwmPacketBody temppacket;

#ifdef I18N
  setlocale(LC_CTYPE, "");
#endif

  if ((char *)&temppacket.add_config_data.window_gravity - 
      (char *)&temppacket != 21 * sizeof (long)) {
    fprintf (stderr, "Bad packet structure. Can't run. Sorry\n");
    exit (1);
  }

  OpenConsole();
  init_globals();
  init_winlists();
  
  temp = argv[0];
  s = strrchr (argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  if((argc != 6) && (argc != 7)) {
    fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",Module,
      VERSION);
    ShutMeDown (1);
  }

  Fvwm_fd[0] = atoi(argv[1]);
  Fvwm_fd[1] = atoi(argv[2]);
  init_display();
  init_boxes();
  
  signal (SIGPIPE, DeadPipe);  

  read_in_resources (argv[3]);
  
  assert (globals.managers);
  fd_width = GetFdWidth();

  SetMessageMask(Fvwm_fd,M_CONFIGURE_WINDOW | M_RES_CLASS | M_RES_NAME |
                 M_ADD_WINDOW | M_DESTROY_WINDOW | M_ICON_NAME |
                 M_DEICONIFY | M_ICONIFY | M_END_WINDOWLIST |
                 M_NEW_DESK | M_NEW_PAGE | M_FOCUS_CHANGE | M_WINDOW_NAME);

  SendInfo (Fvwm_fd, "Send_WindowList", 0);
  
  main_loop();

  ConsoleMessage ("Shouldn't be here\n");
  
  return 0;
}
