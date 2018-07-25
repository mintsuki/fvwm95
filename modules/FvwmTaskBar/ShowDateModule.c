#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>

#include <fvwm/fvwmlib.h>

#include "Goodies.h"
#include "GoodyLoadable.h"
#include "Mallocs.h"
#include "FvwmTaskBar.h"

#include <X11/xpm.h>

struct DateInfo {
    char *id;
    char *clockfmt;
    char *command;
    char *tip;
    char *hourlycommand;
    Time lastclick;
    int clock_width;
    int fontheight;
    int last_date;
    int my_offset;
};

/* let's ensure unique symbols */
#define dateLoad      ShowDateModuleLoad
#define dateSymbol    ShowDateModuleSymbol
#define dateInit      ShowDateModuleInit
#define dateDraw      ShowDateModuleDraw
#define dateInMouse   ShowDateModuleSeeMouse
#define dateClick     ShowDateModuleHandleIconClick
#define dateTipWindow ShowDateModuleCreateTipWindow
#define dateParse     ShowDateModuleParseResource

#define SetIconTip     ShowDateModuleSetIconTip
#define SetIconCommand ShowDateModuleSetIconCommand

void *dateInit(char *id, int k);
int dateParse(struct DateInfo *v,char *tline, char *Module,int Clength) ;
void dateLoad(struct DateInfo *v,Display *dpy,Drawable win) ;
void dateDraw(struct DateInfo *v,Display *dpy,Drawable win) ;
int dateInMouse(struct DateInfo *v,int x, int y) ;
void dateClick(struct DateInfo *mif,XEvent event); 
void dateTipWindow(struct DateInfo *v);


struct GoodyLoadable dateSymbol = {
  (LoadableInit_f)            &dateInit,
  (LoadableParseResource_f)   &dateParse,
  (LoadableLoad_f)            &dateLoad,
  (LoadableDraw_f)            &dateDraw,
  (LoadableSeeMouse_f)        &dateInMouse,
  (LoadableCreateTipWindow_f) &dateTipWindow,
  (HandleIconClick_f)         &dateClick
};


extern int RowHeight;

extern Display *dpy;
extern Window Root, win;
extern int win_width, win_height, win_y, win_border, d_depth,
       ScreenWidth, ScreenHeight, RowHeight; 
extern Pixel back, fore;
extern GC blackgc, hilite, shadow, checkered;
extern  XFontStruct *StatusFont;
char *StatusFont_string = "fixed";

extern GC dategc;
extern GC statusgc;

Pixmap pmask, pclip;


#define gray_width  8
#define gray_height 8
extern unsigned char gray_bits[];



void *dateInit(char *id, int k)
{
  struct DateInfo *p;

  p = (struct DateInfo*)calloc(1, sizeof(struct DateInfo));
  if (p == NULL) {
    perror("FvwmTaskBar.ShowDateModule.dateInit()");
    return NULL;
  }
  p->id = id;
  p->clockfmt = NULL;
  p->command = NULL;
  p->last_date = -1;
  p->tip = NULL;
  p->hourlycommand = NULL;
  return p;
}


void SetIconTip(struct DateInfo *mif, char *c)
{
  if (mif == NULL) return;
  if (mif->tip != NULL) free(mif->tip);
  mif->tip = c;
}

void SetIconCommand(struct DateInfo *mif, char *c)
{
  if (mif == NULL) return;
  if (mif->command != NULL) free(mif->command);
  mif->command = c;
}


/* Parse 'goodies' specific resources */
int dateParse(struct DateInfo *v, char *tline, char *Module, int Clength) 
{
  char *s;

  if(strncasecmp(tline,CatString3(Module, "ShowDateModuleClockFormat",v->id),
			  Clength+25+strlen(v->id))==0) {
    UpdateString(&(v->clockfmt), &tline[Clength+26+strlen(v->id)]);
    if(v->clockfmt[strlen(v->clockfmt)-1] == '\n')
      v->clockfmt[strlen(v->clockfmt)-1] = 0;
    return 1;
  } else if(strncasecmp(tline, CatString3(Module, "ShowDateModuleStatusFont",v->id),
                          Clength+24+strlen(v->id))==0) {
    CopyString(&(StatusFont_string),&tline[Clength+25+strlen(v->id)]);
    return 1;
  } else if(strncasecmp(tline, CatString3(Module, "ShowDateModuleCommand",v->id),
                               Clength+21+strlen(v->id))==0) {                                   
    CopyString(&s, &tline[Clength+22+strlen(v->id)]);
    SetIconCommand(v,s);
    return(1);
  } else if (strncasecmp(tline, CatString3(Module, "ShowDateModuleHourlyCommand",v->id),
			 Clength+27+strlen(v->id))==0) {                                   
    CopyString(&(v->hourlycommand), &tline[Clength+28+strlen(v->id)]);
    return(1);
  } else return 0;  
}


void dateLoad(struct DateInfo *v,Display *dpy,Drawable win) 
{
  if ((StatusFont = XLoadQueryFont(dpy, StatusFont_string)) == NULL) {
    if ((StatusFont = XLoadQueryFont(dpy, "fixed")) == NULL) {
      ConsoleMessage("FvwmTaskBar.ShowDateModule.dateLoad():Couldn't load fixed font.\n");
      StatusFont=NULL;
      return;
    }
  }

  v->fontheight = StatusFont->ascent + StatusFont->descent;
  
  /*statusgc = XCreateGC(dpy, Root, gcmask, &gcval);*/
  
  if (v->clockfmt) {
    struct tm *tms;
    static time_t timer;
    static char str[24];
    time(&timer);
    tms = localtime(&timer);
    strftime(str, 24,v->clockfmt, tms);
    v->clock_width = XTextWidth(StatusFont, str, strlen(str)) + 4;
  }
  else v->clock_width = XTextWidth(StatusFont, "XX:XX", 5) + 4;
  v->my_offset=icons_offset;
  icons_offset += v->clock_width;
 }

void Draw3dBox(Window wn, int x, int y, int w, int h);


void dateDraw(struct DateInfo *v,Display *dpy,Drawable win) {
  struct tm *tms;
  static char str[40];
  static time_t timer;
  static int last_hour = -1;
  unsigned long gcmask;
  XGCValues gcval;

  time(&timer);
  tms = localtime(&timer);
  if (v->clockfmt) {
    strftime(str, 24, v->clockfmt, tms);
    if (str[0] == '0') str[0]=' ';
  } else {
    strftime(str, 15, "%R", tms);
  }
  XClearArea(dpy, win, win_width-stwin_width+icons_offset, 1, v->clock_width, RowHeight-2, False);
  gcmask = GCForeground | GCBackground | GCFont | GCGraphicsExposures | GCClipMask;
  gcval.foreground = fore;
  gcval.background = back;
  gcval.font = StatusFont->fid;
  gcval.graphics_exposures = False;
  gcval.clip_mask=0;

  XChangeGC(dpy, statusgc, gcmask, &gcval);

  XDrawString(dpy,win,statusgc,
	      win_width - stwin_width +icons_offset+4,
	      ((RowHeight - v->fontheight) >> 1) +StatusFont->ascent,
	      str, strlen(str));
  v->my_offset=icons_offset;
  icons_offset+=v->clock_width;

  if (v->hourlycommand) {
    if (tms->tm_min == 0 && tms->tm_hour != last_hour) {
      last_hour = tms->tm_hour;
      SendFvwmPipe(v->hourlycommand, 0);
    }
  }

/*
  if (Tip.open) {
    if (Tip.type == DATE_TIP)
      if (tms->tm_mday != v->last_date) / * reflect date change * /
        dateTipWindow(v); / * This automatically deletes any old window * /
    v->last_date = tms->tm_mday;
  }
 */ 


}

int dateInMouse(struct DateInfo *v,int x, int y) {
  int clockl = win_width - stwin_width+v->my_offset;
  int clockr = win_width - stwin_width +v->my_offset+ v->clock_width;
  return (x>=clockl && x<clockr && y>1 && y<RowHeight-2);
}


void dateTipWindow(struct DateInfo *v) {
  struct tm *tms;
  static time_t timer;
  static char str[40];

  time(&timer);
  tms = localtime(&timer);
  strftime(str, 40, "%A, %B %d, %Y", tms);
  v->last_date = tms->tm_mday;

  PopupTipWindow(win_width, 0, str);
}


void dateClick(struct DateInfo *mif, XEvent event)
{
  if (mif == NULL) return;
  if (mif->command == NULL) return;
  if (event.xbutton.time - mif->lastclick < 250) {
    SendFvwmPipe(mif->command, 0);
#ifdef __DEBUG__
    printf("\"%s\"\n",mif->command);fflush(stdout);
#endif
  }
  mif->lastclick = event.xbutton.time;
}
