/****************************************************************************
 * This module is all original code 
 * by Rob Nation 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/


/***********************************************************************
 *
 * fvwm window border drawing code
 *
 ***********************************************************************/

#include <FVWMconfig.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

void DrawLinePattern(Window win,
                     GC ReliefGC,
                     GC ShadowGC,
                     int num_coords,
                     int *x_coord, 
                     int *y_coord,
                     int *line_style,
                     int th);

/* macro to change window background color/pixmap */
#define ChangeWindowColor(window,color) {\
        if (NewColor)\
           {\
             if (Scr.d_depth >= 2) attributes.background_pixel = color;\
             XChangeWindowAttributes(dpy, window, valuemask, &attributes);\
             XClearWindow(dpy,window);\
           }\
         }

extern Window PressedW;
XGCValues Globalgcv;
unsigned long Globalgcm;
/****************************************************************************
 *
 * Redraws the windows borders
 *
 ****************************************************************************/
void SetBorder (FvwmWindow *t, Bool onoroff,Bool force,Bool Mapped, 
		Window expose_win)
  {
  int    i, x, y;
  Pixel  BackColor, ForeColor, TitleBackColor, TitleTextColor;
  Pixmap BackPixmap, TextColor;
  GC     ReliefGC, ShadowGC;
  Bool   NewColor = False;
  XSetWindowAttributes attributes;
  unsigned long valuemask;
  static unsigned int corners[4];
  Window w;

  corners[0] = TOP_HILITE | LEFT_HILITE;
  corners[1] = TOP_HILITE | RIGHT_HILITE;
  corners[2] = BOTTOM_HILITE | LEFT_HILITE;
  corners[3] = BOTTOM_HILITE | RIGHT_HILITE;
  
  if (!t) return;
  
  if (onoroff) 
    {
    /* on */

    /* don't re-draw just for kicks */
    if ((!force) && (Scr.Hilite == t)) return;

    if (Scr.Hilite != t) NewColor = True;
      
    /* make sure that the previously highlighted window got unhighlighted */
    if ((Scr.Hilite != t) && (Scr.Hilite != NULL))
      SetBorder(Scr.Hilite, False, False, True, None);

    /* set the keyboard focus */
    if ((Mapped) && (t->flags&MAPPED) && (Scr.Hilite != t))
      w = t->w;
    else if ((t->flags & ICONIFIED) &&
            (Scr.Hilite != t) && (!(t->flags & SUPPRESSICON)))
      w = t->icon_w;
    Scr.Hilite = t;

    BackPixmap = Scr.gray_pixmap;
    TitleBackColor = Scr.ActiveTitleColors.back;
    TitleTextColor = Scr.ActiveTitleColors.fore;
    }
  else
    {
    /* off */

    /* don't re-draw just for kicks */
    if ((!force) && (Scr.Hilite != t)) return;
      
    if(Scr.Hilite == t)
      {
      Scr.Hilite = NULL;
      NewColor = True;
      }
      
    if (t->flags & STICKY)
      {
      BackPixmap = Scr.sticky_gray_pixmap;
      TitleBackColor = Scr.StickyColors.back;
      TitleTextColor = Scr.StickyColors.fore;
      }
    else
      {
      BackPixmap = Scr.light_gray_pixmap;
      TitleBackColor = t->BackPixel;
      TitleTextColor = t->TextPixel;
      }
    }

  ReliefGC = Scr.WinReliefGC;
  ShadowGC = Scr.WinShadowGC;
  BackColor = Scr.WinColors.back;
  ForeColor = Scr.WinColors.fore;

  if (t->flags & ICONIFIED)
    {
    DrawIconWindow(t);
    return;
    }

  valuemask = CWBorderPixel;
  attributes.border_pixel = Scr.WinColors.shadow;
  if (Scr.d_depth < 2)
    {
    attributes.background_pixmap = BackPixmap;
    valuemask |= CWBackPixmap;
    }
  else
    {
    attributes.background_pixel = BackColor;
    valuemask |= CWBackPixel;
    }
  
  if(t->flags & (TITLE | BORDER))
    {
    XSetWindowBorder(dpy, t->Parent, BackColor);
    XSetWindowBorder(dpy, t->frame, BackColor);
    }

  /* Area for optimization? */

  if(t->flags & TITLE)
    {
    Window *win;

    /* fore color for selected window title bar */
    ChangeWindowColor (t->title_w, TitleBackColor);

    for (i=0, win=t->left_w; i<Scr.nr_left_buttons; i++, win++)
      {
      if (*win != None)
        {
        ChangeWindowColor (*win, TitleBackColor);
        if(flush_expose(*win) || (expose_win == *win) ||
           (expose_win == None))
          {
          if ((i == 0) && (t->title_icon)) /* title icon present? */
            {
            XGCValues gcv;
            unsigned long gcm;

            gcm = GCClipMask|GCClipXOrigin|GCClipYOrigin;
            gcv.clip_mask = t->title_icon->mask;
            gcv.clip_x_origin = 0;
            gcv.clip_y_origin = 0;
            XChangeGC(dpy, Scr.ScratchGC3, gcm, &gcv);
            
            XCopyArea (dpy, t->title_icon->picture, *win,
                       Scr.ScratchGC3, 0, 0,
                       t->title_icon->width, t->title_icon->height,
                       gcv.clip_x_origin, gcv.clip_y_origin);

            gcm = GCClipMask;
            gcv.clip_mask = None;
            XChangeGC(dpy, Scr.ScratchGC3, gcm, &gcv);
            }
          else
            {	
            RelieveButton (t, *win, 0, 0,
                           Scr.button_width, Scr.button_height,
                           ReliefGC, ShadowGC,
                           (PressedW == *win));
            XCopyArea (dpy, Scr.button_pixmap[i<<1], *win,
                       Scr.WinGC, 0, 0,
                       Scr.button_width-3, Scr.button_height-3,
                       (PressedW == *win) ? 2 : 1,
                       (PressedW == *win) ? 2 : 1);
            }

          }
        }
	  
      }
	     
    for (i=0, win=t->right_w; i<Scr.nr_right_buttons; i++, win++)
      {
      if (*win != None)
        {
        ChangeWindowColor(*win, TitleBackColor);
        if (flush_expose(*win) || (expose_win == *win) ||
           (expose_win == None))
          {
          RelieveButton (t, *win, 0, 0,
                         Scr.button_width, Scr.button_height,
                         ReliefGC, ShadowGC,
                         (PressedW == *win));
          if ((i == 1) && (t->flags & MAXIMIZED))
            {
            XCopyArea (dpy, Scr.button_pixmap[(i<<1)], *win,
                       Scr.WinGC, 0, 0,
                       Scr.button_width-3, Scr.button_height-3,
                       (PressedW == *win) ? 2 : 1,
                       (PressedW == *win) ? 2 : 1);
            }
          else
            {
            XCopyArea (dpy, Scr.button_pixmap[(i<<1)+1], *win,
                       Scr.WinGC, 0, 0,
                       Scr.button_width-3, Scr.button_height-3,
                       (PressedW == *win) ? 2 : 1,
                       (PressedW == *win) ? 2 : 1);
            }

          }
        }

      }

    SetTitleBar (t, onoroff, False);
    }

  if(t->flags & BORDER)
    {
    Window *side, *corner;

    /* draw relief lines */
    y = t->frame_height - (t->corner_width << 1);
    x = t->frame_width  - (t->corner_width << 1) + t->bw;

    for (i=0, side=t->sides, corner=t->corners; i<4; i++, side++, corner++)
      {
      ChangeWindowColor(*side, BackColor);
      if((flush_expose(*side)) || (expose_win == *side)||
         (expose_win == None))
        {
        /* index    side
         * 0        TOP
         * 1        RIGHT
         * 2        BOTTOM
         * 3        LEFT
         */
	      
        RelieveWindow(t, *side, 0, 0,
                      ((i & 1) ? t->boundary_width : x),
                      ((i & 1) ? y : t->boundary_width),
                      ReliefGC, ShadowGC, (0x0001 << i));
        }
      ChangeWindowColor(*corner, BackColor);
      if((flush_expose(*corner)) || (expose_win == *corner)||
         (expose_win == None))
        {
        RelieveWindow(t, *corner, 0, 0, t->corner_width,
                      ((i>>1) ? t->corner_width+t->bw : t->corner_width),
                      ReliefGC, ShadowGC, corners[i]);
        }
      }
    }
  else      /* no decorative border */
    {
    /* for mono - put a black border on 
     * for color, make it the color of the decoration background */
    if(t->boundary_width < 2)
      {
      flush_expose (t->frame);
      if(Scr.d_depth < 2)
        {
        XSetWindowBorder (dpy, t->frame, ForeColor);
        XSetWindowBorder (dpy, t->Parent, ForeColor);
        XSetWindowBackgroundPixmap (dpy, t->frame, BackPixmap);
        XClearWindow (dpy, t->frame);
        XSetWindowBackgroundPixmap (dpy, t->Parent, BackPixmap);
        XClearWindow (dpy, t->Parent);
        }
      else
        {
        XSetWindowBackground (dpy, t->frame, BackColor);
        XSetWindowBorder (dpy, t->frame, Scr.WinColors.shadow);
        XClearWindow (dpy, t->frame);
        XSetWindowBackground (dpy, t->Parent, Scr.WinColors.shadow);
        XSetWindowBorder (dpy, t->Parent, Scr.WinColors.shadow);
        XClearWindow (dpy, t->Parent);
        XSetWindowBorder (dpy, t->w, Scr.WinColors.shadow);	      
        }
      }
    else
      {
      XSetWindowBorder (dpy, t->Parent, Scr.WinColors.shadow);
      XSetWindowBorder (dpy, t->frame, Scr.WinColors.shadow);

      ChangeWindowColor (t->frame, BackColor);
      if((flush_expose(t->frame)) || (expose_win == t->frame)||
         (expose_win == None))
        {
        if(t->boundary_width > 2)
          {
          RelieveWindow(t, t->frame, 0, 0, t->frame_width + t->bw,
                        t->frame_height + t->bw,
                        ReliefGC, ShadowGC,
                        TOP_HILITE | LEFT_HILITE | RIGHT_HILITE |
                        BOTTOM_HILITE);
          }
        else
          {
          RelieveWindow(t, t->frame, 0, 0, t->frame_width + t->bw,
                        t->frame_height + t->bw,
                        ReliefGC, ShadowGC,
                        TOP_HILITE | LEFT_HILITE | RIGHT_HILITE |
                        BOTTOM_HILITE);	      
          }
        }
      else
        {
        XSetWindowBackground(dpy, t->Parent, Scr.WinColors.shadow);
        }
      }
    }

  /* Sync to make the border-color change look fast! */
  XSync(dpy, 0);
  }


/****************************************************************************
 *
 *  Redraws just the title bar
 *
 ****************************************************************************/
void SetTitleBar (FvwmWindow *t, Bool onoroff, Bool NewTitle)
  {
  int x, y, w, i;
  Pixel Forecolor, BackColor;

  if (!t) return;
  if (!(t->flags & TITLE)) return;

  if (onoroff) 
    {
    Forecolor = Scr.ActiveTitleColors.fore;
    BackColor = Scr.ActiveTitleColors.back;
    }
  else
    {
    Forecolor = t->TextPixel;
    BackColor = t->BackPixel;
    }

  flush_expose(t->title_w);
  
  if(t->name != (char *)NULL)
    {
    w = XTextWidth(Scr.WindowFont.font,t->name,strlen(t->name));
    if (w > t->title_width-12) w = t->title_width-4;
    if (w < 0) w = 0;
    }
  else
    {
    w = 0;
    }

  x = Scr.button_width*t->nr_left_buttons + 6;
  y = ((Scr.WindowFont.y + t->title_height) >> 1) - 1; 
  
  NewFontAndColor(Scr.WindowFont.font->fid, Forecolor, BackColor);
  
  if (NewTitle) XClearWindow (dpy, t->title_w);

#ifdef I18N
#undef FONTSET
#define FONTSET Scr.WindowFont.fontset
#endif /* I18N */

  /* for mono, we clear an area in the title bar where the window
   * title goes, so that its more legible. For color, no need */
  if(Scr.d_depth<2)
    {
    XFillRectangle(dpy,t->title_w,
                   Scr.BlackGC,
                   x - 2, 0, w+4, t->title_height);
      
    if(t->name != (char *)NULL)
      XDrawString (dpy, t->title_w,Scr.ScratchGC3, x, y,
                   t->name, strlen(t->name));
    }
  else
    { 
    if(t->name != (char *)NULL)
      XDrawString (dpy, t->title_w, Scr.ScratchGC3, x, y,
                   t->name, strlen(t->name));
    }

  XFlush(dpy);
  }


/****************************************************************************
 *
 *  Draws the relief pattern around a window (win95)
 *
 ****************************************************************************/
inline void RelieveWindow(FvwmWindow *t, Window win,
                          int x, int y, int w, int h,
                          GC ReliefGC, GC ShadowGC, int hilite)
  {
  XSegment seg[4];
  int i;
  int edge;

  edge = 0; 
  if ((win == t->sides[0]) || (win == t->sides[1])||
      (win == t->sides[2]) || (win == t->sides[3])) edge = -1;
  else if (win == t->corners[0]) edge = 1;
  else if (win == t->corners[1]) edge = 2;
  else if (win == t->corners[2]) edge = 3;
  else if (win == t->corners[3]) edge = 4;

  /* window sides */
  if (edge == -1)
    {
    switch (hilite)
      {
      case LEFT_HILITE:
            i = 0;
            seg[i].x1 = x+1;      seg[i].y1   = y;
            seg[i].x2 = x+1;      seg[i++].y2 = h+y-1;
            XDrawSegments(dpy, win, ReliefGC, seg, i);
            break;

      case TOP_HILITE:
            i = 0;
            seg[i].x1 = x;        seg[i].y1   = y+1;
            seg[i].x2 = w+x-1;    seg[i++].y2 = y+1;
            XDrawSegments(dpy, win, ReliefGC, seg, i);
            break;

      case RIGHT_HILITE:
            i = 0;
            seg[i].x1 = w+x-1;    seg[i].y1   = y;
            seg[i].x2 = w+x-1;    seg[i++].y2 = h+y-1;
            XDrawSegments(dpy, win, Scr.BlackGC, seg, i);
            i = 0;
            seg[i].x1 = w+x-2;    seg[i].y1   = y;
            seg[i].x2 = w+x-2;    seg[i++].y2 = h+y-1;
            XDrawSegments(dpy, win, ShadowGC, seg, i);
            break;

      case BOTTOM_HILITE:
            i = 0;
            seg[i].x1 = x;        seg[i].y1   = h+y-1;
            seg[i].x2 = w+x-1;    seg[i++].y2 = h+y-1;
            XDrawSegments(dpy, win, Scr.BlackGC, seg, i);
            i = 0;
            seg[i].x1 = x;        seg[i].y1   = h+y-2;
            seg[i].x2 = w+x-1;    seg[i++].y2 = h+y-2;
            XDrawSegments(dpy, win, ShadowGC, seg, i);
            break;

      }
    return;
    }

  /* corners */
  if (edge >= 1 && edge <= 4)
    {
    switch (edge)
      {
      case 1:
            i = 0;
            seg[i].x1 = x+1;      seg[i].y1   = y+1;
            seg[i].x2 = x+1;      seg[i++].y2 = h+y-1;
            seg[i].x1 = x+1;      seg[i].y1   = y+1;
            seg[i].x2 = w+x-1;    seg[i++].y2 = y+1;
            XDrawSegments(dpy, win, ReliefGC, seg, i);
            break;

      case 2:
            i = 0;
            seg[i].x1 = x;        seg[i].y1   = y+1;
            seg[i].x2 = w+x-2;    seg[i++].y2 = y+1;
            XDrawSegments(dpy, win, ReliefGC, seg, i);
            i = 0;
            seg[i].x1 = w+x-1;    seg[i].y1   = y;
            seg[i].x2 = w+x-1;    seg[i++].y2 = h+y-1;
            XDrawSegments(dpy, win, Scr.BlackGC, seg, i);
            i = 0;
            seg[i].x1 = w+x-2;    seg[i].y1   = y+1;
            seg[i].x2 = w+x-2;    seg[i++].y2 = h+y-1;
            XDrawSegments(dpy, win, ShadowGC, seg, i);
            break;

      case 3:
            i = 0;
            seg[i].x1 = x+1;      seg[i].y1   = y;
            seg[i].x2 = x+1;      seg[i++].y2 = h+y-2;
            XDrawSegments(dpy, win, ReliefGC, seg, i);
            i = 0;
            seg[i].x1 = x;        seg[i].y1   = h+y-1;
            seg[i].x2 = w+x-1;    seg[i++].y2 = h+y-1;
            XDrawSegments(dpy, win, Scr.BlackGC, seg, i);
            i = 0;
            seg[i].x1 = x+1;      seg[i].y1   = h+y-2;
            seg[i].x2 = w+x-1;    seg[i++].y2 = h+y-2;
            XDrawSegments(dpy, win, ShadowGC, seg, i);
            break;

      case 4:
            i = 0;
            seg[i].x1 = w+x-1;    seg[i].y1   = y;
            seg[i].x2 = w+x-1;    seg[i++].y2 = h+y-1;
            seg[i].x1 = x;        seg[i].y1   = h+y-1;
            seg[i].x2 = w+x-1;    seg[i++].y2 = h+y-1;
            XDrawSegments(dpy, win, Scr.BlackGC, seg, i);
            i = 0;
            seg[i].x1 = w+x-2;    seg[i].y1   = y;
            seg[i].x2 = w+x-2;    seg[i++].y2 = h+y-2;
            seg[i].x1 = x;        seg[i].y1   = h+y-2;
            seg[i].x2 = w+x-2;    seg[i++].y2 = h+y-2;
            XDrawSegments(dpy, win, ShadowGC, seg, i);
            break;

      }
    return;
    }

  /* any other rectangular window */
  if (edge == 0)
    {
    i = 0;
    seg[i].x1 = x+1;      seg[i].y1   = y+1;
    seg[i].x2 = w+x-2;    seg[i++].y2 = y+1;
    seg[i].x1 = x+1;      seg[i].y1   = y+1;
    seg[i].x2 = x+1;      seg[i++].y2 = h+y-2;
    XDrawSegments(dpy, win, ReliefGC, seg, i);
    i = 0;
    seg[i].x1 = x+1;      seg[i].y1   = h+y-2;
    seg[i].x2 = w+x-2;    seg[i++].y2 = h+y-2;
    seg[i].x1 = w+x-2;    seg[i].y1   = y+1;
    seg[i].x2 = w+x-2;    seg[i++].y2 = h+y-2;
    XDrawSegments(dpy, win, ShadowGC, seg, i);
    i = 0;
    seg[i].x1 = x;        seg[i].y1   = h+y-1;
    seg[i].x2 = w+x-1;    seg[i++].y2 = h+y-1;
    seg[i].x1 = w+x-1;    seg[i].y1   = y;
    seg[i].x2 = w+x-1;    seg[i++].y2 = h+y-1;
    XDrawSegments(dpy, win, Scr.BlackGC, seg, i);

    return;
    }
  }

void RelieveButton (FvwmWindow *t, Window win, int x, int y, int w, int h,
                     GC ReliefGC, GC ShadowGC, int state)
  {
  int i;
  XSegment seg[4];

  if (state) /* 1 = pressed */
    {
    i = 0;
    seg[i].x1 = x;        seg[i].y1   = y;
    seg[i].x2 = w+x-1;    seg[i++].y2 = y;
    seg[i].x1 = x;        seg[i].y1   = y;
    seg[i].x2 = x;        seg[i++].y2 = h+y-1;
    XDrawSegments(dpy, win, Scr.BlackGC, seg, i);
    i = 0;
    seg[i].x1 = x+1;      seg[i].y1   = y+1;
    seg[i].x2 = w+x-2;    seg[i++].y2 = y+1;
    seg[i].x1 = x+1;      seg[i].y1   = y+1;
    seg[i].x2 = x+1;      seg[i++].y2 = h+y-2;
    XDrawSegments(dpy, win, ShadowGC, seg, i);
    i = 0;
    seg[i].x1 = x+1;      seg[i].y1   = h+y-1;
    seg[i].x2 = w+x-1;    seg[i++].y2 = h+y-1;
    seg[i].x1 = w+x-1;    seg[i].y1   = y+1;
    seg[i].x2 = w+x-1;    seg[i++].y2 = h+y-1;
    XDrawSegments(dpy, win, ReliefGC, seg, i);
    }
  else
    {
    i = 0;
    seg[i].x1 = x;        seg[i].y1   = y;
    seg[i].x2 = w+x-2;    seg[i++].y2 = y;
    seg[i].x1 = x;        seg[i].y1   = y;
    seg[i].x2 = x;        seg[i++].y2 = h+y-2;
    XDrawSegments(dpy, win, ReliefGC, seg, i);
    i = 0;
    seg[i].x1 = x+1;      seg[i].y1   = h+y-2;
    seg[i].x2 = w+x-2;    seg[i++].y2 = h+y-2;
    seg[i].x1 = w+x-2;    seg[i].y1   = y+1;
    seg[i].x2 = w+x-2;    seg[i++].y2 = h+y-2;
    XDrawSegments(dpy, win, ShadowGC, seg, i);
    i = 0;
    seg[i].x1 = x;        seg[i].y1   = h+y-1;
    seg[i].x2 = w+x-1;    seg[i++].y2 = h+y-1;
    seg[i].x1 = w+x-1;    seg[i].y1   = y;
    seg[i].x2 = w+x-1;    seg[i++].y2 = h+y-1;
    XDrawSegments(dpy, win, Scr.BlackGC, seg, i);
    }
  }


/***********************************************************************
 *
 *  Procedure:
 *      Setupframe - set window sizes, this was called from either
 *              AddWindow, EndResize, or HandleConfigureNotify.
 *
 *  Inputs:
 *      tmp_win - the FvwmWindow pointer
 *      x       - the x coordinate of the upper-left outer corner of the frame
 *      y       - the y coordinate of the upper-left outer corner of the frame
 *      w       - the width of the frame window w/o border
 *      h       - the height of the frame window w/o border
 *
 *  Special Considerations:
 *      This routine will check to make sure the window is not completely
 *      off the display, if it is, it'll bring some of it back on.
 *
 *      The tmp_win->frame_XXX variables should NOT be updated with the
 *      values of x,y,w,h prior to calling this routine, since the new
 *      values are compared against the old to see whether a synthetic
 *      ConfigureNotify event should be sent.  (It should be sent if the
 *      window was moved but not resized.)
 *
 ************************************************************************/

void SetupFrame(FvwmWindow *tmp_win, int x, int y, int w, int h,
                Bool sendEvent) 
{
  XEvent client_event;
  XWindowChanges frame_wc, xwc;
  unsigned long frame_mask, xwcm;
  int    cx, cy, i;
  Bool   Resized = False;
  int    xwidth, ywidth;
  Window *win;

  /* if windows is not being maximized, save size in case of maximization */
  if (!(tmp_win->flags & MAXIMIZED))
    {
    tmp_win->orig_x  = x;
    tmp_win->orig_y  = y;
    tmp_win->orig_wd = w;
    tmp_win->orig_ht = h;
    }

/* This screws up the Taskbar's AutoHide mode when the southern
   and eastern most pages are selected.  Is this really
   necessary anyway?  Will removing this break anything else? */

/*
  if (x >= Scr.MyDisplayWidth + Scr.VxMax - Scr.Vx - 16)
    x = Scr.MyDisplayWidth + Scr.VxMax - Scr.Vx - 16;

  if (y >= Scr.MyDisplayHeight + Scr.VyMax - Scr.Vy - 16)
    y = Scr.MyDisplayHeight + Scr.VyMax - Scr.Vy - 16;
*/

  /*
   * According to the July 27, 1988 ICCCM draft, we should send a
   * "synthetic" ConfigureNotify event to the client if the window
   * was moved but not resized.
   */
  if ((x != tmp_win->frame_x || y != tmp_win->frame_y) &&
      (w == tmp_win->frame_width && h == tmp_win->frame_height))
    sendEvent = TRUE;

  if((w != tmp_win->frame_width) || (h != tmp_win->frame_height))
    Resized = True;

  xwcm = CWX | CWY | CWWidth | CWHeight;

  if(Resized)
    {
    if (tmp_win->flags & TITLE) 
      tmp_win->title_height = Scr.TitleHeight + tmp_win->bw;

    tmp_win->title_width = w - (tmp_win->boundary_width << 1) + tmp_win->bw;
    if(tmp_win->title_width < 1) tmp_win->title_width = 1;

    if (tmp_win->flags & TITLE) 
      {
      /* setup titlebar window */

      tmp_win->title_x = tmp_win->boundary_width;

      if(tmp_win->title_x >= w - tmp_win->boundary_width)
        tmp_win->title_x = -10;

      tmp_win->title_y = tmp_win->boundary_width;
	  
      xwc.width  = tmp_win->title_width;
      xwc.height = tmp_win->title_height;

      xwc.x = tmp_win->title_x;
      xwc.y = tmp_win->title_y;

      XConfigureWindow(dpy, tmp_win->title_w, xwcm, &xwc);

      /* setup left button windows */

      /* mini-icon first */

      if (tmp_win->title_icon)
        {
        xwc.height = tmp_win->title_icon->height;
        xwc.width = tmp_win->title_icon->width;
        }
      else
        {
        xwc.height = Scr.button_height;
        xwc.width = Scr.button_width;
        }

      xwc.y = tmp_win->boundary_width +
              ((tmp_win->title_height - xwc.height) >> 1) +1;
      xwc.x = tmp_win->boundary_width +3;

      win = tmp_win->left_w;
      if (*win != None)
        {
        if(xwc.x + xwc.width >= w-tmp_win->boundary_width)
          xwc.x = -xwc.width;

        XConfigureWindow(dpy, *win, xwcm, &xwc);
        xwc.x += xwc.width +2;
        }

      win++;

      xwc.height = Scr.button_height;
      xwc.width = Scr.button_width;

      xwc.y = tmp_win->boundary_width +
              ((tmp_win->title_height - xwc.height) >> 1) +1;

      for(i=1; i<Scr.nr_left_buttons; i++, win++)
        {
        if (*win != None)
          {
          if(xwc.x + Scr.button_width >= w-tmp_win->boundary_width)
            xwc.x = -Scr.button_width;

          XConfigureWindow(dpy, *win, xwcm, &xwc);
          xwc.x += Scr.button_width +2;
          }
        }

      /* setup right button windows */

      xwc.x = w-tmp_win->boundary_width + tmp_win->bw -2;

      for(i=0, win=tmp_win->right_w; i<Scr.nr_right_buttons; i++, win++)
        {
        if (*win != None)
          {
          xwc.x -= Scr.button_width;
          if(xwc.x <= tmp_win->boundary_width)
            xwc.x = -Scr.button_width;

          XConfigureWindow(dpy, *win, xwcm, &xwc);
          if (i == 0) xwc.x -= 2;
          }
        }
      }

    if(tmp_win->flags & BORDER)
      {
      /* setup side windows */

      tmp_win->corner_width = Scr.TitleHeight + tmp_win->bw + 
                              tmp_win->boundary_width;

      if (w < (tmp_win->corner_width << 1)) tmp_win->corner_width = w/3;
      if (h < (tmp_win->corner_width << 1)) tmp_win->corner_width = h/3;

      xwidth = w - (tmp_win->corner_width << 1) + tmp_win->bw;
      ywidth = h - (tmp_win->corner_width << 1);

      if (xwidth < 2) xwidth = 2;
      if (ywidth < 2) ywidth = 2;

      for(i=0, win=tmp_win->sides; i<4; i++, win++)
        {
        if (i == 0)
          {
          xwc.x = tmp_win->corner_width;
          xwc.y = 0;
          xwc.height = tmp_win->boundary_width;
          xwc.width  = xwidth;
          }
        else if (i == 1)
          {
          xwc.x = w - tmp_win->boundary_width + tmp_win->bw;	
          xwc.y = tmp_win->corner_width;
          xwc.width  = tmp_win->boundary_width;
          xwc.height = ywidth;
          }
        else if (i == 2)
          {
          xwc.x = tmp_win->corner_width;
          xwc.y = h - tmp_win->boundary_width + tmp_win->bw;
          xwc.height = tmp_win->boundary_width + tmp_win->bw;
          xwc.width  = xwidth;
          }
        else
          {
          xwc.x = 0;
          xwc.y = tmp_win->corner_width;
          xwc.width  = tmp_win->boundary_width;
          xwc.height = ywidth;
          }
        XConfigureWindow(dpy, *win, xwcm, &xwc);
        }

      /* setup corner windows */

      xwc.width  = tmp_win->corner_width;
      xwc.height = tmp_win->corner_width;

      for(i=0, win=tmp_win->corners; i<4; i++, win++)
        {
        if(i & 1)
          xwc.x = w - tmp_win->corner_width + tmp_win->bw;
        else
          xwc.x = 0;
	      
        if(i >> 1)
          xwc.y = h - tmp_win->corner_width;
        else
          xwc.y = 0;

        XConfigureWindow(dpy, *win, xwcm, &xwc);
        }
      }
    }

  tmp_win->attr.width  = w - (tmp_win->boundary_width << 1);
  tmp_win->attr.height = h - tmp_win->title_height 
                           - (tmp_win->boundary_width << 1);

  cx = tmp_win->boundary_width - tmp_win->bw;
  cy = tmp_win->title_height + tmp_win->boundary_width - tmp_win->bw;

  XResizeWindow(dpy, tmp_win->w, tmp_win->attr.width,
		tmp_win->attr.height);
  XMoveResizeWindow(dpy, tmp_win->Parent, cx,cy,
		    tmp_win->attr.width, tmp_win->attr.height);

  /* 
   * fix up frame and assign size/location values in tmp_win
   */
  frame_wc.x = tmp_win->frame_x = x;
  frame_wc.y = tmp_win->frame_y = y;
  frame_wc.width = tmp_win->frame_width = w;
  frame_wc.height = tmp_win->frame_height = h;
  frame_mask = (CWX | CWY | CWWidth | CWHeight);
  XConfigureWindow (dpy, tmp_win->frame, frame_mask, &frame_wc);

#ifdef SHAPE
  if (ShapesSupported)
    if ((Resized) && (tmp_win->wShaped)) SetShape (tmp_win, w);
#endif /* SHAPE */

  XSync (dpy, 0);

  if (sendEvent)
    {
    client_event.type = ConfigureNotify;
    client_event.xconfigure.display = dpy;
    client_event.xconfigure.event   = tmp_win->w;
    client_event.xconfigure.window  = tmp_win->w;
      
    client_event.xconfigure.x = x + tmp_win->boundary_width;
    client_event.xconfigure.y = y + tmp_win->title_height
                                  + tmp_win->boundary_width;
    client_event.xconfigure.width  = w - (tmp_win->boundary_width << 1);
    client_event.xconfigure.height = h - (tmp_win->boundary_width << 1)
                                       - tmp_win->title_height;

    client_event.xconfigure.border_width = tmp_win->bw;

    /* Real ConfigureNotify events say we're above title window, so ... */
    /* what if we don't have a title ????? */

    client_event.xconfigure.above = tmp_win->frame;
    client_event.xconfigure.override_redirect = False;

    XSendEvent(dpy, tmp_win->w, False, StructureNotifyMask, &client_event);
    }

  BroadcastConfig (M_CONFIGURE_WINDOW, tmp_win);
}


/****************************************************************************
 *
 * Sets up the shaped window borders 
 * 
 ****************************************************************************/
void SetShape(FvwmWindow *tmp_win, int w)
{
#ifdef SHAPE
  if (ShapesSupported)
  {
    XRectangle rect;

    XShapeCombineShape (dpy, tmp_win->frame, ShapeBounding,
  		        tmp_win->boundary_width,
		        tmp_win->title_height + tmp_win->boundary_width,
		        tmp_win->w,
		        ShapeBounding, ShapeSet);
    if (tmp_win->title_w) 
      {
      /* windows w/ titles */
      rect.x = tmp_win->boundary_width;
      rect.y = tmp_win->title_y;
      rect.width  = w - (tmp_win->boundary_width << 1) + tmp_win->bw;
      rect.height = tmp_win->title_height;
      
      
      XShapeCombineRectangles(dpy, tmp_win->frame, ShapeBounding,
                              0, 0, &rect, 1, ShapeUnion, Unsorted);
      }
  }
#endif
}
