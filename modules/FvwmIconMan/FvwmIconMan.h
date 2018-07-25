#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#ifdef I18N
#include <X11/Xlocale.h>
#define XDrawString(t,u,v,w,x,y,z) XmbDrawString(t,u,man->ButtonFontset,v,w,x,y,z)
#endif

#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#ifndef DEFAULT_ACTION
#define DEFAULT_ACTION "Iconify"
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) > (b) ? (b) : (a))
#endif

#ifndef FVWM_VERSION
#define FVWM_VERSION 2
#endif

#ifdef MALLOC_H
#include <malloc.h>
#endif

#define PRINT_CONSOLE
#if 0
#define PRINT_DEBUG      
#endif

#if !defined (PRINT_DEBUG) && defined (__GNUC__)
#define ConsoleDebug(fmt, args...)
#else
extern void ConsoleDebug(char *fmt, ...);
#endif

typedef unsigned long Ulong;
typedef unsigned char Uchar;

typedef signed char Schar;


typedef struct {
  Ulong paging_enabled;
} m_toggle_paging_data;

typedef struct {
  Ulong desknum;
} m_new_desk_data;

typedef struct {
  Ulong app_id;
  Ulong frame_id;
  Ulong dbase_entry;
  Ulong xpos;
  Ulong ypos;
  Ulong width;
  Ulong height;
  Ulong desknum;
  Ulong windows_flags;
  Ulong window_title_height;
  Ulong window_border_width;
  Ulong window_base_width;
  Ulong window_base_height;
  Ulong window_resize_width_inc;
  Ulong window_resize_height_inc;
  Ulong window_min_width;
  Ulong window_min_height;
  Ulong window_max_width_inc;
  Ulong window_max_height_inc;
  Ulong icon_label_id;
  Ulong icon_pixmap_id;
  Ulong window_gravity;
} m_add_config_data;
  
typedef struct {
  Ulong x, y, desknum;
} m_new_page_data;

typedef struct {
  Ulong app_id, frame_id, dbase_entry;
} m_minimal_data;

typedef struct {
  Ulong app_id, frame_id, dbase_entry;
  Ulong xpos, ypos, icon_width, icon_height;
} m_icon_data;

typedef struct {
  Ulong app_id, frame_id, dbase_entry;
  union {
    Ulong name_long[1];
    Uchar name[4];
  } name;
} m_name_data;

typedef struct {
  Ulong start, type, len, time /* in fvwm 2 only */;
} FvwmPacketHeader;

typedef union {
  m_toggle_paging_data toggle_paging_data;
  m_new_desk_data      new_desk_data;
  m_add_config_data    add_config_data;
  m_new_page_data      new_page_data;
  m_minimal_data       minimal_data;
  m_icon_data          icon_data;
  m_name_data          name_data;
} FvwmPacketBody;

typedef enum {
  SHOW_GLOBAL = 0,
  SHOW_DESKTOP = 1,
  SHOW_PAGE = 2
} Resolution;

typedef enum
{ BUTTON_FLAT,
  BUTTON_UP,
  BUTTON_DOWN
} ButtonState;

/* The clicks must be the first three elements in this type, X callbacks
	depend on it! */
typedef enum
{ CLICK1,
  CLICK2,
  CLICK3,
  SELECT,
  NUM_ACTIONS
} Action;

typedef enum {
  PLAIN_CONTEXT,
  FOCUS_CONTEXT,
  SELECT_CONTEXT,
  FOCUS_SELECT_CONTEXT,
  NUM_CONTEXTS
} Contexts;

typedef enum {
  TITLE_NAME    = 1,
  ICON_NAME     = 2,
  RESOURCE_NAME = 4,
  CLASS_NAME    = 8,
  ALL_NAME      = 15
} NameType;

typedef struct win_list {
  int n;
  struct win_data *head, *tail;
} WinList;

typedef struct string_list {
  NameType type;
  char *string;
  struct string_list *next;
} StringEl;

typedef struct {
  Uchar mask;
  StringEl *list;
} StringList;

typedef struct {
  Resolution res;
  Window theWindow;
  Pixel backcolor[NUM_CONTEXTS], forecolor[NUM_CONTEXTS];
  Pixel hicolor[NUM_CONTEXTS], shadowcolor[NUM_CONTEXTS];
  GC hiContext[NUM_CONTEXTS], backContext[NUM_CONTEXTS], 
    reliefContext[NUM_CONTEXTS];
  GC shadowContext[NUM_CONTEXTS], flatContext[NUM_CONTEXTS];
  XFontStruct *ButtonFont;
#ifdef I18N
  XFontSet ButtonFontset;
#endif
  int fontheight, boxheight;
  int win_width, win_height;
  int win_x, win_y, win_title, win_border;
  WinList icon_list;
  StringList show;
  StringList dontshow;
  char *actions[NUM_ACTIONS];
  char *fontname;
  char *backColorName[NUM_CONTEXTS];
  char *foreColorName[NUM_CONTEXTS];
  char *geometry;
  Uchar use_titlename;
  Schar current_box, last_box, focus_box;
  Uchar cursor_in_window;
  Uchar window_up, window_mapped;
  Schar grow_direction;
  ButtonState buttonState[NUM_CONTEXTS];
  Uchar followFocus;
  Uchar sort;
} WinManager;

typedef struct win_data {
  Ulong desknum;
  long x, y, width, height;
  Ulong app_id;
  char *resname;
  char *classname;
  char *titlename;
  char *iconname;
  char **name;      /* either titlename or iconname */
  struct win_data *win_prev, *win_next, *icon_prev, *icon_next;
  WinManager *manager;
  Uchar iconified;
  Uchar in_iconlist;
  Uchar complete;
  Uchar focus;
  Uchar sticky;
  Uchar winlistskip;
  int app_id_set : 1;
  int geometry_set : 1;
} WinData;

typedef struct {
  Ulong desknum;
  Ulong x, y;             /* of the view window */
  Ulong screenx, screeny; /* screen dimensions */
  WinManager *managers;
  int num_managers;
  WinData *focus_win;
} GlobalData;

typedef struct {
  char *name;
  ButtonState state;
  char *forecolor[2]; /* 0 is mono, 1 is color */
  char *backcolor[2]; /* 0 is mono, 1 is color */
} ContextDefaults;

typedef WinList HashTab[256];

extern char *actionNames[NUM_ACTIONS];
extern char *contextNames[NUM_CONTEXTS];

extern GlobalData globals;
extern int Fvwm_fd[2];
extern int x_fd;
extern Display *theDisplay;
extern char *Module;
extern int ModuleLen;
extern ContextDefaults contextDefaults[];

extern void ReadFvwmPipe();
extern void *Malloc (size_t size);
extern void Free (void *p);
extern void ConsoleMessage(char *fmt, ...);
extern void ShutMeDown (int flag);
extern void DeadPipe (int nothing);
extern void SendFvwmPipe(char *message, unsigned long window);
extern char *copy_string (char **target, char *src);

extern void init_globals (void);
extern int allocate_managers (int num);

extern WinData *new_windata (void);
extern void free_windata (WinData *p);
extern int check_win_complete (WinData *p);
extern int set_win_manager (WinData *win, Uchar mask);
extern void init_winlists (void);
extern void insert_win_iconlist (WinData *win);
extern void delete_win_iconlist (WinData *win, WinManager *man);
extern void delete_win_hashtab (WinData *win);
extern void insert_win_hashtab (WinData *win);
extern WinData *find_win_hashtab (Ulong id);
extern void print_iconlist (void);
extern void walk_hashtab (void (*func)(void *));
extern void print_stringlist (StringList *list);
extern void add_to_stringlist (StringList *list, char *s);
extern void update_window_stuff (WinManager *man);
extern void print_managers (void);

extern void init_display (void);
extern void xevent_loop (void);
extern void init_window (int man_id);
extern void init_boxes (void);
extern void draw_button (WinManager *man, WinData *win, int button );
extern void draw_added_icon (WinManager *man);
extern void draw_deleted_icon (WinManager *man);
extern int win_to_box (WinManager *man, WinData *win);
extern void draw_window (WinManager *man);
extern WinManager *find_windows_manager (Window win);
extern void map_new_manger (WinManager *man);
extern void move_highlight (WinManager *man, int to);
extern int move_win_iconlist (WinData *win);

extern char *safemalloc(int length);
extern void SendText(int *fd,char *message,unsigned long window);
extern void SetMessageMask(int *fd, unsigned long mask);
extern void SendInfo(int *fd,char *message,unsigned long window);
extern int ReadFvwmPacket(int fd, unsigned long *header, unsigned long **body);
extern int matchWildcards(char *pattern, char *string);
#if FVWM_VERSION == 2
extern void *GetConfigLine(int *fd, char **tline);
#endif
