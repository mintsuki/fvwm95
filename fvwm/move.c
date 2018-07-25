/*
 * This module is all original code 
 * by Rob Nation 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 *
 * code for moving windows
 *
 ***********************************************************************/

#include <FVWMconfig.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <X11/keysym.h>
#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

extern XEvent Event;
extern int menuFromFrameOrWindowOrTitlebar;
Bool NeedToResizeToo;

/****************************************************************************
 *
 * Start a window move operation
 *
 ****************************************************************************/
void move_window(XEvent *eventp,Window w,FvwmWindow *tmp_win,
		     unsigned long context, char *action,int* Module)
{
  int FinalX, FinalY;
  int val1, val2, val1_unit,val2_unit,n;
  XGCValues gcv;

  n = GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit);

  if (DeferExecution(eventp,&w,&tmp_win,&context, MOVE,ButtonPress))
    return;

  /* Change DrawGC line width according to window's border width */
  gcv.line_width = tmp_win->boundary_width - 1;
  XChangeGC(dpy, Scr.DrawGC, GCLineWidth, &gcv);

  /* gotta have a window */
  w = tmp_win->frame;
  if(tmp_win->flags & ICONIFIED)
    {
      if(tmp_win->icon_pixmap_w != None)
	{
	  XUnmapWindow(dpy,tmp_win->icon_w);
	  w = tmp_win->icon_pixmap_w;
	}
      else
	w = tmp_win->icon_w;
    }

  if(n == 2)
    {
      FinalX = val1*val1_unit/100;
      FinalY = val2*val2_unit/100;
    }
  else
    InteractiveMove(&w,tmp_win,&FinalX,&FinalY,eventp);
      

  if (w == tmp_win->frame)
    {
      SetupFrame (tmp_win, FinalX, FinalY,
		  tmp_win->frame_width, tmp_win->frame_height,FALSE);
    }
  else /* icon window */
    {
      tmp_win->flags |= ICON_MOVED;
      tmp_win->icon_x_loc = FinalX ;
      tmp_win->icon_xl_loc = FinalX -
	    (tmp_win->icon_w_width - tmp_win->icon_p_width)/2;
      tmp_win->icon_y_loc = FinalY; 
      Broadcast(M_ICON_LOCATION,7,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,
		tmp_win->icon_x_loc,tmp_win->icon_y_loc,
		tmp_win->icon_w_width, tmp_win->icon_w_height
		+tmp_win->icon_p_height);
      XMoveWindow(dpy,tmp_win->icon_w, 
		  tmp_win->icon_xl_loc, FinalY+tmp_win->icon_p_height);
      if(tmp_win->icon_pixmap_w != None)
	{
	  XMapWindow(dpy,tmp_win->icon_w);
	  XMoveWindow(dpy, tmp_win->icon_pixmap_w, tmp_win->icon_x_loc,FinalY);
	  XMapWindow(dpy,w);
	}
      
    }
  
  return;
}



/****************************************************************************
 *
 * Move the rubberband around, return with the new window location
 *
 ****************************************************************************/
void moveLoop(FvwmWindow *tmp_win, int XOffset, int YOffset, int Width,
	      int Height, int *FinalX, int *FinalY,Bool opaque_move,
	      Bool AddWindow)
{
  Bool finished = False;
  Bool done;
  int xl,yt,delta_x,delta_y;

  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,&xl, &yt,
		&JunkX, &JunkY, &JunkMask);
  xl += XOffset;
  yt += YOffset;

  if(AddWindow)
    MoveOutline(Scr.Root, xl, yt, Width,Height);

  DisplayPosition(tmp_win,xl+Scr.Vx,yt+Scr.Vy,True);

  while (!finished)
    {
      /* block until there is an interesting event */
      XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask | KeyPressMask |
		 PointerMotionMask | ButtonMotionMask | ExposureMask, &Event);
      StashEventTime(&Event);      

      /* discard any extra motion events before a logical release */
      if (Event.type == MotionNotify) 
	{
	  while(XCheckMaskEvent(dpy, PointerMotionMask | ButtonMotionMask |
				ButtonPressMask |ButtonRelease, &Event))
	    {
	      StashEventTime(&Event);
	      if(Event.type == ButtonRelease) break;
	    }
	}

      done = FALSE;
      /* Handle a limited number of key press events to allow mouseless
       * operation */
      if(Event.type == KeyPress)
	Keyboard_shortcuts(&Event,ButtonRelease);
      switch(Event.type)
	{
	case KeyPress:
	  /* simple code to bag out of move - CKH */
	  if (XLookupKeysym(&(Event.xkey),0) == XK_Escape)
	    {
	      if(!opaque_move)
		MoveOutline(Scr.Root, 0, 0, 0, 0);
	      *FinalX = tmp_win->frame_x;
	      *FinalY = tmp_win->frame_y;
	      finished = TRUE;
	    }
	  done = TRUE;
	  break;
	case ButtonPress:
	  XAllowEvents(dpy,ReplayPointer,CurrentTime);
	  if((Event.xbutton.button == 1)&&(Event.xbutton.state & ShiftMask))
	    {
	      NeedToResizeToo = True;
	      /* Fallthrough to button-release */
	    }
	  else
	    {
	      done = 1;
	      break;
	    }
	case ButtonRelease:
	  if(!opaque_move)
	    MoveOutline(Scr.Root, 0, 0, 0, 0);
	  xl = Event.xmotion.x_root + XOffset;
	  yt = Event.xmotion.y_root + YOffset;

	  /* Resist moving windows over the edge of the screen! */
	  if(((xl + Width) >= Scr.MyDisplayWidth)&&
	     ((xl + Width) < Scr.MyDisplayWidth+Scr.MoveResistance))
	    xl = Scr.MyDisplayWidth - Width - tmp_win->bw;
	  if((xl <= 0)&&(xl > -Scr.MoveResistance))
	    xl = 0;
	  if(((yt + Height) >= Scr.MyDisplayHeight)&&
	     ((yt + Height) < Scr.MyDisplayHeight+Scr.MoveResistance))
	    yt = Scr.MyDisplayHeight - Height - tmp_win->bw;
	  if((yt <= 0)&&(yt > -Scr.MoveResistance))
	    yt = 0;

	  *FinalX = xl;
	  *FinalY = yt;

	  done = TRUE;
	  finished = TRUE;
	  break;

	case MotionNotify:
	  xl = Event.xmotion.x_root;
	  yt = Event.xmotion.y_root;
	  HandlePaging(Scr.MyDisplayWidth,Scr.MyDisplayHeight,&xl,&yt,
		       &delta_x,&delta_y,False);
	  /* redraw the rubberband */
	  xl += XOffset;
	  yt += YOffset;

	  /* Resist moving windows over the edge of the screen! */
	  if(((xl + Width) >= Scr.MyDisplayWidth)&&
	     ((xl + Width) < Scr.MyDisplayWidth+Scr.MoveResistance))
	    xl = Scr.MyDisplayWidth - Width - tmp_win->bw;
	  if((xl <= 0)&&(xl > -Scr.MoveResistance))
	    xl = 0;
	  if(((yt + Height) >= Scr.MyDisplayHeight)&&
	     ((yt + Height) < Scr.MyDisplayHeight+Scr.MoveResistance))
	    yt = Scr.MyDisplayHeight - Height - tmp_win->bw;
	  if((yt <= 0)&&(yt > -Scr.MoveResistance))
	    yt = 0;

	  if(!opaque_move)
	    MoveOutline(Scr.Root, xl, yt, Width,Height);
	  else
	    {
	      if (tmp_win->flags & ICONIFIED)
		{
		  tmp_win->icon_x_loc = xl ;
		  tmp_win->icon_xl_loc = xl -
		    (tmp_win->icon_w_width - tmp_win->icon_p_width)/2;
		  tmp_win->icon_y_loc = yt; 
		  if(tmp_win->icon_pixmap_w != None)
		    XMoveWindow (dpy, tmp_win->icon_pixmap_w,
				 tmp_win->icon_x_loc,yt);
                  else if (tmp_win->icon_w != None)
		    XMoveWindow(dpy, tmp_win->icon_w,tmp_win->icon_xl_loc,
				yt+tmp_win->icon_p_height);
		    
		}
	      else
		XMoveWindow(dpy,tmp_win->frame,xl,yt);
	    }
	  DisplayPosition(tmp_win,xl+Scr.Vx,yt+Scr.Vy,False);
	  done = TRUE;
	  break;

	default:
	  break;
	}
      if(!done)
	{
	  if(!opaque_move)
	    MoveOutline(Scr.Root,0,0,0,0);
   	  DispatchEvent();
	  if(!opaque_move)
	    MoveOutline(Scr.Root, xl, yt, Width, Height);
	}
    }
}

/***********************************************************************
 *
 *  Procedure:
 *      DisplayPosition - display the position in the dimensions window
 *
 *  Inputs:
 *      tmp_win - the current fvwm window
 *      x, y    - position of the window
 *
 ************************************************************************/

void DisplayPosition (FvwmWindow *tmp_win, int x, int y,int Init)
{
  char str [100];
  int offset;
  
  (void) sprintf (str, " %+-4d %+-4d ", x, y);
  if(Init)
    {
      XClearWindow(dpy,Scr.SizeWindow);
      if(Scr.d_depth >= 2)
	RelieveWindow(tmp_win,Scr.SizeWindow,0,0,
		      Scr.SizeStringWidth+ SIZE_HINDENT*2,
		      Scr.StdFont.height + SIZE_VINDENT*2,
		      Scr.WinReliefGC,Scr.WinShadowGC, FULL_HILITE);
    }
  else
    {
      XClearArea(dpy,Scr.SizeWindow,SIZE_HINDENT,SIZE_VINDENT,Scr.SizeStringWidth,
		 Scr.StdFont.height,False);
    }

  offset = (Scr.SizeStringWidth + SIZE_HINDENT*2
	    - XTextWidth(Scr.StdFont.font,str,strlen(str)))/2;
#ifdef I18N
#undef FONTSET
#define FONTSET Scr.StdFont.fontset
#endif
  XDrawString (dpy, Scr.SizeWindow, Scr.WinGC,
	       offset,
	       Scr.StdFont.font->ascent + SIZE_VINDENT,
	       str, strlen(str));
}


/****************************************************************************
 *
 * For menus, move, and resize operations, we can effect keyboard 
 * shortcuts by warping the pointer.
 *
 ****************************************************************************/
void Keyboard_shortcuts(XEvent *Event, int ReturnEvent)  
{
  int x,y,x_root,y_root;
  int move_size,x_move,y_move;
  KeySym keysym;

  /* Pick the size of the cursor movement */
  move_size = Scr.EntryHeight;
  if(Event->xkey.state & ControlMask)
    move_size = 1;
  if(Event->xkey.state & ShiftMask)
    move_size = 100;

  keysym = XLookupKeysym(&Event->xkey,0);

  x_move = 0;
  y_move = 0;
  switch(keysym)
    {
    case XK_Up:
    case XK_k:
    case XK_p:
      y_move = -move_size;
      break;
    case XK_Down:
    case XK_n:
    case XK_j:
      y_move = move_size;
      break;
    case XK_Left:
    case XK_b:
    case XK_h:
      x_move = -move_size;
      break;
    case XK_Right:
    case XK_f:
    case XK_l:
      x_move = move_size;
      break;
    case XK_Return:
    case XK_space:
      /* beat up the event */
      Event->type = ReturnEvent;
      break;
    case XK_Escape:
      /* simple code to bag out of move - CKH */
      /* return keypress event instead */
      Event->type = KeyPress;
      Event->xkey.keycode = XKeysymToKeycode(Event->xkey.display,keysym);
      break;
    default:
      break;
    }
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &Event->xany.window,
		&x_root, &y_root, &x, &y, &JunkMask);

  if((x_move != 0)||(y_move != 0))
    {
      /* beat up the event */
      XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, x_root+x_move,
		   y_root+y_move);

      /* beat up the event */
      Event->type = MotionNotify;
      Event->xkey.x += x_move;
      Event->xkey.y += y_move;
      Event->xkey.x_root += x_move;
      Event->xkey.y_root += y_move;
    }
}


void InteractiveMove(Window *win, FvwmWindow *tmp_win, int *FinalX, int *FinalY, XEvent *eventp)
{
  extern int Stashed_X, Stashed_Y;
  int origDragX,origDragY,DragX, DragY, DragWidth, DragHeight;
  int XOffset, YOffset;
  Window w;

  Bool opaque_move = False;
  
  w = *win;
 
  InstallRootColormap();
  if (menuFromFrameOrWindowOrTitlebar) 
    {
      /* warp the pointer to the cursor position from before menu appeared*/
      /*XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, Stashed_X,Stashed_Y);*/
      XFlush(dpy);
    }


  DragX = eventp->xbutton.x_root;
  DragY = eventp->xbutton.y_root;
  /* If this is left commented out, then the move starts from the button press 
   * location instead of the current location, which seems to be an
   * improvement */
  /*  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
      &DragX, &DragY,	&JunkX, &JunkY, &JunkMask);*/

  if(!GrabEm(MOVE))
    {
      XBell(dpy,Scr.screen);
      return;
    }

  XGetGeometry(dpy, w, &JunkRoot, &origDragX, &origDragY,
	       (unsigned int *)&DragWidth, (unsigned int *)&DragHeight, 
	       &JunkBW,  &JunkDepth);

  if(DragWidth*DragHeight <
     (Scr.OpaqueSize*Scr.MyDisplayWidth*Scr.MyDisplayHeight)/100)
    opaque_move = True;
  else
    XGrabServer(dpy);
  
  if((!opaque_move)&&(tmp_win->flags & ICONIFIED))
    XUnmapWindow(dpy,w);

  DragWidth += JunkBW;
  DragHeight+= JunkBW;
  XOffset = origDragX - DragX;
  YOffset = origDragY - DragY;
  XMapRaised(dpy,Scr.SizeWindow);
  moveLoop(tmp_win, XOffset,YOffset,DragWidth,DragHeight, FinalX,FinalY,
	   opaque_move,False);

  XUnmapWindow(dpy,Scr.SizeWindow);
  UninstallRootColormap();

  if(!opaque_move)
    XUngrabServer(dpy);
  UngrabEm();

}
