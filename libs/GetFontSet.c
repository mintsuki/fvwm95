/* This file brings from GetFont.c */

#include <FVWMconfig.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlocale.h>

#include <fvwm/fvwmlib.h>

XFontSet GetFontSetOrFixedCLocale(Display *disp, char *fontname);
/*
** loads fontset or "fixed" on failure
*/
XFontSet GetFontSetOrFixed(Display *disp, char *fontname)
{
  XFontSet fontset;
  char **ml;
  int mc;
  char *ds;

  if ((fontset = XCreateFontSet(disp,fontname,&ml,&mc,&ds))==NULL)
  {
    fprintf(stderr,
            "[GetFontSetOrFixed]: WARNING -- can't get fontset %s, trying 'fixed'\n",
            fontname);
    /* fixed should always be avail, so try that */
    if ((fontset = XCreateFontSet(disp,"fixed",&ml,&mc,&ds))==NULL) 
    {
      fprintf(stderr,"[GetFontSetOrFixed]: ERROR -- can't get fontset 'fixed'\n");
      fprintf(stderr,"[GetFontSetOrFixed]: Trying C locale as last resort\n");
      fontset = GetFontSetOrFixedCLocale(disp, fontname);
    }
  }
  return fontset;
}

XFontSet GetFontSetOrFixedCLocale(Display *disp, char *fontname)
{
  XFontSet fontset;
  char **ml;
  int mc;
  char *ds;

  setlocale(LC_CTYPE, "C");
  if ((fontset = XCreateFontSet(disp,fontname,&ml,&mc,&ds))==NULL)
  {
    fprintf(stderr,
            "[GetFontSetOrFixedCLocale]: WARNING -- can't get fontset %s, trying 'fixed'\n",
            fontname);
    /* fixed should always be avail, so try that */
    if ((fontset = XCreateFontSet(disp,"fixed",&ml,&mc,&ds))==NULL)
    {
      fprintf(stderr,"[GetFontSetOrFixedCLocale]: ERROR -- can't get fontset 'fixed'\n");
    }
  }
  setlocale(LC_CTYPE, "");
  return fontset;
}

