/****************************************************************************
 * This module is all original code 
 * by Rob Nation 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

#include <FVWMconfig.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

static char *exec_shell_name="/bin/sh";

#include <X11/xpm.h>

/***********************************************************************
 *
 *  Procedure:
 *	DeferExecution - defer the execution of a function to the
 *	    next button press if the context is C_ROOT
 *
 *  Inputs:
 *      eventp  - pointer to XEvent to patch up
 *      w       - pointer to Window to patch up
 *      tmp_win - pointer to FvwmWindow Structure to patch up
 *	context	- the context in which the mouse button was pressed
 *	func	- the function to defer
 *	cursor	- the cursor to display while waiting
 *      finishEvent - ButtonRelease or ButtonPress; tells what kind of event to
 *                    terminate on.
 *
 ***********************************************************************/
int DeferExecution(XEvent *eventp, Window *w,FvwmWindow **tmp_win,
		   unsigned long *context, int cursor, int FinishEvent)

{
  int done;
  int finished = 0;
  Window dummy;
  Window original_w;

  original_w = *w;

  if((*context != C_ROOT)&&(*context != C_NO_CONTEXT))
  {
    if((FinishEvent == ButtonPress)||((FinishEvent == ButtonRelease) &&
                                      (eventp->type != ButtonPress)))
    {
      return FALSE;
    }
  }
  if(!GrabEm(cursor))
  {
    XBell(dpy,Scr.screen);
    return True;
  }
  
  while (!finished)
  {
    done = 0;
    /* block until there is an event */
    XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
               ExposureMask |KeyPressMask | VisibilityChangeMask |
               ButtonMotionMask| PointerMotionMask/* | EnterWindowMask | 
                                                     LeaveWindowMask*/, eventp);
    StashEventTime(eventp);

    if(eventp->type == KeyPress)
      Keyboard_shortcuts(eventp,FinishEvent);	
    if(eventp->type == FinishEvent)
      finished = 1;
    if(eventp->type == ButtonPress)
    {
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
      done = 1;
    }
    if(eventp->type == ButtonRelease)
      done = 1;
    if(eventp->type == KeyPress)
      done = 1;
      
    if(!done)
    {
      DispatchEvent();
    }

  }

  
  *w = eventp->xany.window;
  if(((*w == Scr.Root)||(*w == Scr.NoFocusWin))
     && (eventp->xbutton.subwindow != (Window)0))
  {
    *w = eventp->xbutton.subwindow;
    eventp->xany.window = *w;
  }
  if (*w == Scr.Root)
  {
    *context = C_ROOT;
    XBell(dpy,Scr.screen);
    UngrabEm();
    return TRUE;
  }
  if (XFindContext (dpy, *w, FvwmContext, (caddr_t *)tmp_win) == XCNOENT)
  {
    *tmp_win = NULL;
    XBell(dpy,Scr.screen);
    UngrabEm();
    return (TRUE);
  }

  if(*w == (*tmp_win)->Parent)
    *w = (*tmp_win)->w;

  if(original_w == (*tmp_win)->Parent)
    original_w = (*tmp_win)->w;
  
  /* this ugly mess attempts to ensure that the release and press
   * are in the same window. */
  if((*w != original_w)&&(original_w != Scr.Root)&&
     (original_w != None)&&(original_w != Scr.NoFocusWin))
    if(!((*w == (*tmp_win)->frame)&&
         (original_w == (*tmp_win)->w)))
    {
      *context = C_ROOT;
      XBell(dpy,Scr.screen);
      UngrabEm();
      return TRUE;
    }
  
  *context = GetContext(*tmp_win,eventp,&dummy);
  
  UngrabEm();
  return FALSE;
}




/**************************************************************************
 *
 * Moves focus to specified window 
 *
 *************************************************************************/
void FocusOn(FvwmWindow *t,int DeIconifyOnly, int RaiseWarp)
{
#ifndef NON_VIRTUAL
  int dx,dy;
  int cx,cy;
#endif
  int x,y;

  if(t == (FvwmWindow *)0)
    return;

  if(t->Desk != Scr.CurrentDesk && RaiseWarp)
  {
    changeDesks(0,t->Desk);
  }

#ifndef NON_VIRTUAL
  if(t->flags & ICONIFIED)
  {
    cx = t->icon_xl_loc + t->icon_w_width/2;
    cy = t->icon_y_loc + t->icon_p_height + ICON_HEIGHT/2;
  }
  else
  {
    cx = t->frame_x + t->frame_width/2;
    cy = t->frame_y + t->frame_height/2;
  }

  dx = (cx + Scr.Vx)/Scr.MyDisplayWidth*Scr.MyDisplayWidth;
  dy = (cy +Scr.Vy)/Scr.MyDisplayHeight*Scr.MyDisplayHeight;

  if (RaiseWarp)
    MoveViewport(dx,dy,True);
#endif

  if(t->flags & ICONIFIED)
  {
    x = t->icon_xl_loc + t->icon_w_width/2;
    y = t->icon_y_loc + t->icon_p_height + ICON_HEIGHT/2;
  }
  else
  {
    x = t->frame_x;
    y = t->frame_y;
  }

  if (RaiseWarp) {
#if 0 /* don't want to warp the pointer by default anymore */
    if(!(t->flags & ClickToFocus))
      XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, x+2,y+2);
#endif /* 0 */
    RaiseWindow(t);
    KeepOnTop();

    /* If the window is still not visible, make it visible! */
    if(((t->frame_x + t->frame_height)< 0)||(t->frame_y + t->frame_width < 0)||
       (t->frame_x >Scr.MyDisplayWidth)||(t->frame_y>Scr.MyDisplayHeight))
    {
      SetupFrame(t,0,0,t->frame_width, t->frame_height,False);
      if(!(t->flags & ClickToFocus))
        XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, 2,2);
    }
    UngrabEm();
  }
  SetFocus(t->w,t,1); /*0);*/
}


/**************************************************************************
 *
 * Moves pointer to specified window 
 *
 *************************************************************************/
void WarpOn(FvwmWindow *t,int warp_x, int x_unit, int warp_y, int y_unit)
{
#ifndef NON_VIRTUAL
  int dx,dy;
  int cx,cy;
#endif
  int x,y;

  if(t == (FvwmWindow *)0 || (t->flags & ICONIFIED && t->icon_w == None))
    return;

  if(t->Desk != Scr.CurrentDesk)
  {
    changeDesks(0,t->Desk);
  }

#ifndef NON_VIRTUAL
  if(t->flags & ICONIFIED)
  {
    cx = t->icon_xl_loc + t->icon_w_width/2;
    cy = t->icon_y_loc + t->icon_p_height + ICON_HEIGHT/2;
  }
  else
  {
    cx = t->frame_x + t->frame_width/2;
    cy = t->frame_y + t->frame_height/2;
  }

  dx = (cx + Scr.Vx)/Scr.MyDisplayWidth*Scr.MyDisplayWidth;
  dy = (cy +Scr.Vy)/Scr.MyDisplayHeight*Scr.MyDisplayHeight;

  MoveViewport(dx,dy,True);
#endif

  if(t->flags & ICONIFIED)
  {
    x = t->icon_xl_loc + t->icon_w_width/2 + 2;
    y = t->icon_y_loc + t->icon_p_height + ICON_HEIGHT/2 + 2;
  }
  else
  {
    if (x_unit != Scr.MyDisplayWidth)
      x = t->frame_x + 2 + warp_x;
    else
      x = t->frame_x + 2 + (t->frame_width - 4) * warp_x / 100;
    if (y_unit != Scr.MyDisplayHeight) 
      y = t->frame_y + 2 + warp_y;
    else
      y = t->frame_y + 2 + (t->frame_height - 4) * warp_y / 100;
  }
  if (warp_x >= 0 && warp_y >= 0) {
    XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, x, y);
  }
  RaiseWindow(t);
  KeepOnTop();

  /* If the window is still not visible, make it visible! */
  if(((t->frame_x + t->frame_height)< 0)||(t->frame_y + t->frame_width < 0)||
     (t->frame_x >Scr.MyDisplayWidth)||(t->frame_y>Scr.MyDisplayHeight))
  {
    SetupFrame(t,0,0,t->frame_width, t->frame_height,False);
    XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, 2,2);
  }
  UngrabEm();
}


   
/***********************************************************************
 *
 *  Procedure:
 *	(Un)Maximize a window.
 *
 ***********************************************************************/
void Maximize(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	      unsigned long context, char *action, int *Module)
{
  int new_width, new_height, new_x, new_y;
  int val1, val2, val1_unit, val2_unit, n;

  if (DeferExecution(eventp, &w, &tmp_win, &context, SELECT, ButtonRelease))
    return;

  if(tmp_win == NULL)
    return;
  
  if(check_allowed_function2(F_MAXIMIZE, tmp_win) == 0)
  {
    XBell(dpy, Scr.screen);
    return;
  }
  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
  if(n != 2)
  {
    val1 = 100;
    val2 = 100;
    val1_unit = Scr.MyDisplayWidth;
    val2_unit = Scr.MyDisplayHeight;
  }
  
  if (tmp_win->flags & MAXIMIZED)
  {
    tmp_win->flags &= ~MAXIMIZED;
    SetupFrame(tmp_win, tmp_win->orig_x, tmp_win->orig_y, tmp_win->orig_wd,
               tmp_win->orig_ht,TRUE);
    SetBorder(tmp_win,True,True,True,None);
  }
  else
  {
    new_width = tmp_win->frame_width;      
    new_height = tmp_win->frame_height;
    new_x = tmp_win->frame_x;
    new_y = tmp_win->frame_y;
    if(val1 >0)
    {
      new_width = val1*val1_unit/100;
      new_x = 0;
    }
    if(val2 >0)
    {
      new_height = val2*val2_unit/100;
      new_y = 0;
    }
    if((val1==0)&&(val2==0))
    {
      new_x = 0;
      new_y = 0;
      new_height = Scr.MyDisplayHeight;
      new_width = Scr.MyDisplayWidth;
    }
    tmp_win->flags |= MAXIMIZED;
    ConstrainSize (tmp_win, &new_width, &new_height);
    SetupFrame(tmp_win,new_x,new_y,new_width,new_height,TRUE);
    SetBorder(tmp_win,Scr.Hilite == tmp_win,True,True,tmp_win->right_w[0]);
  }
}

/* For Ultrix 4.2 */
#include <sys/types.h>
#include <sys/time.h>


MenuRoot *FindPopup(char *action)
  {
  char *tmp;
  MenuRoot *mr;
  int x, y, dummy;

  action = GetNextToken(action, &tmp);
  
  if (tmp == NULL) return NULL;

  mr = Scr.AllMenus;
  while (mr != NULL)
    {
    if (mr->name != NULL)
      if (strcasecmp(tmp, mr->name)== 0)
        {
        free(tmp);

        /* we found the menu, now check for additional parameters
           (coordinates)
         */
        mr->has_coords = False;
        if (action[0] == 0) return mr;

        if (GetTwoArguments(action, &(mr->x), &(mr->y), &dummy, &dummy) == 2)
          mr->has_coords = True;

        return mr;
        }
    mr = mr->next;
    }
  free(tmp);
  return NULL;
  }

      
  
void Bell(XEvent *eventp,Window w,FvwmWindow *tmp_win,unsigned long context,
	  char *action, int *Module)
{
  XBell(dpy, Scr.screen);
}


char *last_menu = NULL;
void add_item_to_menu(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,
		      char *action, int *Module)
{
  MenuRoot *mr;
  char *token, *rest, *item;

  rest = GetNextToken(action, &token);
  mr = FindPopup(token);

  if(mr == NULL)
    mr = NewMenuRoot(token, F_POPUP);
  free(token);

  if(last_menu != NULL)
    free(last_menu);

  CopyString(&last_menu, mr->name);
  rest = GetNextToken(rest, &item);

  AddToMenu(mr, item, rest);
  free(item);
  
  MakeMenu(mr);
  return;
}


void add_another_item(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,
		      char *action, int *Module)
{
  MenuRoot *mr;
  char *rest, *item;

  if(last_menu == NULL) return;

  mr = FindPopup(last_menu);
  if(mr == NULL) return;

  rest = GetNextToken(action, &item);

  AddToMenu(mr, item,rest);
  free(item);
  
  MakeMenu(mr);
  return;
}

void destroy_menu(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                  unsigned long context,
                  char *action, int *Module)
{
  MenuRoot *mr;

  char *token, *rest;

  rest = GetNextToken(action, &token);
  mr = FindPopup(token);
  if(mr == NULL) return;
  DestroyMenu(mr);
  return;
}

void add_item_to_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,
		      char *action, int *Module)
{
  MenuRoot *mr;
  char *token, *rest, *item;

  rest = GetNextToken(action, &token);
  mr = FindPopup(token);
  if(mr == NULL)
    mr = NewMenuRoot(token, F_FUNCTION);
  if(last_menu != NULL)
    free(last_menu);
  last_menu = token;
  rest = GetNextToken(rest, &item);

  AddToMenu(mr, item, rest);
  free(item);
  
  return;
}
  

void Nop_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,unsigned long context,
              char *action, int *Module)
{

}



void movecursor(XEvent *eventp,Window w,FvwmWindow *tmp_win,unsigned long context,
		char *action, int *Module)
{
#ifndef NON_VIRTUAL
  int x,y,delta_x,delta_y,warp_x,warp_y;
  int val1, val2, val1_unit,val2_unit,n;

  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);

  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
                 &x,&y,&JunkX, &JunkY, &JunkMask);
  delta_x = 0;
  delta_y = 0;
  warp_x = 0;
  warp_y = 0;
  if(x >= Scr.MyDisplayWidth -2)
  {
    delta_x = Scr.EdgeScrollX;
    warp_x = Scr.EdgeScrollX - 4;
  }
  if(y>= Scr.MyDisplayHeight -2)
  {
    delta_y = Scr.EdgeScrollY;
    warp_y = Scr.EdgeScrollY - 4;      
  }
  if(x < 2)
  {
    delta_x = -Scr.EdgeScrollX;
    warp_x =  -Scr.EdgeScrollX + 4;
  }
  if(y < 2)
  {
    delta_y = -Scr.EdgeScrollY;
    warp_y =  -Scr.EdgeScrollY + 4;
  }
  if(Scr.Vx + delta_x < 0)
    delta_x = -Scr.Vx;
  if(Scr.Vy + delta_y < 0)
    delta_y = -Scr.Vy;
  if(Scr.Vx + delta_x > Scr.VxMax)
    delta_x = Scr.VxMax - Scr.Vx;
  if(Scr.Vy + delta_y > Scr.VyMax)
    delta_y = Scr.VyMax - Scr.Vy;
  if((delta_x!=0)||(delta_y!=0))
  {
    MoveViewport(Scr.Vx + delta_x,Scr.Vy+delta_y,True);
    XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, 
                 Scr.MyDisplayHeight, 
                 x - warp_x,
                 y - warp_y);
  }
#endif
  XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, 
	       Scr.MyDisplayHeight, x + val1*val1_unit/100-warp_x,
	       y+val2*val2_unit/100 - warp_y);
}


void iconify_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context,char *action, int *Module)

{
  long val1;
  int val1_unit, n;

  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT, 
		     ButtonRelease))
    return;

  n = GetOneArgument(action, &val1, &val1_unit);

  if (tmp_win->flags & ICONIFIED)
  {
    if(val1 <=0)
      DeIconify(tmp_win);
  }
  else
  {
    if(check_allowed_function2(F_ICONIFY,tmp_win) == 0)
    {
      XBell(dpy, Scr.screen);
      return;
    }
    if(val1 >=0)
      Iconify(tmp_win,eventp->xbutton.x_root-5,eventp->xbutton.y_root-5);
  }
}

void raise_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context, char *action, int *Module)
{
  char *junk, *junkC;
  unsigned long junkN;
  int junkD, method, BoxJunk[4];

  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
    return;
      
  if(tmp_win)
    RaiseWindow(tmp_win);

  if (LookInList(Scr.TheList,tmp_win->realname, &tmp_win->class, &junk, &junk,
                 &junkD, &junkD, &junkD, &junkC, &junkC, &junkN,
		 BoxJunk, &method)& STAYSONTOP_FLAG)
    tmp_win->flags |= ONTOP;
  KeepOnTop();
}

void lower_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context,char *action, int *Module)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT, ButtonRelease))
    return;

  LowerWindow(tmp_win);
  
  tmp_win->flags &= ~ONTOP;
}

void destroy_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action, int *Module)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, DESTROY, ButtonRelease))
    return;

  if(check_allowed_function2(F_DESTROY,tmp_win) == 0)
  {
    XBell(dpy, Scr.screen);
    return;
  }
  
  if (XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
		   &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
    Destroy(tmp_win);
  else
    XKillClient(dpy, tmp_win->w);
  XSync(dpy,0);
}

void delete_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context,char *action, int *Module)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, DESTROY,ButtonRelease))
    return;

  if(check_allowed_function2(F_DELETE,tmp_win) == 0)
  {
    XBell(dpy, Scr.screen);
    return;
  }
  
  if (tmp_win->flags & DoesWmDeleteWindow)
  {
    send_clientmessage (tmp_win->w, _XA_WM_DELETE_WINDOW, CurrentTime);
    return;
  }
  else
    XBell (dpy, Scr.screen);
  XSync(dpy,0);
}

void close_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context,char *action, int *Module)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context, DESTROY,ButtonRelease))
    return;

  if(check_allowed_function2(F_CLOSE,tmp_win) == 0)
  {
    XBell(dpy, Scr.screen);
    return;
  }
  
  if (tmp_win->flags & DoesWmDeleteWindow)
  {
    send_clientmessage (tmp_win->w, _XA_WM_DELETE_WINDOW, CurrentTime);
    return;
  }
  else if (XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
			&JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
    Destroy(tmp_win);
  else
    XKillClient(dpy, tmp_win->w);
  XSync(dpy,0);
}      

void restart_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action, int *Module)
{
  Done(1, action);
}

void exec_setup(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                unsigned long context,char *action, int *Module)
{
  char *arg=NULL;

  action = GetNextToken(action,&arg);

  if (arg && (strcmp(arg,"")!=0)) /* specific shell was specified */
  {
    exec_shell_name = strdup(arg);
  }
  else /* no arg, so use $SHELL -- not working??? */
  {
    if (getenv("SHELL"))
      exec_shell_name = strdup(getenv("SHELL"));
    else
      exec_shell_name = strdup("/bin/sh"); /* if $SHELL not set, use default */
  }
}

void exec_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		   unsigned long context,char *action, int *Module)
{
  char *cmd = NULL;
 
  /* if it doesn't already have an 'exec' as the first word, add that
   * to keep down number of procs started */

  /* need to parse string better to do this right though, so not doing this
     for now... */
  if (0 && strncasecmp(action, "exec", 4) != 0)
    {
    cmd = (char *)safemalloc(strlen(action)+6);
    strcpy(cmd, "exec ");
    strcat(cmd, action);
    }
  else
    {
    cmd = strdup(action);
    }

  /* Use to grab the pointer here, but the fork guarantees that
   * we wont be held up waiting for the function to finish,
   * so the pointer-gram just caused needless delay and flashing
   * on the screen */
  /* Thought I'd try vfork and _exit() instead of regular fork().
   * The man page says that its better. */
  /* Not everyone has vfork! */
  /* We do now */
  if (!(vfork())) /* child process */
    {
    if (execl(exec_shell_name, exec_shell_name, "-c", cmd, NULL)==-1)
      {
#if defined(sun) && !defined(SVR4)
      /* XXX: dirty hack for SunOS 4.x */
      extern char *sys_errlist[];

      /* no strerror() function, see sys_errlist[] instead */
      fvwm_msg(ERR,"exec_function","execl failed (%s)",sys_errlist[errno]);
#else
      fvwm_msg(ERR,"exec_function","execl failed (%s)",strerror(errno));
#endif
      exit(100);
      }
    }
  free(cmd);
  return;
}

void refresh_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		      unsigned long context, char *action, int *Module)
{
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  
  valuemask = (CWBackPixel);
  attributes.background_pixel = 0;
  attributes.backing_store = NotUseful;
  w = XCreateWindow (dpy, Scr.Root, 0, 0,
		     (unsigned int) Scr.MyDisplayWidth,
		     (unsigned int) Scr.MyDisplayHeight,
		     (unsigned int) 0,
		     CopyFromParent, (unsigned int) CopyFromParent,
		     (Visual *) CopyFromParent, valuemask,
		     &attributes);
  XMapWindow (dpy, w);
  XDestroyWindow (dpy, w);
  XFlush (dpy);
}


void stick_function(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		    unsigned long context, char *action, int *Module)
{
  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

  if(tmp_win->flags & STICKY)
  {
    tmp_win->flags &= ~STICKY;
  }
  else
  {
    tmp_win->flags |=STICKY;
  }
  BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);
  SetTitleBar(tmp_win,(Scr.Hilite==tmp_win),True);
}

void wait_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	       unsigned long context,char *action, int *Module)
{
  Bool done = False;
  extern FvwmWindow *Tmp_win;

  while(!done)
  {
    if(My_XNextEvent(dpy, &Event))
    {
      DispatchEvent ();
      if(Event.type == MapNotify)
      {
        if((Tmp_win)&&(matchWildcards(action,Tmp_win->name)==True))
          done = True;
        if((Tmp_win)&&(Tmp_win->class.res_class)&&
           (matchWildcards(action,Tmp_win->class.res_class)==True))
          done = True;
        if((Tmp_win)&&(Tmp_win->class.res_name)&&
           (matchWildcards(action,Tmp_win->class.res_name)==True))
          done = True;
      }
    }
  }
}


void raise_it_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		   unsigned long context, char *action, int *Module)
{
  long val1;
  int  val1_unit, n;

  n = GetOneArgument(action, &val1, &val1_unit);

  if(val1 != 0)
  {
    FocusOn((FvwmWindow *)val1, 0, 1);
    if (((FvwmWindow *)(val1))->flags & ICONIFIED)
    {
      DeIconify((FvwmWindow *)val1);
      FocusOn((FvwmWindow *)val1, 0, 1);
    }
  }
}


void focus_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action, int *Module)
{
  long val1;
  int  val1_unit, n;

  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

  n = GetOneArgument(action, &val1, &val1_unit);

  if (!n) val1 = 1;

  FocusOn(tmp_win, 0, val1);
}


void warp_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
               unsigned long context, char *action, int *Module)
{
  int val1_unit, val2_unit, n;
  int val1, val2;

  if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
    return;

  n = GetTwoArguments (action, &val1, &val2, &val1_unit, &val2_unit);

  if (n == 2)
    WarpOn (tmp_win, val1, val1_unit, val2, val2_unit);
  else
    WarpOn (tmp_win, 0, 0, 0, 0);
}


void popup_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int *Module)
{
  MenuRoot *menu;
  extern int menuFromFrameOrWindowOrTitlebar;
  extern FvwmWindow *ButtonWindow;

  ButtonWindow = tmp_win;
  menu = FindPopup(action);
  if(menu == NULL)
  {
    fvwm_msg(ERR,"popup_func","No such menu %s",action);
    return;
  }
  ActiveItem = NULL;
  ActiveMenu = NULL;
  menuFromFrameOrWindowOrTitlebar = FALSE;
  do_menu(menu,0);
}

void staysup_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int *Module)
{
  MenuRoot *menu;
  extern int menuFromFrameOrWindowOrTitlebar;
  char *default_action = NULL, *menu_name = NULL;
  extern int menu_aborted;

  action = GetNextToken(action, &menu_name);
  GetNextToken(action, &default_action);
  menu = FindPopup(menu_name);
  if(menu == NULL)
  {
    if(menu_name != NULL)
    {
      fvwm_msg(ERR,"staysup_func","No such menu %s",menu_name);
      free(menu_name);
    }
    if(default_action != NULL)
      free(default_action);
    return;
  }
  ActiveItem = NULL;
  ActiveMenu = NULL;
  menuFromFrameOrWindowOrTitlebar = FALSE;

  /* See bottom of windows.c for rationale behind this */
  if (eventp->type == ButtonPress)
    do_menu(menu,1);
  else
    do_menu(menu,0);

  if(menu_name != NULL)
    free(menu_name);
  if((menu_aborted)&&(default_action != NULL))
    ExecuteFunction(default_action,tmp_win,eventp,context,*Module);
  if(default_action != NULL)
    free(default_action);
}


void quit_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	       unsigned long context, char *action,int *Module)
{
  if (master_pid != getpid())
    kill(master_pid, SIGTERM);
  Done(0,NULL);
}

void quit_screen_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                      unsigned long context, char *action,int *Module)
{
  Done(0,NULL);
}

void raiselower_func(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int *Module)
{
  char *junk, *junkC;
  unsigned long junkN;
  int junkD,method, BoxJunk[4];

  if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
    return;
  if(tmp_win == NULL)
    return;
  
  if((tmp_win == Scr.LastWindowRaised)||
     (tmp_win->flags & VISIBLE))
  {
    LowerWindow(tmp_win);
    tmp_win->flags &= ~ONTOP;
  }
  else
  {
    RaiseWindow(tmp_win);
    if (LookInList(Scr.TheList,tmp_win->realname, &tmp_win->class,&junk,&junk,
                   &junkD,&junkD, &junkD, &junkC,&junkC,&junkN,BoxJunk,
                   &method)&STAYSONTOP_FLAG)
      tmp_win->flags |= ONTOP;	    
    KeepOnTop();
  }
}

void SetEdgeScroll(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                   unsigned long context, char *action,int* Module)
{
  int val1, val2, val1_unit,val2_unit,n;

  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
  if(n != 2)
  {
    fvwm_msg(ERR,"SetEdgeScroll","EdgeScroll requires two arguments");
    return;
  }

  /*
  ** if edgescroll >1000 and < 100000m
  ** wrap at edges of desktop (a "spherical" desktop)
  */
  if (val1 >= 1000) 
  {
    val1 /= 1000;
    Scr.flags |= EdgeWrapX;
  }
  else
  {
    Scr.flags &= ~EdgeWrapX;
  }
  if (val2 >= 1000) 
  {
    val2 /= 1000;
    Scr.flags |= EdgeWrapY;
  }
  else
  {
    Scr.flags &= ~EdgeWrapY;
  }

  Scr.EdgeScrollX = val1*val1_unit/100;
  Scr.EdgeScrollY = val2*val2_unit/100;

  checkPanFrames();
}

void SetEdgeResistance(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                       unsigned long context, char *action,int* Module)
{
  int val1, val2, val1_unit,val2_unit,n;
  
  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
  if(n != 2)
  {
    fvwm_msg(ERR,"SetEdgeResistance","EdgeResistance requires two arguments");
    return;
  }

  Scr.ScrollResistance = val1;
  Scr.MoveResistance = val2;
}

void SetColormapFocus(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                      unsigned long context, char *action,int* Module)
{
  if (strncasecmp(action,"FollowsFocus",12)==0)
  {
    Scr.ColormapFocus = COLORMAP_FOLLOWS_FOCUS;
  }
  else if (strncasecmp(action,"FollowsMouse",12)==0)
  {
    Scr.ColormapFocus = COLORMAP_FOLLOWS_MOUSE;
  }
  else
  {
    fvwm_msg(ERR,"SetColormapFocus",
             "ColormapFocus requires 1 arg: FollowsFocus or FollowsMouse");
    return;
  }
}

void SetClick(XEvent *eventp,Window w,FvwmWindow *tmp_win,
              unsigned long context, char *action,int* Module)
{
  long val1;
  int val1_unit,n;

  n = GetOneArgument(action, &val1, &val1_unit);
  if(n != 1)
  {
    fvwm_msg(ERR,"SetClick","ClickTime requires 1 argument");
    return;
  }

  Scr.ClickTime = val1;
}

void SetXOR(XEvent *eventp,Window w,FvwmWindow *tmp_win,
            unsigned long context, char *action,int* Module)
{
  long val1;
  int val1_unit,n;
  XGCValues gcv;
  unsigned long gcm;

  n = GetOneArgument(action, &val1, &val1_unit);
  if(n != 1)
  {
    fvwm_msg(ERR,"SetXOR","XORValue requires 1 argument");
    return;
  }

#if 0  /* 1 for XFree86 W32P */
  gcm = GCFunction|GCLineWidth|GCForeground|GCSubwindowMode;
  gcv.function   = GXxor;
  gcv.line_width = 0;
  gcv.foreground = val1;
  gcv.subwindow_mode = IncludeInferiors;
#else
  gcm = GCFunction|GCLineWidth|GCFillStyle|GCForeground|GCStipple|
        GCBackground|GCSubwindowMode|GCPlaneMask; 
  gcv.function   = GXxor;
  gcv.line_width = 1;
  gcv.plane_mask = AllPlanes;
  gcv.stipple    = Scr.gray_bitmap;
  gcv.fill_style = FillOpaqueStippled;
  gcv.foreground = val1;
  gcv.subwindow_mode = IncludeInferiors;
#endif
  Scr.DrawGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
}

void SetOpaque(XEvent *eventp,Window w,FvwmWindow *tmp_win,
	       unsigned long context, char *action,int* Module)
{
  long val1;
  int val1_unit,n;

  n = GetOneArgument(action, &val1, &val1_unit);
  if(n != 1)
  {
    fvwm_msg(ERR,"SetOpaque","OpaqueMoveSize requires 1 argument");
    return;
  }

  Scr.OpaqueSize = val1;
}


void SetDeskSize(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                 unsigned long context, char *action,int* Module)
{
  int val1, val2, val1_unit,val2_unit,n;

  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);
  if(n != 2)
  {
    fvwm_msg(ERR,"SetDeskSize","DesktopSize requires two arguments");
    return;
  }
  if((val1_unit != Scr.MyDisplayWidth)||
     (val2_unit != Scr.MyDisplayHeight))
  {
    fvwm_msg(ERR,"SetDeskSize","DeskTopSize arguments should be unitless");
  }

  Scr.VxMax = val1;
  Scr.VyMax = val2;
  Scr.VxMax = Scr.VxMax*Scr.MyDisplayWidth - Scr.MyDisplayWidth;
  Scr.VyMax = Scr.VyMax*Scr.MyDisplayHeight - Scr.MyDisplayHeight;
  if(Scr.VxMax <0)
    Scr.VxMax = 0;
  if(Scr.VyMax <0)
    Scr.VyMax = 0;
  Broadcast(M_NEW_PAGE,5,Scr.Vx,Scr.Vy,Scr.CurrentDesk,Scr.VxMax,Scr.VyMax,0,0);

  checkPanFrames();
}

#ifdef XPM
char *PixmapPath = FVWM_ICONDIR;
void setPixmapPath(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                   unsigned long context, char *action,int* Module)
{
  static char *ptemp = NULL;
  char *tmp;

  if(ptemp == NULL)
    ptemp = PixmapPath;

  if((PixmapPath != ptemp)&&(PixmapPath != NULL))
    free(PixmapPath);
  tmp = stripcpy(action);
  PixmapPath = envDupExpand(tmp, 0);
  free(tmp);
}
#endif

char *IconPath = FVWM_ICONDIR;
void setIconPath(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                 unsigned long context, char *action,int* Module)
{
  static char *ptemp = NULL;
  char *tmp;

  if(ptemp == NULL)
    ptemp = IconPath;

  if((IconPath != ptemp)&&(IconPath != NULL))
    free(IconPath);
  tmp = stripcpy(action);
  IconPath = envDupExpand(tmp, 0);
  free(tmp);
}

#ifdef FVWM_MODULEDIR
char *ModulePath = FVWM_MODULEDIR;
#else
char *ModulePath = FVWMDIR;
#endif
void setModulePath(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                   unsigned long context, char *action,int* Module)
{
  static char *ptemp = NULL;
  char *tmp;

  if(ptemp == NULL)
    ptemp = ModulePath;

  if((ModulePath != ptemp)&&(ModulePath != NULL))
    free(ModulePath);
  tmp = stripcpy(action);
  ModulePath = envDupExpand(tmp, 0);
  free(tmp);
}

void SetMenuColors(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		   unsigned long context, char *action,int* Module)
  {
  XGCValues gcv;
  unsigned long gcm;
  char *menufore = NULL, *menuback = NULL, *selfore=NULL, *selback=NULL;
  extern char *white, *black;
  FvwmWindow *hilight;

  action = GetNextToken(action, &menufore);
  action = GetNextToken(action, &menuback);
  action = GetNextToken(action, &selfore);
  GetNextToken(action, &selback);
  if(Scr.d_depth > 2)
    {
    if(menufore != NULL) Scr.MenuColors.fore = GetColor(menufore);
    if(menuback != NULL) Scr.MenuColors.back = GetColor(menuback);
    if(selfore != NULL) Scr.SelColors.fore = GetColor(selfore);
    if(selback != NULL) Scr.SelColors.back = GetColor(selback);
    Scr.MenuColors.hilite = GetHilite(Scr.MenuColors.back);
    Scr.MenuColors.shadow = GetShadow(Scr.MenuColors.back);
    }
  else
    {
    Scr.MenuColors.back = GetColor(white);
    Scr.MenuColors.fore = GetColor(black); 
    Scr.SelColors.back = GetColor(white);
    Scr.SelColors.fore = GetColor(black); 
    Scr.MenuColors.hilite = GetColor(white);
    Scr.MenuColors.shadow = GetColor(black);
    }

  gcm = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
    GCBackground|GCFillStyle|GCFont;
  gcv.function   = GXcopy;
  gcv.plane_mask = AllPlanes;
  gcv.graphics_exposures = False;
  gcv.line_width = 0;
  gcv.fill_style = FillSolid;
  gcv.font = Scr.StdFont.font->fid;

  gcv.foreground = Scr.SelColors.fore;
  gcv.background = Scr.SelColors.back;
  if (Scr.SelGC != NULL) XFreeGC(dpy, Scr.SelGC);
  Scr.SelGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcv.foreground = Scr.MenuColors.fore;
  gcv.background = Scr.MenuColors.back;
  if(Scr.MenuGC != NULL) XFreeGC(dpy, Scr.MenuGC);
  Scr.MenuGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  gcv.foreground = Scr.MenuColors.hilite;
  gcv.background = Scr.MenuColors.shadow;
  if(Scr.MenuReliefGC != NULL) XFreeGC(dpy, Scr.MenuReliefGC);
  Scr.MenuReliefGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  gcv.foreground = Scr.MenuColors.shadow;
  gcv.background = Scr.MenuColors.hilite;
  if(Scr.MenuShadowGC != NULL) XFreeGC(dpy, Scr.MenuShadowGC);
  Scr.MenuShadowGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  MakeMenus();

  if (menufore) free(menufore);
  if (menuback) free(menuback);
  if (selfore) free(selfore);
  if (selback) free(selback);
  }

void SetHiColor(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int* Module)
  {
  XGCValues gcv;
  unsigned long gcm;
  char *hifore=NULL, *hiback=NULL;
  extern char *white, *black;
  FvwmWindow *hilight;
  
  action = GetNextToken(action, &hifore);
  GetNextToken(action, &hiback);
  if(Scr.d_depth > 2)
    {
    if(hifore != NULL) Scr.ActiveTitleColors.fore = GetColor(hifore);
    if(hiback != NULL) Scr.ActiveTitleColors.back = GetColor(hiback);
    }
  else
    {
    Scr.ActiveTitleColors.back = GetColor(white);
    Scr.ActiveTitleColors.fore = GetColor(black); 
    }

  gcm = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
    GCBackground|GCFillStyle|GCFont;
  gcv.function   = GXcopy;
  gcv.plane_mask = AllPlanes;
  gcv.graphics_exposures = False;
  gcv.line_width = 0;
  gcv.foreground = Scr.ActiveTitleColors.fore;
  gcv.background = Scr.ActiveTitleColors.back;
  gcv.fill_style = FillSolid;
  gcv.font = Scr.StdFont.font->fid;
  if (Scr.HiGC != NULL) XFreeGC(dpy, Scr.HiGC);
  Scr.HiGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  if((Scr.flags & WindowsCaptured)&&(Scr.Hilite != NULL))
    {
    hilight = Scr.Hilite;
    SetBorder(Scr.Hilite,False,True,True,None);
    SetBorder(hilight,True,True,True,None);
    }

  if (hifore) free(hifore);
  if (hiback) free(hiback);
  }

void SetStdColor(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int* Module)
  {
  XGCValues gcv;
  unsigned long gcm;
  char *winfore=NULL, *winback=NULL, *stdfore=NULL, *stdback=NULL;
  extern char *white,*black;
  FvwmWindow *hilight;

  action = GetNextToken(action,&winfore);
  action = GetNextToken(action,&winback);
  action = GetNextToken(action,&stdfore);
  GetNextToken(action,&stdback);
  if(Scr.d_depth > 2)
    {
    if (winfore != NULL) Scr.WinColors.fore = GetColor(winfore);
    if (winback != NULL) Scr.WinColors.back = GetColor(winback);
    if (stdfore != NULL) Scr.TitleColors.fore = GetColor(stdfore);
    if (stdback != NULL) Scr.TitleColors.back = GetColor(stdback);
    Scr.WinColors.hilite = GetHilite(Scr.WinColors.back);
    Scr.WinColors.shadow = GetShadow(Scr.WinColors.back);
    }
  else
    {
    Scr.TitleColors.fore = GetColor(black);
    Scr.TitleColors.back = GetColor(white);
    Scr.WinColors.fore = GetColor(black);
    Scr.WinColors.back = GetColor(white);
    Scr.WinColors.shadow = GetColor(black);
    Scr.WinColors.hilite = GetColor(white);
    }
  gcm = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
    GCBackground|GCFont|GCFillStyle;
  gcv.fill_style = FillSolid;
  gcv.plane_mask = AllPlanes;
  gcv.function = GXcopy;
  gcv.graphics_exposures = False;
  gcv.line_width = 0;
  gcv.font = Scr.StdFont.font->fid;

  gcv.foreground = Scr.WinColors.hilite;
  gcv.background = Scr.WinColors.shadow;
  if(Scr.WinReliefGC != NULL) XFreeGC(dpy, Scr.WinReliefGC);
  Scr.WinReliefGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  gcv.foreground = Scr.WinColors.shadow;
  gcv.background = Scr.WinColors.hilite;
  if(Scr.WinShadowGC != NULL) XFreeGC(dpy, Scr.WinShadowGC);
  Scr.WinShadowGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  gcv.foreground = Scr.WinColors.fore;
  gcv.background = Scr.WinColors.back;
  if(Scr.WinGC != NULL) XFreeGC(dpy, Scr.WinGC);
  Scr.WinGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  if((Scr.flags & WindowsCaptured)&&(Scr.Hilite != NULL))
    {
    hilight = Scr.Hilite;
    SetBorder(Scr.Hilite, False, True, True, None);
    SetBorder(hilight, True, True, True, None);
    }

  if (winfore) free(winfore);
  if (winback) free(winback);
  if (stdfore) free(stdfore);
  if (stdback) free(stdback);
  }

void SetStickyColor(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		unsigned long context, char *action,int* Module)
  {
  char *fore=NULL, *back=NULL;
  extern char *white, *black;
  FvwmWindow *hilight;

  action = GetNextToken(action, &fore);
  GetNextToken(action, &back);
  if(Scr.d_depth > 2)
    {
    if (fore != NULL) Scr.StickyColors.fore = GetColor(fore);
    if (back != NULL) Scr.StickyColors.back = GetColor(back);
    }
  else
    {
    Scr.StickyColors.back = Scr.TitleColors.back;
    Scr.StickyColors.fore = Scr.TitleColors.fore; 
    }

  if((Scr.flags & WindowsCaptured)&&(Scr.Hilite != NULL))
    {
    hilight = Scr.Hilite;
    SetBorder(Scr.Hilite,False,True,True,None);
    SetBorder(hilight,True,True,True,None);
    }

  if (fore) free(fore);
  if (back) free(back);
  }

void SetMenuFont(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int* Module)
  {
  XGCValues gcv;
  unsigned long gcm;
  char *font= NULL;
  extern char *white,*black;
  int wid,hei;
#ifdef I18N
  XFontSetExtents *fset_extents;
  XFontStruct **fs_list;
  char **ml;
#endif

  action = GetNextToken(action, &font);

#ifdef I18N
  if ((font == NULL)||
      (Scr.StdFont.fontset = GetFontSetOrFixed(dpy, font)) == NULL)
#else
  if ((font == NULL)||
      (Scr.StdFont.font = GetFontOrFixed(dpy, font)) == NULL)
#endif
    {
    fprintf(stderr,
            "[SetMenuFont]: ERROR -- Couldn't load font '%s' or 'fixed'\n",
            (font==NULL)?("NULL"):(font));
    exit(1);
    }
#ifdef I18N
  XFontsOfFontSet(Scr.StdFont.fontset, &fs_list, &ml);
  Scr.StdFont.font = fs_list[0];
  fset_extents = XExtentsOfFontSet(Scr.StdFont.fontset);
  Scr.StdFont.height = fset_extents->max_logical_extent.height;
#else
  Scr.StdFont.height = Scr.StdFont.font->ascent + Scr.StdFont.font->descent;
#endif
  Scr.StdFont.y = Scr.StdFont.font->ascent;
  Scr.EntryHeight = Scr.StdFont.height + HEIGHT_EXTRA;

  gcm = GCFont;
  gcv.font = Scr.StdFont.font->fid;
  if (Scr.MenuGC != NULL) XChangeGC(dpy, Scr.MenuGC, gcm, &gcv);
  if (Scr.SelGC != NULL) XChangeGC(dpy, Scr.SelGC, gcm, &gcv);
  if (Scr.MenuReliefGC != NULL) XChangeGC(dpy, Scr.MenuReliefGC, gcm, &gcv);
  if (Scr.MenuShadowGC != NULL) XChangeGC(dpy, Scr.MenuShadowGC, gcm, &gcv);

  if(Scr.SizeWindow != None)
    {
    Scr.SizeStringWidth = XTextWidth (Scr.StdFont.font,
                                      " +8888 x +8888 ", 15);
    wid = Scr.SizeStringWidth+SIZE_HINDENT*2;
    hei = Scr.StdFont.height+SIZE_VINDENT*2;

    /* re-position coordinate window (centered) */
    XMoveResizeWindow(dpy,Scr.SizeWindow,
                      Scr.MyDisplayWidth/2 -wid/2,
                      Scr.MyDisplayHeight/2 - hei/2,
                      wid,hei);
    }

  if(Scr.SizeWindow != None)
    {
    XSetWindowBackground(dpy,Scr.SizeWindow,Scr.WinColors.back);
    }

  MakeMenus();

  free(font);
  }

void LoadIconFont(XEvent *eventp,Window w,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int* Module)
{
  char *font;
  FvwmWindow *tmp;
#ifdef I18N
  XFontSetExtents *fset_extents;
  XFontStruct **fs_list;
  char **ml;
#endif

  action = GetNextToken(action,&font);

#ifdef I18N
  if ((Scr.IconFont.fontset = GetFontSetOrFixed(dpy, font)) == NULL)
#else
  if ((Scr.IconFont.font = GetFontOrFixed(dpy, font))==NULL)
#endif
  {
    fvwm_msg(ERR,"LoadIconFont","Couldn't load font '%s' or 'fixed'\n",
            font);
    free(font);
    return;
  }
#ifdef I18N
  XFontsOfFontSet(Scr.IconFont.fontset, &fs_list, &ml);
  Scr.IconFont.font = fs_list[0];
  fset_extents = XExtentsOfFontSet(Scr.IconFont.fontset);
  Scr.IconFont.height =
    fset_extents->max_logical_extent.height;
#else
  Scr.IconFont.height=
    Scr.IconFont.font->ascent+Scr.IconFont.font->descent;
#endif
  Scr.IconFont.y = Scr.IconFont.font->ascent;

  free(font);
  tmp = Scr.FvwmRoot.next;
  while(tmp != NULL)
  {
    RedoIconName(tmp);

    if(tmp->flags& ICONIFIED)
    {
      DrawIconWindow(tmp);
    }
    tmp = tmp->next;
  }
}

void LoadWindowFont(XEvent *eventp,Window win,FvwmWindow *tmp_win,
                    unsigned long context, char *action,int* Module)
{
  char *font;
  FvwmWindow *tmp,*hi;
  int x,y,w,h,extra_height;
  XFontStruct *newfont;
#ifdef I18N
  XFontSetExtents *fset_extents;
  XFontStruct **fs_list;
  char **ml;
#endif

  action = GetNextToken(action,&font);

#ifdef I18N
  if ((Scr.WindowFont.fontset = GetFontSetOrFixed(dpy,font)) != NULL)
  {
    XFontsOfFontSet(Scr.WindowFont.fontset, &fs_list, &ml);
    Scr.WindowFont.font = fs_list[0];
    fset_extents = XExtentsOfFontSet(Scr.WindowFont.fontset);
    Scr.WindowFont.height = fset_extents->max_logical_extent.height;
#else
  if ((newfont = GetFontOrFixed(dpy, font))!=NULL)
  {
    Scr.WindowFont.font = newfont;
    Scr.WindowFont.height=
      Scr.WindowFont.font->ascent+Scr.WindowFont.font->descent;
#endif
    Scr.WindowFont.y = Scr.WindowFont.font->ascent;
    extra_height = Scr.TitleHeight;
    Scr.TitleHeight = Scr.WindowFont.font->ascent+Scr.WindowFont.font->descent+3;
    extra_height -= Scr.TitleHeight;
    tmp = Scr.FvwmRoot.next;
    hi = Scr.Hilite;
    while(tmp != NULL)
    {
      x = tmp->frame_x;
      y = tmp->frame_y;
      w = tmp->frame_width;
      h = tmp->frame_height-extra_height;
      tmp->frame_x = 0;
      tmp->frame_y = 0;
      tmp->frame_height = 0;
      tmp->frame_width = 0;
      SetupFrame(tmp,x,y,w,h,True);
      SetTitleBar(tmp,True,True);
      SetTitleBar(tmp,False,True);
      tmp = tmp->next;
    }
    SetTitleBar(hi,True,True);
    
  }
  else
  {
    fvwm_msg(ERR,"LoadWindowFont","Couldn't load font '%s' or 'fixed'\n",
             font);
  }

  free(font);
}

/*****************************************************************************
 * 
 * Changes a button decoration style
 *
 * Rewritten by dtrg to use XPM's rather than a coordinate list 5/5/96
 *
 ****************************************************************************/
void ButtonStyle(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                 unsigned long context, char *action, int* Module)
{
#ifdef XPM
  int n, button;
  char filename[256];
  XWindowAttributes root_attr;
  XpmAttributes xpm_attributes;
  extern char *PixmapPath;
  char *path = NULL;
  Pixmap junkmaskPixmap;

  n = sscanf(action, "%d %s", &button, filename);
  if (n != 2)
    {
    fvwm_msg(ERR,"ButtonStyle","Syntax error");
    return;
    }
  button--;

  if ((button<0) || (button>9))
    {
    fvwm_msg(ERR,"ButtonStyle","Button number must be 1..10");
    return;
    }

  path = findIconFile(filename, PixmapPath, R_OK);
  if(path == NULL)
    {
    fvwm_msg(ERR,"ButtonStyle","XPM file not found");
    return;
    }  

  XGetWindowAttributes(dpy, Scr.Root, &root_attr);
  xpm_attributes.colormap  = root_attr.colormap;
  xpm_attributes.closeness = 40000; /* Allow for "similar" colors */
  xpm_attributes.valuemask = XpmSize | XpmReturnPixels | 
                             XpmColormap | XpmCloseness;

/*
  fprintf(stderr, "Loading XPM file %s\n", path);

  if(Scr.button_pixmap[button] != None)
    XFreePixmap(dpy, Scr.button_pixmap[button]);
*/

  if(XpmReadFileToPixmap(dpy, Scr.Root, path,
			 &Scr.button_pixmap[button], 
			 &junkmaskPixmap, 
			 &xpm_attributes) != XpmSuccess) 
    {
    fvwm_msg(ERR,"ButtonStyle","Couldn't load XPM file");
    free(path);
    return;
    }

  if ((xpm_attributes.width  != 13) ||
      (xpm_attributes.height != 11))
    {
    fvwm_msg(ERR,"ButtonStyle","Warning: XPM file isn't the right size");
    free(path);
    return;
    }
  free(path);
#endif
}

void SafeDefineCursor(Window w, Cursor cursor)
{
  if (w) XDefineCursor(dpy,w,cursor);
}

void CursorStyle(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                 unsigned long context, char *action,int* Module)
{
  char *cname=NULL, *newcursor=NULL;
  int index,nc,i;
  FvwmWindow *fw;
  MenuRoot *mr;

  action = GetNextToken(action,&cname);
  action = GetNextToken(action,&newcursor);
  if (!cname || !newcursor)
  {
    fvwm_msg(ERR,"CursorStyle","Bad cursor style");
    return;
  }
  if (StrEquals("POSITION",cname)) index = POSITION;
  else if (StrEquals("DEFAULT",cname)) index = DEFAULT;
  else if (StrEquals("SYS",cname)) index = SYS;
  else if (StrEquals("TITLE",cname)) index = TITLE_CURSOR;
  else if (StrEquals("MOVE",cname)) index = MOVE;
  else if (StrEquals("MENU",cname)) index = MENU;
  else if (StrEquals("WAIT",cname)) index = WAIT;
  else if (StrEquals("SELECT",cname)) index = SELECT;
  else if (StrEquals("DESTROY",cname)) index = DESTROY;
  else if (StrEquals("LEFT",cname)) index = LEFT;
  else if (StrEquals("RIGHT",cname)) index = RIGHT;
  else if (StrEquals("TOP",cname)) index = TOP;
  else if (StrEquals("BOTTOM",cname)) index = BOTTOM;
  else if (StrEquals("TOP_LEFT",cname)) index = TOP_LEFT;
  else if (StrEquals("TOP_RIGHT",cname)) index = TOP_RIGHT;
  else if (StrEquals("BOTTOM_LEFT",cname)) index = BOTTOM_LEFT;
  else if (StrEquals("BOTTOM_RIGHT",cname)) index = BOTTOM_RIGHT;
  else
  {
    fvwm_msg(ERR,"CursorStyle","Unknown cursor name %s",cname);
    return;
  }
  nc = atoi(newcursor);
  if ((nc < 0) || (nc >= XC_num_glyphs) || ((nc % 2) != 0))
  {
    fvwm_msg(ERR,"CursorStyle","Bad cursor number %s",newcursor);
    return;
  }

  /* replace the cursor defn */
  if (Scr.FvwmCursors[index]) XFreeCursor(dpy,Scr.FvwmCursors[index]);
  Scr.FvwmCursors[index] = XCreateFontCursor(dpy,nc);

  /* redefine all the windows using cursors */
  fw = Scr.FvwmRoot.next;
  while(fw != NULL)
  {
    for (i=0;i<4;i++)
    {
      SafeDefineCursor(fw->corners[i],Scr.FvwmCursors[TOP_LEFT+i]);
      SafeDefineCursor(fw->sides[i],Scr.FvwmCursors[TOP+i]);
    }
    for (i=0;i<Scr.nr_left_buttons;i++)
    {
      SafeDefineCursor(fw->left_w[i],Scr.FvwmCursors[SYS]);
    }
    for (i=0;i<Scr.nr_right_buttons;i++)
    {
      SafeDefineCursor(fw->right_w[i],Scr.FvwmCursors[SYS]);
    }
    SafeDefineCursor(fw->title_w, Scr.FvwmCursors[TITLE_CURSOR]);      
    fw = fw->next;
  }

  /* Do the menus for good measure */
  mr = Scr.AllMenus;
  while(mr != NULL)
  {
    SafeDefineCursor(mr->w,Scr.FvwmCursors[MENU]);
    mr = mr->next;
  }
}

/**************************************************************************
 *
 * Direction = 1 ==> "Next" operation
 * Direction = -1 ==> "Previous" operation 
 *
 **************************************************************************/
FvwmWindow *Circulate(char *action, int Direction, char **restofline)
{
  int l,pass = 0;
  FvwmWindow *fw, *found = NULL;
  char *t,*tstart,*name = NULL, *expression, *condition, *prev_condition=NULL;
  char *orig_expr;
  Bool needsIconic = 0;
  Bool needsNormal = 0;
  Bool needsCurrentDesk = 0;
  Bool needsCurrentScreen = 0;
  Bool needsVisible = 0;
  Bool needsInvisible = 0;
  char *AnyWindow = "*";
  Bool useCirculateHit = 0;
  Bool useCirculateHitIcon = 0;

  l=0;

  if(action == NULL)
    return NULL;

  t = action;
  while(isspace((unsigned char)*t)&&(*t!= 0))
    t++;
  if(*t == '[')
  {
    t++;
    tstart = t;

    while((*t !=0)&&(*t != ']'))
    {
      t++;
      l++;
    }
    if(*t == 0)
    {
      fvwm_msg(ERR,"Circulate","Conditionals require closing brace");
      return NULL;
    }
      
    *restofline = t+1;
      
    orig_expr = expression = safemalloc(l+1);
    strncpy(expression,tstart,l);
    expression[l] = 0;
    expression = GetNextToken(expression,&condition);
    while((condition != NULL)&&(strlen(condition) > 0))
    {
      if(strcasecmp(condition,"iconic")==0)
        needsIconic = 1;
      else if(strcasecmp(condition,"!iconic")==0)
        needsNormal = 1;
      else if(strcasecmp(condition,"CurrentDesk")==0)
        needsCurrentDesk = 1;
      else if(strcasecmp(condition,"Visible")==0)
        needsVisible = 1;
      else if(strcasecmp(condition,"!Visible")==0)
        needsInvisible = 1;
      else if(strcasecmp(condition,"CurrentScreen")==0)
        needsCurrentScreen = 1;
      else if(strcasecmp(condition,"CirculateHit")==0)
        useCirculateHit = 1;
      else if(strcasecmp(condition,"CirculateHitIcon")==0)
        useCirculateHitIcon = 1;
      else
      {
        name = condition;
        condition = NULL;
      }
      if(prev_condition)free(prev_condition);
      prev_condition = condition;
      expression = GetNextToken(expression,&condition);
    }
    if(prev_condition != NULL)
      free(prev_condition);
    if(orig_expr != NULL)
      free(orig_expr);
  }
  else
    *restofline = t;

  if(name == NULL)
    name = AnyWindow;

  if(Scr.Focus != NULL)
  {
    if(Direction == 1)
      fw = Scr.Focus->prev;
    else
      fw = Scr.Focus->next;
  }
  else
    fw = Scr.FvwmRoot.prev;  

  while((pass < 3)&&(found == NULL))
  {
    while((fw != NULL)&&(found==NULL)&&(fw != &Scr.FvwmRoot))
    {
      /* Make CirculateUp and CirculateDown take args. by Y.NOMURA */
      if (((matchWildcards(name, fw->name)) ||
           (matchWildcards(name, fw->icon_name))||
           (fw->class.res_class &&
            matchWildcards(name, fw->class.res_class))||
           (fw->class.res_name &&
            matchWildcards(name, fw->class.res_name)))&&
          ((useCirculateHit)||!(fw->flags & CirculateSkip))&&
          (((useCirculateHitIcon)&&(fw->flags & ICONIFIED))||
           !((fw->flags & CirculateSkipIcon)&&(fw->flags & ICONIFIED)))&&
          (((!needsIconic)||(fw->flags & ICONIFIED))&&
           ((!needsNormal)||(!(fw->flags & ICONIFIED)))&&
           ((!needsCurrentDesk)||(fw->Desk == Scr.CurrentDesk))&&
           ((!needsVisible)||(fw->flags & VISIBLE))&&
           ((!needsInvisible)||(!(fw->flags & VISIBLE)))&&
           ((!needsCurrentScreen)||((fw->frame_x < Scr.MyDisplayWidth)&&
                                    (fw->frame_y < Scr.MyDisplayHeight)&&
                                    (fw->frame_x+fw->frame_width > 0)&&
                                    (fw->frame_y+fw->frame_height > 0)))))
        found = fw;
      else
      {
        if(Direction == 1)
          fw = fw->prev;
        else
          fw = fw->next;
      }
    }
    if((fw == NULL)||(fw == &Scr.FvwmRoot))
    {
      if(Direction == 1)
      {
        /* Go to end of list */
        fw = &Scr.FvwmRoot;
        while((fw) && (fw->next != NULL))
        {
          fw = fw->next;
        }
      }
      else
      {
        /* GO to top of list */
        fw = Scr.FvwmRoot.next;
      }
    }
    pass++;
  }
  if((name != NULL)&&(name != AnyWindow))
    free(name);
  return found;

}

void PrevFunc(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
              unsigned long context, char *action,int* Module)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, -1, &restofline);
  if(found != NULL)
  {
    ExecuteFunction(restofline,found,eventp,C_WINDOW,*Module);
  }

}

void NextFunc(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
              unsigned long context, char *action,int* Module)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 1, &restofline);
  if(found != NULL)
  {
    ExecuteFunction(restofline,found,eventp,C_WINDOW,*Module);
  }

}

void NoneFunc(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
              unsigned long context, char *action,int* Module)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 1, &restofline);
  if(found == NULL)
  {
    ExecuteFunction(restofline,NULL,eventp,C_ROOT,*Module);
  }
}

void CurrentFunc(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
              unsigned long context, char *action,int* Module)
{
  FvwmWindow *found;
  char *restofline;

  found = Circulate(action, 0, &restofline);
  if(found != NULL)
  {
    ExecuteFunction(restofline,found,eventp,C_WINDOW,*Module);
  }
}

void WindowIdFunc(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                  unsigned long context, char *action,int* Module)
{
  FvwmWindow *found=NULL,*t;
  char *actioncopy,*restofline,*num;
  unsigned long win;

  actioncopy = strdup(action);
  restofline = GetNextToken(actioncopy, &num);
#ifdef HAVE_STRTOUL
  win = strtoul(num,NULL,0);
#else
  win = (unsigned long)strtol(num,NULL,0); /* SunOS doesn't have strtoul */
#endif
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
  {
    if (t->w == win)
    {
      found = t;
      break;
    }
  }
  if(found)
  {
    ExecuteFunction(restofline,found,eventp,C_WINDOW,*Module);
  }
  if (actioncopy)
    free(actioncopy);
}

 
void module_zapper(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
                   unsigned long context, char *action,int* Module)
{
  char *condition;

  GetNextToken(action,&condition);
  KillModuleByName(condition);
  free(condition);
}

/***********************************************************************
 *
 *  Procedure:
 *	Reborder - Removes fvwm border windows
 *
 ************************************************************************/
void Recapture(XEvent *eventp,Window junk,FvwmWindow *tmp_win,
               unsigned long context, char *action,int* Module)
{
  FvwmWindow *tmp,*next;		/* temp fvwm window structure */
  Window w;
  extern Bool PPosOverride;
  unsigned long data[1];
  extern long isIconicState;
  unsigned char *prop;
  Atom atype;
  int aformat,i;
  unsigned int nchildren;
  unsigned long nitems, bytes_remain;
  Window root, parent, *children;

  if(!XQueryTree(dpy, Scr.Root, &root, &parent, &children, &nchildren))
    return;

  BlackoutScreen();

  PPosOverride = True;
  /* put a border back around all windows */
  XGrabServer (dpy);
  tmp = Scr.FvwmRoot.next;
  for(i=0;i<nchildren;i++)
  {
    if(XFindContext(dpy, children[i], FvwmContext, 
                    (caddr_t *)&tmp)!=XCNOENT)
    {
      isIconicState = DontCareState;
      if(XGetWindowProperty(dpy,tmp->w,_XA_WM_STATE,0L,3L,False,
                            _XA_WM_STATE,
                            &atype,&aformat,&nitems,&bytes_remain,&prop)==
         Success)
      {
        if(prop != NULL)
        {
          isIconicState = *(long *)prop;
          XFree(prop);
        }
      }
      next = tmp->next;
      data[0] = (unsigned long) tmp->Desk;
      XChangeProperty (dpy, tmp->w, _XA_WM_DESKTOP, _XA_WM_DESKTOP, 32,
                       PropModeReplace, (unsigned char *) data, 1);
	  
      XSelectInput(dpy, tmp->w, 0);
      w = tmp->w;
      XUnmapWindow(dpy,tmp->frame);
      XUnmapWindow(dpy,w);
      RestoreWithdrawnLocation (tmp,True); 
      Destroy(tmp);
      Event.xmaprequest.window = w;
      HandleMapRequestKeepRaised(BlackoutWin);
      tmp = next;
    }
  }
  UnBlackoutScreen();
  isIconicState = DontCareState; /* needs to be down here instead of above? */
  if(nchildren > 0)
    XFree((char *)children);
  PPosOverride = False;
  KeepOnTop();
  XUngrabServer (dpy);
  XSync(dpy,0);

}
