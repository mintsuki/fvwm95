#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/X.h>

#include <fvwm/fvwmlib.h>

#include "GoodyLoadable.h"
#include "FvwmTaskBar.h"

#include <X11/xpm.h>

/*
 * Be careful to avoid name collisions.
 * It's better to name all symbols used for external reference as
 * ModuleNameSymbol, where ModuleName is the name of the module
 * and Symbol is the name you would rather use.
 * More then one module may want to use a proc called Load !
 */

#include <unistd.h>
extern char *IconPath, *PixmapPath;

struct MyInfo {
    char *id;
    /* other stuff */
    char *command;
    char *tip;
    char *icon;
    Pixmap icon_pix;
    Pixmap icon_mask;
    XpmAttributes icon_attr;
    int offset;
    int visible;
    Time lastclick;
};
 

void *GoodyModuleInit(char *id, int k);
int GoodyModuleParseResource(struct MyInfo *mif,char *tline,char *Module,int Clength);
void GoodyModuleLoad(struct MyInfo *mif,Display *dpy,Drawable win);
void GoodyModuleDraw(struct MyInfo *mif,Display *dpy,Window win);
int GoodyModuleSeeMouse(struct MyInfo *mif,int x,int y);
void GoodyModuleCreateIconTipWindow_(struct MyInfo *mif);
void GoodyModuleIconClick(struct MyInfo *mif,XEvent event);

struct GoodyLoadable GoodyModuleSymbol = {
  (LoadableInit_f)            &GoodyModuleInit,
  (LoadableParseResource_f)   &GoodyModuleParseResource,
  (LoadableLoad_f)            &GoodyModuleLoad,
  (LoadableDraw_f)            &GoodyModuleDraw,
  (LoadableSeeMouse_f)        &GoodyModuleSeeMouse,
  (LoadableCreateTipWindow_f) &GoodyModuleCreateIconTipWindow_,
  (HandleIconClick_f)         &GoodyModuleIconClick
};

extern int win_width,stwin_width;
extern int RowHeight;

        

void *GoodyModuleInit(char *id, int k)
{
  struct MyInfo *mif;

#ifdef __DEBUG__
  printf("FvwmTaskBar.GoodyModule.Init(\"%s\")\n", id);
  fflush(stdout);
#endif

  mif = (struct MyInfo*)calloc(1, sizeof(struct MyInfo));
  if (mif == NULL) {
    perror("FvwmTaskBar.GoodyModule.Init()");
    return NULL;
  }

  mif->id = id;
  mif->command = NULL;
  mif->icon = NULL;
  mif->tip = NULL;
  mif->lastclick = 0;
  return mif;
}


void GoodyModuleSetIcon(struct MyInfo *mif, char *i)
{
  char *path;
#ifdef __DEBUG__
  fprintf(stderr, "FvwmTaskBar.GoodyModule.AddIcon(*,\"%s\")\n", i);
#endif
  if ((path=findIconFile(i,PixmapPath,R_OK)) ||
      (path=findIconFile(i,IconPath,R_OK))) {
    free(i);
    i = path;
  }

  if (mif == NULL) return;
  if (mif->icon != NULL) free(mif->icon);
  mif->icon = i;
}


#define SetIcon GoodyModuleSetIcon

void GoodyModuleSetIconCommand(struct MyInfo *mif, char *c)
{
  if (mif == NULL) return;
  if (mif->command != NULL) free(mif->command);
  mif->command = c;
}


#define SetIconCommand GoodyModuleSetIconCommand

void GoodyModuleSetIconTip(struct MyInfo *mif, char *c)
{
  if (mif == NULL) return;
  if (mif->tip != NULL) free(mif->tip);
  mif->tip = c;
}


#define SetIconTip GoodyModuleSetIconTip

int GoodyModuleParseResource(struct MyInfo *mif, char *tline,
                             char *Module, int Clength)
{
  char *s;

#ifdef __DEBUG__
  printf("FvwmTaskBar.GoodyModule.ParseResource(\"%s\",\"%s\",*)\n",
         mif->id, tline);
  fflush(stdout);
#endif

  if (mif == NULL) return 0;

  s = (char *) calloc(100, sizeof(char));
  if (s == NULL) {
    perror("FvwmTaskBar.GoodyModule.ParseGoodyIconResource()");
    return 0;
  }

  if(strncasecmp(tline,CatString3(Module, "GoodyModuleIcon",mif->id),
                               Clength+15+strlen(mif->id))==0) {                                   
    CopyString(&s, &tline[Clength+16+strlen(mif->id)]);
    SetIcon(mif,s);
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "GoodyModuleCommand",mif->id),
                               Clength+18+strlen(mif->id))==0) {                                   
    CopyString(&s, &tline[Clength+19+strlen(mif->id)]);
    SetIconCommand(mif,s);
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "GoodyModuleTip",mif->id),
                               Clength+14+strlen(mif->id))==0) {                                   
    CopyString(&s, &tline[Clength+15+strlen(mif->id)]);
    SetIconTip(mif,s);
    return(1);
  } else return 0;
}



void GoodyModuleLoad(struct MyInfo *mif, Display *dpy, Drawable win)
{
#ifdef __DEBUG__
  fprintf(stderr, "FvwmTaskBar.GoodyModule.LoadModule()\n");
#endif

  if (XpmReadFileToPixmap(dpy, win, mif->icon,
                          &(mif->icon_pix), &(mif->icon_mask),
                          &(mif->icon_attr)) == XpmSuccess) {
    mif->visible = True;
    if ((mif->icon_attr.valuemask & XpmSize) == 0) {
      mif->icon_attr.width = 16;
      mif->icon_attr.height = 16;
    }
#if 0  /* does not appear to be needed */
    goodies_width += mif->icon_attr.width + 2;
#endif
    mif->offset = icons_offset;
    icons_offset += mif->icon_attr.width + 2;

#ifdef __DEBUG__
    fprintf(stderr, "    Loaded %s width=%d height=%d\n",
            mif->icon, mif->icon_attr.width, mif->icon_attr.height);
#endif   

  } else {
    mif->visible = False;
    fprintf(stderr, "FvwmTaskBar.GoodyModule.LoadModule(): error loading %s\n"
                    "(FvwmTaskBarGoodyModuleIcon%s\n",
            mif->icon, mif->id);
  }
}


#define start (win_width-stwin_width)

extern GC statusgc;

void GoodyModuleDraw(struct MyInfo *mif, Display *dpy, Window win)
{
  XGCValues gcv;
  unsigned long GoodyModulegcm = GCClipMask |
                                 GCClipXOrigin | GCClipYOrigin;
#define gcm GoodyModulegcm

  if (mif == NULL) return;

  if (mif->visible) {
    gcv.clip_mask = mif->icon_mask;
    gcv.clip_x_origin = start + icons_offset+3;
    gcv.clip_y_origin = ((RowHeight - mif->icon_attr.height) >> 1);

    XChangeGC(dpy,statusgc,gcm,&gcv);      
    XCopyArea(dpy, mif->icon_pix, win, statusgc, 0, 0,
              mif->icon_attr.width, mif->icon_attr.height,
              gcv.clip_x_origin, gcv.clip_y_origin);

    mif->offset = icons_offset;
    icons_offset += mif->icon_attr.width+2;
  }
}


int GoodyModuleSeeMouse(struct MyInfo *mif, int x, int y)
{
  int xl, xr;

  if (mif == NULL) return 0;
  xl = win_width-stwin_width+mif->offset;
  xr = win_width-stwin_width+mif->offset + mif->icon_attr.width;
  return (x>=xl && x<xr && y>1 && y<RowHeight-2);
}


void GoodyModuleCreateIconTipWindow_(struct MyInfo *mif)
{
  if (mif == NULL) return;
  if (mif->tip == NULL) return;
  PopupTipWindow(win_width-stwin_width+mif->offset, 0, mif->tip );
}

#undef __DEBUG__

void GoodyModuleIconClick(struct MyInfo *mif, XEvent event)
{
  if (mif == NULL) return;
  if (mif->command == NULL) return;
  if (event.xbutton.time - mif->lastclick < 250) {
    SendFvwmPipe(mif->command, 0);
#ifdef __DEBUG__
    printf("\"%s\"\n", mif->command);
    fflush(stdout);
#endif
  }
  mif->lastclick = event.xbutton.time;
}

