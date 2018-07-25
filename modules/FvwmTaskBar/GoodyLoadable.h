#ifndef __Goody_Loadable__
#define __Goody_Loadable__

#ifndef __Goodies_c__
extern int win_width;
extern int stwin_width;
extern int icons_offset;
extern int goodies_width;  /* beynon - is this necessary? */
extern int RenewGoodies;
#endif

typedef void* (*LoadableInit_f)(char *, int);
typedef int   (*LoadableParseResource_f)(void *,char *,char *,int);
typedef void  (*LoadableLoad_f)(void *,Display *,Drawable);
typedef void  (*LoadableDraw_f)(void *,Display *,Drawable);
typedef int   (*LoadableSeeMouse_f)(void *,int x,int y);
typedef void  (*LoadableCreateTipWindow_f)(void *);
typedef void  (*HandleIconClick_f)(void *,XEvent event);

struct GoodyLoadable{
  LoadableInit_f LoadableInit;
  LoadableParseResource_f LoadableParseResource;
  LoadableLoad_f LoadableLoad;
  LoadableDraw_f LoadableDraw;
  LoadableSeeMouse_f LoadableSeeMouse;
  LoadableCreateTipWindow_f LoadableCreateTipWindow;
  HandleIconClick_f HandleIconClick;
};

int ParseGLinfo(char *tline,char *Module,int Clength);	            
void LoadableLoad(Display *dpy,Drawable win);
void LoadableDraw(Display *dpy,Drawable win);
int LoadableSeeMouse(int x,int y);
int CreateLoadableTipWindow(int k);
int IsLoadableTip(int type);
void HandleLoadableClick(XEvent event,int k);
void PopupTipWindow(int px, int py, char *text);
char *EnvExpand(char *str);

#endif
