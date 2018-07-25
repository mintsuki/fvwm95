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


/***********************************************************************
 *
 * $XConsortium: menus.h,v 1.24 89/12/10 17:46:26 jim Exp $
 *
 * twm menus include file
 *
 * 17-Nov-87 Thomas E. LaStrange		File created
 *
 ***********************************************************************/

#ifndef _MENUS_
#define _MENUS_

/* Function types used for formatting menus */

#define FUNC_NO_WINDOW 0
#define FUNC_NEEDS_WINDOW 0
#define FUNC_POPUP 1
#define FUNC_TITLE 2
#define FUNC_NOP 3

#include <fvwm/fvwmlib.h>

typedef struct MenuItem
{
    struct MenuItem *next;	/* next menu item */
    struct MenuItem *prev;	/* prev menu item */
    char *item;			/* the character string displayed on left*/
    char *item2;	        /* the character string displayed on right*/
    Picture *picture;           /* Pixmap to show  above label*/
    Picture *lpicture;          /* Pixmap to show to left of label */
    char *action;		/* action to be performed */
    short item_num;		/* item number of this menu */
    short x;			/* x coordinate for text (item) */
    short x2;			/* x coordinate for text (item2) */
    short xp;                   /* x coordinate for picture */
    short y_offset;		/* y coordinate for item */
    short y_height;		/* y height for item */
    short func_type;		/* type of built in function */
    short state;		/* video state, 0 = normal, 1 = reversed */
    short strlen;		/* strlen(item) */
    short strlen2;		/* strlen(item2) */
    short hotkey;		/* Hot key offset (pete@tecc.co.uk).
				   0 - No hot key
				   +ve - offset to hot key char in item
				   -ve - offset to hot key char in item2
				   (offsets have 1 added, so +1 or -1
				   refer to the *first* character)
				   */
} MenuItem;

typedef struct MenuRoot
{
    struct MenuItem *first;	/* first item in menu */
    struct MenuItem *last;	/* last item in menu */
    struct MenuRoot *next;	/* next in list of root menus */
    char *name;			/* name of root */
    Window w;			/* the window of the menu */
    short height;		/* height of the menu */
    short width;		/* width of the menu for 1st col */
    short width2;		/* width of the menu for 2nd col */
    short width0;               /* width of the menu-left-picture col */
    short items;		/* number of items in the menu */
    Bool in_use;
    int func;
    int x, y;
    Bool has_coords;		/* true if x and y are valid */
    Picture *sidePic;
    Pixel sideColor;
    Bool colorize;
    short xoffset;
} MenuRoot;

typedef struct Binding
{
  char IsMouse;           /* Is it a mouse or key binding 1= mouse; */
  int Button_Key;         /* Mouse Button number of Keycode */
  char *key_name;         /* In case of keycode, give the key_name too */
  int Context;            /* Contex is Fvwm context, ie titlebar, frame, etc */
  int Modifier;           /* Modifiers for keyboard state */   
  char *Action;           /* What to do? */
  struct Binding *NextBinding; 
} Binding;


#define MENU_ERROR -1
#define MENU_NOP 0
#define MENU_DONE 1
#define SUBMENU_DONE 2


/* Types of events for the FUNCTION builtin */
#define MOTION 'm'
#define IMMEDIATE 'i'
#define CLICK 'c'
#define DOUBLE_CLICK 'd'
#define ONE_AND_A_HALF_CLICKS 'o'

extern MenuRoot *ActiveMenu;
extern MenuItem *ActiveItem;


#endif /* _MENUS_ */
