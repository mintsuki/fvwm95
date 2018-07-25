/* FvwmTaskBar Module for Fvwm. 
 *
 *  Copyright 1994,  Mike Finger (mfinger@mermaid.micro.umn.edu or
 *                               Mike_Finger@atk.com)
 *  Copyright 1995,  Pekka Pietik{inen (pp@netppl.fi)
 *
 * The functions in this source file that are the original work of Mike Finger.
 * This source file has been modified for use with fvwm95look by
 * Pekka Pietik{inen, David Barth, Hector Peraza, etc, etc...
 * 
 * No guarantees or warantees or anything are provided or implied in any way
 * whatsoever. Use this program at your own risk. Permission to use this
 * program for any purpose is given, as long as the copyright is kept intact.
 *
 */

#include <FVWMconfig.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>

#include <fvwm/fvwmlib.h>

#include "ButtonArray.h"
#include "Mallocs.h"
#include "FvwmTaskBar.h"

#ifdef I18N
#ifdef __STDC__
#define XTextWidth(x,y,z) XmbTextEscapement(x ## set,y,z)
#else
#define XTextWidth(x,y,z) XmbTextEscapement(x/**/set,y,z)
#endif
#define XDrawString(t,u,v,w,x,y,z) XmbDrawString(t,u,fontset,v,w,x,y,z)
#endif

extern XFontStruct *ButtonFont, *SelButtonFont;
#ifdef I18N
extern XFontSet ButtonFontset, SelButtonFontset;
#endif
extern Display *dpy;
extern Window win;
extern GC shadow, hilite, graph, whitegc, blackgc, checkered;
extern int button_width;

extern Button *StartButton;

extern int NRows, RowHeight, win_width, stwin_width;

#define MIN_BUTTON_SIZE 32 /*....*/

/*************************************************************************
 *                                                                       *
 *  Button handling functions and procedures                             *
 *                                                                       *
 *************************************************************************/

/* Draws a 3D rectangle */
void Draw3dRect(Window wn, int x, int y, int w, int h, int state)
{
  XClearArea (dpy, wn, x, y, w, h, False);

  switch (state) {
  case BUTTON_UP:
    XDrawLine (dpy, win, hilite, x, y, x+w-2, y);
    XDrawLine (dpy, win, hilite, x, y, x, y+h-2);

    XDrawLine (dpy, win, shadow,  x+1, y+h-2, x+w-2, y+h-2);
    XDrawLine (dpy, win, shadow,  x+w-2, y+h-2, x+w-2, y+1);
    XDrawLine (dpy, win, blackgc, x, y+h-1, x+w-1, y+h-1);
    XDrawLine (dpy, win, blackgc, x+w-1, y+h-1, x+w-1, y);
    break;
  case BUTTON_BRIGHT:
    XFillRectangle (dpy, win, checkered, x+2, y+2, w-4, h-4);
    XDrawLine (dpy, win, hilite, x+2, y+2, x+w-3, y+2);
  case BUTTON_DOWN:
    XDrawLine (dpy, win, blackgc, x, y, x+w-1, y);
    XDrawLine (dpy, win, blackgc, x, y, x, y+h-1);
    
    XDrawLine (dpy, win, shadow, x+1, y+1, x+w-3, y+1);   
    XDrawLine (dpy, win, shadow, x+1, y+1, x+1, y+h-3);   
    XDrawLine (dpy, win, hilite, x+1, y+h-1, x+w-1, y+h-1);
    XDrawLine (dpy, win, hilite, x+w-1, y+h-1, x+w-1, y+1);
    break;
  }
}


/* -------------------------------------------------------------------------
   ButtonNew - Allocates and fills a new button structure
   ------------------------------------------------------------------------- */
Button *ButtonNew(char *title, Picture *p, int state, int count)
  {
  Button *new;

  new = (Button *)safemalloc(sizeof(Button));
  new->title = safemalloc(strlen(title)+1);
  strcpy(new->title, title);
  if (p != NULL) {
    new->p.picture = p->picture;
    new->p.mask    = p->mask;
    new->p.width   = p->width;
    new->p.height  = p->height;
    new->p.depth   = p->depth;
  } else {
    new->p.picture = 0;
  }

  new->state = state;
  new->count = count;
  new->next  = NULL;
  new->needsupdate = 1;

  return new;
  }

/* -------------------------------------------------------------------------
   ButtonDraw - Draws the specified button
   ------------------------------------------------------------------------- */
void ButtonDraw(Button *button, int x, int y, int w, int h)
  {
  static char *t3p = "...";
  int state, x3p, w3p, newx;
  int search_len;
  XFontStruct *font;
#ifdef I18N
  XFontSet fontset;
#endif
  XGCValues gcv;
  unsigned long gcm;
    
  if (button == NULL) return;
  button->needsupdate = 0;
  if (x + w > win_width - stwin_width) return;

  state = button->state;
  Draw3dRect(win, x, y, w, h, state);

  if (state != BUTTON_UP) { x++; y++; }

  if (state == BUTTON_BRIGHT || button == StartButton)
#ifdef I18N
  {
    font = SelButtonFont;
    fontset = SelButtonFontset;
  }
#else
    font = SelButtonFont;
#endif
  else
#ifdef I18N
  {
    font = ButtonFont;
    fontset = ButtonFontset;
  }
#else
    font = ButtonFont;
#endif

  gcm = GCFont;
  gcv.font = font->fid;
  XChangeGC(dpy, graph, gcm, &gcv);

  newx = 4;

  w3p = XTextWidth(font, t3p, 3);

  if ((button->p.picture != 0) &&
      (newx + button->p.width + w3p + 3 < w)) {

    gcm = GCClipMask | GCClipXOrigin | GCClipYOrigin;
    gcv.clip_mask = button->p.mask;
    gcv.clip_x_origin = x + 3;
    gcv.clip_y_origin = y + ((h-button->p.height) >> 1);
    XChangeGC(dpy, hilite, gcm, &gcv);
    XCopyArea(dpy, button->p.picture, win, hilite, 0, 0,
                   button->p.width, button->p.height,
                   gcv.clip_x_origin, gcv.clip_y_origin);
    gcm = GCClipMask;
    gcv.clip_mask = None;
    XChangeGC(dpy, hilite, gcm, &gcv);

    newx += button->p.width+2;
  }

  if (button->title == NULL) return;

  search_len = strlen(button->title);

  if (XTextWidth(font, button->title, search_len) > w-newx-3) {

    while ((x3p = XTextWidth(font, button->title, search_len) + newx) > w-w3p-3)
      if (search_len-- <= 0) break;
    XDrawString(dpy, win, graph, x + x3p, y+ButtonFont->ascent+4, t3p, 3);

    button->truncate = True;
  } else {
    button->truncate = False;
  }

  XDrawString(dpy, win, graph, 
              x+newx, y+font->ascent+4,
              button->title, search_len);
 }


/* -------------------------------------------------------------------------
   ButtonUpdate - Change the name/state of a button
   ------------------------------------------------------------------------- */
int ButtonUpdate(Button *button, char *title, int state)
{
  if (button == NULL) return -1;

  if ((title != NULL) && (button->title != title)) {
    button->title = (char *)realloc(button->title,strlen(title)+1);
    strcpy(button->title,title);
    button->needsupdate = 1;
  }

  if (state != DONT_CARE) {
    if (state != button->state) {
      button->state = state;
      button->needsupdate = 1;
    }
  }

  return 1;
}

/* -------------------------------------------------------------------------
   ButtonDelete - Free space allocated to a button
   ------------------------------------------------------------------------- */
void ButtonDelete(Button *ptr)
{
  if (ptr != NULL) {
    if (ptr->title != NULL) free(ptr->title);
    free(ptr);
  }
}

/* -------------------------------------------------------------------------
   ButtonName - Return the name of the button
   ------------------------------------------------------------------------- */
char *ButtonName(Button *button)
{
  if (button == NULL) return NULL;
  else return button->title;
}


/*************************************************************************
 *                                                                       *
 *  ButtonArray handling functions and procedures                        *
 *                                                                       *
 *************************************************************************/

/* -------------------------------------------------------------------------
   InitArray - Initialize the arrary of buttons
   ------------------------------------------------------------------------- */
void InitArray(ButtonArray *array, int x, int y, int w, int h, int tw)
{
   array->count = 0;
   array->head = array->tail = NULL;
   array->x  = x;
   array->y  = y;
   array->w  = w;
   array->h  = h;
   array->tw = tw;
}

/* -------------------------------------------------------------------------
   UpdateArray - Update the array specifics.  x,y, width, height
   ------------------------------------------------------------------------- */
void UpdateArray(ButtonArray *array,int x,int y,int w, int h, int tw)
{
   Button *temp;

   if (x != -1) array->x = x;
   if (y != -1) array->y = y;
   if (w != -1) array->w = w;
   if (h != -1) array->h = h;
   if (tw != -1) array->tw = tw;
   for(temp=array->head; temp!=NULL; temp=temp->next) temp->needsupdate = 1;
}

/* -------------------------------------------------------------------------
   AddButton - Allocate space for and add the button to the list
   ------------------------------------------------------------------------- */
void AddButton(ButtonArray *array, char *title, Picture *p, int state, int count)
{
  Button *new, *temp;

  new = ButtonNew(title, p, state, count);
  if (array->head == NULL) array->head = new;
  else {
    for (temp=array->head; temp->next; temp=temp->next);
    temp->next = new;
  }
  array->count++;

  ArrangeButtonArray (array);
}

/* -------------------------------------------------------------------------
   ArrangeButtonArray - Rearrange the button size (called from AddButton,
                          RemoveButton, AdjustWindow)
   ------------------------------------------------------------------------- */

void ArrangeButtonArray (ButtonArray *array)
  {
  int tw;
  Button *temp;

  if (!array->count)
    tw = array->w;
  else if (NRows == 1)
         tw = array->w / array->count;
       else
         tw = array->w / ((array->count / NRows)+1);

  if (tw > button_width) tw = button_width;
  if (tw < MIN_BUTTON_SIZE) {
    tw = MIN_BUTTON_SIZE;
    /* bug here */
    /* correction: adjust the size to allow for new buttons */
    AdjustWindow(win_width, RowHeight * (NRows+1));
  }

  if (tw != array->tw) /* update needed */
    {
    array->tw = tw;
    for(temp=array->head; temp!=NULL; temp=temp->next)
      temp->needsupdate=1;
    }
  }

/* -------------------------------------------------------------------------
   UpdateButton - Change the name/state of a button
   ------------------------------------------------------------------------- */
int UpdateButton(ButtonArray *array, int butnum, char *title, int state)
{
  Button *temp;

  for (temp=array->head; temp; temp=temp->next)
    if (temp->count == butnum) break;

  return ButtonUpdate(temp, title, state);
}

/* -------------------------------------------------------------------------
   UpdateButtonPicture - Change the picture of a button
   ------------------------------------------------------------------------- */
int UpdateButtonPicture(ButtonArray *array, int butnum, Picture *p)
{
  Button *temp;

  for (temp=array->head; temp; temp=temp->next)
    if (temp->count == butnum) break;

  if (temp == NULL) return -1;
  if (temp->p.picture != p->picture || temp->p.mask != p->mask) {
    temp->p.picture = p->picture;
    temp->p.mask    = p->mask;
    temp->p.width   = p->width;
    temp->p.height  = p->height;
    temp->p.depth   = p->depth;
    temp->needsupdate = 1;
  }
  return 1;
}

/* -------------------------------------------------------------------------
   RemoveButton - Delete a button from the list
   ------------------------------------------------------------------------- */
void RemoveButton(ButtonArray *array, int butnum)
{
  Button *temp, *temp2;

  if (array->head) {
    if (array->head->count == butnum) {
      temp2 = array->head;
      array->head = array->head->next;
    } else {
      for (temp=array->head, temp2=temp->next;
           temp2;
           temp2=temp2->next, temp=temp->next)
        if (temp2->count == butnum) break;
      if (temp2) temp->next = temp2->next;
    }

    if (temp2) {
      ButtonDelete(temp2);
      array->count--;
    }

    for (temp=array->head; temp; temp=temp->next)
      temp->needsupdate = 1;

    ArrangeButtonArray(array);
  }

}

/* -------------------------------------------------------------------------
   FreeAllButtons - Free the whole array of buttons
   ------------------------------------------------------------------------- */
void FreeAllButtons(ButtonArray *array)
{
  Button *temp, *temp2;

  for(temp=array->head; temp!=NULL; ) {
    temp2 = temp;
    temp  = temp->next;
    ButtonDelete(temp2);
  }
}

/* ------------------------------------------------------------------------
   DrawButtonArray - Draw the whole array (all=1), or only those that need.
   ------------------------------------------------------------------------ */
void DrawButtonArray(ButtonArray *array, int all)
{
  Button *temp;
  int x, y, n;

  x = 0;
  y = array->y;
  n = 1;
  for(temp=array->head; temp!=NULL; temp=temp->next) {
    if ((x + array->tw > array->w) && (n < NRows))
      { x = 0; y += RowHeight+2; ++n; }
    if (temp->needsupdate || all)
      ButtonDraw(temp, x + array->x, y, array->tw-3, array->h);
    x += array->tw;
  }
}

/* -------------------------------------------------------------------------
   RadioButton - Enable button i and verify all others are disabled
   ------------------------------------------------------------------------- */
void RadioButton(ButtonArray *array, int butnum, int state)
{
  Button *button;
  
  for(button=array->head; button!=NULL; button=button->next) {
    if (button->count == butnum) {
      button->state = state;
      button->needsupdate = 1;
    } else {
      if (button->state != BUTTON_UP) {
	button->state = BUTTON_UP;
	button->needsupdate = 1;
      }
    }
  }
}

/* -------------------------------------------------------------------------
   WhichButton - Based on x,y which button was pressed
   ------------------------------------------------------------------------- */
int WhichButton(ButtonArray *array, int xp, int yp)
{
  int   junkx, junky, junkz;
  char *junkt;

  return LocateButton(array, xp, yp, &junkx, &junky, &junkt, &junkz);
}

int LocateButton(ButtonArray *array, int xp,  int yp, 
                                     int *xb, int *yb,
                                     char **name, int *trunc)
{
  Button *temp;
  int num, x, y, n;
   
  if (xp < array->x || xp > array->x+array->w) return -1;

  x = 0;
  y = array->y;
  n = 1;
  for(temp=array->head, num=0; temp!=NULL; temp=temp->next, num++) {
    if((x + array->tw > array->w) && (n < NRows))
      { x = 0; y += RowHeight+2; ++n; }
    if( xp >= x+array->x && xp <= x+array->x+array->tw-3 &&
        yp >= y && yp <= y+array->h) break; 
    x += array->tw;
  }

  if (num<0 || num>array->count-1) num = -1;

  *xb = x+array->x;
  *yb = y;
  if (temp) {
    *name  = temp->title;
    *trunc = temp->truncate;
  } else {
    *name = NULL;
  }

  return (num == -1) ? num : temp->count;
}
