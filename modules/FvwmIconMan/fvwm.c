#include "FvwmIconMan.h"
#include "../../fvwm/fvwm.h"
#include "../../fvwm/module.h"

int window_up = 0;

static int win_in_viewport (WinData *win)
{
  WinManager *manager = win->manager;
  long xmin, xmax, ymin, ymax;
  int flag = 0;
  
  assert (manager);

  switch (manager->res) {
  case SHOW_GLOBAL:
    flag = 1;
    break;

  case SHOW_DESKTOP:
    if (win->sticky || win->desknum == globals.desknum)
      flag = 1;
    break;

  case SHOW_PAGE:
#if 0
    if (win->sticky || (win->desknum == globals.desknum && 
			win->x >= 0 && win->x < globals.screenx &&
			win->y >= 0 && win->y < globals.screeny))
    {
    }
#endif
    xmin = win->x;
    xmax = win->x + win->width;
    ymin = win->y;
    ymax = win->y + win->height;

    /* cases: (1) one of the corners is inside screen - handled here
              (2) one of the edges intersects an edge of the screen
	      (3) (1) with window and screen reversed
     */

/*  case 1:

    xmin >= 0 && xmin < screex && ymin >= 0 && ymin <= screeny
      ||
    xmin >= 0 && xmin < screex && ymax >= 0 && ymax <= screeny
      ||
    xmax >= 0 && xmax < screex && ymin >= 0 && ymin <= screeny
      ||
    xmax >= 0 && xmax < screex && ymax >= 0 && ymax <= screeny

goes to:

    xmin && (ymin || ymax)
      ||
    xmax && (ymin || ymax)

goes to:

    (xmin || xmax) && (ymin || max)

*/

/*  Case 2:
    
    xmin <= 0 && xmax >= 0 && ymin >= 0 && ymin <= screeny ||
    xmin <= screenx && xman >= screenx && ymin >= 0 && ymin <= screeny ||
    ymin <= 0 && ymax >= 0 && xmin >= 0 && xmax <= screenx ||
    ymin <= screeny && ymax >= screeny && xmin >= 0 && xmax <= screenx

goes to:

    (ymin >= 0 && ymin <= screeny) && 
          (xmin <= 0 && xmax >= 0 || xmin <= screenx && xmax >= screenx) ||
    (xmin >= 0 && xmax <= screenx) &&
          (ymin <= 0 && ymax >= 0 || ymin <= screeny && ymax >= screeny)
       
*/

    ConsoleDebug ("Screenx = %d, Screeny = %d\n", globals.screenx,
		  globals.screeny);
    ConsoleDebug ("Window (%s) coords: (%d, %d), (%d, %d)\n", win->iconname, xmin, ymin, xmax, ymax);
    if (win->sticky) {
      ConsoleDebug ("Sticky\n");
      flag = 1;
    }
    else if (win->desknum == globals.desknum) {
      if (((xmin >= 0 && xmin < globals.screenx) ||
	   (xmax >= 0 && xmax < globals.screenx)) &&
	  ((ymin >= 0 && ymin < globals.screeny) ||
	   (ymax >= 0 && ymax < globals.screeny))) {
	ConsoleDebug ("Window in screen\n");
	flag = 1;
      }
      else if (((ymin >= 0 && ymin < globals.screeny) && 
	       ((xmin <= 0 && xmax >= 0) || 
		(xmin < globals.screenx && xmax >= globals.screenx))) ||
	       ((xmin >= 0 && xmax <= globals.screenx) &&
	       ((ymin <= 0 && ymax >= 0) || 
		(ymin < globals.screeny && ymax >= globals.screeny)))) {
	ConsoleDebug ("Screen - window cross\n");
	flag = 1;
      }
      else if (((0 > xmin && 0 < xmax) ||
		(globals.screenx > xmin && globals.screenx < xmax)) &&
	       ((0 > ymin && 0 < ymax) ||
		(globals.screeny > 0 && globals.screeny < ymax))) {
	ConsoleDebug ("Screen in window\n");
	flag = 1;
      }
      else {
	ConsoleDebug ("Not in view\n");
	ConsoleDebug ("xmin = %d\txmax = %d\n", xmin, xmax);
	ConsoleDebug ("ymin = %d\tymax = %d\n", ymin, ymax);
	ConsoleDebug ("screenx = %d\tscreeny = %d\n", globals.screenx, 
		      globals.screeny);
	ConsoleDebug ("Expr: %d\n", 
		      ((0 >= xmin && 0) < xmax ||
		       (globals.screenx >= xmin && globals.screenx < xmax)) &&
		      ((0 >= ymin && 0 < ymax) ||
		       (globals.screeny >= 0 && globals.screeny < ymax)));
	  
      }
    }
    else {
      ConsoleDebug ("Not on desk\n");
    }
  }
  return flag;
}

static void reordered_iconlist (WinManager *man)
{
  WinData *p;
  int i;

  assert (man);

  ConsoleDebug ("Possibly reordered list, moving focus\n");

  for (p = man->icon_list.head, i = 0; p && !p->focus; 
       p = p->icon_next, i++) ;
    
  ConsoleDebug ("Focus was: %d, is %d\n", man->focus_box, i);
    
  man->focus_box = i;
}

static void check_in_iconlist (WinData *win, int draw)
{
  int in_viewport;

  if (win->manager && win->complete && !win->winlistskip) {
    in_viewport = win_in_viewport (win);
    if (win->in_iconlist == 0 && in_viewport) {
      insert_win_iconlist (win);
      reordered_iconlist (win->manager);
      if (draw)
	draw_added_icon (win->manager);
    }
    else if (win->in_iconlist && !in_viewport) {
      delete_win_iconlist (win, win->manager);
      reordered_iconlist (win->manager);
      if (draw)
	draw_deleted_icon (win->manager);
    }
  }
}

WinData *id_to_win (Ulong id)
{
  WinData *win;
  win = find_win_hashtab (id);
  if (win == NULL) {
    win = new_windata ();
    win->app_id = id;
    win->app_id_set = 1;
    insert_win_hashtab (win);
  }
  return win;
}

static void set_win_configuration (WinData *win, FvwmPacketBody *body)
{
  win->desknum = body->add_config_data.desknum;
  win->x = body->add_config_data.xpos;
  win->y = body->add_config_data.ypos;
  win->width = body->add_config_data.width;
  win->height = body->add_config_data.height;
  win->geometry_set = 1;


#if 0
  if (body->add_config_data.windows_flags & ICONIFIED) {
    win->iconified = 1;
  }
  else {
    win->iconified = 0;
    ConsoleDebug ("set_win_configuration: win(%d)->iconified = 0\n", 
		  win->app_id);
  }
#endif
  if (body->add_config_data.windows_flags & STICKY)
    win->sticky = 1;
  else
    win->sticky = 0;

  if (body->add_config_data.windows_flags & WINDOWLISTSKIP)
    win->winlistskip = 1;
  else
    win->winlistskip = 0;
}  

static void configure_window (FvwmPacketBody *body)
{
  Ulong app_id = body->add_config_data.app_id;
  WinData *win;
  WinManager *man;
  ConsoleDebug ("configure_window: %d\n", app_id);

  man = find_windows_manager (body->add_config_data.app_id);
  if (man) {
    man->win_x = body->add_config_data.xpos;
    man->win_y = body->add_config_data.ypos;
    man->win_title = body->add_config_data.window_title_height;
    man->win_border = body->add_config_data.window_border_width;
    if (man->win_border)
      man->win_border++;
    ConsoleDebug ("New Window: x, y: %d %d. title, border: %d %d\n",
                  man->win_x, man->win_y, man->win_title, man->win_border);
  }

  win = id_to_win (app_id);

  set_win_configuration (win, body);

  check_win_complete (win);
  check_in_iconlist (win, 1);
}

static void focus_change (FvwmPacketBody *body)
{ 
  Ulong app_id = body->minimal_data.app_id;
  WinData *win = id_to_win (app_id);
  int box;

  ConsoleDebug ("Focus Change\n");
  ConsoleDebug ("\tID: %d\n", app_id);

  if (globals.focus_win) {
    globals.focus_win->focus = 0;
  }
  win->focus = 1;

  if ( globals.focus_win &&
       globals.focus_win->complete &&
       globals.focus_win->manager ) {
    globals.focus_win->manager->focus_box = -1;
    draw_window (globals.focus_win->manager);
    globals.focus_win = NULL;
  }

  if ( win->complete  &&
       win->in_iconlist  &&
       win->manager->window_up  &&
       win->manager->followFocus ) {
    box = win_to_box(win->manager, win);
    win->manager->focus_box = box;
    draw_window (win->manager);
    if (globals.focus_win && globals.focus_win->manager != win->manager)
      draw_window (globals.focus_win->manager);
  }

  globals.focus_win = win;
  ConsoleDebug ("leaving focus_change\n");
}

static void res_name (FvwmPacketBody *body)
{
  Ulong app_id = body->name_data.app_id;
  Uchar *name = body->name_data.name.name;
  WinData *win;
  WinManager *oldman;
  int new;

  ConsoleDebug ("In res_name\n");

  win = id_to_win (app_id);

  copy_string (&win->resname, (char *)name);
  oldman = win->manager;
  new = set_win_manager (win, ALL_NAME);
  if (new) {
    if (oldman && win->in_iconlist) {
      delete_win_iconlist  (win, oldman);
      if (win == globals.focus_win)
	oldman->focus_box = -1;
      draw_deleted_icon (oldman);
    }
    assert (!win->in_iconlist);
  }
  
  check_win_complete (win);
  check_in_iconlist (win, 1);
  ConsoleDebug ("Exiting res_name\n");
}

static void class_name (FvwmPacketBody *body)
{
  Ulong app_id = body->name_data.app_id;
  Uchar *name = body->name_data.name.name;
  WinData *win;
  WinManager *oldman;
  int new;

  ConsoleDebug ("In class_name\n");

  win = id_to_win (app_id);

  copy_string (&win->classname, (char *)name);
  oldman = win->manager;
  new = set_win_manager (win, ALL_NAME);
  if (new) {
    if (oldman && win->in_iconlist) {
      delete_win_iconlist  (win, oldman);
      if (win == globals.focus_win)
	oldman->focus_box = -1;
      draw_deleted_icon (oldman);
    }
    assert (!win->in_iconlist);
  }
  
  check_win_complete (win);
  check_in_iconlist (win, 1);
  ConsoleDebug ("Exiting class_name\n");
}

static void icon_name (FvwmPacketBody *body)
{
  WinData *win;
  WinManager *oldman;
  Ulong app_id;
  Uchar *name = body->name_data.name.name;
  int moved = 0, new;

  ConsoleDebug ("In icon_name\n");

  app_id = body->name_data.app_id;

  win = id_to_win (app_id);
  copy_string (&win->iconname, (char *)name);
  oldman = win->manager;
  new = set_win_manager (win, ALL_NAME);
  check_win_complete (win);
  if (new) {
    if (oldman && win->in_iconlist) {
      delete_win_iconlist  (win, oldman);
      if (win == globals.focus_win)
	oldman->focus_box = -1;
      draw_deleted_icon (oldman);
    }
    assert (!win->in_iconlist);
    check_in_iconlist (win, 1);
  }
  else {
    if (win->in_iconlist && 
	!win->manager->use_titlename && win->manager->sort) {
      moved = move_win_iconlist (win);
    }
    if (moved) 
      reordered_iconlist (win->manager);
    if ((moved || win->in_iconlist) && win->complete)
      draw_window (win->manager);
  }

  ConsoleDebug ("Exiting icon_name\n");
}

static void window_name (FvwmPacketBody *body)
{
  WinData *win;
  WinManager *oldman;
  Ulong app_id;
  Uchar *name = body->name_data.name.name;
  int moved = 0, new;

  ConsoleDebug ("In window_name\n");

  app_id = body->name_data.app_id;

  win = id_to_win (app_id);
  copy_string (&win->titlename, (char *)name);

  oldman = win->manager;
  new = set_win_manager (win, ALL_NAME);
  check_win_complete (win);
  if (new) {
    if (oldman && win->in_iconlist) {
      delete_win_iconlist  (win, oldman);
      if (win == globals.focus_win)
	oldman->focus_box = -1;
      draw_deleted_icon (oldman);
    }
    assert (!win->in_iconlist);
    check_in_iconlist (win, 1);
  }
  else {
    if (win->in_iconlist && 
	win->manager->use_titlename && win->manager->sort) {
      moved = move_win_iconlist (win);
    }
    if (moved) 
      reordered_iconlist (win->manager);
    if ((moved || win->in_iconlist) && win->complete)
      draw_window (win->manager);
  }

  ConsoleDebug ("Exiting window_name\n");
}

static void new_window (FvwmPacketBody *body)
{
  WinData *win;
  WinManager *man;

  man = find_windows_manager (body->add_config_data.app_id);
  if (man) {
    man->win_x = body->add_config_data.xpos;
    man->win_y = body->add_config_data.ypos;
    man->win_title = body->add_config_data.window_title_height;
    man->win_border = body->add_config_data.window_border_width;
    if (man->win_border)
      man->win_border++;
    ConsoleDebug ("New Window: x, y: %d %d. title, border: %d %d\n",
                  man->win_x, man->win_y, man->win_title, man->win_border);
  }


  win = new_windata();
  if (!(body->add_config_data.windows_flags & TRANSIENT)) {
    win->app_id = body->add_config_data.app_id;
    win->app_id_set = 1;
    set_win_configuration (win, body);

    insert_win_hashtab (win);
    check_win_complete (win);
    check_in_iconlist (win, 1);
  }
}

static void destroy_window (FvwmPacketBody *body)
{
  WinData *win;
  Ulong app_id;

  app_id = body->minimal_data.app_id;
  win = id_to_win (app_id);
  delete_win_hashtab (win);
  if (globals.focus_win == win)
    globals.focus_win = NULL;
  if (win->in_iconlist) {
    delete_win_iconlist (win, win->manager);
    reordered_iconlist (win->manager);
    update_window_stuff (win->manager);
  }
  free_windata (win);
}

static void iconify (FvwmPacketBody *body, int dir)
{
  Ulong app_id = body->minimal_data.app_id;
  WinData *win;
  int box;
  
  win = id_to_win (app_id);
  
  if (dir == 0) {
    if (win->iconified == 0) {
      ConsoleDebug ("Already deiconified\n");
      return;
    }
    else {
      ConsoleDebug ("iconify: win(%d)->iconified = 0\n", win->app_id);
      win->iconified = 0;
    }
  }
  else {
    if (win->iconified == 1) {
      ConsoleDebug ("Already iconified\n");
      return;
    }
    else {
      win->iconified = 1;
    }
  }
  
  check_win_complete (win);
  check_in_iconlist (win, 1);
  if (win->complete && win->in_iconlist) {
    box = win_to_box (win->manager, win);
    if (box >= 0)
      draw_button (win->manager, win, box );
    else
      ConsoleMessage ("Internal error in iconify\n");
  }
}

void update_win_in_hashtab (void *arg)
{
  WinData *p = (WinData *)arg;
  check_in_iconlist (p, 0);
}

static void new_desk (FvwmPacketBody *body)
{
  int i;

  globals.desknum = body->new_desk_data.desknum;
  walk_hashtab (update_win_in_hashtab);

  for (i = 0; i < globals.num_managers; i++)
    update_window_stuff (&globals.managers[i]);
}

static void ProcessMessage (Ulong type, FvwmPacketBody *body)
{
  int i;

  ConsoleDebug ("FVWM Message type: %d\n", type); 

  switch(type) {
  case M_CONFIGURE_WINDOW:
    ConsoleDebug ("DEBUG::M_CONFIGURE_WINDOW\n");
    configure_window (body);
    break;

  case M_FOCUS_CHANGE:
    ConsoleDebug ("DEBUG::M_FOCUS_CHANGE\n");
    focus_change (body);
    break;

  case M_RES_NAME:
    ConsoleDebug ("DEBUG::M_RES_NAME\n");
    res_name (body);
    break;

  case M_RES_CLASS:
    ConsoleDebug ("DEBUG::M_RES_CLASS\n");
    class_name (body);
    break;

  case M_MAP:
    ConsoleDebug ("DEBUG::M_MAP\n");
    break;

  case M_ADD_WINDOW:
    ConsoleDebug ("DEBUG::M_ADD_WINDOW\n");
    new_window (body);
    break;

  case M_DESTROY_WINDOW:
    ConsoleDebug ("DEBUG::M_DESTROY_WINDOW\n");
    destroy_window (body);
    break;

  case M_WINDOW_NAME:
    ConsoleDebug ("DEBUG::M_WINDOW_NAME\n");
    window_name (body);
    break;

  case M_ICON_NAME:
    ConsoleDebug ("DEBUG::M_ICON_NAME\n");
    icon_name (body);
    break;

  case M_DEICONIFY:
    ConsoleDebug ("DEBUG::M_DEICONIFY\n");
    iconify (body, 0);
    break;

  case M_ICONIFY:
    ConsoleDebug ("DEBUG::M_ICONIFY\n");
    iconify (body, 1);
    break;

  case M_END_WINDOWLIST:
    ConsoleDebug ("DEBUG::M_END_WINDOWLIST\n");
    ConsoleDebug (">>>>>>>>>>>>>>>>>>>>>>>End window list<<<<<<<<<<<<<<<\n");
    if (globals.focus_win && globals.focus_win->in_iconlist) {
	globals.focus_win->manager->focus_box = 
	  win_to_box (globals.focus_win->manager, globals.focus_win);
    }
    for (i = 0; i < globals.num_managers; i++)
      init_window (i);
    print_iconlist();
    break;

  case M_NEW_DESK:
    ConsoleDebug ("DEBUG::M_NEW_DESK\n");
    new_desk (body);
    break;

  case M_NEW_PAGE:
    ConsoleDebug ("DEBUG::M_NEW_PAGE\n");
    globals.x = body->new_page_data.x;
    globals.y = body->new_page_data.y;
    globals.desknum = body->new_page_data.desknum;
    break;

  default:
    break;
  }
}

void ReadFvwmPipe()
{
  int body_length;
  FvwmPacketHeader header;
  FvwmPacketBody *body;

  ConsoleDebug("DEBUG: entering ReadFvwmPipe\n");
  body_length = ReadFvwmPacket(Fvwm_fd[1], (unsigned long *) &header,
                 (unsigned long **)&body);
  body_length -= HEADER_SIZE;
  if (header.start == START_FLAG) {
    ProcessMessage (header.type, body);
    if (body_length) {
      free (body);
    }
  }
  else {
    DeadPipe (1);
  }
  ConsoleDebug("DEBUG: leaving ReadFvwmPipe\n");
}

