#include <stdio.h>
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include <fvwm/fvwmlib.h>
#include "GoodyLoadable.h"
#include "FvwmTaskBar.h"
#include "Goodies.h"

extern int win_width;
extern int stwin_width;
extern GC statusgc;
extern int icons_offset;

void RestoreGC(void);

int Report = 1;

#define PathSize 1000


struct GoodyLoadableInfo {
	char *file;
	char *symbol;
	char *id;
	void *handle;
	void *data;	
};
	
struct GoodyLoadableInfo  **GLI = NULL;
struct GoodyLoadable	  **GL  = NULL;
int GLfree = -1,
    GLsize = -1,
    GLtip  = -1;

#define GLS 10


/* Why two checks ?
        because when there is an error (say not enough memory)
        the routines don't exit but simply do nothing.
        This allows FvwmTaskBar keep running even if you misspelled
        something in your .fvwmrc

   Why no ShrinkGL ?
   	because we don't need it :)
   	
   Why GLS ? Why such a bad list ?
   	first you don't need more then 10 icons there..definitely not more
   	then 30 !
   	secondly, we do this only on startup.
   	thirdly it's not that bad - we keep only pointers. the extra space
   	used is measured in bytes..
   	
   Why NULL ?
   	Hmm it seems like a sane idea. If it so happens that your whatever
   	platform you are running this in give NULL pointers when you call calloc
   	do this : 
   	  1.replace everywhere in the file calloc with mycalloc
   	  2.define a mycalloc at the beginning of this file:
   	  void *mycalloc(size_t a, sizeo_t b)
   	  {
   	    void *aa, *bb;
   	    aa = calloc(a, b);
   	    if (aa != NULL) return aa;
   	    bb = calloc(a, b);
     	    free(aa); / * some error checking needed here * /
   	    return(bb);
   	  }
   	  
   	If your calloc can utilize same address for storing two pieces of data
   	then I'm sorry ! do something else
   	
*/   	

/* looks like we will have to introduce plugins directory */
#ifndef PLUGINS
#define PLUGINS "/usr/local/lib/X11/fvwm95"
#endif
char *plugins = PLUGINS;

void **Handles = NULL;
int HandlesFree = -1;
int HandlesSize = -1;    	   	        
        

void InitHandles(void)
{
  Handles = (void**)calloc(GLS, sizeof(void *));
  if (Handles == NULL) {
    perror("FvwmTaskBar.GoodyLoadable.InitHandles()");
    return;
  }
  HandlesFree = 0;
  HandlesSize = GLS;
  GLtip = -1;
}

void ExpandHandles(void)
{
  void **h;
  int k;

  if ((HandlesFree < 0) || (HandlesSize < 0) || (Handles == NULL)) {
    InitHandles();
    return;
  }

  h = (void**)calloc(GLS+HandlesSize, sizeof(void *));
  if (h == NULL) {
    perror("FvwmTaskBar.GoodyLoadable.ExpandHandles()");
    return;
  }

  for (k=0; k<HandlesFree; k++) h[k] = Handles[k];
  free(Handles);
  Handles = h;
  HandlesSize += GLS;
}

int isSO(char *name)
{
  int k;

  if (name == NULL) return 0;
  k = strlen(name);
  if (k < 3) return 0;
  return((name[k-1] == 'o') && (name[k-2] == 's') && (name[k-3] == '.'));
}

void AddHandle(char *plugin)
{
  if (!isSO(plugin)) return;
  if ((Handles == NULL) || (HandlesFree < 0 ) || (HandlesSize < 0))
    InitHandles();
  if (HandlesFree+1 > HandlesSize) ExpandHandles();
  if (HandlesFree+1 > HandlesSize) return;
  Handles[HandlesFree] = NULL;
#if defined(NO_DL)
  Handles[HandlesFree] = dlopen(0, RTLD_NOW);
#else
  Handles[HandlesFree] = dlopen(plugin, RTLD_NOW);
#endif
  if(Handles[HandlesFree] == NULL) {
    if (Report) {
      printf("FvwmTaskBar.GoodyLoadable.AddHandle(\"%s\"): error\n%s\n",
             plugin, dlerror());
    }
    return;
  }
  HandlesFree++;
  if (Report) {
    printf("FvwmTaskBar.GoodyLoadable.AddHandle(\"%s\"): plugin \"%s\" loaded.\n",
           plugin, plugin);
    fflush(stdout);
  }
}

/* please don't reuse me !*/
char path[PathSize];

void LoadPlugins(void)
{
  DIR *dir;
  struct dirent *dp;

  dir = NULL;
  dir = opendir(plugins);
  if (dir == NULL) {
    perror("FvwmTaskBar.GoodyLoadable.LoadPlugins()");
    return;
  }

  while ((dp = readdir(dir)) != NULL) {
    path[0] = 0;
    strcat(strcat(path, plugins), "/");
    AddHandle(strcat(path, dp->d_name));
  }
  (void) closedir(dir);
}
	     
    
void InitGL(void)
{
  GL = (struct GoodyLoadable**)calloc(GLS, sizeof(struct GoodyLoadable *));
  GLI = (struct GoodyLoadableInfo**)calloc(GLS, sizeof(struct GoodyLoadableInfo *));
  if ((GL == NULL) || (GLI == NULL)) {
    perror("FvwmTaskBar.GoodyLoadable.InitGL()");
    return;
  }
  GLfree = 0;
  GLsize = GLS;
}


void ExpandGL(void)
{
  struct GoodyLoadable **_GL;
  struct GoodyLoadableInfo **_GLI;
  int k;

  _GL = (struct GoodyLoadable**)calloc(GLsize+GLS, sizeof(struct GoodyLoadable *));
  _GLI = (struct GoodyLoadableInfo**)calloc(GLsize+GLS, sizeof(struct GoodyLoadableInfo *));
  if ((GL == NULL) || (GLI == NULL)) {
    perror("FvwmTaskBar.GoodyLoadable.ExpandGL()");
    return;
  }
  for (k=0; k<GLfree; k++) {
    _GL[k] = GL[k];
    _GLI[k] = GLI[k];
  }
  free(GL); GL = _GL;
  free(GLI); GLI = _GLI;
  GLsize += GLS;
}


void AddGLSymbol(char *symbol)
{
#ifdef __DEBUG__
  printf("FvwmTaskBar.GoodyLoadable.AddGLSymbol(\"%s\")\n", symbol);
  fflush(stdout);
#endif

  if ((GLfree < 0) || (GLsize < 0) || (GL == NULL) || (GLI == NULL)) 
    InitGL();

  if ((GLfree < 0) || (GLsize < 0) || (GL == NULL) || (GLI == NULL))
    return;

  if (GLfree+1 > GLsize) ExpandGL();
  if (GLfree+1 > GLsize) return;

  GL[GLfree] = NULL;
  GLI[GLfree] = NULL;

  GLI[GLfree] = (struct GoodyLoadableInfo*)calloc(1, sizeof(struct GoodyLoadableInfo));
  if (GLI[GLfree] == NULL) {
    perror("FvwmTaskBar.GoodyLoadable.AddGLinfo()");fflush(stderr);
    return;
  }
  GLI[GLfree]->symbol = symbol;
  GLI[GLfree]->id = (char*)calloc(20, sizeof(char));
  if (GLI[GLfree]->id == NULL) {
    perror("FvwmTaskBar.GoodyLoadable.AddGLSymbol()");
    fflush(stderr);
    free(GLI[GLfree]);
    return;
  }         
  sprintf(GLI[GLfree]->id, "%d", GLfree+1);
  GLI[GLfree]->data = NULL;
  GLfree++;           
}

void GLsetId(char *id)
{
  if (GLfree <= 0) return;
  if (GLI[GLfree-1] == NULL) return;

  free(GLI[GLfree-1]->id);
  GLI[GLfree-1]->id = id;
}


/* code searching loadables in this proc.. */
void LoadGL(int k)
{
  char *error;
  int i;

#ifdef __DEBUG__
  printf("FvwmTaskBar.GoodyLoadable.LoadGL(%d)\n", k);
  fflush(stdout);
#endif

  if ((k < 0) || (k >= GLfree)) return;
/*
  GLI[k]->handle = dlopen(GLI[k]->file, RTLD_NOW);
  if (!GLI[k]->handle) {
    fprintf(stderr,"FvwmTaskBar.GoodyLoadable.LoadGL():file %s:%s\n",GLI[k]->file,dlerror());
    fflush(stderr);
    return;
  }
*/

  if ((Handles == NULL) || (HandlesFree < 0) || (HandlesSize < 0))
    LoadPlugins();

  if ((Handles == NULL) || (HandlesFree < 0) || (HandlesSize < 0))
    return;

  if (GLI[k]->symbol == NULL) return;       

  if (HandlesFree == 0) {
    fprintf(stderr,"FvwmTaskBar.GoodyLoadable.LoadGL(): no plugins loaded\n");
    fflush(stderr);
    return;
  }

  i = 0;
  do {
    GL[k] = dlsym(Handles[i], GLI[k]->symbol);
    error = dlerror();
    i++;
  } while ((error != NULL) && (i < HandlesFree));

  if (error == NULL)
    GLI[k]->data = (*(GL[k]->LoadableInit))(GLI[k]->id, k);
  else
    GL[k] = NULL;
}

int ParseGLinfo(char *tline, char *Module, int Clength)
{
  int k;
  int go = 1;
  char *s;

  s = (char*)calloc(100, sizeof(char));
  if (s == NULL) {
    perror("FvwmTaskBar.GoodyLoadable.ParseGLResource()");
    return 0;
  }

  if(strncasecmp(tline,CatString3(Module, "GoodyLoadablePlugins",""),
                               Clength+20)==0) {
    CopyString(&s, &tline[Clength+21]);
    plugins = s;
    LoadPlugins();
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "GoodyLoadableQuiet",""),
                               Clength+18)==0) {
    Report = 0;
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "GoodyLoadableID",""),
                               Clength+15)==0) {
    CopyString(&s, &tline[Clength+16]);
    GLsetId(s);
    return(1);
  } else if(strncasecmp(tline,CatString3(Module, "GoodyLoadableSymbol",""),
                               Clength+19)==0) {
    CopyString(&s, &tline[Clength+19]);
    AddGLSymbol(s);
    return(1);
  } else {
    for (k=0; go && (k<GLfree); k++) {
      if (GL[k] == NULL) LoadGL(k);
      if (GL[k] != NULL) go = !(*(GL[k]->LoadableParseResource))(GLI[k]->data,tline,Module,Clength);
    }
    return go;
  }
}
  
void LoadableLoad(Display *dpy, Drawable win)
{
  int k;

  for (k=0; k<GLfree; k++) {
    if (GL[k] != NULL)
      (*(GL[k]->LoadableLoad))(GLI[k]->data, dpy, win);
  }
}

void LoadableDraw(Display *dpy,Drawable win)
{
  int k;

  for (k=0; k<GLfree; k++) {
    if (GL[k] != NULL)
      (*(GL[k]->LoadableDraw))(GLI[k]->data, dpy, win);
  }
  /* no need anymore - only modules */ /*RestoreGC(); */
}

int LoadableSeeMouse(int x, int y)
{
  int k;
  int go;

  go = 1;
  for (k=0; k<GLfree; k++)
  if (GL[k] != NULL) {
    if((*(GL[k]->LoadableSeeMouse))(GLI[k]->data, x, y)) return k+1;
  }

  return 0;
}	

int CreateLoadableTipWindow(int k)
{
  k--;
  if ((k < 0) || (k >= GLfree) || (GL[k] == NULL)) return 0;
  if (k != GLtip) {
    GLtip = k;
    (*(GL[k]->LoadableCreateTipWindow))(GLI[k]->data);
  }
  return GLOADABLE_TIP - k;
}

int IsLoadableTip(int type) {
  int k = GLOADABLE_TIP - type;
  return ((k >= 0) && (k < GLfree));
}

void HandleLoadableClick(XEvent event, int k)
{
  k--;
  if ((k < 0) || (k >= GLfree) || (GL[k] == NULL)) return;
  (*(GL[k]->HandleIconClick))(GLI[k]->data, event);
}

char *EnvExpand(char *str)
{
  return envDupExpand(str, 0);
}
