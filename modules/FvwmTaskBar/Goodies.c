#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/extensions/shape.h>
#include <X11/xpm.h>

#include <fvwm/fvwmlib.h>

#include "Goodies.h"
#define __Goodies_c__
#include "GoodyLoadable.h"
#include "Mallocs.h"
#include "Colors.h"
#include "FvwmTaskBar.h"


int RenewGoodies = 0;

extern Display *dpy;
extern Window Root, win;
extern int win_width, win_height, win_y, win_border, d_depth,
       ScreenWidth, ScreenHeight, RowHeight; 
extern Pixel back, fore;
extern int Clength;
extern GC blackgc, hilite, shadow, checkered;

GC statusgc, dategc;
XFontStruct *StatusFont;
#ifdef I18N
XFontSet StatusFontset;
#ifdef __STDC__
#define XTextWidth(x,y,z) XmbTextEscapement(x ## set,y,z)
#else
#define XTextWidth(x,y,z) XmbTextEscapement(x/**/set,y,z)
#endif
#define XDrawString(t,u,v,w,x,y,z) XmbDrawString(t,u,StatusFontset,v,w,x,y,z)
#endif
int stwin_width = 100, old_stwin_width = 100, goodies_width = 0;
int anymail, unreadmail, newmail, mailcleared = 0;
int fontheight, clock_width;
int BellVolume = DEFAULT_BELL_VOLUME;
Pixmap mailpix, wmailpix, pmask, pclip, speakerpix, speakeroffpix,s_mask;
XpmAttributes s_attr;
char *DateFore = "black",
     *DateBack = "LightYellow";
int ShowTips = False;
int MaxTipLines = DEFAULT_MAX_TIP_LINES;
char *statusfont_string = "fixed";
int last_date = -1;

int icons_offset=0;

void DrawVolume(void);
void RedrawWindow(int);

#define gray_width  8
#define gray_height 8
extern unsigned char gray_bits[];

/*                x  y  w  h  tw th open type *text   win **lines nlines free*/
TipStruct Tip = { 0, 0, 0, 0,  0, 0,   0,   0, NULL, None,  NULL,     0,  0 };


/* Parse 'goodies' specific resources */
void GoodiesParseConfig(char *tline, char *Module) {
  if(strncasecmp(tline,CatString3(Module, "BellVolume",""),
				Clength+10)==0) {
    BellVolume = atoi(&tline[Clength+11]);
  }  else if(strncasecmp(tline, CatString3(Module, "StatusFont",""),
                          Clength+10)==0) {
    CopyString(&statusfont_string,&tline[Clength+11]);
  } else if(strncasecmp(tline,CatString3(Module, "TipsFore",""),
                               Clength+8)==0) {
    CopyString(&DateFore, &tline[Clength+9]);
  } else if(strncasecmp(tline,CatString3(Module, "TipsBack",""),
                               Clength+8)==0) {
    CopyString(&DateBack, &tline[Clength+9]);
  }  else if(strncasecmp(tline,CatString3(Module, "ShowTips",""),
                               Clength+8)==0) {
    ShowTips = True;
  }  else if(strncasecmp(tline,CatString3(Module, "MaxTipLines",""),
                               Clength+11)==0) {
    MaxTipLines = atoi(&tline[Clength+12]);
  } else if(ParseGLinfo(tline,Module,Clength)){
  }
}

void RestoreGC(void)
{
  XGCValues gcval;
  unsigned long gcmask;

  gcmask = GCForeground | GCBackground | GCFont | GCGraphicsExposures;
  gcval.foreground = fore;
  gcval.background = back;
  gcval.font = StatusFont->fid;
  gcval.graphics_exposures = False;
  XChangeGC(dpy, statusgc, gcmask, &gcval);
}

void InitGoodies() {
  XGCValues gcval;
  unsigned long gcmask;
#ifdef I18N
  char **ml;
  int mc;
  char *ds;
  XFontStruct **fs_list;
#endif
  
#ifdef I18N
  if ((StatusFontset=GetFontSetOrFixed(dpy,statusfont_string))==NULL) {
      ConsoleMessage("Couldn't load statusfont fontset...exiting !\n");
      exit(1);
  }
  XFontsOfFontSet(StatusFontset,&fs_list,&ml);
  StatusFont = fs_list[0];
#else
  if ((StatusFont = GetFontOrFixed(dpy, statusfont_string)) == NULL) {
    ConsoleMessage("Couldn't load statusfont font, exiting...\n");
    exit(1);
  }
#endif

  fontheight = StatusFont->ascent + StatusFont->descent;
  
  gcmask = GCForeground | GCBackground | GCFont | GCGraphicsExposures;
  gcval.foreground = fore;
  gcval.background = back;
  gcval.font = StatusFont->fid;
  gcval.graphics_exposures = False;
  statusgc = XCreateGC(dpy, Root, gcmask, &gcval);
  
  RenewGoodies = 0;
  
  icons_offset = 0;
  LoadableLoad(dpy, win);
  goodies_width += 7;

  stwin_width = goodies_width+icons_offset;

  Tip.lines = (char**)safemalloc(MaxTipLines*sizeof(char*));
}

void Draw3dBox(Window wn, int x, int y, int w, int h)
{
/*  XClearArea(dpy, wn, x, y, w, h, False);*/
  
  XDrawLine(dpy, win, shadow, x, y, x+w-2, y);
  XDrawLine(dpy, win, shadow, x, y, x, y+h-2);
  
  XDrawLine(dpy, win, hilite, x, y+h-1, x+w-1, y+h-1);
  XDrawLine(dpy, win, hilite, x+w-1, y+h-1, x+w-1, y);
}

void DrawGoodies() {
  struct tm *tms;
  static time_t timer;
  unsigned long gcmask;
  XGCValues gcval;

  time(&timer);
  tms = localtime(&timer); 

  stwin_width = goodies_width + icons_offset;
  if (stwin_width != old_stwin_width) {
    old_stwin_width = stwin_width;
    RedrawWindow(1);
  }
  Draw3dBox(win, win_width - stwin_width-1, 0, stwin_width+1, RowHeight);
  icons_offset = 0;
  LoadableDraw(dpy, win);

  gcmask = GCClipMask;;
  gcval.clip_mask = 0; 
  XChangeGC(dpy, statusgc, gcmask, &gcval);

  if (Tip.open) {
    last_date = tms->tm_mday;
    RedrawTipWindow();
  }

  if (RenewGoodies) {
    RenewGoodies = 0;
    DrawGoodies();
    DrawGoodies();
  }     
}

void RedrawTipWindow() {
  if (Tip.text) {
    int i, y;
    y = StatusFont->ascent + StatusFont->descent;
    for (i=0; i<Tip.nlines; i++) {
      XDrawString(dpy, Tip.win, dategc, 3, y,
                  Tip.lines[i], strlen(Tip.lines[i]));
      y += (StatusFont->ascent + StatusFont->descent + 4);
    }
    XRaiseWindow(dpy, Tip.win);  /*****************/
  }
}

void PopupTipWindow(int px, int py, char *text) {
  int newx, newy;
  Window child;
  int tw;
  char *sb;

  if (!ShowTips) return;

  if (Tip.win != None) DestroyTipWindow();

  UpdateString(&Tip.text, text);

  /* parse string, and find width and height */
  /*fprintf(stderr, "PopupTipWindow(): tip text='%s' %d\n", text, MAX_TIP_LINES);*/
  Tip.tw = Tip.th = 0;
  sb = strtok(Tip.text, "\n");
  for (Tip.nlines = 0; Tip.nlines < MaxTipLines; Tip.nlines++) {
    if (sb == NULL) break;
    Tip.lines[Tip.nlines] = sb;
    /* fprintf(stderr, "PopupTipWindow(): parse line='%s'\n", sb); */

    tw = XTextWidth(StatusFont, sb, strlen(sb)) + 6;
    if (tw > Tip.tw) Tip.tw = tw;
    Tip.th += StatusFont->ascent + StatusFont->descent + 4;

    sb = strtok(NULL, "\n");
  }
  if (sb && Tip.nlines == MaxTipLines) {
    Tip.lines[MaxTipLines-1] = "(more...)";
  }
  XTranslateCoordinates(dpy, win, Root, px, py, &newx, &newy, &child);

  Tip.x = newx;
  if (win_y == win_border)
    Tip.y = newy + RowHeight;
  else
    Tip.y = newy - Tip.th -2;

  Tip.w = Tip.tw;
  Tip.h = Tip.th;

  if (Tip.x+Tip.tw+4 > ScreenWidth-5) Tip.x = ScreenWidth-Tip.tw-9;
  if (Tip.x < 5) Tip.x = 5;

  CreateTipWindow(Tip.x, Tip.y, Tip.w, Tip.h);
  if (Tip.open) XMapRaised(dpy, Tip.win);
}

void ShowTipWindow(int open) {
  if (open) {
    if (Tip.win != None) {
      XMapRaised(dpy, Tip.win);
    }
  } else {
    XUnmapWindow(dpy, Tip.win);
  }
  Tip.open = open;
}

void CreateTipWindow(int x, int y, int w, int h) {
  unsigned long gcmask;
  unsigned long winattrmask = CWBackPixel | CWBorderPixel | CWEventMask |
                              CWSaveUnder | CWOverrideRedirect;
  XSetWindowAttributes winattr;
  GC cgc, gc0, gc1;
  XGCValues gcval;
  Pixmap pchk;

  winattr.background_pixel = GetColor(DateBack);
  winattr.border_pixel = GetColor("black");
  winattr.override_redirect = True;
  winattr.save_under = True;
  winattr.event_mask = ExposureMask;

  Tip.win = XCreateWindow(dpy, Root, x, y, w+4, h+4, 0,
                          CopyFromParent, CopyFromParent, CopyFromParent,
                          winattrmask, &winattr);

  gcmask = GCForeground | GCBackground | GCFont | GCGraphicsExposures;
  gcval.graphics_exposures = False;
  gcval.foreground = GetColor(DateFore);
  gcval.background = GetColor(DateBack);
  gcval.font = StatusFont->fid;
  dategc = XCreateGC(dpy, Root, gcmask, &gcval);

  pmask = XCreatePixmap(dpy, Root, w+4, h+4, 1);
  pclip = XCreatePixmap(dpy, Root, w+4, h+4, 1);

  gcmask = GCForeground | GCBackground | GCFillStyle | GCStipple |
           GCGraphicsExposures;
  gcval.foreground = 1;
  gcval.background = 0;
  gcval.fill_style = FillStippled;
  pchk = XCreatePixmapFromBitmapData(dpy, Root, (char *)gray_bits,
                                     gray_width, gray_height, 1, 0, 1);
  gcval.stipple = pchk;
  cgc = XCreateGC(dpy, pmask, gcmask, &gcval);

  gcmask = GCForeground | GCBackground | GCGraphicsExposures | GCFillStyle;
  gcval.graphics_exposures = False;
  gcval.fill_style = FillSolid;
  gcval.foreground = 0;
  gcval.background = 0;
  gc0 = XCreateGC(dpy, pmask, gcmask, &gcval);

  gcval.foreground = 1;
  gcval.background = 1;
  gc1 = XCreateGC(dpy, pmask, gcmask, &gcval);

  XFillRectangle(dpy, pmask, gc0, 0, 0, w+4, h+4);
  XFillRectangle(dpy, pmask, cgc, 3, 3, w+4, h+4);
  XFillRectangle(dpy, pmask, gc1, 0, 0, w+1, h+1);

  XFillRectangle(dpy, pclip, gc0, 0, 0, w+4, h+4);
  XFillRectangle(dpy, pclip, gc1, 1, 1, w-1, h-1);

  XShapeCombineMask(dpy, Tip.win, ShapeBounding, 0, 0, pmask, ShapeSet);
  XShapeCombineMask(dpy, Tip.win, ShapeClip,     0, 0, pclip, ShapeSet);

  XFreeGC(dpy, gc0);
  XFreeGC(dpy, gc1);
  XFreeGC(dpy, cgc);
  XFreePixmap(dpy, pchk);
}

void DestroyTipWindow() {
  XFreePixmap(dpy, pclip);
  XFreePixmap(dpy, pmask);
  XFreeGC(dpy, dategc);
  XDestroyWindow(dpy, Tip.win);
  if (Tip.text) { free(Tip.text); Tip.text = NULL; }
  Tip.win = None;
}

void CheckAndShowTipWindow(int tip_type) {
  if (!Tip.open) {
    ShowTipWindow(1);
    Tip.type = tip_type;
  }
}

void CheckAndDestroyTipWindow(int tip_type) {
  if (Tip.open && Tip.type == tip_type) {
    ShowTipWindow(0);
    if (Tip.win != None) DestroyTipWindow();
    Tip.type = NO_TIP;
  }
}
