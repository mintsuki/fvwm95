#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>

#include <X11/X.h>

#include <fvwm/fvwmlib.h>

#include "GoodyLoadable.h"
#include "FvwmTaskBar.h"

#include <X11/xpm.h>

/*
 * Be careful to avoid name collisions.
 * It's best to name all symbols used for external reference as
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
    int show; /* whether to show the icon */
    time_t lastchecked;
    char *lock;
};
 
void *CheckLockModuleInit(char *id, int k);
int CheckLockModuleParseResource(struct MyInfo *mif,char *tline,char *Module,int Clength);
void CheckLockModuleLoad(struct MyInfo *mif,Display *dpy,Drawable win);
void CheckLockModuleDraw(struct MyInfo *mif,Display *dpy,Window win);
int CheckLockModuleSeeMouse(struct MyInfo *mif,int x,int y);
void CheckLockModuleCreateIconTipWindow_(struct MyInfo *mif);
void CheckLockModuleIconClick(struct MyInfo *mif,XEvent event);

struct GoodyLoadable CheckLockModuleSymbol = {
  (LoadableInit_f)            &CheckLockModuleInit,
  (LoadableParseResource_f)   &CheckLockModuleParseResource,
  (LoadableLoad_f)            &CheckLockModuleLoad,
  (LoadableDraw_f)            &CheckLockModuleDraw,
  (LoadableSeeMouse_f)        &CheckLockModuleSeeMouse,
  (LoadableCreateTipWindow_f) &CheckLockModuleCreateIconTipWindow_,
  (HandleIconClick_f)         &CheckLockModuleIconClick
};

extern int win_width, stwin_width;
extern int RowHeight;

        

void *CheckLockModuleInit(char *id, int k)
{
  struct MyInfo *mif;

#ifdef __DEBUG__
  printf("FvwmTaskBar.CheckLockModule.Init(\"%s\")\n", id); fflush(stdout);
#endif

  mif = (struct MyInfo*)calloc(1, sizeof(struct MyInfo));
  if(mif == NULL) {
    perror("FvwmTaskBar.CheckLockModule.Init()");
    return NULL;
  }
  mif->id = id;
  mif->command = NULL;
  mif->icon = NULL;
  mif->tip = NULL;
  mif->lastclick = 0;
  mif->show = 0; /* don't show up by default */
  mif->lastchecked = 0;
  mif->lock = NULL;

  return mif;
}


void CheckLockModuleSetIcon(struct MyInfo *mif, char *i)
{
  char *path;
#ifdef __DEBUG__
  fprintf(stderr, "FvwmTaskBar.CheckLockModule.AddIcon(*,\"%s\")\n", i);
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

#define SetIcon CheckLockModuleSetIcon


void CheckLockModuleSetIconCommand(struct MyInfo *mif, char *c)
{
  if (mif == NULL) return;
  if (mif->command != NULL) free(mif->command);
  mif->command = c;
}

#define SetIconCommand CheckLockModuleSetIconCommand


void CheckLockModuleSetIconTip(struct MyInfo *mif, char *c)
{
  if (mif == NULL) return;
  if (mif->tip != NULL) free(mif->tip);
  mif->tip = c;
}

#define SetIconTip CheckLockModuleSetIconTip


void CheckLockModuleSetLock(struct MyInfo *mif, char *c)
{
  if (mif == NULL) return;
  if (mif->lock != NULL) free(mif->lock);
  mif->lock = EnvExpand(c);
  free(c);
}

#define SetLock CheckLockModuleSetLock


int CheckLockModuleParseResource(struct MyInfo *mif, char *tline,
                                 char *Module, int Clength)
{
  char *s;

#ifdef __DEBUG__
  printf("FvwmTaskBar.CheckLockModule.ParseResource(\"%s\",\"%s\",*)\n",
         mif->id, tline);
  fflush(stdout);
#endif

  if (mif == NULL) return 0;

  s = (char *) calloc(100, sizeof(char));
  if (s == NULL) {
    perror("FvwmTaskBar.CheckLockModule.ParseGoodyIconResource()");
    return 0;
  }

  if(strncasecmp(tline,CatString3(Module, "CheckLockModuleIcon",mif->id),
                               Clength+19+strlen(mif->id))==0) {
    CopyString(&s, &tline[Clength+20+strlen(mif->id)]);
    SetIcon(mif,s);
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "CheckLockModuleCommand",mif->id),
                               Clength+22+strlen(mif->id))==0) {
    CopyString(&s, &tline[Clength+23+strlen(mif->id)]);
    SetIconCommand(mif,s);
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "CheckLockModuleLockFile",mif->id),
                               Clength+23+strlen(mif->id))==0) {
    CopyString(&s, &tline[Clength+24+strlen(mif->id)]);
    SetLock(mif,s);
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "CheckLockModuleTip",mif->id),
                               Clength+18+strlen(mif->id))==0) {
    CopyString(&s, &tline[Clength+19+strlen(mif->id)]);
    SetIconTip(mif,s);
    return(1);
  } else return 0;
}


void CheckLockModuleLoad(struct MyInfo *mif, Display *dpy, Drawable win)
{
#ifdef __DEBUG__
  fprintf(stderr, "FvwmTaskBar.CheckLockModule.LoadModule()\n");
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
    goodies_width += mif->icon_attr.width+2;
#endif
    mif->offset = icons_offset;
    icons_offset += mif->icon_attr.width+2;

#ifdef __DEBUG__
  fprintf(stderr, "    Loaded %s width=%d height=%d\n",
          mif->icon, mif->icon_attr.width, mif->icon_attr.height);
#endif   

  } else {
    mif->visible = False;
    fprintf(stderr, "FvwmTaskBar.CheckLockModule.LoadModule(): error loading %s\n"
                    "(FvwmTaskBarCheckLockModuleIcon%s)\n",
                    mif->icon, mif->id);
  }
}

void CheckLockModule_check_lock(struct MyInfo *mif)
{
  struct stat buf;
  int lock_status;

  if (mif == NULL) return;
  if (mif->lock == NULL) return;
  if (stat(mif->lock, &buf) < 0)
    lock_status = 0;
  else
    lock_status = 1;

  if (lock_status != mif->show) {
    mif->show = lock_status;
    RenewGoodies = 1;
  }
}


#define check_lock CheckLockModule_check_lock
#define start      (win_width-stwin_width)

extern GC statusgc;

void CheckLockModuleDraw(struct MyInfo *mif, Display *dpy, Window win)
{
  XGCValues gcv;
  time_t now;

  unsigned long CheckLockModulegcm = GCClipMask |
                                     GCClipXOrigin | GCClipYOrigin;
#define gcm CheckLockModulegcm

  if (mif == NULL) return;

  now = time(NULL);
  if (now-mif->lastchecked > 2) {
    mif->lastchecked = now;
    check_lock(mif);
  }
                       
  if (mif->visible && mif->show) {
    XClearArea(dpy, win, win_width-stwin_width+icons_offset, 1,
                         mif->icon_attr.width, RowHeight-2, False);
    gcv.clip_mask = mif->icon_mask;
    gcv.clip_x_origin = start + icons_offset+3;
    gcv.clip_y_origin = ((RowHeight - mif->icon_attr.height) >> 1);

    XChangeGC(dpy, statusgc, gcm, &gcv);      
    XCopyArea(dpy, mif->icon_pix, win, statusgc, 0, 0,
	      mif->icon_attr.width, mif->icon_attr.height,
	      gcv.clip_x_origin,
	      gcv.clip_y_origin);

    mif->offset = icons_offset;
    icons_offset += mif->icon_attr.width+2;
  }

}
         
         
int CheckLockModuleSeeMouse(struct MyInfo *mif, int x, int y)
{
  int xl, xr;

  if (mif == NULL) return 0;
  if (mif->show == 0) return 0;
  xl = win_width - stwin_width + mif->offset;
  xr = win_width - stwin_width + mif->offset + mif->icon_attr.width;
  return (x>=xl && x<xr && y>1 && y<RowHeight-2);
}


void CheckLockModuleCreateIconTipWindow_(struct MyInfo *mif)
{
  if (mif == NULL) return;
  if (mif->tip == NULL) return;
  PopupTipWindow(win_width - stwin_width + mif->offset, 0, mif->tip);
}

#undef __DEBUG__

void CheckLockModuleIconClick(struct MyInfo *mif, XEvent event)
{
  if (mif == NULL) return;
  if (mif->command == NULL) return;
  if (event.xbutton.time - mif->lastclick < 250) {
    SendFvwmPipe(mif->command, 0);
    #ifdef __DEBUG__
    printf("\"%s\"\n",mif->command); fflush(stdout);
    #endif
  }
  mif->lastclick = event.xbutton.time;
}
