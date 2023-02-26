/****************************************************************************
 * This module is all original code 
 * by Rob Nation 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/****************************************************************************
 *
 * Assorted odds and ends
 *
 **************************************************************************/


#include <FVWMconfig.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>

#include "fvwm.h"
#include <X11/Xatom.h>
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

FvwmWindow *FocusOnNextTimeStamp = NULL;

char NoName[] = "Untitled"; /* name if no name in XA_WM_NAME */
char NoClass[] = "NoClass"; /* Class if no res_class in class hints */
char NoResource[] = "NoResource"; /* Class if no res_name in class hints */

FILE *console = NULL;

/**************************************************************************
 * 
 * Releases dynamically allocated space used to store window/icon names
 *
 **************************************************************************/
void FreeWindowNames (FvwmWindow *tmp)
{
  free_icon_name(tmp);
  free_window_name(tmp);
}

void free_icon_name (FvwmWindow *tmp)
{
  if (!tmp) return;

  if (tmp->icon_name != NULL &&
      tmp->icon_name != NoName &&
      tmp->icon_name != tmp->name &&
      tmp->icon_name != tmp->class.res_name &&
      tmp->icon_name != tmp->class.res_class)
    XFree (tmp->icon_name);

  tmp->icon_name = NULL;
}

void free_window_name (FvwmWindow *tmp)
{
  if (!tmp) return;

  if (tmp->name != NULL &&
      tmp->name != NoName &&
      tmp->name != tmp->icon_name &&
      tmp->name != tmp->class.res_name &&
      tmp->name != tmp->class.res_class)
    XFree (tmp->name);

  tmp->name = NULL;
}

/***************************************************************************
 *
 * Handles destruction of a window 
 *
 ****************************************************************************/
void Destroy(FvwmWindow *Tmp_win)
{ 
  int i;
  extern FvwmWindow *ButtonWindow;
  extern FvwmWindow *colormap_win;
  extern Bool PPosOverride;

  /*
   * Warning, this is also called by HandleUnmapNotify; if it ever needs to
   * look at the event, HandleUnmapNotify will have to mash the UnmapNotify
   * into a DestroyNotify.
   */
  if(!Tmp_win)
    return;

/* 
** I think that commenting this Unmap out will actually create the
** illusion of closing faster, hopefully - CKH
*/
  XUnmapWindow(dpy, Tmp_win->frame);

  if(!PPosOverride)
    XSync(dpy,0);
  
  if(Tmp_win == Scr.Hilite)
    Scr.Hilite = NULL;
  
  Broadcast(M_DESTROY_WINDOW,3,Tmp_win->w,Tmp_win->frame,
	    (unsigned long)Tmp_win,0,0,0,0);

  if(Scr.PreviousFocus == Tmp_win)
    Scr.PreviousFocus = NULL;

  if(ButtonWindow == Tmp_win)
    ButtonWindow = NULL;

  if((Tmp_win == Scr.Focus)&&(Tmp_win->flags & ClickToFocus))
    {
      if(Tmp_win->next)
	{
	  HandleHardFocus(Tmp_win->next);
	}
      else
	SetFocus(Scr.NoFocusWin, NULL,1);
    }
  else if(Scr.Focus == Tmp_win)
    SetFocus(Scr.NoFocusWin, NULL,1);

  if(Tmp_win == FocusOnNextTimeStamp)
    FocusOnNextTimeStamp = NULL;

  if(Tmp_win == Scr.Ungrabbed)
    Scr.Ungrabbed = NULL;

  if(Tmp_win == Scr.pushed_window)
    Scr.pushed_window = NULL;

  if(Tmp_win == colormap_win)
    colormap_win = NULL;

  XDestroyWindow(dpy, Tmp_win->frame);
  XDeleteContext(dpy, Tmp_win->frame, FvwmContext);

  XDestroyWindow(dpy, Tmp_win->Parent);

  XDeleteContext(dpy, Tmp_win->Parent, FvwmContext);

  XDeleteContext(dpy, Tmp_win->w, FvwmContext);
  
  if ((Tmp_win->icon_w)&&(Tmp_win->flags & PIXMAP_OURS))
    XFreePixmap(dpy, Tmp_win->iconPixmap);
  
  if (Tmp_win->icon_w)
    {
      XDestroyWindow(dpy, Tmp_win->icon_w);
      XDeleteContext(dpy, Tmp_win->icon_w, FvwmContext);
    }
  if((Tmp_win->flags &ICON_OURS)&&(Tmp_win->icon_pixmap_w != None))
    XDestroyWindow(dpy, Tmp_win->icon_pixmap_w);
  if(Tmp_win->icon_pixmap_w != None)
    XDeleteContext(dpy, Tmp_win->icon_pixmap_w, FvwmContext);

  if (Tmp_win->flags & TITLE)
    {
      XDeleteContext(dpy, Tmp_win->title_w, FvwmContext);
      for(i=0;i<Scr.nr_left_buttons;i++)
	XDeleteContext(dpy, Tmp_win->left_w[i], FvwmContext);
      for(i=0;i<Scr.nr_right_buttons;i++)
	if(Tmp_win->right_w[i] != None)
	  XDeleteContext(dpy, Tmp_win->right_w[i], FvwmContext);
    }
  if (Tmp_win->flags & BORDER)
    {
      for(i=0;i<4;i++)
	XDeleteContext(dpy, Tmp_win->sides[i], FvwmContext);
      for(i=0;i<4;i++)
	XDeleteContext(dpy, Tmp_win->corners[i], FvwmContext);
    }
  
  if (Tmp_win->title_icon != NULL)
    DestroyPicture(dpy, Tmp_win->title_icon);

  Tmp_win->prev->next = Tmp_win->next;
  if (Tmp_win->next != NULL)
    Tmp_win->next->prev = Tmp_win->prev;
  FreeWindowNames (Tmp_win);
  if (Tmp_win->wmhints)					
    XFree ((char *)Tmp_win->wmhints);
  /* removing NoClass change for now... */
#if 0
  if (Tmp_win->class.res_name)  
    XFree ((char *)Tmp_win->class.res_name);
  if (Tmp_win->class.res_class) 
    XFree ((char *)Tmp_win->class.res_class);
#else
  if (Tmp_win->class.res_name && Tmp_win->class.res_name != NoResource)
    XFree ((char *)Tmp_win->class.res_name);
  if (Tmp_win->class.res_class && Tmp_win->class.res_class != NoClass)
    XFree ((char *)Tmp_win->class.res_class);
#endif /* 0 */
  if(Tmp_win->mwm_hints)
    XFree((char *)Tmp_win->mwm_hints);

  if(Tmp_win->cmap_windows != (Window *)NULL)
    XFree((void *)Tmp_win->cmap_windows);

  free((char *)Tmp_win);

  if(!PPosOverride)
    XSync(dpy,0);
  return;
}



/**************************************************************************
 *
 * Removes expose events for a specific window from the queue 
 *
 *************************************************************************/
int flush_expose (Window w)
{
  XEvent dummy;
  int i=0;
  
  while (XCheckTypedWindowEvent (dpy, w, Expose, &dummy))i++;
  return i;
}



/***********************************************************************
 *
 *  Procedure:
 *	RestoreWithdrawnLocation
 * 
 *  Puts windows back where they were before fvwm took over 
 *
 ************************************************************************/
void RestoreWithdrawnLocation (FvwmWindow *tmp,Bool restart)
{
  int a,b,w2,h2;
  unsigned int bw,mask;
  XWindowChanges xwc;
  
  if(!tmp)
    return;
  
  if (XGetGeometry (dpy, tmp->w, &JunkRoot, &xwc.x, &xwc.y, 
		    &JunkWidth, &JunkHeight, &bw, &JunkDepth)) 
    {
      XTranslateCoordinates(dpy,tmp->frame,Scr.Root,xwc.x,xwc.y,
			    &a,&b,&JunkChild);
      xwc.x = a + tmp->xdiff;
      xwc.y = b + tmp->ydiff;
      xwc.border_width = tmp->old_bw;
      mask = (CWX | CWY|CWBorderWidth);
      
      /* We can not assume that the window is currently on the screen.
       * Although this is normally the case, it is not always true.  The
       * most common example is when the user does something in an
       * application which will, after some amount of computational delay,
       * cause the window to be unmapped, but then switches screens before
       * this happens.  The XTranslateCoordinates call above will set the
       * window coordinates to either be larger than the screen, or negative.
       * This will result in the window being placed in odd, or even
       * unviewable locations when the window is remapped.  The followin code
       * forces the "relative" location to be within the bounds of the display.
       *
       * gpw -- 11/11/93
       *
       * Unfortunately, this does horrendous things during re-starts, 
       * hence the "if(restart) clause (RN) 
       *
       * Also, fixed so that it only does this stuff if a window is more than
       * half off the screen. (RN)
       */
      
      if(!restart)
	{
	  /* Don't mess with it if its partially on the screen now */
	  if((tmp->frame_x < 0)||(tmp->frame_y<0)||
	     (tmp->frame_x >= Scr.MyDisplayWidth)||
	     (tmp->frame_y >= Scr.MyDisplayHeight))
	    {
	      w2 = (tmp->frame_width>>1);
	      h2 = (tmp->frame_height>>1);
	      if (( xwc.x < -w2) || (xwc.x > (Scr.MyDisplayWidth-w2 )))
		{
		  xwc.x = xwc.x % Scr.MyDisplayWidth;
		  if ( xwc.x < -w2 )
		    xwc.x += Scr.MyDisplayWidth;
		}
	      if ((xwc.y < -h2) || (xwc.y > (Scr.MyDisplayHeight-h2 )))
		{
		  xwc.y = xwc.y % Scr.MyDisplayHeight;
		  if ( xwc.y < -h2 )
		    xwc.y += Scr.MyDisplayHeight;
		}
	    }
	}
      XReparentWindow (dpy, tmp->w,Scr.Root,xwc.x,xwc.y);
      
      if((tmp->flags & ICONIFIED)&&(!(tmp->flags & SUPPRESSICON)))
	{
	  if (tmp->icon_w) 
	    XUnmapWindow(dpy, tmp->icon_w);
	  if (tmp->icon_pixmap_w) 
	    XUnmapWindow(dpy, tmp->icon_pixmap_w);	  
	}
      
      XConfigureWindow (dpy, tmp->w, mask, &xwc);
      if(!restart)
	XSync(dpy,0);
    }
}


/***************************************************************************
 *
 * ICCCM Client Messages - Section 4.2.8 of the ICCCM dictates that all
 * client messages will have the following form:
 *
 *     event type	ClientMessage
 *     message type	_XA_WM_PROTOCOLS
 *     window		tmp->w
 *     format		32
 *     data[0]		message atom
 *     data[1]		time stamp
 *
 ****************************************************************************/
void send_clientmessage (Window w, Atom a, Time timestamp)
{
  XClientMessageEvent ev;
  
  ev.type = ClientMessage;
  ev.window = w;
  ev.message_type = _XA_WM_PROTOCOLS;
  ev.format = 32;
  ev.data.l[0] = a;
  ev.data.l[1] = timestamp;
  XSendEvent (dpy, w, False, 0L, (XEvent *) &ev);
}





/****************************************************************************
 *
 * Records the time of the last processed event. Used in XSetInputFocus
 *
 ****************************************************************************/
Time lastTimestamp = CurrentTime;	/* until Xlib does this for us */

Bool StashEventTime (XEvent *ev)
{
  Time NewTimestamp = CurrentTime;
  
  switch (ev->type) 
    {
    case KeyPress:
    case KeyRelease:
      NewTimestamp = ev->xkey.time;
      break;
    case ButtonPress:
    case ButtonRelease:
      NewTimestamp = ev->xbutton.time;
      break;
    case MotionNotify:
      NewTimestamp = ev->xmotion.time;
      break;
    case EnterNotify:
    case LeaveNotify:
      NewTimestamp = ev->xcrossing.time;
      break;
    case PropertyNotify:
      NewTimestamp = ev->xproperty.time;
      break;
    case SelectionClear:
      NewTimestamp = ev->xselectionclear.time;
      break;
    case SelectionRequest:
      NewTimestamp = ev->xselectionrequest.time;
      break;
    case SelectionNotify:
      NewTimestamp = ev->xselection.time;
      break;
    default:
      return False;
    }
  /* Only update is the new timestamp is later than the old one, or
   * if the new one is from a time at least 30 seconds earlier than the
   * old one (in which case the system clock may have changed) */
  if((NewTimestamp > lastTimestamp)||((lastTimestamp - NewTimestamp) > 30000))
    lastTimestamp = NewTimestamp;
  if(FocusOnNextTimeStamp)
    {
      SetFocus(FocusOnNextTimeStamp->w,FocusOnNextTimeStamp,1);      
      FocusOnNextTimeStamp = NULL;
    }
  return True;
}





int GetTwoArguments(char *action, int *val1, int *val2, int *val1_unit, int *val2_unit)
{
  char c1, c2;
  int n;

  *val1 = 0;
  *val2 = 0;
  *val1_unit = Scr.MyDisplayWidth;
  *val2_unit = Scr.MyDisplayHeight;

  n = sscanf(action,"%d %d", val1, val2);
  if(n == 2)
    return 2;

  c1 = 's';
  c2 = 's';
  n = sscanf(action,"%d%c %d%c", val1, &c1, val2, &c2);

  if(n != 4)
    return 0;
  
  if((c1 == 'p')||(c1 == 'P'))
    *val1_unit = 100;

  if((c2 == 'p')||(c2 == 'P'))
    *val2_unit = 100;

  return 2;
}


int GetOneArgument(char *action, long *val1, int *val1_unit)
{
  char c1;
  int n;

  *val1 = 0;
  *val1_unit = Scr.MyDisplayWidth;

  n = sscanf(action,"%ld", val1);
  if(n == 1)
    return 1;

  c1 = '%';
  n = sscanf(action,"%ld%c", val1, &c1);

  if(n != 2)
    return 0;
  
  if((c1 == 'p')||(c1 == 'P'))
    *val1_unit = 100;

  return 1;
}


/*****************************************************************************
 *
 * Grab the pointer and keyboard
 *
 ****************************************************************************/
Bool GrabEm(int cursor)
{
  int i=0,val=0;
  unsigned int mask;

  XSync(dpy,0);
  /* move the keyboard focus prior to grabbing the pointer to
   * eliminate the enterNotify and exitNotify events that go
   * to the windows */
  if(Scr.PreviousFocus == NULL)
    Scr.PreviousFocus = Scr.Focus;
  SetFocus(Scr.NoFocusWin,NULL,0);
  mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|PointerMotionMask
    | EnterWindowMask | LeaveWindowMask;
  while((i<1000)&&(val=XGrabPointer(dpy, Scr.Root, True, mask,
				    GrabModeAsync, GrabModeAsync, Scr.Root,
				    Scr.FvwmCursors[cursor], CurrentTime)!=
		   GrabSuccess))
    {
      i++;
      /* If you go too fast, other windows may not get a change to release
       * any grab that they have. */
      usleep(1000);
    }

  /* If we fall out of the loop without grabbing the pointer, its
     time to give up */
  XSync(dpy,0);
  if(val!=GrabSuccess)
    {
      return False;
    }
  return True;
}


/*****************************************************************************
 *
 * UnGrab the pointer and keyboard
 *
 ****************************************************************************/
void UngrabEm()
{
  Window w;

  XSync(dpy,0);
  XUngrabPointer(dpy,CurrentTime);

  if(Scr.PreviousFocus != NULL)
    {
      w = Scr.PreviousFocus->w;

      /* if the window still exists, focus on it */
      if (w)
	{
	  SetFocus(w,Scr.PreviousFocus,0);
	}
      Scr.PreviousFocus = NULL;
    }
  XSync(dpy,0);
}



/****************************************************************************
 *
 * Keeps the "StaysOnTop" windows on the top of the pile.
 * This is achieved by clearing a flag for OnTop windows here, and waiting
 * for a visibility notify on the windows. Exeption: OnTop windows which are
 * obscured by other OnTop windows, which need to be raised here.
 *
 ****************************************************************************/
void KeepOnTop()
{
  FvwmWindow *t;

  /* flag that on-top windows should be re-raised */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if((t->flags & ONTOP)&&!(t->flags & VISIBLE))
	{
	  RaiseWindow(t);
	  t->flags &= ~RAISED;
	}
      else
	t->flags |= RAISED;
    }
}


/**************************************************************************
 * 
 * Unmaps a window on transition to a new desktop
 *
 *************************************************************************/
void UnmapIt(FvwmWindow *t)
{
  XWindowAttributes winattrs;
  unsigned long eventMask;
  /*
   * Prevent the receipt of an UnmapNotify, since that would
   * cause a transition to the Withdrawn state.
   */
  XGetWindowAttributes(dpy, t->w, &winattrs);
  eventMask = winattrs.your_event_mask;
  XSelectInput(dpy, t->w, eventMask & ~StructureNotifyMask);
  if(t->flags & ICONIFIED)
    {
      if(t->icon_pixmap_w != None)
	XUnmapWindow(dpy,t->icon_pixmap_w);
      if(t->icon_w != None)
	XUnmapWindow(dpy,t->icon_w);
    }
  else if(t->flags & (MAPPED|MAP_PENDING))
    {
      XUnmapWindow(dpy,t->frame);
    }
  XSelectInput(dpy, t->w, eventMask);
}

/**************************************************************************
 * 
 * Maps a window on transition to a new desktop
 *
 *************************************************************************/
void MapIt(FvwmWindow *t)
{
  if(t->flags & ICONIFIED)
    {
      if(t->icon_pixmap_w != None)
	XMapWindow(dpy,t->icon_pixmap_w);
      if(t->icon_w != None)
	XMapWindow(dpy,t->icon_w);
    }
  else if(t->flags & MAPPED)
    {
      XMapWindow(dpy,t->frame);
      t->flags |= MAP_PENDING;
      XMapWindow(dpy, t->Parent);
   }
}




void RaiseWindow(FvwmWindow *t)
{
  FvwmWindow *t2;
  int count, i;
  Window *wins;

  /* raise the target, at least */
  count = 1;
  Broadcast(M_RAISE_WINDOW,3,t->w,t->frame,(unsigned long)t,0,0,0,0);
  
  for (t2 = Scr.FvwmRoot.next; t2 != NULL; t2 = t2->next)
    {
      if(t2->flags & ONTOP)
	count++;
      if((t2->flags & TRANSIENT) &&(t2->transientfor == t->w)&&
	 (t2 != t))
	{
	  count++;
	  Broadcast(M_RAISE_WINDOW,3,t2->w,t2->frame,(unsigned long) t2,
		    0,0,0,0);
	  if ((t2->flags & ICONIFIED)&&(!(t2->flags & SUPPRESSICON)))
	    {
	      count += 2;
	    }	  
	}
    }
  if ((t->flags & ICONIFIED)&&(!(t->flags & SUPPRESSICON)))
    {
      count += 2;
    }

  wins = (Window *)safemalloc(count*sizeof(Window));

  i=0;

  /* ONTOP windows on top */
  for (t2 = Scr.FvwmRoot.next; t2 != NULL; t2 = t2->next)
    {
      if(t2->flags & ONTOP)
	{
	  Broadcast(M_RAISE_WINDOW,3,t2->w,t2->frame,(unsigned long) t2,
		    0,0,0,0);
	  wins[i++] = t2->frame;
	}
    }

  /* now raise transients */
#ifndef DONT_RAISE_TRANSIENTS
  for (t2 = Scr.FvwmRoot.next; t2 != NULL; t2 = t2->next)
    {
      if((t2->flags & TRANSIENT) &&(t2->transientfor == t->w)&&
	 (t2 != t)&&(!(t2->flags & ONTOP)))
        {
	  wins[i++] = t2->frame;
	  if ((t2->flags & ICONIFIED)&&(!(t2->flags & SUPPRESSICON)))
	    {
	      if(!(t2->flags & NOICON_TITLE)) wins[i++] = t2->icon_w;
	      if(t->icon_pixmap_w) wins[i++] = t2->icon_pixmap_w;
	    }
	}
    }
#endif
  if ((t->flags & ICONIFIED)&&(!(t->flags & SUPPRESSICON)))
    {
      if(!(t->flags & NOICON_TITLE)) wins[i++] = t->icon_w;
      if (t->icon_pixmap_w) wins[i++] = t->icon_pixmap_w;
    }
  if(!(t->flags & ONTOP))
    wins[i++] = t->frame;
  if(!(t->flags & ONTOP))
    Scr.LastWindowRaised = t;

  if(i > 0)
    XRaiseWindow(dpy,wins[0]);

  XRestackWindows(dpy,wins,i);
  free(wins);
  raisePanFrames();
}


void LowerWindow(FvwmWindow *t)
{
  XLowerWindow(dpy,t->frame);

  Broadcast(M_LOWER_WINDOW,3,t->w,t->frame,(unsigned long)t,0,0,0,0);

  if((t->flags & ICONIFIED)&&(!(t->flags & SUPPRESSICON)))
    {
      XLowerWindow(dpy, t->icon_w);
      XLowerWindow(dpy, t->icon_pixmap_w);
    }
  Scr.LastWindowRaised = (FvwmWindow *)0;
}

/****************************************************************************
 * 
 * Copies a string into a new, malloc'ed string
 * Strips leading spaces and trailing spaces and new lines
 *
 ****************************************************************************/ 
char *stripcpy(char *source)
{
  char *tmp,*ptr;
  int len;

  if(source == NULL)
    return NULL;

  while(isspace((unsigned char)*source))
    source++;
  len = strlen(source);
  tmp = source + len -1;
  while(((isspace((unsigned char)*tmp))||(*tmp == '\n'))&&(tmp >=source))
    {
      tmp--;
      len--;
    }
  ptr = safemalloc(len+1);
  strncpy(ptr,source,len);
  ptr[len]=0;
  return ptr;
}
  


/****************************************************************************
 *
 * Gets the next "word" of input from char string indata.
 * "word" is a string with no spaces, or a qouted string.
 * Return value is ptr to indata,updated to point to text after the word
 * which is extracted.
 * token is the extracted word, which is copied into a malloced
 * space, and must be freed after use. 
 *
 **************************************************************************/
char *GetNextToken(char *indata,char **token)
{ 
  char *t,*start, *end, *text;

  t = indata;
  if(t == NULL)
    {
      *token = NULL;
      return NULL;
    }
  while(isspace((unsigned char)*t)&&(*t != 0))t++;
  start = t;
  while((!isspace((unsigned char)*t))&&(*t != 0))
    {
      /* Check for qouted text */
      if(*t == '"')
	{
	  t++;
	  while((*t != '"')&&(*t != 0))
	    {
	      /* Skip over escaped text, ie \" or \space */
	      if((*t == '\\')&&(*(t+1) != 0))
		t++;
	      t++;
	    }
	  if(*t == '"')
	    t++;
	}
      else
	{
	  /* Skip over escaped text, ie \" or \space */
	  if((*t == '\\')&&(*(t+1) != 0))
	    t++;
	  t++;
	}
    }
  end = t;

  text = safemalloc(end-start+1);
  *token = text;

  while(start < end)
    {
      /* Check for qouted text */
      if(*start == '"')
	{	
	  start++;
	  while((*start != '"')&&(*start != 0))
	    {
	      /* Skip over escaped text, ie \" or \space */
	      if((*start == '\\')&&(*(start+1) != 0))
		start++;
	      *text++ = *start++;
	    }
	  if(*start == '"')
	    start++;
	}
      else
	{
	  /* Skip over escaped text, ie \" or \space */
	  if((*start == '\\')&&(*(start+1) != 0))
	    start++;
	  *text++ = *start++;
	}
    }
  *text = 0;
  if(*end != 0)
    end++;

  return end;
}

/*
** GetToken: destructively rips next token from string, returning it
**           (you should free returned string later)
*/
char *GetToken(char **pstr)
{
  char *new_pstr=NULL,*tok,*ws;

  if (!pstr) return NULL;

  ws = GetNextToken(*pstr, &tok);
  while(ws && (*ws != '\0') && isspace(*ws)) ws++;

  if (*ws)
    new_pstr = strdup(ws);

  free(*pstr);
  *pstr = new_pstr;

  return tok;
}

void HandleHardFocus(FvwmWindow *t)
{
  int x,y;

  FocusOnNextTimeStamp = t;
  Scr.Focus = NULL;
  /* Do something to guarantee a new time stamp! */
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		&JunkX, &JunkY, &x, &y, &JunkMask);
  GrabEm(WAIT);
  XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, 
	       Scr.MyDisplayHeight, 
	       x + 2,y+2);
  XSync(dpy,0);
  XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, 
	       Scr.MyDisplayHeight, 
	       x ,y);
  UngrabEm();
}

/******************************************************************************
  OpenConsole - Open the console as a way of sending messages
******************************************************************************/
void OpenConsole()
{
#ifndef NO_CONSOLE
  console=fopen("/dev/console","w");
  if (console != NULL)
    fvwm_msg(DBG, "FvwmErrorHandler", "OpenConsole successful...\n");
#endif
}


/*
** fvwm_msg: used to send output from fvwm to files and or stderr/stdout
**
** type -> DBG == Debug, ERR == Error, INFO == Information, WARN == Warning
** id -> name of function, or other identifier
*/
void fvwm_msg(int type, char *id, char *msg,...)
{
  char *typestr;
  va_list args;
  int error=0;

  switch(type)
  {
    case DBG:
#if 0
      if (!debugging)
        return;
#endif /* 0 */
      typestr="<<DEBUG>> ";
      break;
    case ERR:
      typestr="<<ERROR>> ";
      break;
    case WARN:
      typestr="<<WARNING>> ";
      break;
    case INFO:
    default:
      typestr="";
      break;
  }

  va_start(args,msg);

  if (console == NULL) {
    fprintf(stderr,"Fvwm-95: in function %s: %s", id, typestr);
    vfprintf(stderr, msg, args);
    fprintf(stderr,"\n");
  } else {
    fprintf(console,"Fvwm-95: in function %s: %s", id, typestr);
    vfprintf(console, msg, args);
    fprintf(console,"\n");
  }

  if (type == ERR)
  {
    char tmp[1024]; /* I hate to use a fixed length but this will do for now */
    sprintf(tmp,"[FVWM95][%s]: %s ", id, typestr);
    vsprintf(tmp+strlen(tmp), msg, args);
    tmp[strlen(tmp)+1] = '\0';
    tmp[strlen(tmp)] = '\n';
    BroadcastName(M_ERROR,0,0,0,tmp);
  }

  va_end(args);
} /* fvwm_msg */

