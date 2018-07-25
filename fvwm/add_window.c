/****************************************************************************
 * This module is based on Twm, but has been siginificantly modified 
 * by Rob Nation
 ****************************************************************************/
/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/


/**********************************************************************
 *
 * Add a new window, put the titlbar and other stuff around
 * the window
 *
 **********************************************************************/
#include <FVWMconfig.h>
#include <fvwm/fvwmlib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fvwm.h"
#include <X11/Xatom.h>
#include "misc.h"
#include "screen.h"
#ifdef SHAPE
#include <X11/extensions/shape.h>
#include <X11/Xresource.h>
#endif /* SHAPE */
#include "module.h"

/* Used to parse command line of clients for specific desk requests. */
/* Todo: check for multiple desks. */
static XrmDatabase db;
static XrmOptionDescRec table [] = {
  /* Want to accept "-workspace N" or -xrm "fvwm*desk:N" as options
   * to specify the desktop. I have to include dummy options that
   * are meaningless since Xrm seems to allow -w to match -workspace
   * if there would be no ambiguity. */
    {"-workspacf",      "*junk",        XrmoptionSepArg, (caddr_t) NULL}, 
    {"-workspace",	"*desk",	XrmoptionSepArg, (caddr_t) NULL},
    {"-xrn",		NULL,		XrmoptionResArg, (caddr_t) NULL},
    {"-xrm",		NULL,		XrmoptionResArg, (caddr_t) NULL},
};

extern char *IconPath, *PixmapPath;

/***********************************************************************
 *
 *  Procedure:
 *	AddWindow - add a new window to the fvwm list
 *
 *  Returned Value:
 *	(FvwmWindow *) - pointer to the FvwmWindow structure
 *
 *  Inputs:
 *	w	- the window id of the window to add
 *	iconm	- flag to tell if this is an icon manager window
 *
 ***********************************************************************/
FvwmWindow *AddWindow(Window w)
{
  FvwmWindow *tmp_win;		        /* new fvwm window structure */
  unsigned long valuemask;		/* mask for create windows */
  XSetWindowAttributes attributes;	/* attributes for create windows */
  int i, a, b, width, height;
  char *value, *tvalue;
  unsigned long tflag;
  int Desk, border_width, resize_width;
  extern Bool NeedToResizeToo;
  extern FvwmWindow *colormap_win;
  char *forecolor = NULL, *backcolor = NULL;
  int client_argc;
  char **client_argv = NULL, *str_type;
  Bool status;
  XrmValue rm_value;
  XTextProperty text_prop;
  extern Bool PPosOverride;
#ifdef I18N
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytesafter;
  char **list;
  int num;
#endif

  NeedToResizeToo = False;
  /* allocate space for the fvwm window */
  tmp_win = (FvwmWindow *)calloc(1, sizeof(FvwmWindow));

  if (tmp_win == (FvwmWindow *)0) return NULL;

  tmp_win->flags = 0;
  tmp_win->w = w;

  tmp_win->cmap_windows = (Window *)NULL;

  tmp_win->title_pixmap_file = NULL;
  tmp_win->title_icon = NULL;

  if(!PPosOverride)
    if (XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
		     &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
      {
	free((char *)tmp_win);
	return(NULL);
      }

  if ( XGetWMName(dpy, tmp_win->w, &text_prop) != 0 ) {
#ifdef I18N
    if (text_prop.value) text_prop.nitems = strlen(text_prop.value);
    if (XmbTextPropertyToTextList(dpy, &text_prop, &list, &num) >= Success
	&& num > 0 && *list)
#ifndef EVIL
      {
	tmp_win->name = *list;
	tmp_win->realname = *list;
      }
    else
      {
	tmp_win->name = NoName;
	tmp_win->realname = NoName;
      }
#else /* EVIL */
      {
	tmp_win->name = *list;
	tmp_win->realname = *list;
	if((tmp_win->name != NULL) && (strcmp(tmp_win->name, "")==0)) {
	  XGetWMName(dpy, tmp_win->w, &text_prop);
	  tmp_win->name = (char *)text_prop.value ;
	  tmp_win->realname = (char *)text_prop.value ;
	}
      }
    else
      {
	XGetWMName(dpy, tmp_win->w, &text_prop);
	tmp_win->name = (char *)text_prop.value ;
	tmp_win->realname = (char *)text_prop.value ;
      }
#endif /* EVIL */
#else /* I18N */
    tmp_win->name = (char *)text_prop.value;
    tmp_win->realname = (char *)text_prop.value;
#endif /* I18N */
  } else {
    tmp_win->name = NoName;
    tmp_win->realname = NoName;
  }

#ifdef I18N
  if (XGetWindowProperty(dpy, tmp_win->w, XA_WM_ICON_NAME, 0L, 200L, False,
			AnyPropertyType, &actual_type, &actual_format, &nitems,
			&bytesafter,(unsigned char **)&tmp_win->icon_name)
      == Success && actual_type != None) {
    text_prop.value = tmp_win->icon_name;
    text_prop.encoding = actual_type;
    text_prop.format = actual_format;
    text_prop.nitems = nitems;
    if (XmbTextPropertyToTextList(dpy, &text_prop, &list, &num) >= Success
	&& num > 0 && *list)
#ifndef EVIL
      tmp_win->icon_name = *list;
    else
      tmp_win->icon_name = NULL;
#else /* EVIL */
      {
	tmp_win->icon_name = *list;
	if((tmp_win->icon_name != NULL) && (strcmp(tmp_win->icon_name, "")==0)) {
	  XGetWMIconName (dpy, tmp_win->w, &text_prop);
	  tmp_win->icon_name = (char *) text_prop.value;
      }
    }
   else
    {
       XGetWMIconName (dpy, tmp_win->w, &text_prop);
       tmp_win->icon_name = (char *)text_prop.value ;
    }
#endif /* EVIL */
  }
  else
    tmp_win->icon_name = NULL;
#else /* I18N */
  XGetWMIconName (dpy, tmp_win->w, &text_prop);
  tmp_win->icon_name = (char *) text_prop.value;
#endif /* I18N */
  if (tmp_win->icon_name != (char *)NULL) 
      tmp_win->realname = tmp_win->icon_name;

  tmp_win->class.res_name = NULL;
  tmp_win->class.res_class = NULL;
  XGetClassHint(dpy, tmp_win->w, &tmp_win->class);

  if ( tmp_win->class.res_name != NULL) {
    tmp_win->realname = tmp_win->class.res_name;
    if ( tmp_win->icon_name == NULL ) 
      tmp_win->icon_name = tmp_win->class.res_name;
  } else {
    if ( tmp_win->class.res_class != NULL) {
      tmp_win->realname = tmp_win->class.res_class;
      if ( tmp_win->icon_name == NULL ) 
         tmp_win->icon_name = tmp_win->class.res_class;
    }
  }

  FetchWmProtocols (tmp_win);
  FetchWmColormapWindows (tmp_win);
  if(!(XGetWindowAttributes(dpy,tmp_win->w,&(tmp_win->attr))))
    tmp_win->attr.colormap = Scr.FvwmRoot.attr.colormap;

  tmp_win->wmhints = XGetWMHints(dpy, tmp_win->w);

  if(XGetTransientForHint(dpy, tmp_win->w,  &tmp_win->transientfor))
    tmp_win->flags |= TRANSIENT;
  else
    tmp_win->flags &= ~TRANSIENT;

  tmp_win->old_bw = tmp_win->attr.border_width;

#ifdef SHAPE
  if (ShapesSupported)
  {
    int xws, yws, xbs, ybs;
    unsigned wws, hws, wbs, hbs;
    int boundingShaped, clipShaped;
    
    XShapeSelectInput (dpy, tmp_win->w, ShapeNotifyMask);
    XShapeQueryExtents (dpy, tmp_win->w,
			&boundingShaped, &xws, &yws, &wws, &hws,
			&clipShaped, &xbs, &ybs, &wbs, &hbs);
    tmp_win->wShaped = boundingShaped;
  }
#endif /* SHAPE */


  /* if the window is in the NoTitle list, or is a transient,
   *  dont decorate it.
   * If its a transient, and DecorateTransients was specified,
   *  decorate anyway
   */
  /*  Assume that we'll decorate */
  tmp_win->flags |= BORDER;
  tmp_win->flags |= TITLE;
  tmp_win->title_height = Scr.TitleHeight + tmp_win->bw;

  tflag = LookInList(Scr.TheList,tmp_win->realname,&tmp_win->class,
                     &value, &tvalue, &Desk,
		     &border_width, &resize_width,
                     &forecolor,&backcolor,&tmp_win->buttons, 
		     tmp_win->IconBox,&(tmp_win->BoxFillMethod));

  tmp_win->title_pixmap_file = tvalue;

  GetMwmHints(tmp_win);
  GetOlHints(tmp_win);

  FetchWmProtocols(tmp_win);
  SelectDecor(tmp_win, tflag, border_width, resize_width); 

  tmp_win->flags |= tflag & ALL_COMMON_FLAGS;
  /* find a suitable icon pixmap */
  if(tflag & ICON_FLAG)
    {
      /* an icon was specified */
      tmp_win->icon_bitmap_file = value;
    }
  else if((tmp_win->wmhints)
	  &&(tmp_win->wmhints->flags & (IconWindowHint|IconPixmapHint)))
    {
      /* window has its own icon */
      tmp_win->icon_bitmap_file = NULL;
    }
  else
    {
      /* use default icon */
      tmp_win->icon_bitmap_file = Scr.DefaultIcon;
    }

  GetWindowSizeHints (tmp_win);

  /* Tentative size estimate */
  tmp_win->frame_width = tmp_win->attr.width+2*tmp_win->boundary_width;
  tmp_win->frame_height = tmp_win->attr.height + tmp_win->title_height+
    2*tmp_win->boundary_width;

  ConstrainSize(tmp_win, &tmp_win->frame_width, &tmp_win->frame_height);

  /* Find out if the client requested a specific desk on the command line. */
  if (XGetCommand (dpy, tmp_win->w, &client_argv, &client_argc)) {
      XrmParseCommand (&db, table, 4, "fvwm", &client_argc, client_argv);
      status = XrmGetResource (db, "fvwm.desk", "Fvwm.Desk", &str_type, &rm_value);
      if ((status == True) && (rm_value.size != 0)) {
          Desk = atoi(rm_value.addr);
	  tflag |= STARTSONDESK_FLAG;
      }
      XrmDestroyDatabase (db);
      db = NULL;
  }

  if(!PlaceWindow(tmp_win, tflag, Desk))
    return NULL;

  /*
   * Make sure the client window still exists.  We don't want to leave an
   * orphan frame window if it doesn't.  Since we now have the server
   * grabbed, the window can't disappear later without having been
   * reparented, so we'll get a DestroyNotify for it.  We won't have
   * gotten one for anything up to here, however.
   */
  XGrabServer(dpy);
  if(XGetGeometry(dpy, w, &JunkRoot, &JunkX, &JunkY,
		  &JunkWidth, &JunkHeight,
		  &JunkBW,  &JunkDepth) == 0)
    {
      free((char *)tmp_win);
      XUngrabServer(dpy);
      return(NULL);
    }

  XSetWindowBorderWidth (dpy, tmp_win->w,0);
/*
  XGetWMIconName (dpy, tmp_win->w, &text_prop);
  tmp_win->icon_name = (char *) text_prop.value;
  if(tmp_win->icon_name==(char *)NULL)
    tmp_win->icon_name = tmp_win->name;
*/

  tmp_win->flags &= ~ICONIFIED;
  tmp_win->flags &= ~ICON_UNMAPPED;
  tmp_win->flags &= ~MAXIMIZED;

  tmp_win->TextPixel = Scr.TitleColors.fore;
  tmp_win->BackPixel = Scr.TitleColors.back;

  if(forecolor != NULL)
    {
      XColor color;

      if((XParseColor (dpy, Scr.FvwmRoot.attr.colormap, forecolor, &color))
	 &&(XAllocColor (dpy, Scr.FvwmRoot.attr.colormap, &color)))
	{
	  tmp_win->TextPixel = color.pixel; 
	}
    }
  if(backcolor != NULL)
    {
      XColor color;

      if((XParseColor (dpy, Scr.FvwmRoot.attr.colormap,backcolor, &color))
	 &&(XAllocColor (dpy, Scr.FvwmRoot.attr.colormap, &color)))

	{
	  tmp_win->BackPixel = color.pixel; 
	}
    }


  /* add the window into the fvwm list */
  tmp_win->next = Scr.FvwmRoot.next;
  if (Scr.FvwmRoot.next != NULL)
    Scr.FvwmRoot.next->prev = tmp_win;
  tmp_win->prev = &Scr.FvwmRoot;
  Scr.FvwmRoot.next = tmp_win;
  
  /* create windows */
  tmp_win->frame_x = tmp_win->attr.x + tmp_win->old_bw - tmp_win->bw;
  tmp_win->frame_y = tmp_win->attr.y + tmp_win->old_bw - tmp_win->bw;

  tmp_win->frame_width = tmp_win->attr.width+2*tmp_win->boundary_width;
  tmp_win->frame_height = tmp_win->attr.height + tmp_win->title_height+
    2*tmp_win->boundary_width;
  ConstrainSize(tmp_win, &tmp_win->frame_width, &tmp_win->frame_height);

  valuemask = CWBorderPixel | CWCursor | CWEventMask; 
  if(Scr.d_depth < 2)
    {
      attributes.background_pixmap = Scr.light_gray_pixmap ;
      if(tmp_win->flags & STICKY)
	attributes.background_pixmap = Scr.sticky_gray_pixmap;
      valuemask |= CWBackPixmap;
    }
  else
    {
      attributes.background_pixel = Scr.WinColors.back;
      valuemask |= CWBackPixel;
    }

  attributes.border_pixel = Scr.WinColors.fore;

  attributes.cursor = Scr.FvwmCursors[DEFAULT];
  attributes.event_mask = (SubstructureRedirectMask | ButtonPressMask | 
			   ButtonReleaseMask |EnterWindowMask | 
			   LeaveWindowMask |ExposureMask);

  /* What the heck, we'll always reparent everything from now on! */
  /* create window frame */
  tmp_win->frame = 
    XCreateWindow (dpy, Scr.Root, tmp_win->frame_x,tmp_win->frame_y, 
		   tmp_win->frame_width, tmp_win->frame_height,
		   tmp_win->bw,CopyFromParent, InputOutput,
		   CopyFromParent, valuemask, &attributes);

  attributes.save_under = FALSE;

  /* Thats not all, we'll double-reparent the window ! */
  attributes.cursor = Scr.FvwmCursors[DEFAULT];
  /* create parent window */
  tmp_win->Parent = 
    XCreateWindow (dpy, tmp_win->frame,
		   tmp_win->boundary_width, 
		   tmp_win->boundary_width+tmp_win->title_height,
		   (tmp_win->frame_width - 2*tmp_win->boundary_width),
		   (tmp_win->frame_height - 2*tmp_win->boundary_width - 
		    tmp_win->title_height),tmp_win->bw, CopyFromParent,
		   InputOutput,CopyFromParent, valuemask,&attributes);  

  attributes.event_mask = (ButtonPressMask|ButtonReleaseMask|ExposureMask|
			   EnterWindowMask|LeaveWindowMask);
  tmp_win->title_x = tmp_win->title_y = 0;
  tmp_win->title_w = 0;
  tmp_win->title_width = tmp_win->frame_width - 2*tmp_win->corner_width
    - 3 + tmp_win->bw;
  if(tmp_win->title_width < 1)
    tmp_win->title_width = 1;
  if(tmp_win->flags & BORDER)
    {
      /* Just dump the windows any old place and left SetupFrame take
       * care of the mess */

      /* create corner windows */
      for(i=0;i<4;i++)
	{
	  attributes.cursor = Scr.FvwmCursors[TOP_LEFT+i];	  
	  tmp_win->corners[i] = 
	    XCreateWindow (dpy, tmp_win->frame, 0,0,
			   tmp_win->corner_width, tmp_win->corner_width,
			   0, CopyFromParent,InputOutput,
			   CopyFromParent, valuemask,&attributes);
	}
    }

  if (tmp_win->flags & TITLE)
    {
    unsigned long t_valuemask;
    XSetWindowAttributes t_attributes;

      t_valuemask = valuemask;
      t_attributes = attributes;
      tmp_win->title_x = tmp_win->boundary_width +tmp_win->title_height+1; 
      tmp_win->title_y = tmp_win->boundary_width;
      t_attributes.cursor = Scr.FvwmCursors[TITLE_CURSOR];
      if(Scr.d_depth < 2)
        {
        t_attributes.background_pixmap = Scr.light_gray_pixmap ;
        if(tmp_win->flags & STICKY)
	  t_attributes.background_pixmap = Scr.sticky_gray_pixmap;
        t_valuemask |= CWBackPixmap;
        }
      else
        {
        t_attributes.background_pixel = tmp_win->BackPixel;
        t_valuemask |= CWBackPixel;
        }
      t_attributes.border_pixel = tmp_win->TextPixel;
      /* create title window */
      tmp_win->title_w = 
	XCreateWindow (dpy, tmp_win->frame, tmp_win->title_x, tmp_win->title_y,
		       tmp_win->title_width, tmp_win->title_height,0,
		       CopyFromParent, InputOutput, CopyFromParent,
		       t_valuemask,&t_attributes);

      /* create button windows */
      t_attributes.cursor = Scr.FvwmCursors[SYS];
      for(i=4;i>=0;i--)
	{
	  if((i<Scr.nr_left_buttons)&&(tmp_win->left_w[i] > 0))
	    {
	      tmp_win->left_w[i] =
		XCreateWindow (dpy, tmp_win->frame, tmp_win->title_height*i, 0,
			       tmp_win->title_height, tmp_win->title_height, 0,
			       CopyFromParent, InputOutput,
			       CopyFromParent, t_valuemask, &t_attributes);
	    }
	  else
	    tmp_win->left_w[i] = None;

	  if((i<Scr.nr_right_buttons)&&(tmp_win->right_w[i] >0))
	    {
	      tmp_win->right_w[i] =
		XCreateWindow (dpy, tmp_win->frame, 
			       tmp_win->title_width-
			       tmp_win->title_height*(i+1),
			       0, tmp_win->title_height,
			       tmp_win->title_height, 
			       0, CopyFromParent, InputOutput,
			       CopyFromParent, t_valuemask, &t_attributes);
	    }
	  else
	    tmp_win->right_w[i] = None;
	}

    /* create the title bar icon */
    tmp_win->title_icon = CachePicture(dpy, Scr.Root, 
                                       IconPath, PixmapPath, 
                                       tmp_win->title_pixmap_file);
    }

  if(tmp_win->flags & BORDER)
    {
    /* create side windows */
      for(i=0;i<4;i++)
	{
	  attributes.cursor = Scr.FvwmCursors[TOP+i];
	  tmp_win->sides[i] = 
	    XCreateWindow (dpy, tmp_win->frame, 0, 0, tmp_win->boundary_width,
			   tmp_win->boundary_width, 0, CopyFromParent,
			   InputOutput, CopyFromParent,valuemask,
			   &attributes);
	}
    }

  XMapSubwindows (dpy, tmp_win->frame);
  XRaiseWindow(dpy,tmp_win->Parent);
  XReparentWindow(dpy, tmp_win->w, tmp_win->Parent,0,0);

  valuemask = (CWEventMask | CWDontPropagate);
  attributes.event_mask = (StructureNotifyMask | PropertyChangeMask | 
			   VisibilityChangeMask |  EnterWindowMask | 
			   LeaveWindowMask | 
			   ColormapChangeMask | FocusChangeMask);

  attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;

  XChangeWindowAttributes (dpy, tmp_win->w, valuemask, &attributes);
  if ( XGetWMName(dpy, tmp_win->w, &text_prop) != 0 ) 
#ifdef I18N
  {
    if (text_prop.value) text_prop.nitems = strlen(text_prop.value);
    if (XmbTextPropertyToTextList(dpy, &text_prop, &list, &num) >= Success
	&& num > 0 && *list)
#ifndef EVIL
      tmp_win->name = *list;
    else
      tmp_win->name = NoName;
#else /* EVIL */
      {
	tmp_win->name = *list;
	tmp_win->realname = *list;
	if((tmp_win->name != NULL) && (strcmp(tmp_win->name, "")==0)) {
	  XGetWMName(dpy, tmp_win->w, &text_prop);
	  tmp_win->name = (char *)text_prop.value ;
	}
      }
    else
      {
	XGetWMName(dpy, tmp_win->w, &text_prop);
	tmp_win->name = (char *)text_prop.value ;
      }
#endif /* EVIL */
  }
#else /* I18N */
    tmp_win->name = (char *)text_prop.value ;
#endif /* I18N */
  else
    tmp_win->name = NoName;
  
  XAddToSaveSet(dpy, tmp_win->w);

  /*
   * Reparenting generates an UnmapNotify event, followed by a MapNotify.
   * Set the map state to FALSE to prevent a transition back to
   * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
   * again in HandleMapNotify.
   */
  tmp_win->flags &= ~MAPPED;
  width =  tmp_win->frame_width;
  tmp_win->frame_width = 0;
  height = tmp_win->frame_height;
  tmp_win->frame_height = 0;
  SetupFrame (tmp_win, tmp_win->frame_x, tmp_win->frame_y,width,height, True);

  /* wait until the window is iconified and the icon window is mapped
   * before creating the icon window 
   */
  tmp_win->icon_w = None;
  GrabButtons(tmp_win);
  GrabKeys(tmp_win);

  XSaveContext(dpy, tmp_win->w, FvwmContext, (caddr_t) tmp_win);  
  XSaveContext(dpy, tmp_win->frame, FvwmContext, (caddr_t) tmp_win);
  XSaveContext(dpy, tmp_win->Parent, FvwmContext, (caddr_t) tmp_win);
  if (tmp_win->flags & TITLE)
    {
      XSaveContext(dpy, tmp_win->title_w, FvwmContext, (caddr_t) tmp_win);
      for(i=0;i<Scr.nr_left_buttons;i++)
	XSaveContext(dpy, tmp_win->left_w[i], FvwmContext, (caddr_t) tmp_win);
      for(i=0;i<Scr.nr_right_buttons;i++)
	if(tmp_win->right_w[i] != None)
	  XSaveContext(dpy, tmp_win->right_w[i], FvwmContext, 
		       (caddr_t) tmp_win);
    }
  if (tmp_win->flags & BORDER)
    {
      for(i=0;i<4;i++)
	{
	  XSaveContext(dpy, tmp_win->sides[i], FvwmContext, (caddr_t) tmp_win);
	  XSaveContext(dpy,tmp_win->corners[i],FvwmContext, (caddr_t) tmp_win);
	}
    }
  RaiseWindow(tmp_win);
  KeepOnTop();
  XUngrabServer(dpy);

  XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
		   &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
  XTranslateCoordinates(dpy,tmp_win->frame,Scr.Root,JunkX,JunkY,
			&a,&b,&JunkChild);
  tmp_win->xdiff -= a;
  tmp_win->ydiff -= b;
  if(tmp_win->flags & ClickToFocus)
    {
      /* need to grab all buttons for window that we are about to
       * unhighlight */
      for(i=0;i<3;i++)
	if(Scr.buttons2grab & (1<<i))
	  {
	    XGrabButton(dpy,(i+1),0,tmp_win->frame,True,
			ButtonPressMask, GrabModeSync,GrabModeAsync,None,
			Scr.FvwmCursors[SYS]);
	    XGrabButton(dpy,(i+1),LockMask,tmp_win->frame,True,
			ButtonPressMask, GrabModeSync,GrabModeAsync,None,
			Scr.FvwmCursors[SYS]);
	  }
    }

  BroadcastConfig(M_ADD_WINDOW,tmp_win);

  BroadcastName(M_WINDOW_NAME,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,tmp_win->name);
  BroadcastName(M_ICON_NAME,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,tmp_win->icon_name);
  if (tmp_win->icon_bitmap_file != NULL &&
      tmp_win->icon_bitmap_file != Scr.DefaultIcon)
    BroadcastName(M_ICON_FILE,tmp_win->w,tmp_win->frame,
		   (unsigned long)tmp_win,tmp_win->icon_bitmap_file);
  BroadcastName(M_RES_CLASS,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,tmp_win->class.res_class);
  BroadcastName(M_RES_NAME,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,tmp_win->class.res_name);
  if (tmp_win->title_icon != NULL)
    Broadcast(M_MINI_ICON, 6,
              tmp_win->w, /* Watch Out ! : I reduced the set of infos... */
              tmp_win->title_icon->picture,
              tmp_win->title_icon->mask,
              tmp_win->title_icon->width,
              tmp_win->title_icon->height,
              tmp_win->title_icon->depth, 0);

  FetchWmProtocols (tmp_win);
  FetchWmColormapWindows (tmp_win);
  if(!(XGetWindowAttributes(dpy,tmp_win->w,&(tmp_win->attr))))
    tmp_win->attr.colormap = Scr.FvwmRoot.attr.colormap;
  if(NeedToResizeToo)
    {
      XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, 
		   Scr.MyDisplayHeight, 
		   tmp_win->frame_x + (tmp_win->frame_width>>1), 
		   tmp_win->frame_y + (tmp_win->frame_height>>1));
      Event.xany.type = ButtonPress;
      Event.xbutton.button = 1;
      Event.xbutton.x_root = tmp_win->frame_x + (tmp_win->frame_width>>1);
      Event.xbutton.y_root = tmp_win->frame_y + (tmp_win->frame_height>>1);
      Event.xbutton.x = (tmp_win->frame_width>>1);
      Event.xbutton.y = (tmp_win->frame_height>>1);
      Event.xbutton.subwindow = None;
      Event.xany.window = tmp_win->w;
      resize_window(&Event , tmp_win->w, tmp_win, C_WINDOW, "", 0);
    }
  InstallWindowColormaps(colormap_win);
  return (tmp_win);
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabButtons - grab needed buttons for the window
 *
 *  Inputs:
 *	tmp_win - the fvwm window structure to use
 *
 ***********************************************************************/
void GrabButtons(FvwmWindow *tmp_win)
{
  Binding *MouseEntry;

  MouseEntry = Scr.AllBindings;
  while(MouseEntry != (Binding *)0)
    {
      if((MouseEntry->Action != NULL)&&(MouseEntry->Context & C_WINDOW)
	 &&(MouseEntry->IsMouse == 1))
	{
	  if(MouseEntry->Button_Key >0)
	    {
	      XGrabButton(dpy, MouseEntry->Button_Key, MouseEntry->Modifier, 
			  tmp_win->w, 
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None, 
			  Scr.FvwmCursors[DEFAULT]);
	      if(MouseEntry->Modifier != AnyModifier)
		{
		  XGrabButton(dpy, MouseEntry->Button_Key, 
			      (MouseEntry->Modifier | LockMask),
			      tmp_win->w, 
			      True, ButtonPressMask | ButtonReleaseMask,
			      GrabModeAsync, GrabModeAsync, None, 
			      Scr.FvwmCursors[DEFAULT]);
		}
	    }
	  else
	    {
	      XGrabButton(dpy, 1, MouseEntry->Modifier, 
			  tmp_win->w, 
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None, 
			  Scr.FvwmCursors[DEFAULT]);
	      XGrabButton(dpy, 2, MouseEntry->Modifier, 
			  tmp_win->w, 
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None, 
			  Scr.FvwmCursors[DEFAULT]);
	      XGrabButton(dpy, 3, MouseEntry->Modifier, 
			  tmp_win->w, 
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None, 
			  Scr.FvwmCursors[DEFAULT]);
	      if(MouseEntry->Modifier != AnyModifier)
		{
		  XGrabButton(dpy, 1,
			      (MouseEntry->Modifier | LockMask),
			      tmp_win->w, 
			      True, ButtonPressMask | ButtonReleaseMask,
			      GrabModeAsync, GrabModeAsync, None, 
			      Scr.FvwmCursors[DEFAULT]);
		  XGrabButton(dpy, 2,
			      (MouseEntry->Modifier | LockMask),
			      tmp_win->w, 
			      True, ButtonPressMask | ButtonReleaseMask,
			      GrabModeAsync, GrabModeAsync, None, 
			      Scr.FvwmCursors[DEFAULT]);
		  XGrabButton(dpy, 3,
			      (MouseEntry->Modifier | LockMask),
			      tmp_win->w, 
			      True, ButtonPressMask | ButtonReleaseMask,
			      GrabModeAsync, GrabModeAsync, None, 
			      Scr.FvwmCursors[DEFAULT]);
		}
	    }
	}
      MouseEntry = MouseEntry->NextBinding;
    }
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabKeys - grab needed keys for the window
 *
 *  Inputs:
 *	tmp_win - the fvwm window structure to use
 *
 ***********************************************************************/
void GrabKeys(FvwmWindow *tmp_win)
{
  Binding *tmp;
  for (tmp = Scr.AllBindings; tmp != NULL; tmp = tmp->NextBinding)
    {
      if((tmp->Context & (C_WINDOW|C_TITLE|C_RALL|C_LALL|C_SIDEBAR))&&
	 (tmp->IsMouse == 0))
	{
	  XGrabKey(dpy, tmp->Button_Key, tmp->Modifier, tmp_win->frame, True,
		   GrabModeAsync, GrabModeAsync);
	  if(tmp->Modifier != AnyModifier)
	    {
	      XGrabKey(dpy, tmp->Button_Key, tmp->Modifier|LockMask, 
		       tmp_win->frame, True,
		       GrabModeAsync, GrabModeAsync);	      
	    }
	}
    }
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	FetchWMProtocols - finds out which protocols the window supports
 *
 *  Inputs:
 *	tmp - the fvwm window structure to use
 *
 ***********************************************************************/
void FetchWmProtocols (FvwmWindow *tmp)
{
  unsigned long flags = 0L;
  Atom *protocols = NULL, *ap;
  int i, n;
  Atom atype;
  int aformat;
  unsigned long bytes_remain,nitems;

  if(tmp == NULL) return;
  /* First, try the Xlib function to read the protocols.
   * This is what Twm uses. */
  if (XGetWMProtocols (dpy, tmp->w, &protocols, &n)) 
    {
      for (i = 0, ap = protocols; i < n; i++, ap++) 
	{
	  if (*ap == (Atom)_XA_WM_TAKE_FOCUS) flags |= DoesWmTakeFocus;
	  if (*ap == (Atom)_XA_WM_DELETE_WINDOW) flags |= DoesWmDeleteWindow;
	}
      if (protocols) XFree ((char *) protocols);
    }
  else
    {
      /* Next, read it the hard way. mosaic from Coreldraw needs to 
       * be read in this way. */
      if ((XGetWindowProperty(dpy, tmp->w, _XA_WM_PROTOCOLS, 0L, 10L, False,
			      _XA_WM_PROTOCOLS, &atype, &aformat, &nitems,
			      &bytes_remain,
			      (unsigned char **)&protocols))==Success)
	{
	  for (i = 0, ap = protocols; i < nitems; i++, ap++) 
	    {
	      if (*ap == (Atom)_XA_WM_TAKE_FOCUS) flags |= DoesWmTakeFocus;
	      if (*ap == (Atom)_XA_WM_DELETE_WINDOW) flags |= DoesWmDeleteWindow;
	    }
	  if (protocols) XFree ((char *) protocols);
	}
    }
  tmp->flags |= flags;
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	GetWindowSizeHints - gets application supplied size info
 *
 *  Inputs:
 *	tmp - the fvwm window structure to use
 *
 ***********************************************************************/
void GetWindowSizeHints(FvwmWindow *tmp)
{
  long supplied = 0;

  if (!XGetWMNormalHints (dpy, tmp->w, &tmp->hints, &supplied))
    tmp->hints.flags = 0;

  /* Beat up our copy of the hints, so that all important field are
   * filled in! */
  if (tmp->hints.flags & PResizeInc) 
    {
      if (tmp->hints.width_inc == 0) tmp->hints.width_inc = 1;
      if (tmp->hints.height_inc == 0) tmp->hints.height_inc = 1;
    }
  else
    {
      tmp->hints.width_inc = 1;
      tmp->hints.height_inc = 1;
    }
  
  /*
   * ICCCM says that PMinSize is the default if no PBaseSize is given,
   * and vice-versa.
   */

  if(!(tmp->hints.flags & PBaseSize))
    {
      if(tmp->hints.flags & PMinSize)
	{
	  tmp->hints.base_width = tmp->hints.min_width;
	  tmp->hints.base_height = tmp->hints.min_height;      
	}
      else
	{
	  tmp->hints.base_width = 0;
	  tmp->hints.base_height = 0;
	}
    }
  if(!(tmp->hints.flags & PMinSize))
    {
      tmp->hints.min_width = tmp->hints.base_width;
      tmp->hints.min_height = tmp->hints.base_height;            
    }
  if(!(tmp->hints.flags & PMaxSize))
    {
      tmp->hints.max_width = MAX_WINDOW_WIDTH;
      tmp->hints.max_height = MAX_WINDOW_HEIGHT;
    }
  if(tmp->hints.max_width < tmp->hints.min_width)
    tmp->hints.max_width = MAX_WINDOW_WIDTH;    
  if(tmp->hints.max_height < tmp->hints.min_height)
    tmp->hints.max_height = MAX_WINDOW_HEIGHT;    

  /* Zero width/height windows are bad news! */
  if(tmp->hints.min_height <= 0)
    tmp->hints.min_height = 1;
  if(tmp->hints.min_width <= 0)
    tmp->hints.min_width = 1;

  if(!(tmp->hints.flags & PWinGravity))
    {
      tmp->hints.win_gravity = NorthWestGravity;
      tmp->hints.flags |= PWinGravity;
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	LookInList - look through a list for a window name, or class
 *
 *  Returned Value:
 *	the ptr field of the list structure or NULL if the name 
 *	or class was not found in the list
 *
 *  Inputs:
 *	list	- a pointer to the head of a list
 *	name	- a pointer to the name to look for
 *	class	- a pointer to the class to look for
 *
 ***********************************************************************/
unsigned long LookInList(name_list *list, char *name, XClassHint *class, 
			 char **value, char **tvalue, int *Desk, int *border_width,
			 int *resize_width, char **forecolor, char **backcolor,
                         unsigned long * buttons, int *IconBox, int *BoxFillMethod)
{
  name_list *nptr;
  unsigned long retval = 0;

  *value     = NULL;
  *tvalue    = NULL;
  *forecolor = NULL;
  *backcolor = NULL;
  *Desk      = 0;
  *buttons   = 0;
  *BoxFillMethod = 0;
  *border_width = 0;
  *resize_width = 0;
  IconBox[0] = -1;
  IconBox[1] = -1;
  IconBox[2] = Scr.MyDisplayWidth;
  IconBox[3] = Scr.MyDisplayHeight;

  /* look for the name first */
  for (nptr = list; nptr != NULL; nptr = nptr->next)
  {
      if (class->res_class != NULL || class->res_name != NULL)
	{
	  /* first look for the res_class  (lowest priority) */
	  if (matchWildcards(nptr->name,class->res_class) == TRUE)
	    {
	      if(nptr->value  != NULL) *value  = nptr->value;
	      if(nptr->tvalue != NULL) *tvalue = nptr->tvalue;
	      if(nptr->off_flags & STARTSONDESK_FLAG)
		*Desk = nptr->Desk;
	      if(nptr->off_flags & BW_FLAG)
		*border_width = nptr->border_width;
	      if(nptr->off_flags & FORE_COLOR_FLAG)
		*forecolor = nptr->ForeColor;
	      if(nptr->off_flags & BACK_COLOR_FLAG)
		*backcolor = nptr->BackColor;
	      if(nptr->off_flags & NOBW_FLAG)
		*resize_width = nptr->resize_width;
	      retval |= nptr->off_flags;
	      retval &= ~(nptr->on_flags);
              *buttons |= nptr->off_buttons;
              *buttons &= ~(nptr->on_buttons);
	      if(nptr->BoxFillMethod != 0)
		*BoxFillMethod = nptr->BoxFillMethod;
	      if(nptr->IconBox[0] >= 0)
		{
		  IconBox[0] = nptr->IconBox[0];
		  IconBox[1] = nptr->IconBox[1];
		  IconBox[2] = nptr->IconBox[2];
		  IconBox[3] = nptr->IconBox[3];
		}
	    }

	  /* look for the res_name next */
	  if (matchWildcards(nptr->name,class->res_name) == TRUE)
	    {
	      if(nptr->value  != NULL) *value  = nptr->value;
	      if(nptr->tvalue != NULL) *tvalue = nptr->tvalue;
	      if(nptr->off_flags & STARTSONDESK_FLAG)
		*Desk = nptr->Desk;
	      if(nptr->off_flags & FORE_COLOR_FLAG)
		*forecolor = nptr->ForeColor;
	      if(nptr->off_flags & BACK_COLOR_FLAG)
		*backcolor = nptr->BackColor;
	      if(nptr->off_flags & BW_FLAG)
		*border_width = nptr->border_width;
	      if(nptr->off_flags & NOBW_FLAG)
		*resize_width = nptr->resize_width;
	      retval |= nptr->off_flags;
	      retval &= ~(nptr->on_flags);
              *buttons |= nptr->off_buttons;
              *buttons &= ~(nptr->on_buttons);
	      if(nptr->BoxFillMethod != 0)
		*BoxFillMethod = nptr->BoxFillMethod;
	      if(nptr->IconBox[0] >= 0)
		{
		  IconBox[0] = nptr->IconBox[0];
		  IconBox[1] = nptr->IconBox[1];
		  IconBox[2] = nptr->IconBox[2];
		  IconBox[3] = nptr->IconBox[3];
		}
	    }

	  if (!strcmp(nptr->name,class->res_class))
	    {
	      if(nptr->value  != NULL) *value  = nptr->value;
	      if(nptr->tvalue != NULL) *tvalue = nptr->tvalue;
	      if(nptr->off_flags & STARTSONDESK_FLAG)
		*Desk = nptr->Desk;
	      if(nptr->off_flags & BW_FLAG)
		*border_width = nptr->border_width;
	      if(nptr->off_flags & FORE_COLOR_FLAG)
		*forecolor = nptr->ForeColor;
	      if(nptr->off_flags & BACK_COLOR_FLAG)
		*backcolor = nptr->BackColor;
	      if(nptr->off_flags & NOBW_FLAG)
		*resize_width = nptr->resize_width;
	      retval |= nptr->off_flags;
	      retval &= ~(nptr->on_flags);
              *buttons |= nptr->off_buttons;
              *buttons &= ~(nptr->on_buttons);
	      if(nptr->BoxFillMethod != 0)
		*BoxFillMethod = nptr->BoxFillMethod;
	      if(nptr->IconBox[0] >= 0)
		{
		  IconBox[0] = nptr->IconBox[0];
		  IconBox[1] = nptr->IconBox[1];
		  IconBox[2] = nptr->IconBox[2];
		  IconBox[3] = nptr->IconBox[3];
		}
	    }

	  /* look for the res_name next */
	  if (!strcmp(nptr->name,class->res_name))
	    {
	      if(nptr->value  != NULL) *value  = nptr->value;
	      if(nptr->tvalue != NULL) *tvalue = nptr->tvalue;
	      if(nptr->off_flags & STARTSONDESK_FLAG)
		*Desk = nptr->Desk;
	      if(nptr->off_flags & FORE_COLOR_FLAG)
		*forecolor = nptr->ForeColor;
	      if(nptr->off_flags & BACK_COLOR_FLAG)
		*backcolor = nptr->BackColor;
	      if(nptr->off_flags & BW_FLAG)
		*border_width = nptr->border_width;
	      if(nptr->off_flags & NOBW_FLAG)
		*resize_width = nptr->resize_width;
	      retval |= nptr->off_flags;
	      retval &= ~(nptr->on_flags);
              *buttons |= nptr->off_buttons;
              *buttons &= ~(nptr->on_buttons);
	      if(nptr->BoxFillMethod != 0)
		*BoxFillMethod = nptr->BoxFillMethod;
	      if(nptr->IconBox[0] >= 0)
		{
		  IconBox[0] = nptr->IconBox[0];
		  IconBox[1] = nptr->IconBox[1];
		  IconBox[2] = nptr->IconBox[2];
		  IconBox[3] = nptr->IconBox[3];
		}
	    }
      }
      else
      {
          if (matchWildcards(nptr->name,name) == TRUE)
          {
	      if(nptr->value  != NULL) *value  = nptr->value;
	      if(nptr->tvalue != NULL) *tvalue = nptr->tvalue;
	      if(nptr->off_flags & STARTSONDESK_FLAG)
	        *Desk = nptr->Desk;
	      if(nptr->off_flags & FORE_COLOR_FLAG)
	        *forecolor = nptr->ForeColor;
	      if(nptr->off_flags & BACK_COLOR_FLAG)
	        *backcolor = nptr->BackColor;
	      if(nptr->off_flags & BW_FLAG)
	        *border_width = nptr->border_width;
	      if(nptr->off_flags & NOBW_FLAG)
	        *resize_width = nptr->resize_width;
	      retval |= nptr->off_flags;
	      retval &= ~(nptr->on_flags);
              *buttons |= nptr->off_buttons;
              *buttons &= ~(nptr->on_buttons);
	      if(nptr->BoxFillMethod != 0)
	        *BoxFillMethod = nptr->BoxFillMethod;
	      if(nptr->IconBox[0] >= 0)
	        {
	          IconBox[0] = nptr->IconBox[0];
	          IconBox[1] = nptr->IconBox[1];
	          IconBox[2] = nptr->IconBox[2];
	          IconBox[3] = nptr->IconBox[3];
	        }
          }

          if (!strcmp(nptr->name,name) == TRUE)
          {
	      if(nptr->value  != NULL) *value  = nptr->value;
	      if(nptr->tvalue != NULL) *tvalue = nptr->tvalue;
	      if(nptr->off_flags & STARTSONDESK_FLAG)
	        *Desk = nptr->Desk;
	      if(nptr->off_flags & FORE_COLOR_FLAG)
	        *forecolor = nptr->ForeColor;
	      if(nptr->off_flags & BACK_COLOR_FLAG)
	        *backcolor = nptr->BackColor;
	      if(nptr->off_flags & BW_FLAG)
	        *border_width = nptr->border_width;
	      if(nptr->off_flags & NOBW_FLAG)
	        *resize_width = nptr->resize_width;
	      retval |= nptr->off_flags;
	      retval &= ~(nptr->on_flags);
              *buttons |= nptr->off_buttons;
              *buttons &= ~(nptr->on_buttons);
	      if(nptr->BoxFillMethod != 0)
	        *BoxFillMethod = nptr->BoxFillMethod;
	      if(nptr->IconBox[0] >= 0)
	        {
	          IconBox[0] = nptr->IconBox[0];
	          IconBox[1] = nptr->IconBox[1];
	          IconBox[2] = nptr->IconBox[2];
	          IconBox[3] = nptr->IconBox[3];
	        }
           }
      }

  }
  return retval;
}
