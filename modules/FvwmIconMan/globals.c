#include "FvwmIconMan.h"

GlobalData globals;
ContextDefaults contextDefaults[] = {
  { "plain", BUTTON_UP, { "black", "white" }, { "white", "gray"} },
  { "focus", BUTTON_UP, { "white", "gray" }, { "black", "white" } },
  { "select", BUTTON_FLAT, { "black", "white" }, { "white", "gray" } },
  { "focusandselect", BUTTON_FLAT, { "white", "gray" }, { "black", "white" } }
};

int Fvwm_fd[2];
int x_fd;
char *Module = "*FvwmIconMan";
int ModuleLen = 12;

static void init_win_manager (int id)
{
  int i;

  globals.managers[id].res = SHOW_PAGE;
  globals.managers[id].icon_list.n = 0;
  globals.managers[id].icon_list.head = NULL;  
  globals.managers[id].icon_list.tail = NULL;
  globals.managers[id].window_up = 0;
  globals.managers[id].window_mapped = 0;
  globals.managers[id].fontname = NULL;
  for ( i = 0; i < NUM_CONTEXTS; i++ ) {
    globals.managers[id].backColorName[i] = NULL;
    globals.managers[id].foreColorName[i] = NULL;
    globals.managers[id].buttonState[i] = contextDefaults[i].state;
  }
  globals.managers[id].geometry = NULL;
  globals.managers[id].show.list = NULL;
  globals.managers[id].show.mask = ALL_NAME;
  globals.managers[id].dontshow.list = NULL;
  globals.managers[id].dontshow.mask = ALL_NAME;
  globals.managers[id].grow_direction = ForgetGravity;
  globals.managers[id].use_titlename = 1;
  globals.managers[id].followFocus = 0;
  globals.managers[id].sort = 1;
  globals.managers[id].focus_box = -1;
  globals.managers[id].actions[CLICK1] = NULL;
  globals.managers[id].actions[CLICK2] = NULL;
  globals.managers[id].actions[CLICK3] = NULL;
  globals.managers[id].actions[SELECT] = NULL;
  copy_string (&globals.managers[id].actions[CLICK1], DEFAULT_ACTION);
  copy_string (&globals.managers[id].actions[CLICK2], DEFAULT_ACTION);
  copy_string (&globals.managers[id].actions[CLICK3], DEFAULT_ACTION);
  copy_string (&globals.managers[id].actions[SELECT], "Nop");
}  

void print_managers (void)
{
#ifdef PRINT_DEBUG
  int i;

  for (i = 0; i < globals.num_managers; i++) {
    ConsoleDebug ("Manager %d:\n", i + 1);
    if (globals.managers[i].res == SHOW_GLOBAL)
      ConsoleDebug ("ShowGlobal\n");
    else if (globals.managers[i].res == SHOW_DESKTOP)
      ConsoleDebug ("ShowDesktop\n");
    else if (globals.managers[i].res == SHOW_PAGE)
      ConsoleDebug ("ShowPage\n");
    
    ConsoleDebug ("DontShow:\n");
    print_stringlist (&globals.managers[i].dontshow);
    ConsoleDebug ("Show:\n");
    print_stringlist (&globals.managers[i].show);
    
    ConsoleDebug ("Font: %s\n", globals.managers[i].fontname);
    ConsoleDebug ("Geometry: %s\n", globals.managers[i].geometry);
    ConsoleDebug ("\n");
  }

#endif

}

int allocate_managers (int num)
{
  int i;

  if (globals.managers) {
    ConsoleMessage ("Already have set the number of managers\n");
    return 0;
  }
  
  if (num < 1) {
    ConsoleMessage ("Can't have %d managers\n", num);
    return 0;
  }

  globals.num_managers = num;
  globals.managers = (WinManager *)safemalloc (num * sizeof (WinManager));
  
  for (i = 0; i < num; i++) {
    init_win_manager (i);
  }
  
  return 1;
}

void init_globals (void)
{
  globals.desknum = ULONG_MAX;
  globals.x = ULONG_MAX;
  globals.y = ULONG_MAX;
  globals.screenx = 0;
  globals.screeny = 0;
  globals.num_managers = 1;
  globals.managers = NULL;
  globals.focus_win = NULL;
}
