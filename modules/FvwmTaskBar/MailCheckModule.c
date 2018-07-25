#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>

#include <X11/X.h>

#include <fvwm/fvwmlib.h>
#include "GoodyLoadable.h"
#include "Goodies.h"
#include "Mallocs.h"
#include "FvwmTaskBar.h"

#include <X11/xpm.h>

/* Be careful to avoid name collisions.
 * It's best to name all symbols used for external reference as
 * MailCheckModuleNameSymbol, where MailCheckModuleName is the name of the
 * MailCheckModule and Symbol is the name you would rather use.
 * More then one MailCheckModule may want to use a proc called Load !
 */

#include <unistd.h>
extern char *IconPath, *PixmapPath;

#define HDR_DELIM "From "
#define HDR_DELIM_LEN 5
#define HDR_FROM  "From: "
#define HDR_FROM_LEN 6
#define HDR_SUBJ  "Subject: "
#define HDR_SUBJ_LEN 9
typedef struct MailHdr_s {
  char *sbFrom, *sbSubj;
  struct MailHdr_s *pNext;
} MailHdr_S;

#define DEFAULT_MAILTIP_FORMAT0 1
#define DEFAULT_MAILTIP_FORMAT1 0
#define DEFAULT_MAILTIP_FORMAT2 20
#define DEFAULT_MAILTIP_FORMAT3 2
#define DEFAULT_MAILTIP_FORMAT4 30

#define eDisabled  0
#define eAnyChange 1
#define eBigger    2
#define eTouched   3

#define HasNoMail      0
#define HasMail        1
#define HasUnreadMail  2
#define HasNewMail     4
#define HasChangedMail 8

struct MyInfo{
         char *id;
/* other stuff */
         int   GoodyNum;
         char *command;  /* action to execute on double click */
         char *NewMailcommand; /* action to execute when mail arrives */

         char *icon;  /* icon to show when there is mail */
         char *tip;
         Pixmap icon_pix;
         Pixmap icon_mask;
         XpmAttributes icon_attr;      

         int AutoMailTip;
         int MailTipFormat[5];
         char *MailBuf;
         int  fMailBufChanged;
         char *MailHeaderTipText;
         int MailTipUnblankScreen;
         int NoSmartFrom;
         Display *dpy;

         char *newicon; /* icon to show when there are new mail */
         char *newtip;
         Pixmap newicon_pix;
         Pixmap newicon_mask;
         XpmAttributes newicon_attr;

         char *unreadicon; /* icon to show when there is unread mail */
         char *unreadtip;
         Pixmap unreadicon_pix;
         Pixmap unreadicon_mask;
         XpmAttributes unreadicon_attr;
         
         int offset;
         int visible;
         Time lastclick;
         int show; /* status of mailbox. 0 not to show anything */
         time_t lastchecked;
         char *lock;
         off_t mailsize;         
};
 
static int DoAutoMailTip(struct MyInfo *mif, int fForce);

void MailCheckModule_getstatus(struct MyInfo *);


void *MailCheckModuleInit(char *id, int);
int MailCheckModuleParseResource(struct MyInfo *mif,char *tline,char *MailCheckModule,int Clength);
void MailCheckModuleLoad(struct MyInfo *mif,Display *dpy,Drawable win);
void MailCheckModuleDraw(struct MyInfo *mif,Display *dpy,Window win);
int MailCheckModuleSeeMouse(struct MyInfo *mif,int x,int y);
void MailCheckModuleCreateIconTipWindow_(struct MyInfo *mif);
void MailCheckModuleIconClick(struct MyInfo *mif,XEvent event);

struct GoodyLoadable MailCheckModuleSymbol = {
  (LoadableInit_f)            &MailCheckModuleInit,
  (LoadableParseResource_f)   &MailCheckModuleParseResource,
  (LoadableLoad_f)            &MailCheckModuleLoad,
  (LoadableDraw_f)            &MailCheckModuleDraw,
  (LoadableSeeMouse_f)        &MailCheckModuleSeeMouse,
  (LoadableCreateTipWindow_f) &MailCheckModuleCreateIconTipWindow_,
  (HandleIconClick_f)         &MailCheckModuleIconClick
};

extern int win_width,stwin_width;
extern int RowHeight;
extern Display *dpy;
        
void *MailCheckModuleInit(char *id, int k)
{
  struct MyInfo *mif;

#ifdef __DEBUG__
  printf("FvwmTaskBar.MailCheckModule.Init(\"%s\")\n", id);
  fflush(stdout);
#endif

  mif = (struct MyInfo*)calloc(1, sizeof(struct MyInfo));
  if (mif == NULL) {
    perror("FvwmTaskBar.MailCheckModule.Init()");
    return NULL;
  }
  mif->id = id;
  mif->GoodyNum = k;
  mif->command = NULL;
  mif->NewMailcommand = NULL;
  mif->icon = NULL;
  mif->AutoMailTip = eDisabled;
  mif->MailTipFormat[0] = DEFAULT_MAILTIP_FORMAT0;
  mif->MailTipFormat[1] = DEFAULT_MAILTIP_FORMAT1;
  mif->MailTipFormat[2] = DEFAULT_MAILTIP_FORMAT2;
  mif->MailTipFormat[3] = DEFAULT_MAILTIP_FORMAT3;
  mif->MailTipFormat[4] = DEFAULT_MAILTIP_FORMAT4;
  mif->MailBuf = NULL;
  mif->fMailBufChanged = False;
  mif->MailHeaderTipText = NULL;
  mif->MailTipUnblankScreen = False;
  mif->NoSmartFrom = False;
  mif->newicon = NULL;
  mif->unreadicon = NULL;
  mif->tip = "You have mail";
  mif->newtip = "You have new mail";
  mif->unreadtip = "You have unread mail";
  mif->lastclick = 0;
  mif->show = HasNoMail; /* don't show by default */
  mif->lastchecked = 0;
  mif->lock = NULL;
  return mif;
}


void MailCheckModuleSetIcon(struct MyInfo *mif, char *i)
{
  char *path;
#ifdef __DEBUG__
  fprintf(stderr, "FvwmTaskBar.MailCheckModule.AddIcon(*,\"%s\")\n", i);
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


#define SetIcon MailCheckModuleSetIcon

void MailCheckModuleSetNewIcon(struct MyInfo *mif, char *i)
{
  char *path;
#ifdef __DEBUG__
  fprintf(stderr, "FvwmTaskBar.MailCheckModule.AddIcon(*,\"%s\")\n", i);
#endif
  if ((path=findIconFile(i,PixmapPath,R_OK)) ||
      (path=findIconFile(i,IconPath,R_OK))) {
    free(i);
    i = path;
  }

  if (mif == NULL) return;
  if (mif->newicon != NULL) free(mif->newicon);
  mif->newicon = i;
}


#define SetNewIcon MailCheckModuleSetNewIcon

void MailCheckModuleSetUnreadIcon(struct MyInfo *mif, char *i)
{
  char *path;
#ifdef __DEBUG__
  fprintf(stderr, "FvwmTaskBar.MailCheckModule.AddIcon(*,\"%s\")\n", i);
#endif
  if ((path=findIconFile(i,PixmapPath,R_OK)) ||
      (path=findIconFile(i,IconPath,R_OK))) {
    free(i);
    i = path;
  }

  if (mif == NULL) return;
  if (mif->unreadicon != NULL) free(mif->unreadicon);
  mif->unreadicon = i;
}


#define SetUnreadIcon MailCheckModuleSetUnreadIcon

void MailCheckModuleSetIconCommand(struct MyInfo *mif, char *c)
{
  if (mif == NULL) return;
  if (mif->command != NULL) free(mif->command);
  mif->command = c;
}


#define SetIconCommand MailCheckModuleSetIconCommand

void MailCheckModuleSetNewMailCommand(struct MyInfo *mif, char *c)
{
  if (mif == NULL) return;
  if (mif->NewMailcommand != NULL) free(mif->NewMailcommand);
  mif->NewMailcommand = c;
}

#define SetNewMailCommand MailCheckModuleSetNewMailCommand


void MailCheckModuleSetIconTip(struct MyInfo *mif, char *c)
{
  if (mif == NULL) return;
  /*if (mif->tip != NULL) free(mif->tip);*/
  mif->tip = c;
}

#define SetIconTip MailCheckModuleSetIconTip


void MailCheckModuleSetLock(struct MyInfo *mif, char *c)
{
  if (mif == NULL) return;
  if (mif->lock != NULL) free(mif->lock);
  mif->lock = EnvExpand(c);
  free(c);
}

#define SetLock MailCheckModuleSetLock


int MailCheckModuleParseResource(struct MyInfo *mif, char *tline,
                                 char *Module, int Clength)
{
  char *s;

#ifdef __DEBUG__
  printf("FvwmTaskBar.MailCheckModule.ParseResource(\"%s\",\"%s\",*)\n",
         mif->id,tline);
  fflush(stdout);
#endif

  if (mif == NULL) return 0;
  s = (char *) calloc(256, sizeof(char));
  if (s == NULL) {
    perror("FvwmTaskBar.MailCheckModule.ParseGoodyIconResource()");
    return 0;
  }

  if(strncasecmp(tline,CatString3(Module, "MailCheckModuleMailIcon",mif->id),
                               Clength+23+strlen(mif->id))==0) {
    CopyString(&s, &tline[Clength+24+strlen(mif->id)]);
    SetIcon(mif,s);
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "MailCheckModuleUnreadMailIcon",mif->id),
                               Clength+29+strlen(mif->id))==0) {
    CopyString(&s, &tline[Clength+30+strlen(mif->id)]);
    SetUnreadIcon(mif,s);
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "MailCheckModuleNewMailIcon",mif->id),
                               Clength+26+strlen(mif->id))==0) {
    CopyString(&s, &tline[Clength+27+strlen(mif->id)]);
    SetNewIcon(mif,s);
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "MailCheckModuleCommand",mif->id),
                               Clength+22+strlen(mif->id))==0) {
    CopyString(&s, &tline[Clength+23+strlen(mif->id)]);
    SetIconCommand(mif,s);
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "MailCheckModuleNewMailCommand",mif->id),
                               Clength+29+strlen(mif->id))==0) {
    CopyString(&s, &tline[Clength+30+strlen(mif->id)]);
    SetNewMailCommand(mif,s);
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "MailCheckModuleMailFile",mif->id),
                               Clength+23+strlen(mif->id))==0) {
    CopyString(&s, &tline[Clength+24+strlen(mif->id)]);
    SetLock(mif,s);
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "MailCheckModuleTip",mif->id),
                               Clength+18+strlen(mif->id))==0) {
    CopyString(&s, &tline[Clength+19+strlen(mif->id)]);
    SetIconTip(mif,s);
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "MailCheckModuleNewMailTip",mif->id),
                               Clength+25+strlen(mif->id))==0) {
    CopyString(&s, &tline[Clength+26+strlen(mif->id)]);
    mif->newtip=s;
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "MailCheckModuleUnreadMailTip",mif->id),
                               Clength+28+strlen(mif->id))==0) {
    CopyString(&s, &tline[Clength+29+strlen(mif->id)]);
    mif->unreadtip=s;
    return(1);
#if 1
  } else if(strncasecmp(tline,CatString3(Module, "MailCheckModuleAutoMailTip",mif->id),
			Clength+26+strlen(mif->id))==0) {
    if (strcasecmp(&tline[Clength+27+strlen(mif->id)],
		   "MailFileTouched") == 0) {
      mif->AutoMailTip = eTouched;
    } else if (strcasecmp(&tline[Clength+27+strlen(mif->id)],
			  "MailFileBigger") == 0) {
      mif->AutoMailTip = eBigger;
    } else if (strcasecmp(&tline[Clength+27+strlen(mif->id)],
			  "MailFileAnyChange") == 0) {
      mif->AutoMailTip = eAnyChange;
    } else {
      mif->AutoMailTip = eAnyChange;
    }
    return(1);
  } else if(strncasecmp(tline,CatString3(Module,"MailCheckModuleMailTipUnblankScreen",mif->id), Clength+35+strlen(mif->id))==0) {
    mif->MailTipUnblankScreen = True;
    return(1);
  } else if(strncasecmp(tline,CatString3(Module,"MailCheckModuleMailTipNoSmartFrom",mif->id), Clength+33+strlen(mif->id))==0) {
    mif->NoSmartFrom = True;
    return(1);
  } else if(strncasecmp(tline,CatString3(Module,"MailCheckModuleMailTipFormat",mif->id), Clength+28+strlen(mif->id))==0) {
    int tmp[5];
    (void)sscanf(&tline[Clength+29+strlen(mif->id)], "%d %d %d %d %d",
		 &(tmp[0]), &(tmp[1]), &(tmp[2]), &(tmp[3]), &(tmp[4]));
    if (tmp[0] != 1 && tmp[0] != 2) {
      ConsoleMessage("MailTipFormat arg1 must be 1 or 2\n");
    } else if (tmp[1] < 0 || tmp[2] < 0 || tmp[3] < 0 || tmp[4] < 0) {
      ConsoleMessage("MailTipFormat arg2-5 must be > 0\n");
    } else {
      mif->MailTipFormat[0] = tmp[0];
      mif->MailTipFormat[1] = tmp[1];
      mif->MailTipFormat[2] = tmp[2];
      mif->MailTipFormat[3] = tmp[3];
      mif->MailTipFormat[4] = tmp[4];
      /*
      ConsoleMessage(stderr, "Found MailTip options %d %d %d %d %d",
		     mif->MailTipFormat[0], mif->MailTipFormat[1],
		     mif->MailTipFormat[2], mif->MailTipFormat[3],
		     mif->MailTipFormat[4]);
      */
    }
    return(1);
#endif
  } else return 0;
}


void MailCheckModuleLoad(struct MyInfo *mif, Display *dpy, Drawable win)
{
#ifdef __DEBUG__
  fprintf(stderr, "FvwmTaskBar.MailCheckModule.LoadMailCheckModule()\n");
#endif

  MailCheckModule_getstatus(mif);

  mif->visible = False;

  if (XpmReadFileToPixmap(dpy, win, mif->icon,
                          &(mif->icon_pix), &(mif->icon_mask),
                          &(mif->icon_attr)) != XpmSuccess) {
    fprintf(stderr,"FvwmTaskBar.MailCheckModule.LoadMailCheckModule(): error loading %s\n"
                   "  (FvwmTaskBarMailCheckModuleIcon%s)\n",
                    mif->icon, mif->id);
    return;
  }

  if (XpmReadFileToPixmap(dpy, win, mif->newicon,
                          &(mif->newicon_pix), &(mif->newicon_mask),
                          &(mif->newicon_attr)) != XpmSuccess) {
    fprintf(stderr,"FvwmTaskBar.MailCheckModule.LoadMailCheckModule(): error loading %s\n"
                   "  (FvwmTaskBarMailCheckModuleIcon%s)\n",
                    mif->newicon, mif->id);
    return;
  }

  if (XpmReadFileToPixmap(dpy, win, mif->unreadicon,
                          &(mif->unreadicon_pix), &(mif->unreadicon_mask),
                          &(mif->unreadicon_attr)) != XpmSuccess) {
    fprintf(stderr,"FvwmTaskBar.MailCheckModule.LoadMailCheckModule(): error loading %s\n"
                   "  (FvwmTaskBarMailCheckModuleIcon%s)\n",
                    mif->unreadicon, mif->id);
    return;
  }

  mif->dpy = dpy;
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
    fprintf(stderr,"       Loaded %s width=%d height=%d\n",
            mif->icon, mif->icon_attr.width, mif->icon_attr.height);
#endif   

}

/*-----------------------------------------------------*/
/* Get file modification time                          */
/* (based on the code of 'coolmail' By Byron C. Darrah */
/*-----------------------------------------------------*/
#define get_status MailCheckModule_getstatus

void get_status(struct MyInfo *mif)
{
   static off_t oldsize = 0;
   off_t  newsize;
   struct stat st;
   int fd;

   fd = open (mif->lock, O_RDONLY, 0);
   if (fd < 0)
   {
      
      mif->show=HasNoMail;
      newsize = 0;
   }
   else
   {
      fstat(fd, &st);
      newsize = st.st_size;
      mif->show=0;
      if (newsize > 0)
         mif->show=mif->show | HasMail;
      

      if (st.st_mtime >= st.st_atime && newsize > 0)
         mif->show=mif->show | HasUnreadMail;
     
        

      if (newsize > mif->mailsize && (mif->show & HasUnreadMail)) {
         mif->show=mif->show | HasNewMail;
         RenewGoodies=1;
      }
      
   }

   if (newsize != oldsize) {
     if (mif->MailBuf) free(mif->MailBuf);
     mif->MailBuf = (char*)safemalloc(newsize+1);
     if (read(fd, mif->MailBuf, newsize) == newsize) {
       mif->MailBuf[newsize] = '\0';
     } else {
       mif->MailBuf[0] = '\0';
     }
     mif->show |= HasChangedMail;
     mif->fMailBufChanged = True;
     oldsize = newsize;
   }
   close(fd);

   mif->mailsize = newsize;
}

/*---------------------------------------------------------------------------*/

void MailCheckModule_check_lock(struct MyInfo *mif)
{
  int lock_status;

  if (mif == NULL) return;
  if (mif->lock == NULL) return;
  lock_status = mif->show;
  get_status(mif);
  if (lock_status != mif->show) { 
    RenewGoodies = 1;
    if (mif->show & HasNewMail) {
      if (mif->NewMailcommand != NULL)
	  SendFvwmPipe(mif->NewMailcommand,0); 
    }               
  }
}

#define check_lock MailCheckModule_check_lock
         
#define start (win_width-stwin_width)

extern GC statusgc;

void MailCheckModuleDraw(struct MyInfo *mif, Display *dpy, Window win)
{
  XGCValues gcv;
  time_t now;

  unsigned long MailCheckModulegcm = GCClipMask |
                                     GCClipXOrigin | GCClipYOrigin;
#define gcm MailCheckModulegcm

  if (mif == NULL) return;

  now = time(NULL);
  if ((now-mif->lastchecked > 2)) {
    mif->lastchecked = now;
    check_lock(mif);
  }
                       
  if (mif->visible) {
    if (mif->show & HasNewMail) {
      gcv.clip_mask = mif->newicon_mask;
      gcv.clip_x_origin=start + icons_offset+3;
      gcv.clip_y_origin=((RowHeight - mif->newicon_attr.height) >> 1);
             
      XChangeGC(dpy,statusgc,gcm,&gcv);      
      XCopyArea(dpy,mif->newicon_pix, win, statusgc, 0, 0,
		mif->newicon_attr.width, mif->newicon_attr.height,
		gcv.clip_x_origin,
		gcv.clip_y_origin);
	     
	     
      mif->offset=icons_offset;
      icons_offset+=mif->newicon_attr.width+2;

    } else if (mif->show & HasUnreadMail) {
      gcv.clip_mask = mif->unreadicon_mask;
      gcv.clip_x_origin=start + icons_offset+3;
      gcv.clip_y_origin=((RowHeight - mif->unreadicon_attr.height) >> 1);
             
      XChangeGC(dpy,statusgc,gcm,&gcv);      
      XCopyArea(dpy,mif->unreadicon_pix, win, statusgc, 0, 0,
		mif->unreadicon_attr.width, mif->unreadicon_attr.height,
		gcv.clip_x_origin,
		gcv.clip_y_origin);
	     
	     
      mif->offset=icons_offset;
      icons_offset+=mif->unreadicon_attr.width+2;

    } else if (mif->show & HasMail) {
      gcv.clip_mask = mif->icon_mask;
      gcv.clip_x_origin=start + icons_offset+3;
      gcv.clip_y_origin=((RowHeight - mif->icon_attr.height) >> 1);
             
      XChangeGC(dpy,statusgc,gcm,&gcv);      
      XCopyArea(dpy,mif->icon_pix, win, statusgc, 0, 0,
		mif->icon_attr.width, mif->icon_attr.height,
		gcv.clip_x_origin,
		gcv.clip_y_origin);
	     
	     
      mif->offset=icons_offset;
      icons_offset+=mif->icon_attr.width+2;
    }

    /* auto display mail */
    if (mif->AutoMailTip != eDisabled) {
      if (mif->show & HasMail &&
	  ((mif->AutoMailTip == eAnyChange && mif->show & HasChangedMail) ||
	   (mif->AutoMailTip == eBigger && mif->show & HasNewMail) ||
	   (mif->AutoMailTip == eTouched && mif->show & HasUnreadMail))) {
	if (DoAutoMailTip(mif, 0)) CheckAndShowTipWindow(mif->GoodyNum);
      } else if ((mif->show & HasMail) == 0) {
	CheckAndDestroyTipWindow(mif->GoodyNum);
      }
    }
  }
}


int MailCheckModuleSeeMouse(struct MyInfo *mif, int x, int y)
{
  int xl, xr;

  if (mif == NULL) return 0;
  if (mif->show == 0) return 0;
  xl = win_width-stwin_width + mif->offset;
  xr = win_width-stwin_width + mif->offset;
  if (mif->show == HasMail)
    xr += mif->icon_attr.width;
  else
    xr += mif->newicon_attr.width;
  return (x>=xl && x<xr && y>1 && y<RowHeight-2);
}


void MailCheckModuleCreateIconTipWindow_(struct MyInfo *mif)
{
  if (mif == NULL) return;
  if (mif->AutoMailTip != eDisabled) {
    DoAutoMailTip(mif, 1);
  } else {
    switch (mif->show) {
      case HasMail:
        if (mif->tip == NULL) return;
        PopupTipWindow(win_width-stwin_width+mif->offset, 0,mif->tip );
        break;

      case HasUnreadMail:
      case HasMail | HasUnreadMail:
        if (mif->unreadtip == NULL) return;
        PopupTipWindow(win_width-stwin_width+mif->offset,0,mif->unreadtip);
        break;

      case HasNewMail:
      case HasMail | HasNewMail:
      case HasMail | HasUnreadMail | HasNewMail:
      case HasUnreadMail | HasNewMail:
        if (mif->newtip == NULL) return;
        PopupTipWindow(win_width-stwin_width+mif->offset,0,mif->newtip);
        break;
    } 
  }
}


#undef __DEBUG__

void MailCheckModuleIconClick(struct MyInfo *mif, XEvent event)
{
  if (mif == NULL) return;
  if (mif->command == NULL) return;
  if (event.xbutton.time - mif->lastclick < 250) {
    SendFvwmPipe(mif->command, 0);
#ifdef __DEBUG__
    printf("\"%s\"\n",mif->command);
    fflush(stdout);
#endif
  }
  mif->lastclick = event.xbutton.time;
}


static char line[256];

static void FreeMailHeaders(MailHdr_S *p) {
  if (!p) return;
  if (p->pNext) {
    FreeMailHeaders(p->pNext);
  }
  if (p->sbFrom) free(p->sbFrom);
  if (p->sbSubj) free(p->sbSubj);
  free(p);
}

/* reads from passed buf, and fills line[] */
static char *GetsBuf(char *buf) {
  int i;
  char ch;
  static char *pCurr;
  if (buf) pCurr = buf;
  if (*pCurr == '\0') return NULL;
  for (i=0; i<256; i++, pCurr++) {
    ch = *pCurr;
    if (ch == '\n' || ch == '\0') {
      if (ch != '\0') pCurr++;
      line[i] = '\0';
      break;
    }
    line[i] = ch;
  }
  return line;
}

static int LeftWhitespace(char *sb) {
  int wRtn = 0;
  while (strchr(" \t\n\0", *sb++)) {
    wRtn++;
  }
  return wRtn;
}

static char *RightWhitespace(char *sb) {
  int i = strlen(sb);
  char *p = sb + i;
  if (i==0) return sb;
  while (p >= sb && strchr(" \t\n\0", *--p)) {
    *p = '\0';
  }
  return sb;
}

char *StrDup(char *sb) {
  char *sbRtn = (char *)safemalloc(strlen(sb) + 1);
  return strcpy(sbRtn, sb);
}

void RemoveSubString(char *sbLeft, char *sbRight) {
  /* remove from sbLeft to sbRight inclusive */
  do {
    *sbLeft++ = *(++sbRight);
  } while (*sbRight);
}

char *RemoveWhitespace(char *sb) {
  char *sbTmp;
  RightWhitespace(sb);
  sbTmp = sb+LeftWhitespace(sb)-1;
  if (sbTmp >= sb) {
    RemoveSubString(sb, sbTmp);
  }
  return sb;
}

char *RemoveDelimText(char *sb, char chLeft, char chRight) {
  char *sbLeft, *sbRight;

  if ((sbLeft = strchr(sb, chLeft)) &&
      sbLeft+1 <= sb + strlen(sb) - 1) {
    if ((sbRight = strchr(sbLeft+1, chRight))) {
      RemoveSubString(sbLeft, sbRight);
      return sb;
    }
  }
  return NULL;
}

char *RemoveAllButDelimText(char *sb, char chLeft, char chRight) {
  char *sbLeft, *sbRight;

  if ((sbLeft = strchr(sb, chLeft)) &&
      sbLeft+1 <= sb + strlen(sb) - 1) {
    if ((sbRight = strchr(sbLeft+1, chRight))) {
      RemoveSubString(sbRight, sb+strlen(sb)-1);
      RemoveSubString(sb, sbLeft);
      return sb;
    }
  }
  return NULL;
}

char *ExtractName(char *sb) {
  char *sbTmp;

  /* remove delimited email addresses */
  sbTmp = StrDup(sb);
  while (RemoveDelimText(sbTmp, '<', '>'))
    ;
  while (RemoveDelimText(sbTmp, '[', ']'))
    ;
  RemoveWhitespace(sbTmp);
  if (strlen(sbTmp) == 0) {
    /* nothing left - revert back */
    free(sbTmp);
    sbTmp = StrDup(sb);
  }

  /* now try and find just the name */
  RemoveAllButDelimText(sbTmp, '(', ')');
  RemoveAllButDelimText(sbTmp, '"', '"');
  RemoveWhitespace(sbTmp);

  return sbTmp;
}

static MailHdr_S *ParseMailHeaders(struct MyInfo *mif, int *pwCount) {
  char *pLine;
  MailHdr_S *MailHdr, *MailHdr_tail, *pNew;
  int wCount = 0;

  /* assume list is empty */
  MailHdr = MailHdr_tail = NULL;
  pNew = NULL;

  /* walk through buffer to extract mail headers */
  pLine = GetsBuf(mif->MailBuf);
  while (pLine != NULL) {
    if (strncmp(pLine, HDR_DELIM, HDR_DELIM_LEN) == 0) {
      pNew = (MailHdr_S *)safemalloc(sizeof(MailHdr_S));
      pNew->sbFrom = pNew->sbSubj = NULL;
      pNew->pNext = NULL;
      if (!MailHdr) {
        MailHdr = MailHdr_tail = pNew;
      } else {  /* append to list */
        MailHdr_tail->pNext = pNew;
        MailHdr_tail = pNew;
      }
      wCount++;
    } else if (!pNew->sbFrom &&
               strncmp(pLine, HDR_FROM, HDR_FROM_LEN) == 0) {
      if (mif->NoSmartFrom) {
	UpdateString(&pNew->sbFrom,      
		     pLine+HDR_FROM_LEN + LeftWhitespace(pLine+HDR_FROM_LEN));
	RightWhitespace(pNew->sbFrom);
      } else {
	char *p = ExtractName(pLine + HDR_FROM_LEN);
	UpdateString(&pNew->sbFrom,p);
	free(p);
      }
    } else if (!pNew->sbSubj &&
               strncmp(pLine, HDR_SUBJ, HDR_SUBJ_LEN) == 0) {
      UpdateString(&pNew->sbSubj,
                   pLine + HDR_SUBJ_LEN + LeftWhitespace(pLine+HDR_SUBJ_LEN));
      RightWhitespace(pNew->sbSubj);
    }
    
    pLine = GetsBuf(NULL);
  }

  *pwCount = wCount;
  return MailHdr;
}

static char *StrNCpyPad(char *sbDst, char *sbSrc, int n, char chPad) {
  int i;
  for (i=0; i<n; i++) {
    if (sbSrc && *sbSrc) {
      *sbDst++ = *sbSrc++;
    } else {
      *sbDst++ = chPad;
    }
  }
  return sbDst;
}

/* allocs and returns tip string */
static char *GetMailHeaders(struct MyInfo *mif) {
  char *sbRtn = NULL, *sbTmp;
  int wCount;
  MailHdr_S *p, *pMailHdr;
  int wSize;

  pMailHdr = ParseMailHeaders(mif, &wCount);

  /* now fill the text string */
  if (mif->MailTipFormat[0] == 1) {  /* single line? */
    wSize = mif->MailTipFormat[1] + mif->MailTipFormat[2] +
      mif->MailTipFormat[3] + mif->MailTipFormat[4] + 1;
  } else {
    wSize = mif->MailTipFormat[1] + mif->MailTipFormat[2] + 1 +
      mif->MailTipFormat[3] + mif->MailTipFormat[4] + 1;
  }
  sbRtn = (char*)safemalloc(wCount*wSize + 1);
  *sbRtn = '\0';
  
  p = pMailHdr;
  sbTmp = sbRtn;
  while (p) {
    StrNCpyPad(sbTmp, "", mif->MailTipFormat[1], ' ');
    sbTmp += mif->MailTipFormat[1];
    StrNCpyPad(sbTmp, p->sbFrom, mif->MailTipFormat[2], ' ');
    sbTmp += mif->MailTipFormat[2];
    if (mif->MailTipFormat[0] != 1) {  /* double line? */
      *sbTmp++ = '\n';
    }
    StrNCpyPad(sbTmp, "", mif->MailTipFormat[3], ' ');
    sbTmp += mif->MailTipFormat[3];
    StrNCpyPad(sbTmp, p->sbSubj, mif->MailTipFormat[4], ' ');
    sbTmp += mif->MailTipFormat[4];
    *sbTmp++ = '\n';
    p = p->pNext;
  }
  *sbTmp = '\0';

  FreeMailHeaders(pMailHdr);

  return sbRtn;
}

static int DoAutoMailTip(struct MyInfo *mif, int fForce) {
  if (mif->fMailBufChanged || fForce) {
    if (mif->MailHeaderTipText) free(mif->MailHeaderTipText);
    mif->MailHeaderTipText = GetMailHeaders(mif);

    PopupTipWindow(win_width-stwin_width+mif->offset,0,mif->MailHeaderTipText);
    if (mif->MailTipUnblankScreen) {
      XForceScreenSaver(mif->dpy, ScreenSaverReset);
    }

    mif->fMailBufChanged = False;
    return 1;
  }
  return 0;
}
