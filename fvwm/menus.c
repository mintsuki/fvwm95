/****************************************************************************
 * This module is all original code 
 * by Rob Nation 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/


/***********************************************************************
 *
 * fvwm menu code
 *
 ***********************************************************************/
#include <FVWMconfig.h>

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <X11/keysym.h>
#include <sys/types.h>
#include <sys/time.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"

int menu_on=0;

MenuRoot *ActiveMenu = NULL;		/* the active menu */
MenuItem *ActiveItem = NULL;		/* the active menu item */

int menuFromFrameOrWindowOrTitlebar = FALSE;

extern int Context,Button;
extern FvwmWindow *ButtonWindow, *Tmp_win;
extern XEvent Event;
int Stashed_X, Stashed_Y, MenuY = 0;

void DrawTrianglePattern(Window,GC,int,int,int,int);
void DrawSeparator(Window, GC,GC,int, int,int,int,int);
void DrawUnderline(Window w, GC gc, int x, int y, char *txt, int off);
int UpdateMenu(int sticks); 
int ButtonPosition(int context, FvwmWindow * t);
int mouse_moved = 0;
int menu_aborted = 0;

int PrevActiveMenuX = -1;

/* Menu side-picture support */
void PaintSidePic(MenuRoot *mr);

extern XContext MenuContext;
/****************************************************************************
 *
 * Initiates a menu pop-up
 *
 * Style = 1 = sticky menu, stays up on initial button release.
 * Style = 0 = transient menu, drops on initial release.
 ***************************************************************************/
int do_menu (MenuRoot *menu, int style)
{
  int prevStashedX=0,prevStashedY=0;
  MenuRoot *PrevActiveMenu=0;
  MenuItem *PrevActiveItem=0;
  int retval=MENU_NOP;
  int x,y;
  Time t0 = 0;
  extern Time lastTimestamp;
  int PrevMenuX = PrevActiveMenuX;

  /* this condition could get ugly */
  if(menu->in_use)
    return MENU_ERROR;

  /* In case we wind up with a move from a menu which is
   * from a window border, we'll return to here to start
   * the move */
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		&x, &y, &JunkX, &JunkY, &JunkMask);    

  if(menu_on)
    {
      prevStashedX = Stashed_X;
      prevStashedY = Stashed_Y;

      PrevActiveMenu = ActiveMenu;
      PrevActiveItem = ActiveItem;

      if(ActiveMenu)
        x = Stashed_X + ActiveMenu->width - 3;

      if(ActiveItem)
	y = ActiveItem->y_offset + MenuY;
    }
  else
    {
      mouse_moved = 0;
      t0 = lastTimestamp;
      if(!GrabEm(MENU))
	{
	XBell(dpy,Scr.screen);
	return MENU_DONE;
	}
      x += 2;
    }
  if (PopUpMenu (menu, x, y))
    {
      retval = UpdateMenu(style);
    }
  else
    XBell (dpy, Scr.screen);

  ActiveMenu = PrevActiveMenu;
  ActiveItem = PrevActiveItem;
  if((ActiveItem)&&(menu_on))
    ActiveItem->state = 1;
  Stashed_X = prevStashedX;
  Stashed_Y = prevStashedY;


  if(!menu_on)
    {
      UngrabEm();
      WaitForButtonsUp();
    }
  if(((lastTimestamp-t0) < 3*Scr.ClickTime) && (mouse_moved == 0))
    menu_aborted = 1;
  else
    menu_aborted = 0;

  PrevActiveMenuX = PrevMenuX;
  return retval;
}

/***********************************************************************
 *
 *  Procedure:
 *	RelieveRectangle - add relief lines to a rectangular window
 *
 ***********************************************************************/
void RelieveRectangle(Window win,int x,int y,int w, int h,GC Hilite,GC Shadow)
  {
  XDrawLine(dpy, win, Hilite, x, y, w+x-1, y);
  XDrawLine(dpy, win, Hilite, x, y, x, h+y-1);

  XDrawLine(dpy, win, Shadow, x, h+y-1, w+x-1, h+y-1);
  XDrawLine(dpy, win, Shadow, w+x-1, y, w+x-1, h+y-1);
  }

/***********************************************************************
 *
 *  Procedure:
 *	RelieveHalfRectangle - add relief lines to the sides only of a
 *      rectangular window
 *
 ***********************************************************************/
void RelieveHalfRectangle(Window win,int x,int y,int w,int h,
			  GC Hilite,GC Shadow)
  {
  XDrawLine(dpy, win, Hilite, x+1, y, x+1, h+y-1);

  XDrawLine(dpy, win, Scr.BlackGC, w+x-1, y-1, w+x-1, h+y);
  XDrawLine(dpy, win, Shadow, w+x-2, y, w+x-2, h+y-1);
  }


/***********************************************************************
 *
 *  Procedure:
 *      PaintEntry - draws a single entry in a poped up menu
 *
 *  If a side picture specified, all entries are shifted to the right
 *
 ***********************************************************************/
void PaintEntry(MenuRoot *mr, MenuItem *mi)
  {
  int y_offset, text_y, d, y_height, y, x;
  GC ShadowGC, ReliefGC, TextGC;

  y_offset = mi->y_offset;
  y_height = mi->y_height;
  text_y = y_offset + Scr.StdFont.y;

  if (mi->picture) text_y += mi->picture->height;

  /* center text vertically if the pixmap is taller */
  if (mi->lpicture)
    {
    y = mi->lpicture->height - Scr.StdFont.height;
    if (y > 1) text_y += y >> 1;
    }    

  ShadowGC = Scr.MenuShadowGC;
  if(Scr.d_depth<2)
    ReliefGC = Scr.MenuShadowGC;
  else
    ReliefGC = Scr.MenuReliefGC;

  TextGC = Scr.MenuGC;

  /* active cursor over entry? */
  if ((mi->state)&&(mi->func_type != F_TITLE)&&
      (mi->func_type != F_NOP)&&*mi->item)
    {
    Globalgcv.foreground = Scr.SelColors.back;
    Globalgcm = GCForeground;
    XChangeGC(dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
    XFillRectangle(dpy, mr->w, Scr.ScratchGC1, mr->xoffset+3, y_offset,
                   mr->width - mr->xoffset-6, y_height);
    TextGC = Scr.SelGC;
    }
  else
    {
    if((!mi->prev)||(!mi->prev->state))
      XClearArea(dpy, mr->w, mr->xoffset, y_offset-1, mr->width,y_height+2,0);
    else
      XClearArea(dpy, mr->w, mr->xoffset, y_offset+1, mr->width,y_height-1,0);
    }

  RelieveHalfRectangle(mr->w, 0, y_offset-1, mr->width,
                       y_height+2, ReliefGC, ShadowGC);

  if(mi->func_type == F_TITLE)
    {
    text_y += HEIGHT_EXTRA>>1;

    if(mi->next != NULL)
      {
      DrawSeparator(mr->w,ShadowGC,ReliefGC,5+mr->xoffset, y_offset+y_height-3,
		    mr->width-6, y_offset+y_height-3,1);
      }
    if(mi != mr->first)
      {
      text_y += HEIGHT_EXTRA_TITLE>>1;
      DrawSeparator(mr->w,ShadowGC,ReliefGC,5+mr->xoffset, y_offset+1,
		    mr->width-6, y_offset+1,1);
      }
    }
  else
    {
    text_y += HEIGHT_EXTRA>>1;
    }

  if(mi->func_type == F_NOP && *mi->item==0) 
    {
    DrawSeparator(mr->w, ShadowGC, ReliefGC, 2+mr->xoffset, 
                      y_offset-1+(HEIGHT_SEPARATOR >> 1),
		      mr->width-3,y_offset-1+(HEIGHT_SEPARATOR >> 1), 0);
    }

  /* Top of the menu */
  if(mi == mr->first) 
    /*DrawSeparator(mr->w,ReliefGC,ReliefGC,0,0, mr->width-1,0,-1);*/
    XDrawLine(dpy, mr->w, ReliefGC, 1, 1, mr->width-3, 1);

  /* Botton of the menu */
  if(mi->next == NULL) 
    DrawSeparator(mr->w,ShadowGC,Scr.BlackGC,1,mr->height-2,
		  mr->width-2, mr->height-2,1);
/* Why is this here; it has no effect! */
#ifdef I18N
#undef FONTSET
#define FONTSET       Scr.StdFont.fontset
#endif
  if (*mi->item) {
    if (check_allowed_function(mi))
      {
      XDrawString(dpy, mr->w, TextGC, 
                  mi->x + mr->xoffset, text_y, mi->item, mi->strlen);
      }
    else
      {
      /* should be a shaded out word, no just re-colored. */
      if (!mi->state)
        XDrawString(dpy, mr->w, ReliefGC,
                    mi->x + mr->xoffset+1, text_y+1, mi->item, mi->strlen);
      XDrawString(dpy, mr->w, ShadowGC,
                  mi->x+mr->xoffset, text_y, mi->item, mi->strlen);
      }
  }

  if (mi->strlen2>0) {
    if (check_allowed_function(mi))
      {
      XDrawString(dpy, mr->w, TextGC,
                  mi->x2+mr->xoffset, text_y, mi->item2, mi->strlen2);
      }
    else
      {
      /* should be a shaded out word, no just re-colored. */
      if (!mi->state)
        XDrawString(dpy, mr->w, ReliefGC,
                    mi->x2+1 + mr->xoffset, text_y+1, mi->item2, mi->strlen2);
      XDrawString(dpy, mr->w, ShadowGC,
                  mi->x2 + mr->xoffset, text_y, mi->item2, mi->strlen2);
      }
  }

  /* pete@tecc.co.uk: If the item has a hot key, underline it */
  if (mi->hotkey > 0) {
    if (check_allowed_function(mi))
      {
      DrawUnderline(mr->w, TextGC, mi->x + mr->xoffset, text_y,
                    mi->item, mi->hotkey - 1);
      }
    else
      {
      /* should be a shaded out underline, no just re-colored. */
      if (!mi->state)
        DrawUnderline(mr->w, ReliefGC, mi->x+1 + mr->xoffset, text_y+1,
                      mi->item, mi->hotkey - 1);
      DrawUnderline(mr->w, ShadowGC, mi->x + mr->xoffset, text_y,
                    mi->item, mi->hotkey - 1);
      }
  }

  if (mi->hotkey < 0) {
    if (check_allowed_function(mi))
      {
      DrawUnderline(mr->w, TextGC, mi->x2 + mr->xoffset, text_y,
                    mi->item2, -1 - mi->hotkey);
      }
    else
      {
      /* should be a shaded out underline, no just re-colored. */
      if (!mi->state)
        DrawUnderline(mr->w, ReliefGC, mi->x2+1 + mr->xoffset, text_y+1,
                      mi->item2, -1 - mi->hotkey);
      DrawUnderline(mr->w, ShadowGC, mi->x2 + mr->xoffset, text_y,
                    mi->item2, -1 - mi->hotkey);
      }
  }

  d=(Scr.EntryHeight-7) >> 1;
  if(mi->func_type == F_POPUP)
    {
    int ym;

    ym = y_offset + (y_height >> 1) -1;
    DrawTrianglePattern(mr->w, TextGC, mr->width-d-8,
		        ym-4, mr->width-d-4, ym+4);
    }

  if(mi->picture)
    {
      x = (mr->width - mi->picture->width) >> 1;
      if(mi->lpicture && x < mr->width0 + 5)
	x = mr->width0 + 5;
	
      if(mi->picture->depth > 0) /* pixmap? */
	{
	  Globalgcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
	  Globalgcv.clip_mask = mi->picture->mask;
	  Globalgcv.clip_x_origin = x;
	  Globalgcv.clip_y_origin = y_offset+1;
	  XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
	  XCopyArea(dpy,mi->picture->picture,mr->w,ReliefGC,
		    0,0,mi->picture->width, mi->picture->height,
		    x,y_offset+1);
	  Globalgcm = GCClipMask;
	  Globalgcv.clip_mask = None;
	  XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
	}
      else
	{
	  XCopyPlane(dpy,mi->picture->picture,mr->w,
		     TextGC,0,0,mi->picture->width,mi->picture->height,
		     x,y_offset+1,1);
	}
    }

  if(mi->lpicture)
    {
      if(mi->picture && *mi->item != 0)
	y = y_offset + mi->y_height - mi->lpicture->height-1;
      else 
	y = y_offset + (mi->y_height >> 1) - (mi->lpicture->height >> 1);
      if(mi->lpicture->depth > 0) /* pixmap? */
	{
	  Globalgcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
	  Globalgcv.clip_mask = mi->lpicture->mask;
	  Globalgcv.clip_x_origin = 9 + mr->xoffset;
	  Globalgcv.clip_y_origin = y;

	  XChangeGC(dpy, ReliefGC, Globalgcm, &Globalgcv);
	  XCopyArea(dpy, mi->lpicture->picture, mr->w, ReliefGC, 0, 0,
		    mi->lpicture->width, mi->lpicture->height,
                    9 + mr->xoffset, y);
	  Globalgcm = GCClipMask;
	  Globalgcv.clip_mask = None;
	  XChangeGC(dpy, ReliefGC, Globalgcm, &Globalgcv);
	}
      else
	{
	  XCopyPlane(dpy, mi->lpicture->picture, mr->w,
		     TextGC, 0, 0, mi->lpicture->width, mi->lpicture->height,
		     9 + mr->xoffset, y, 1);
	}
    }
  return;
}

/****************************************************************************
 * Procedure:
 *	DrawUnderline() - Underline a character in a string (pete@tecc.co.uk)
 *
 * Calculate the pixel offsets to the start of the character position we
 * want to underline and to the next character in the string.  Shrink by
 * one pixel from each end and the draw a line that long two pixels below
 * the character...
 *
 ****************************************************************************/
void  DrawUnderline(Window w, GC gc, int x, int y, char *txt, int posn) 
  {
  int off1 = XTextWidth(Scr.StdFont.font, txt, posn);
  int off2 = XTextWidth(Scr.StdFont.font, txt, posn + 1) - 1;

  XDrawLine(dpy, w, gc, x + off1, y + 1, x + off2, y + 1);
  }

/****************************************************************************
 *
 *  Draws two horizontal lines to form a separator
 *
 ****************************************************************************/
void DrawSeparator(Window w, GC TopGC, GC BottomGC,int x1,int y1,int x2,int y2,
		   int extra_off)
  {
  XDrawLine(dpy, w, TopGC   , x1,           y1,  x2,          y2);
  XDrawLine(dpy, w, BottomGC, x1-extra_off, y1+1,x2+extra_off,y2+1);
  }
    
/****************************************************************************
 *
 *  Draws a little Triangle pattern within a window
 *
 ****************************************************************************/
void DrawTrianglePattern(Window w, GC gc, int l, int t, int r, int b)
{
  int m;
  XPoint points[3];

  m = (t + b) >> 1;

  points[0].x = l;
  points[0].y = t;
  points[1].x = l;
  points[1].y = b;
  points[2].x = r;
  points[2].y = m;

  XFillPolygon(dpy, w, gc, points, 3, Convex, CoordModeOrigin);
}

/************************************************************
 *
 * Draws a picture on the left side of the menu
 *
 ************************************************************/

void PaintSidePic(MenuRoot *mr)
{
  GC ReliefGC, TextGC;

  if(mr->sidePic) {
    if(Scr.d_depth<2)
      ReliefGC = Scr.WinShadowGC;
    else
      ReliefGC = Scr.WinReliefGC;
    TextGC = Scr.WinGC;

    if(mr->colorize) {
      Globalgcv.foreground = mr->sideColor;
      Globalgcm = GCForeground;
      XChangeGC(dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
      XFillRectangle(dpy, mr->w, Scr.ScratchGC1, 3, 3,
                   mr->sidePic->width, mr->height - 6);
    }
    
    if(mr->sidePic->depth > 0) /* pixmap? */
      {
      Globalgcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
      Globalgcv.clip_mask = mr->sidePic->mask;
      Globalgcv.clip_x_origin = 3;
      Globalgcv.clip_y_origin = mr->height - mr->sidePic->height -3;
      
      XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);
      XCopyArea(dpy, mr->sidePic->picture, mr->w,
                ReliefGC, 0, 0,
                mr->sidePic->width, mr->sidePic->height,
                Globalgcv.clip_x_origin, Globalgcv.clip_y_origin);
      Globalgcm = GCClipMask;
      Globalgcv.clip_mask = None;
      XChangeGC(dpy,ReliefGC,Globalgcm,&Globalgcv);    
    } else {
      XCopyPlane(dpy, mr->sidePic->picture, mr->w,
                 TextGC, 0, 0,
                 mr->sidePic->width, mr->sidePic->height,
                 1, mr->height - mr->sidePic->height, 1);
      }
  }
}

/***********************************************************************
 *
 *  Procedure:
 *	PaintMenu - draws the entire menu
 *
 ***********************************************************************/
void PaintMenu(MenuRoot *mr, XEvent *e)
{
  MenuItem *mi;
  
  for (mi = mr->first; mi != NULL; mi = mi->next)
    {
      /* be smart about handling the expose, redraw only the entries
       * that we need to
       */
      if (e->xexpose.y < (mi->y_offset + mi->y_height) &&
	  (e->xexpose.y + e->xexpose.height) > mi->y_offset)
	{
	  PaintEntry(mr, mi);
	}
    }

  /* Side picture support */
  PaintSidePic(mr);

  XSync(dpy, 0);
  return;
}

MenuRoot *PrevMenu = NULL;
MenuItem *PrevItem = NULL;
int PrevY=0;


/***********************************************************************
 *
 *  Procedure:
 *	Updates menu display to reflect the highlighted item
 *
 ***********************************************************************/
int FindEntry(void)
{
  MenuItem *mi;
  MenuRoot *actual_mr;
  int retval = MENU_NOP;
  MenuRoot *PrevPrevMenu;
  MenuItem *PrevPrevItem;
  int PrevPrevY;
  int x, y, ChildY;
  Window Child;

  XQueryPointer( dpy, Scr.Root, &JunkRoot, &Child,
		&JunkX,&ChildY, &x, &y, &JunkMask);
  XQueryPointer( dpy, ActiveMenu->w, &JunkRoot, &JunkChild,
		&JunkX, &ChildY, &x, &y, &JunkMask);

  /* look for the entry that the mouse is in */
  for(mi=ActiveMenu->first; mi; mi=mi->next)
    if(y>=mi->y_offset && y<mi->y_offset+mi->y_height)
      break;
  if(x<3 || x>ActiveMenu->width-3)
    mi = NULL;


  /* if we weren't on the active entry, let's turn the old active one off */
  if ((ActiveItem)&&(mi!=ActiveItem))
    {
      ActiveItem->state = 0;
      PaintEntry(ActiveMenu, ActiveItem);
    }
  /* if we weren't on the active item, change the active item and turn it on */
  if ((mi!=ActiveItem)&&(mi != NULL))
    {
      mi->state = 1;
      PaintEntry(ActiveMenu, mi);
    }
  ActiveItem = mi;

  if(ActiveItem)
    {
      MenuRoot *menu;
      /* create a new sub-menu */
      if((ActiveItem->func_type == F_POPUP) /* use this to prevent automatic
                                               popup, so you have to place
                                               the cursor near the |> symbol 
         && (x > (3*ActiveMenu->width>>2)) */)
	{
	  PrevPrevMenu = PrevMenu;
	  PrevPrevItem = PrevItem;
	  PrevPrevY = PrevY;
	  PrevY = MenuY;
	  PrevMenu = ActiveMenu;
	  PrevItem = ActiveItem;
	  /*******************************************/
	  menu = FindPopup(&ActiveItem->action[5]);
	  if(menu != NULL)
	    {/*
              if (ActiveMenu->has_coords) {
                menu->x = ActiveMenu->x + ActiveMenu->width;
                menu->y = ActiveMenu->y;
              }*/
	      retval = do_menu(menu,0);
	      /* Unfortunately, this is needed (why?) for multi-screen 
		 operation */
	      /* flush_expose(ActiveMenu->w);*/
	      for (mi = ActiveMenu->first; mi != NULL; mi = mi->next)
		{
		  PaintEntry(ActiveMenu, mi);
		}
	      XSync(dpy, 0);
	      MenuY = PrevY;
	      PrevMenu = PrevPrevMenu;
	      PrevItem = PrevPrevItem;
	      PrevY = PrevPrevY;
	    }
	}
    }
  /* end a sub-menu */ 
  if (XFindContext (dpy, Child,MenuContext,(caddr_t *)&actual_mr)==XCNOENT)
    {
      return retval;
    }

  if(actual_mr != ActiveMenu)
    {
      if(actual_mr == PrevMenu)
	{
	  if((PrevItem->y_offset + PrevY > ChildY)||
	     ((PrevItem->y_offset+PrevItem->y_height + PrevY) < ChildY))
	    {
	      return SUBMENU_DONE;
	    }
	}
      else
	return SUBMENU_DONE;
    }
  return retval;
}

/***********************************************************************
 * Procedure
 * 	menuShortcuts() - Menu keyboard processing (pete@tecc.co.uk)
 *
 * Function called from UpdateMenu instead of Keyboard_Shortcuts()
 * when a KeyPress event is received.  If the key is alphanumeric,
 * then the menu is scanned for a matching hot key.  Otherwise if
 * it was the escape key then the menu processing is aborted.
 * If none of these conditions are true, then the default processing
 * routine is called.
 ***********************************************************************/
void menuShortcuts(XEvent *ev) 
{
  MenuItem *mi;
  KeySym keysym = XLookupKeysym(&ev->xkey,0);
  
  /* Try to match hot keys */
  if (((keysym >= XK_a) && (keysym <= XK_z)) ||	/* Only consider alphabetic */
      ((keysym >= XK_0) && (keysym <= XK_9)))	/* ...or numeric keys	*/
    {
      /* Search menu for matching hotkey */
      for (mi = ActiveMenu->first; mi; mi = mi->next) 
	{
	  char key;
	  if (mi->hotkey == 0) continue;	/* Item has no hotkey	*/
	  key = (mi->hotkey > 0) ?		/* Extract hot character */
	    mi->item[mi->hotkey - 1] : mi->item2[-1 - mi->hotkey];
	  
	  /* Convert to lower case to match the keysym */
	  if (isupper(key)) key = tolower(key);
	  
	  if (keysym == key)
	    {		/* Are they equal?		*/
	      ActiveItem = mi;		/* Yes: Make this the active item */
	      ev->type = ButtonRelease;	/* Force a menu exit		*/
	      return;
	    }
	}
    }
  
  switch(keysym)		/* Other special keyboard handling	*/
    {
    case XK_Escape:		/* Escape key pressed. Abort		*/
      ActiveItem = NULL;	/* No selection				*/
      ev->type = ButtonRelease;	/* Make the menu exit			*/
      break;
      
      /* Nothing special --- Allow other shortcuts (cursor movement)	*/
    default:
      Keyboard_shortcuts(ev,ButtonRelease);
      break;
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	Updates menu display to reflect the highlighted item
 * 
 *  Input
 *      sticks = 0, transient style menu, drops on button release
 *      sticks = 1, sticky style, stays up if initial release is close to initial press.
 *  Returns:
 *      0 on error condition
 *      1 on return from submenu to parent menu
 *      2 on button release return
 *
 ***********************************************************************/
int UpdateMenu(int sticks)
{
  int done = 0;
  int retval;
  MenuItem *InitialMI;
  MenuRoot *actual_mr;
  int x_init, y_init,x,y;

  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		&JunkX,&JunkY, &x_init, &y_init, &JunkMask);

  FindEntry();
  InitialMI = ActiveItem;

  while (TRUE)
    {
      /* block until there is an event */
      XMaskEvent(dpy, ButtonPressMask|ButtonReleaseMask|ExposureMask | 
		 KeyPressMask|VisibilityChangeMask|ButtonMotionMask, &Event);
      StashEventTime(&Event);
      done = 0;
      if (Event.type == MotionNotify) 
	{
	  /* discard any extra motion events before a release */
	  while((XCheckMaskEvent(dpy,ButtonMotionMask|ButtonReleaseMask,
				 &Event))&&(Event.type != ButtonRelease));
	}
      /* Handle a limited number of key press events to allow mouseless
       * operation */
      if(Event.type == KeyPress)
	menuShortcuts(&Event);

      switch(Event.type)
	{
	case ButtonRelease:
	  /* The following lines holds the menu when the button is released */
	  if(sticks)
	    {
	      sticks = 0;
	      break;
	    }
	  if (!ActiveItem && (menu_on > 1))
	    {
	      int x,y;
	      XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY, &x, &y, &JunkMask);
	      if((XFindContext(dpy, JunkChild,MenuContext,
			      (caddr_t *)&actual_mr)!=XCNOENT)&&
		 (actual_mr != ActiveMenu))
		{
		  done = 1;
		  break;
		}
	    }
	  PopDownMenu();
	  if(ActiveItem)
	    {
	      done = 1;
	      ExecuteFunction(ActiveItem->action,
			      ButtonWindow, &Event, Context,-1);
	    }
	  ActiveItem = NULL;
	  ActiveMenu = NULL;
	  menuFromFrameOrWindowOrTitlebar = FALSE;
	  return MENU_DONE;

	case KeyPress:
	case VisibilityNotify:
	case ButtonPress:
	  done=1;
	  break;

	case MotionNotify:
	  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
			&JunkX,&JunkY, &x, &y, &JunkMask);
	  if(((x-x_init > 3)||(x_init-x > 3))&&
	     ((y-y_init > 3)||(y_init-y > 3)))
	    mouse_moved = 1;
	  done = 1;

	  retval = FindEntry();
	  if(ActiveItem != InitialMI)
	    sticks = 0;
	  if((retval == MENU_DONE)||(retval == SUBMENU_DONE))
	    {
	      PopDownMenu();	      
	      ActiveItem = NULL;
	      ActiveMenu = NULL;
	      menuFromFrameOrWindowOrTitlebar = FALSE;
	    }

	  if(retval == MENU_DONE)
	    return MENU_DONE;
	  else if (retval == SUBMENU_DONE)
	    return MENU_NOP;

	  break;

	case Expose:
	  /* grab our expose events, let the rest go through */
	  if((XFindContext(dpy, Event.xany.window,MenuContext,
			   (caddr_t *)&actual_mr)!=XCNOENT))
	    {
	      PaintMenu(actual_mr,&Event);
	      done = 1;
	    }
	  break;

	default:
	  break;
	}
      
      if(!done)DispatchEvent();
      XFlush(dpy);
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	PopUpMenu - pop up a pull down menu
 *
 *  Inputs:
 *	menu	- the root pointer of the menu to pop up
 *	x, y	- location of upper left of menu
 *      center	- whether or not to center horizontally over position
 *
 ***********************************************************************/
Bool PopUpMenu (MenuRoot *menu, int x, int y)
{
  if ((!menu)||(menu->w == None)||(menu->items == 0)||(menu->in_use))
    return False;

  menu_on++;
  InstallRootColormap();

  Stashed_X = x;
  Stashed_Y = y;
  
  /* pop up the menu */
  ActiveMenu = menu;
  ActiveItem = NULL;
  
  if((Tmp_win)&&(menu_on == 1)&&(Context&C_LALL))
    {
      y = Tmp_win->frame_y+Tmp_win->boundary_width+Tmp_win->title_height;
      x = Tmp_win->frame_x + Tmp_win->boundary_width + 
	ButtonPosition(Context,Tmp_win)*Tmp_win->title_height+1;
    }
  if((Tmp_win)&&(menu_on==1)&&(Context&C_RALL))
    {
      y = Tmp_win->frame_y+Tmp_win->boundary_width+Tmp_win->title_height;
      x = Tmp_win->frame_x +Tmp_win->frame_width - Tmp_win->boundary_width-
	ButtonPosition(Context,Tmp_win)*Tmp_win->title_height - menu->width+1;
    }
  if((Tmp_win)&&(menu_on==1)&&(Context&C_TITLE))
    {
      y = Tmp_win->frame_y+Tmp_win->boundary_width+Tmp_win->title_height;
      if(x < Tmp_win->frame_x + Tmp_win->title_x)
	x = Tmp_win->frame_x + Tmp_win->title_x;
      if((x + menu->width) >
	 (Tmp_win->frame_x + Tmp_win->title_x +Tmp_win->title_width))	
	x = Tmp_win->frame_x + Tmp_win->title_x +Tmp_win->title_width-
	  menu->width +1;
    }

  if (menu->has_coords)
    {
    /* we already have coordinates explicitly given */
    if (menu->x >= 0)
      x = menu->x;
    else
      x = Scr.MyDisplayWidth - menu->width + menu->x;

    if (menu->y >= 0)
      y = menu->y; /* - menu->height; */
    else
      y = Scr.MyDisplayHeight - menu->height + menu->y;

    Stashed_X = x;
    Stashed_Y = y;
    }

  if (PrevActiveMenuX != -1)
    if (x + menu->width > Scr.MyDisplayWidth - 2) {
       x = PrevActiveMenuX - menu->width + 2;
    }

  /* clip to screen */
  if (x + menu->width > Scr.MyDisplayWidth-2) 
    x = Scr.MyDisplayWidth - menu->width-2;
  if (x < 0) x = 0;

  if (y + menu->height > Scr.MyDisplayHeight-2) 
    {
      y = Scr.MyDisplayHeight - menu->height-2;
      /* Warp pointer to middle of top line */
      /* No with MWM menus! at least only in this case */
      if (menu_on < 1) /* was 2 */
	XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, 
		     Scr.MyDisplayHeight, 
		     x + (menu->width>>1), (y + (Scr.EntryHeight >> 1)));
    }
  if (y < 0) y = 0;

  PrevActiveMenuX = x;

  MenuY = y;
  XMoveWindow(dpy, menu->w, x, y);
  XMapRaised(dpy, menu->w);
  menu->in_use = True;
  return True;
}


/***********************************************************************
 *
 *  Procedure:
 *	PopDownMenu - unhighlight the current menu selection and
 *		take down the menus
 *
 ***********************************************************************/
void PopDownMenu()
{
  if (ActiveMenu == NULL)
    return;
  
  menu_on--;
  
  if (menu_on == 0)
    PrevActiveMenuX = -1;
  if (ActiveItem)
    ActiveItem->state = 0;
  
  XUnmapWindow(dpy, ActiveMenu->w);

  UninstallRootColormap();
  if(!menu_on)
  {
    UngrabEm();
    WaitForButtonsUp();
  }
  XFlush(dpy);
  XSync(dpy,0);
  if (Context & (C_WINDOW | C_FRAME | C_TITLE | C_SIDEBAR))
    menuFromFrameOrWindowOrTitlebar = TRUE;
  else
    menuFromFrameOrWindowOrTitlebar = FALSE;
  ActiveMenu->in_use = FALSE;
}

/***************************************************************************
 * 
 * Wait for all mouse buttons to be released 
 * This can ease some confusion on the part of the user sometimes 
 * 
 * Discard superflous button events during this wait period.
 *
 ***************************************************************************/
void WaitForButtonsUp()
{
  Bool AllUp = False;
  XEvent JunkEvent;
  unsigned int mask;

  while(!AllUp)
    {
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
      XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &JunkX, &JunkY, &JunkX, &JunkY, &mask);    
      
      if((mask&
	  (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask))==0)
	AllUp = True;
    }
  XSync(dpy,0);
  while(XCheckMaskEvent(dpy,
			ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,
			&JunkEvent))
    {
      StashEventTime (&JunkEvent);
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
    }

}


void DestroyMenu(MenuRoot *mr)
{
  MenuItem *mi,*tmp2;
  MenuRoot *tmp, *prev;

  if(mr == NULL)
    return;

  tmp = Scr.AllMenus;
  prev = NULL;
  while((tmp != NULL)&&(tmp != mr))
    {
      prev = tmp;
      tmp = tmp->next;
    }
  if(tmp != mr)
    return;

  if(prev == NULL)
    Scr.AllMenus = mr->next;
  else
    prev->next = mr->next;

  XDestroyWindow(dpy,mr->w);
  XDeleteContext(dpy, mr->w, MenuContext);  
  
  if(mr->sidePic)
    DestroyPicture(dpy, mr->sidePic);

  /* need to free the window list ? */
  mi = mr->first;
  while(mi != NULL)
    {
      tmp2 = mi->next;
      if (mi->item != NULL) free(mi->item);
      if (mi->item2 != NULL) free(mi->item2);
      if (mi->action != NULL) free(mi->action);
      if(mi->picture)
	DestroyPicture(dpy,mi->picture);
      if(mi->lpicture)
	DestroyPicture(dpy,mi->lpicture);
      free(mi);
      mi = tmp2;
    }
  free(mr);
}

/****************************************************************************
 * 
 * Generates the windows for all menus
 *
 ****************************************************************************/ 
void MakeMenus(void)
{
  MenuRoot *mr;

  mr = Scr.AllMenus;
  while(mr != NULL)
    {
      MakeMenu(mr);
      mr = mr->next;
    }
}

/****************************************************************************
 * 
 * Generates the window for a menu
 *
 ****************************************************************************/ 
void MakeMenu(MenuRoot *mr)
{
  MenuItem *cur;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  int y,width;

  if((mr->func != F_POPUP)||(!Scr.flags & WindowsCaptured))
    return;

  /* allow two pixels for top border */
  mr->width = 0;
  mr->width2 = 0;
  mr->width0 = 0;
  for (cur = mr->first; cur != NULL; cur = cur->next)
    {
      width = XTextWidth(Scr.StdFont.font, cur->item, cur->strlen);
      if(cur->picture && width < cur->picture->width)
	width = cur->picture->width;
      if(cur->func_type == F_POPUP)
	width += 15;
      if (width <= 0)
	width = 1;
      if (width > mr->width)
	mr->width = width;

      width = XTextWidth(Scr.StdFont.font, cur->item2, cur->strlen2);
      if (width < 0)
	width = 0;
      if (width > mr->width2)
	mr->width2 = width;
      if((width==0)&&(cur->strlen2>0))
	mr->width2 = 1;

      if(cur->lpicture)
	if(mr->width0 < (cur->lpicture->width+3))
	  mr->width0 = cur->lpicture->width+3;
    }

  /* lets first size the window accordingly */
  mr->width += 10;
  if(mr->width2 > 0)
    mr->width += 5;

  mr->width2 += 15; /*22*/;
  mr->width0 += 7;

  for (y=2, cur = mr->first; cur != NULL; cur = cur->next)
    {
      cur->y_offset = y;
      cur->x = 5+mr->width0;
      if(cur->func_type==F_TITLE)
	{
	  /* Title */
	  if(cur->strlen2  == 0)
	    cur->x = (mr->width+mr->width2+mr->width0
		      - XTextWidth(Scr.StdFont.font, cur->item,
					     cur->strlen))>>1;
	  
	  if((cur->strlen > 0)||(cur->strlen2 > 0))
            cur->y_height = Scr.EntryHeight + HEIGHT_EXTRA_TITLE;
	  else
	    cur->y_height = HEIGHT_SEPARATOR;
	}
      else if((cur->strlen==0)&&(cur->strlen2 == 0))
	/* Separator */
	cur->y_height = HEIGHT_SEPARATOR;
      else
	/* Normal text entry */
	cur->y_height = Scr.EntryHeight;
      if(cur->picture)
	cur->y_height += cur->picture->height;
      if(cur->lpicture && cur->y_height < cur->lpicture->height+4)
	cur->y_height = cur->lpicture->height+4;
      y += cur->y_height;
      if(mr->width2 == 0)
	{
	  cur->x2 = cur->x;
	}
      else
	{
	  cur->x2 = mr->width -5 + mr->width0;
	}
    }
  mr->in_use = 0;
  mr->height = y+3;  /* for Win95 look, was y+2 */

  
#ifndef NO_SAVEUNDERS   
  valuemask = (CWBackPixel | CWEventMask | CWCursor | CWSaveUnder);
#else
  valuemask = (CWBackPixel | CWEventMask | CWCursor);
#endif
  attributes.background_pixel = Scr.MenuColors.back;
  attributes.event_mask = (ExposureMask | EnterWindowMask);
  attributes.cursor = Scr.FvwmCursors[MENU];
#ifndef NO_SAVEUNDERS   
  attributes.save_under = TRUE;
#endif
  if(mr->w != None)
    XDestroyWindow(dpy,mr->w);


  /* Side picture support: this makes the window wider for the picture
     and calculates offset */
  mr->xoffset = 0;
  if(mr->sidePic) {
    if(mr->sidePic->height < mr->height) {
      mr->xoffset = mr->sidePic->width + 3;
    } else {
      DestroyPicture(dpy, mr->sidePic);
      mr->sidePic = NULL;
    }
  }

  mr->width = mr->width0 + mr->width + mr->width2 + mr->xoffset;

  mr->w = XCreateWindow (dpy, Scr.Root, 0, 0, (unsigned int) (mr->width),
			 (unsigned int) mr->height, (unsigned int) 0,
			 CopyFromParent, (unsigned int) InputOutput,
			 (Visual *) CopyFromParent,
			 valuemask, &attributes);
  XSaveContext(dpy,mr->w,MenuContext,(caddr_t)mr);
  
  return;
}

/***********************************************************************
 * Procedure:
 *	scanForHotkeys - Look for hotkey markers in a MenuItem
 * 							(pete@tecc.co.uk)
 * 
 * Inputs:
 *	it	- MenuItem to scan
 * 	which 	- +1 to look in it->item1 and -1 to look in it->item2.
 *
 ***********************************************************************/
void scanForHotkeys(MenuItem *it, int which) 
{
  char *start, *txt;

  start = (which > 0) ? it->item : it->item2;	/* Get start of string	*/
  for (txt = start; *txt != '\0'; txt++) 
    {
      /* Scan whole string	*/
      if (*txt == '&') 
	{		/* A hotkey marker?			*/
	  if (txt[1] == '&') 
	    {	/* Just an escaped &			*/
	      char *tmp;		/* Copy the string down over it	*/
	      for (tmp = txt; *tmp != '\0'; tmp++) tmp[0] = tmp[1];
	      continue;		/* ...And skip to the key char		*/
	    }
	  /* It's a hot key marker - work out the offset value		*/
	  it->hotkey = (1 + (txt - start)) * which;
	  for (; *txt != '\0'; txt++) txt[0] = txt[1];	/* Copy down..	*/
	  return;			/* Only one hotkey per item...	*/
	}
    }
  it->hotkey = 0;		/* No hotkey found.  Set offset to zero	*/
}


void scanForPixmap(char *instring, Picture **p, char identifier) 
{
  char *tstart, *txt, *save_instring;
  int i;
  Picture *pp;
  char name[100];
  extern char *IconPath;
#ifdef XPM
  extern char *PixmapPath;
#endif

  /* save instring in case can't find pixmap */
  save_instring = (char *)safemalloc(strlen(instring)+1);
  strcpy(save_instring,instring);

  /* Scan whole string	*/
  for (txt = instring; *txt != '\0'; txt++) 
    {
      /* A hotkey marker? */
      if (*txt == identifier) 
	{
	  /* Just an escaped &	*/
	  if (txt[1] == identifier) 
	    {	
	      char *tmp;		/* Copy the string down over it	*/
	      for (tmp = txt; *tmp != '\0'; tmp++) tmp[0] = tmp[1];
	      continue;		/* ...And skip to the key char		*/
	    }
	  /* It's a hot key marker - work out the offset value		*/
	  tstart = txt;
	  txt++;
	  i=0;
	  while((*txt != identifier)&&(*txt != '\0')&&(i<99))
	    {
	      name[i] = *txt;
	      txt++;
	      i++;
	    }
	  name[i] = 0;

	  /* Next, check for a color pixmap */
#ifdef XPM
	  pp=CachePicture(dpy,Scr.Root,IconPath,PixmapPath,name);
#else
	  pp=CachePicture(dpy,Scr.Root,IconPath,IconPath,name);
#endif
	  if (*txt != '\0') txt++;
	  while(*txt != '\0')
	    {
	      *tstart++ = *txt++;
	    }
	  *tstart = 0;
          if(!pp)
            strcpy(instring,save_instring);
	  else
	    *p=pp;
          free(save_instring);
	  return;
	}
    }
}

/* Side picture support: this scans for a color int the menu name 
   for colorization */
void scanForColor(char *instring, Pixel *p, Bool *c, char identifier) 
{
  char *tstart, *txt, *save_instring;
  int i;
  char name[100];

  *c = False;

  /* save instring in case can't find pixmap */
  save_instring = (char *)safemalloc(strlen(instring)+1);
  strcpy(save_instring,instring);

  /* Scan whole string        */
  for (txt = instring; *txt != '\0'; txt++) 
    {
      /* A hotkey marker? */
      if (*txt == identifier) 
      {
        /* Just an escaped '^'  */
        if (txt[1] == identifier) 
          {   
            char *tmp;                /* Copy the string down over it */
            for (tmp = txt; *tmp != '\0'; tmp++) tmp[0] = tmp[1];
            continue;         /* ...And skip to the key char          */
          }
        /* It's a hot key marker - work out the offset value          */
        tstart = txt;
        txt++;
        i=0;
        while((*txt != identifier)&&(*txt != '\0')&&(i<99))
          {
            name[i] = *txt;
            txt++;
            i++;
          }
        name[i] = 0;

        *p = GetColor(name);
        *c = True;
        
        if(*txt != '\0')txt++;
        while(*txt != '\0')
          {
            *tstart++ = *txt++;
          }
        *tstart = 0;
          free(save_instring);
        return;
      }
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	AddToMenu - add an item to a root menu
 *
 *  Returned Value:
 *	(MenuItem *)
 *
 *  Inputs:
 *	menu	- pointer to the root menu to add the item
 *	item	- the text to appear in the menu
 *	action	- the string to possibly execute
 *	func	- the numeric function
 *
 * ckh - need to add boolean to say whether or not to expand for pixmaps,
 *       so built in window list can handle windows w/ * and % in title.
 *
 ***********************************************************************/
void AddToMenu(MenuRoot *menu, char *item, char *action)
{
  MenuItem *tmp;
  char *start,*end;

  if(item == NULL)
     return;

  tmp = (MenuItem *)safemalloc(sizeof(MenuItem));
  if (menu->first == NULL)
    {
      menu->first = tmp;
      tmp->prev = NULL;
    }
  else
    {
      menu->last->next = tmp;
      tmp->prev = menu->last;
    }
  menu->last = tmp;
  tmp->picture=NULL;
  tmp->lpicture=NULL;

  /* skip leading spaces */
  /*while(isspace(*item)&&(item != NULL))
    item++;*/
  /* up to first tab goes in "item" field */
  start = item;
  end=item;
  while((*end != '\t')&&(*end != 0))
    end++;
  tmp->item = safemalloc(end-start+1);
  strncpy(tmp->item,start,end-start);
  tmp->item[end-start] = 0;
  tmp->item2 = NULL;
  if(*end=='\t')
    {
      start = end+1;
      while(*end != 0)
	end++;  
      if(end-start != 0)
	{
	  tmp->item2 = safemalloc(end-start+1);
	  strncpy(tmp->item2,start,end-start);
	  tmp->item2[end-start] = 0;
	}
    }

  if (item != (char *)0)
    {
      scanForPixmap(tmp->item,&tmp->picture,'*');
      scanForPixmap(tmp->item,&tmp->lpicture,'%');
      scanForHotkeys(tmp, 1);				/* pete@tecc.co.uk */
      tmp->strlen = strlen(tmp->item);
    }
  else
    tmp->strlen = 0;

  if (tmp->item2 != (char *)0)
    {
      if(!tmp->picture)
        scanForPixmap(tmp->item2,&tmp->picture,'*');
      if(!tmp->lpicture)
        scanForPixmap(tmp->item2,&tmp->lpicture,'%');
      if (tmp->hotkey == 0)
	scanForHotkeys(tmp, -1);	/* pete@tecc.co.uk */
      tmp->strlen2 = strlen(tmp->item2);
    }
  else
    tmp->strlen2 = 0;

  tmp->action = stripcpy(action);
  tmp->next = NULL;
  tmp->state = 0;
  tmp->func_type = find_func_type(tmp->action);
  tmp->item_num = menu->items++;
}

/***********************************************************************
 *
 *  Procedure:
 *	NewMenuRoot - create a new menu root
 *
 *  Returned Value:
 *	(MenuRoot *)
 *
 *  Inputs:
 *	name	- the name of the menu root
 *
 ***********************************************************************/
MenuRoot *NewMenuRoot(char *name, int function_or_popup)
{
  MenuRoot *tmp;
  
  tmp = (MenuRoot *) safemalloc(sizeof(MenuRoot));

  tmp->func = function_or_popup;
  tmp->name = stripcpy(name);

  /* Side picture support */
  tmp->sidePic = NULL;
  scanForPixmap(tmp->name, &tmp->sidePic, '@');
  scanForColor(tmp->name, &tmp->sideColor, &tmp->colorize, '^');

  tmp->first = NULL;
  tmp->last = NULL;
  tmp->items = 0;
  tmp->width = 0;
  tmp->width2 = 0;
  tmp->w = None;
  tmp->next  = Scr.AllMenus;
  Scr.AllMenus = tmp;

  return (tmp);
}



/***********************************************************************
 * change by KitS@bartley.demon.co.uk to correct popups off title buttons
 *
 *  Procedure:
 *ButtonPosition - find the actual position of the button
 *                 since some buttons may be disabled
 *
 *  Returned Value:
 *The button count from left or right taking in to account
 *that some buttons may not be enabled for this window
 *
 *  Inputs:
 *      context - context as per the global Context
 *      t       - the window (FvwmWindow) to test against
 *
 ***********************************************************************/
int ButtonPosition(int context, FvwmWindow * t)
{
  int i;
  int buttons = -1;
  
  if (context&C_RALL)
    {
      for(i=0;i<Scr.nr_right_buttons;i++)
	{
	  if(t->right_w[i])
	    {
	      buttons++;
	    }
	  /* is this the button ? */
	  if (((1<<i)*C_R1) & context)
	    return(buttons);
	}
    }
  else
    {
      for(i=0;i<Scr.nr_left_buttons;i++)
	{
	  if(t->left_w[i])
	    {
	      buttons++;
	    }
	  /* is this the button ? */
	  if (((1<<i)*C_L1) & context)
	    return(buttons);
	}
    }
  /* you never know... */
  return 0;
}

