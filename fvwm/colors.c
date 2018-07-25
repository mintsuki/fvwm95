/****************************************************************************
 * This module is all new
 * by Rob Nation 
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/


#include <FVWMconfig.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>

#include <X11/Xproto.h>
#include <X11/Xatom.h>


#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include <fvwm/version.h>

char *white = "white";
char *black = "black";

extern char *Hiback;
extern char *Hifore;

#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#define max(a,b) (((a)>(b)) ? (a) : (b))
#endif

/****************************************************************************
 *
 * This routine computes the shadow color from the background color
 *
 ****************************************************************************/
Pixel GetShadow(Pixel background) 
{
  XColor bg_color;
  XWindowAttributes attributes;
/*  unsigned int r,g,b; */
  
  XGetWindowAttributes(dpy,Scr.Root,&attributes);
  
  bg_color.pixel = background;
  XQueryColor(dpy,attributes.colormap,&bg_color);
  
/*
  r = bg_color.red % 0xffff;
  g = bg_color.green % 0xffff;
  b = bg_color.blue % 0xffff;
*/
  
  bg_color.red = (unsigned short)((bg_color.red*60)/100); /* was 50% */
  bg_color.green = (unsigned short)((bg_color.green*60)/100);
  bg_color.blue = (unsigned short)((bg_color.blue*60)/100);

/*
  r = r >>1; 
  g = g >>1;
  b = b >>1;
  
  bg_color.red = r;
  bg_color.green = g;
  bg_color.blue = b;
*/

  if(!XAllocColor(dpy,attributes.colormap,&bg_color))
    {
      fvwm_msg(ERR,"GetShadow","can't allocate shadow color");
      bg_color.pixel = background;
    }
  
  return bg_color.pixel;
}

/****************************************************************************
 *
 * This routine computes the hilight color from the background color
 *
 ****************************************************************************/
Pixel GetHilite(Pixel background) 
{
  XColor bg_color, white_p;
  XWindowAttributes attributes;
  
  XGetWindowAttributes(dpy,Scr.Root,&attributes);
  
  bg_color.pixel = background;
  XQueryColor(dpy,attributes.colormap,&bg_color);

  white_p.pixel = GetColor(white);
  XQueryColor(dpy,attributes.colormap,&white_p);
  
  bg_color.red = max((white_p.red/5), bg_color.red);
  bg_color.green = max((white_p.green/5), bg_color.green);
  bg_color.blue = max((white_p.blue/5), bg_color.blue);
  
  bg_color.red = min(white_p.red, (bg_color.red*140)/100);
  bg_color.green = min(white_p.green, (bg_color.green*140)/100);
  bg_color.blue = min(white_p.blue, (bg_color.blue*140)/100);

  if(!XAllocColor(dpy,attributes.colormap,&bg_color))
    {
      fvwm_msg(ERR,"GetHilite","can't allocate hilight color");
      bg_color.pixel = background;
    }
  return bg_color.pixel;
}

#undef min
#ifdef max
#undef max
#endif
  

/***********************************************************************
 *
 *  Procedure:
 *	CreateGCs - open fonts and create all the needed GC's.  I only
 *		    want to do this once, hence the first_time flag.
 *
 ***********************************************************************/
void CreateGCs(void)
{
  XGCValues gcv;
  unsigned long gcm;
  
  /* create scratch GC's */
  gcm = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth;
  gcv.line_width = 0;
  gcv.function = GXcopy;
  gcv.plane_mask = AllPlanes;
  gcv.graphics_exposures = False;
  
  Scr.ScratchGC1 = XCreateGC(dpy, Scr.Root, gcm, &gcv);
  Scr.ScratchGC2 = XCreateGC(dpy, Scr.Root, gcm, &gcv);
  Scr.ScratchGC3 = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcm = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
        GCBackground|GCFillStyle;
  gcv.function = GXcopy;
  gcv.fill_style = FillSolid;
  gcv.plane_mask = AllPlanes;
  gcv.graphics_exposures = False;
  gcv.line_width = 0;
  gcv.foreground = 
  gcv.background = GetColor(black);
  Scr.BlackGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  
}


/****************************************************************************
 * 
 * Loads a single color
 *
 ****************************************************************************/ 
Pixel GetColor(char *name)
{
  XColor color;

  color.pixel = 0;
  if (!XParseColor (dpy, Scr.FvwmRoot.attr.colormap, name, &color)) 
    {
      fvwm_msg(ERR,"GetColor","can't parse color %s", name);
    }
  else if(!XAllocColor (dpy, Scr.FvwmRoot.attr.colormap, &color)) 
    {
      fvwm_msg(ERR,"GetColor","can't allocate color %s", name);
    }
  return color.pixel;
}
