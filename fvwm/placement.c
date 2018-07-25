/****************************************************************************
 * This module is all new
 * by Rob Nation 
 *
 * This code does smart-placement initial window placement stuff
 *
 * Copyright 1994 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved . No guarantees or
 * warrantees of any sort whatsoever are given or implied or anything.
 ****************************************************************************/

#include <FVWMconfig.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"

void SmartPlacement(FvwmWindow *t, int width, int height, int *x, int *y)
{
  int temp_h,temp_w;
  int test_x = 0,test_y = 0;
  int loc_ok = False, tw,tx,ty,th;
  FvwmWindow *test_window;

  temp_h = height;
  temp_w = width;
      
  while(((test_y + temp_h) < (Scr.MyDisplayHeight))&&(!loc_ok))
    {
      test_x = 0;
      while(((test_x + temp_w) < (Scr.MyDisplayWidth))&&(!loc_ok))
	{
	  loc_ok = True;
	  test_window = Scr.FvwmRoot.next;
	  while((test_window != (FvwmWindow *)0)&&(loc_ok == True))
	    {	
	      if(test_window->Desk == Scr.CurrentDesk)
		{
		  if(!(test_window->flags & ICONIFIED)&&(test_window != t))
		    {
		      tw=test_window->frame_width+2*test_window->bw;
		      th=test_window->frame_height+2*test_window->bw;
		      tx = test_window->frame_x;
		      ty = test_window->frame_y;
		      if((tx <= (test_x+width))&&((tx + tw) >= test_x)&&
			 (ty <= (test_y+height))&&((ty + th)>= test_y))
			{
			  loc_ok = False;
			  test_x = tx + tw;
			}
		    }
		}
	      test_window = test_window->next;
	    }
	 test_x +=1;
	}
      test_y +=1;
    }
  if(loc_ok == False)
    {
      *x = -1;
      *y = -1;
      return;
    }
  *x = test_x;
  *y = test_y;
}


/**************************************************************************
 *
 * Handles initial placement and sizing of a new window
 * Returns False in the event of a lost window.
 *
 **************************************************************************/
Bool PlaceWindow(FvwmWindow *tmp_win, unsigned long tflag,int Desk)
{
  FvwmWindow *t;
  int xl = -1,yt,DragWidth,DragHeight;
  int gravx, gravy;			/* gravity signs for positioning */
  extern Bool PPosOverride;
  
  GetGravityOffsets (tmp_win, &gravx, &gravy);


  /* Select a desk to put the window on (in list of priority):
   * 1. Sticky Windows stay on the current desk.
   * 2. Windows specified with StartsOnDesk go where specified
   * 3. Put it on the desk it was on before the restart.
   * 4. Transients go on the same desk as their parents.
   * 5. Window groups stay together (completely untested)
   */
  tmp_win->Desk = Scr.CurrentDesk;
  if (tflag & STICKY_FLAG)
    tmp_win->Desk = Scr.CurrentDesk;
  else if (tflag & STARTSONDESK_FLAG)
    tmp_win->Desk = Desk;
  else
    {
      Atom atype;
      int aformat;
      unsigned long nitems, bytes_remain;
      unsigned char *prop;
      
      if((tmp_win->wmhints)&&(tmp_win->wmhints->flags & WindowGroupHint)&&
	 (tmp_win->wmhints->window_group != None)&&
	 (tmp_win->wmhints->window_group != Scr.Root))
	{
	  /* Try to find the group leader or another window
	   * in the group */
	  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	    {
	      if((t->w == tmp_win->wmhints->window_group)||
		 ((t->wmhints)&&(t->wmhints->flags & WindowGroupHint)&&
		  (t->wmhints->window_group==tmp_win->wmhints->window_group)))
		tmp_win->Desk = t->Desk;
	    }
	}
      if((tmp_win->flags & TRANSIENT)&&(tmp_win->transientfor!=None)&&
	 (tmp_win->transientfor != Scr.Root))
	{
	  /* Try to find the parent's desktop */
	  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	    {
	      if(t->w == tmp_win->transientfor)
		tmp_win->Desk = t->Desk;
	    }
	}	
      
      if ((XGetWindowProperty(dpy, tmp_win->w, _XA_WM_DESKTOP, 0L, 1L, True,
			      _XA_WM_DESKTOP, &atype, &aformat, &nitems,
			      &bytes_remain, &prop))==Success)
	{
	  if(prop != NULL)
	    {
	      tmp_win->Desk = *(unsigned long *)prop;
	      XFree(prop);
	    }
	}
    }
  /* I think it would be good to switch to the selected desk
   * whenever a new window pops up, except during initialization */
  if((!PPosOverride)&&(!(tmp_win->flags & SHOW_ON_MAP)))
    changeDesks(0,tmp_win->Desk);  


  /* Desk has been selected, now pick a location for the window */
  /*
   *  If
   *     o  the window is a transient, or
   * 
   *     o  a USPosition was requested
   * 
   *   then put the window where requested.
   *
   *   If RandomPlacement was specified,
   *       then place the window in a psuedo-random location
   */
  if (!(tmp_win->flags & TRANSIENT) && 
      !(tmp_win->hints.flags & USPosition) &&
      ((tflag & NO_PPOSITION_FLAG)||
       !(tmp_win->hints.flags & PPosition)) &&
      !(PPosOverride) &&
      !((tmp_win->wmhints)&&
	(tmp_win->wmhints->flags & StateHint)&&
	(tmp_win->wmhints->initial_state == IconicState)) )
    {
      /* Get user's window placement, unless RandomPlacement is specified */   
      if(tflag & RANDOM_PLACE_FLAG)
	{
	  if(tflag & SMART_PLACE_FLAG)
	    SmartPlacement(tmp_win,tmp_win->frame_width+2*tmp_win->bw,
			   tmp_win->frame_height+2*tmp_win->bw, 
			   &xl,&yt);
	  if(xl < 0)
	    {
	      /* plase window in a random location */
	      if ((Scr.randomx += Scr.TitleHeight) > Scr.MyDisplayWidth / 2)
		Scr.randomx = Scr.TitleHeight;
	      if ((Scr.randomy += 2*Scr.TitleHeight) > Scr.MyDisplayHeight / 2)
		Scr.randomy = 2 * Scr.TitleHeight;
	      tmp_win->attr.x = Scr.randomx - tmp_win->old_bw;
	      tmp_win->attr.y = Scr.randomy - tmp_win->old_bw;
	    }
	  else
	    {
	      tmp_win->attr.x = xl - tmp_win->old_bw;
	      tmp_win->attr.y = yt - tmp_win->old_bw;
	    }
	  /* patches 11/93 to try to keep the window on the
	   * screen */
	  tmp_win->frame_x = tmp_win->attr.x + tmp_win->old_bw - tmp_win->bw;
	  tmp_win->frame_y = tmp_win->attr.y + tmp_win->old_bw - tmp_win->bw;
	  
	  if(tmp_win->frame_x + tmp_win->frame_width +
	     2*tmp_win->boundary_width> Scr.MyDisplayWidth)
	    {
	      tmp_win->attr.x = Scr.MyDisplayWidth -tmp_win->attr.width
		- tmp_win->old_bw +tmp_win->bw - 2*tmp_win->boundary_width;
	      Scr.randomx = 0;
	    }
	  if(tmp_win->frame_y + 2*tmp_win->boundary_width+tmp_win->title_height
	     + tmp_win->frame_height > Scr.MyDisplayHeight)
	    {
	      tmp_win->attr.y = Scr.MyDisplayHeight -tmp_win->attr.height
		- tmp_win->old_bw +tmp_win->bw - tmp_win->title_height -
		  2*tmp_win->boundary_width;;
	      Scr.randomy = 0;
	    }

	  tmp_win->xdiff = tmp_win->attr.x - tmp_win->bw;
	  tmp_win->ydiff = tmp_win->attr.y - tmp_win->bw;
	}
      else
	{	  
	  xl = -1;
	  yt = -1;
	  if(tflag & SMART_PLACE_FLAG)
	    SmartPlacement(tmp_win,tmp_win->frame_width+2*tmp_win->bw,
			   tmp_win->frame_height+2*tmp_win->bw, 
			   &xl,&yt);
	  if(xl < 0)
	    {
	      if(GrabEm(POSITION))
		{
		  /* Grabbed the pointer - continue */
		  XGrabServer(dpy);	  
		  if(XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
				  (unsigned int *)&DragWidth,
				  (unsigned int *)&DragHeight, 
				  &JunkBW,  &JunkDepth) == 0)
		    {
		      free((char *)tmp_win);
		      XUngrabServer(dpy);
		      return False;
		    }
		  DragWidth = tmp_win->frame_width;
		  DragHeight = tmp_win->frame_height;
		  
		  XMapRaised(dpy,Scr.SizeWindow);
		  moveLoop(tmp_win,0,0,DragWidth,DragHeight,
			   &xl,&yt,False,True);
		  XUnmapWindow(dpy,Scr.SizeWindow);
		  XUngrabServer(dpy);
		  UngrabEm();
		}
	      else
		{
		  /* couldn't grab the pointer - better do something */
		  XBell(dpy,Scr.screen);
		  xl = 0;
		  yt = 0;
		}
	    }
	  tmp_win->attr.y = yt - tmp_win->old_bw + tmp_win->bw;
	  tmp_win->attr.x = xl - tmp_win->old_bw + tmp_win->bw;	      
	  tmp_win->xdiff = xl ;
	  tmp_win->ydiff = yt ;
	}
    }
  else 
    {
      /* the USPosition was specified, or the window is a transient, 
       * or it starts iconic so place it automatically */

      tmp_win->xdiff = tmp_win->attr.x; 
      tmp_win->ydiff =  tmp_win->attr.y;
      /* put it where asked, mod title bar */
      /* if the gravity is towards the top, move it by the title height */
      tmp_win->attr.y -= gravy*(tmp_win->bw-tmp_win->old_bw);
      tmp_win->attr.x -= gravx*(tmp_win->bw-tmp_win->old_bw);
      if(gravy > 0)
        tmp_win->attr.y -= 2*tmp_win->boundary_width + tmp_win->title_height;
      if(gravx > 0)
        tmp_win->attr.x -= 2*tmp_win->boundary_width;
    }
  return True;
}



/************************************************************************
 *
 *  Procedure:
 *	GetGravityOffsets - map gravity to (x,y) offset signs for adding
 *		to x and y when window is mapped to get proper placement.
 * 
 ************************************************************************/
struct _gravity_offset 
{
  int x, y;
};

void GetGravityOffsets (FvwmWindow *tmp,int *xp,int *yp)
{
  static struct _gravity_offset gravity_offsets[11] = 
    {
      {  0,  0 },			/* ForgetGravity */
      { -1, -1 },			/* NorthWestGravity */
      {  0, -1 },			/* NorthGravity */
      {  1, -1 },			/* NorthEastGravity */
      { -1,  0 },			/* WestGravity */
      {  0,  0 },			/* CenterGravity */
      {  1,  0 },			/* EastGravity */
      { -1,  1 },			/* SouthWestGravity */
      {  0,  1 },			/* SouthGravity */
      {  1,  1 },			/* SouthEastGravity */
      {  0,  0 },			/* StaticGravity */
    };
  register int g = ((tmp->hints.flags & PWinGravity) 
		    ? tmp->hints.win_gravity : NorthWestGravity);
  
  if (g < ForgetGravity || g > StaticGravity) 
    *xp = *yp = 0;
  else 
    {
      *xp = (int)gravity_offsets[g].x;
      *yp = (int)gravity_offsets[g].y;
    }
  return;
}
