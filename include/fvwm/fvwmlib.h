#ifndef FVWMLIB_H
#define FVWMLIB_H

#include <FVWMconfig.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifndef HAVE_STRCASECMP
int strcasecmp(const char *s1, const char *s2);
#endif
#ifndef HAVE_STRNCASECMP
int strncasecmp(const char *s1, const char *s2, size_t n);
#endif
#ifndef HAVE_STRERROR
char *strerror(int num);
#endif
char *CatString3(char *a, char *b, char *c);
int StrEquals(char *s1,char *s2);
int getostype(char *buf, int max);
void SendText(int *fd,char *message,unsigned long window);
void SendInfo(int *fd,char *message,unsigned long window);
char *safemalloc(int);
char *findIconFile(char *icon, char *pathlist, int type);
int ReadFvwmPacket(int fd, unsigned long *header, unsigned long **body);
void CopyString(char **dest, char *source);
int GetFdWidth(void);
void *GetConfigLine(int *fd, char **tline);
void SetMessageMask(int *fd, unsigned long mask);
int  envExpand(char *s, int maxstrlen);
char *envDupExpand(const char *s, int extra);

typedef struct PictureThing
{
  struct PictureThing *next;
  char *name;
  Pixmap picture;
  Pixmap mask;
  unsigned int depth;
  unsigned int width;
  unsigned int height;
  unsigned int count;
} Picture;

void InitPictureCMap(Display *, Window);
Picture *GetPicture(Display *, Window, char *iconpath, char *pixmappath,char*);
Picture *CachePicture(Display*,Window,char *iconpath,char *pixmappath,char*);
void DestroyPicture(Display *, Picture *p);

XFontStruct *GetFontOrFixed(Display *disp, char *fontname);
#ifdef I18N
XFontSet GetFontSetOrFixed(Display *disp, char *fontname);
#endif

#endif
