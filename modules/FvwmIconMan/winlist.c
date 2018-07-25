#include "FvwmIconMan.h"

static HashTab hash_tab;

void print_stringlist (StringList *list)
{
  StringEl *p;
  char *s;

  printf ("\tmask = 0x%x\n", list->mask);
  for (p = list->list; p; p = p->next) {
    switch (p->type) {
    case ALL_NAME:
      s = "all";
      break;

    case TITLE_NAME:
      s = "title";
      break;

    case ICON_NAME:
      s = "icon";
      break;

    case RESOURCE_NAME:
      s = "resource";
      break;

    case CLASS_NAME:
      s = "class";
      break;
    }
    ConsoleDebug ("\t%s = %s\n", s, p->string);
  }
}

void add_to_stringlist (StringList *list, char *s)
{
  StringEl *new;
  NameType type;
  char *pat;

  ConsoleDebug ("In add_to_stringlist: %s\n", s);

  pat = strchr (s, '=');
  if (pat) {
    *pat++ = '\0';
    if (!strcmp (s, "icon"))
      type = ICON_NAME;
    else if (!strcmp (s, "title"))
      type = TITLE_NAME;
    else if (!strcmp (s, "resource"))
      type = RESOURCE_NAME;
    else if (!strcmp (s, "class"))
      type = CLASS_NAME;
    else {
      ConsoleMessage ("Bad element in show/dontshow list: %s\n", s);
      return;
    }
  }
  else {
    pat = s;
    type = ALL_NAME;
  }

  ConsoleDebug ("add_to_stringlist: %s %s\n", type == ALL_NAME ? "all" : s, 
		pat);

  new = (StringEl *)safemalloc (sizeof (StringEl));
  new->string = (char *)safemalloc ((strlen (pat) + 1) * sizeof (char));
  new->type = type;

  strcpy (new->string, pat);
  new->next = list->list;
  if (list->list)
    list->mask |= type;
  else
    list->mask = type;
  list->list = new;

  ConsoleDebug ("Exiting add_to_stringlist\n");
}

static int matches_string (NameType type, char *pattern, char *tname, 
			   char *iname, char *rname, char *cname)
{
  int ans = 0;
  
  if (tname && (type == ALL_NAME || type == TITLE_NAME))
    ans |= matchWildcards (pattern, tname);
  if (iname && (type == ALL_NAME || type == ICON_NAME))
    ans |= matchWildcards (pattern, iname);
  if (rname && (type == ALL_NAME || type == RESOURCE_NAME))
    ans |= matchWildcards (pattern, rname);
  if (cname && (type == ALL_NAME || type == CLASS_NAME))
    ans |= matchWildcards (pattern, cname);

  return ans;
}

static int iconmanager_show (WinManager *man, char *tname, char *iname,
			     char *rname, char *cname)
{
  StringEl *string;
  int in_showlist = 0, in_dontshowlist = 0;

  assert (man);

#ifdef PRINT_DEBUG
  ConsoleDebug ("In iconmanager_show: %s:%s : %s %s\n", tname, iname,
		rname, cname);
  ConsoleDebug ("dontshow:\n");
  print_stringlist (&man->dontshow);
  ConsoleDebug ("show:\n");
  print_stringlist (&man->show);
#endif /*PRINT_DEBUG*/

  for (string = man->dontshow.list; string; string = string->next) {
    ConsoleDebug ("Matching: %s\n", string->string);
    if (matches_string (string->type, string->string, tname, iname, 
			rname, cname)) {
      ConsoleDebug ("Dont show\n");
      in_dontshowlist = 1;
      break;
    }
  }
  
  if (!in_dontshowlist) {
    if (man->show.list == NULL) {
      in_showlist = 1;
    }
    else {
      for (string = man->show.list; string; string = string->next) {
	ConsoleDebug ("Matching: %s\n", string->string);
	if (matches_string (string->type, string->string, tname, iname,
			    rname, cname)) {
	  ConsoleDebug ("Show\n");
	  in_showlist = 1;
	  break;
	}
      }
    }
  }

  ConsoleDebug ("returning: %d %d %d\n", in_dontshowlist,
		in_showlist, !in_dontshowlist && in_showlist);

  return (!in_dontshowlist && in_showlist);
}

void print_iconlist (void)
{
#ifdef PRINT_DEBUG
  WinData *p;
  int i;

  ConsoleDebug ("Global Icon List:\n");

  for (i = 0; i < globals.num_managers; i++) {
    ConsoleDebug ("Manager: %d\n", i);
    ConsoleDebug ("Size: %d\n", globals.managers[i].icon_list.n);
    for (p = globals.managers[i].icon_list.head; p; p = p->icon_next) {
      ConsoleDebug ("\ttitlename: %s\n", p->titlename);
      ConsoleDebug ("\ticonname: %s\n", p->iconname);
      ConsoleDebug ("\tresname: %s\n", p->resname);
      ConsoleDebug ("\tclassname: %s\n", p->classname);
      if (p->sticky)
	ConsoleDebug ("\tSticky\n");
      else
	ConsoleDebug ("\tNot sticky\n");
      ConsoleDebug ("\twhere: %d (%d, %d)\n", p->desknum, p->x, p->y);
      if (p->iconified)
	ConsoleDebug ("\tIconified\n");
      else
	ConsoleDebug ("\tUniconified\n");
      ConsoleDebug ("\tID: %d\n", p->app_id);
      ConsoleDebug ("\n");
    }
  }
#endif
}

WinData *new_windata (void)
{
  WinData *new = (WinData *)safemalloc (sizeof (WinData));
  new->desknum = ULONG_MAX;
  new->x = ULONG_MAX;
  new->y = ULONG_MAX;
  new->geometry_set = 0;
  new->app_id = ULONG_MAX;
  new->app_id_set = 0;
  new->resname = NULL;
  new->classname = NULL;
  new->iconname = NULL;
  new->titlename = NULL;
  new->name = NULL;
  new->manager = NULL;
  new->win_prev = new->win_next = NULL;
  new->icon_prev = new->icon_next = NULL;
  new->iconified = 0;
  new->in_iconlist = 0;
  new->complete = 0;
  new->focus = 0;
  new->sticky = 0;
  new->winlistskip = 0;
  return new;
}

void free_windata (WinData *p)
{
  Free (p->resname);
  Free (p->classname);
  Free (p->iconname);
  Free (p);
}

int set_win_manager (WinData *win, Uchar name_mask)
{
  int i;
  char *tname = win->titlename;
  char *iname = win->iconname;
  char *rname = win->resname;
  char *cname = win->classname;
  WinManager *man;

  assert (tname || iname || rname || cname);

  for (i = 0, man = &globals.managers[0]; i < globals.num_managers;
       i++, man++) {
    if (iconmanager_show (man, tname, iname, rname, cname)) {
      if (man != win->manager) {
	win->manager = man;
	if (win->manager->use_titlename)
	  win->name = &win->titlename;
	else
	  win->name = &win->iconname;
	return 1;
      }
      else {
	return 0;
      }
    }
  }

  return 0;
}

int check_win_complete (WinData *p)
{
  if (p->complete)
    return 1;

  ConsoleDebug ("Checking completeness:\n");
  ConsoleDebug ("\ttitlename: %s\n", p->titlename);
  ConsoleDebug ("\ticonname: %s\n", p->iconname);
  ConsoleDebug ("\tres: %s\n", p->resname);
  ConsoleDebug ("\tclass: %s\n", p->classname);
  ConsoleDebug ("\t(x, y): (%d, %d)\n", p->x, p->y);
  ConsoleDebug ("\tapp_id: 0x%x %d\n", p->app_id, p->app_id_set);
  ConsoleDebug ("\tdesknum: %d\n", p->desknum);
  ConsoleDebug ("\tmanager: 0x%x\n", (int)p->manager);

  if (p->geometry_set &&
      p->resname &&
      p->classname &&
      p->iconname &&
      p->titlename &&
      p->manager &&
      p->app_id_set) {
    p->complete = 1;
    ConsoleDebug ("\tcomplete: 1\n\n");
    return 1;
  }

  ConsoleDebug ("\tcomplete: 0\n\n");
  return 0;
}
     
void init_winlists (void)
{
  int i;
  for (i = 0; i < 256; i++) {
    hash_tab[i].n = 0;
    hash_tab[i].head = NULL;
    hash_tab[i].tail = NULL;
  }
}

static void insert_before (WinData *win, WinData *p)
{
  WinList *list;
  ConsoleDebug ("in insert_before\n");

  assert (win->manager);
  list = &win->manager->icon_list;

  if (p) {
    /* insert win before p */
    win->icon_next = p;
    win->icon_prev = p->icon_prev;
    if (p->icon_prev)
      p->icon_prev->icon_next = win;
    else
      list->head = win;
    p->icon_prev = win;
  }
  else {
    /* put win at end of list */
    win->icon_next = NULL;
    win->icon_prev = list->tail;
    if (list->tail)
      list->tail->icon_next = win;
    else
      list->head = win;
    list->tail = win;
  }
  ConsoleDebug ("leaving insert_before\n");
}

static WinData *find_win_insert (WinData *win)
{
  WinList *list;
  WinData *p;
  ConsoleDebug ("in find_win_insert\n");
  
  assert (win->manager);
  assert (win->name);
  list = &win->manager->icon_list;

  if (win->manager->sort) {
    for (p = list->head; 
	 p && strcmp (*win->name, *p->name) > 0; 
	 p = p->icon_next) 
      ;
  }
  else {
    p = NULL;
  }
  
  ConsoleDebug ("leaving find_win_insert\n");

  return p;
}

int move_win_iconlist (WinData *win)
{
  WinData *next;
  int changed = 0;

  ConsoleDebug ("in move_win_iconlist\n");
  
  if (!win->complete) {
    ConsoleMessage ("Internal error in move_win_iconlist\n");
    ShutMeDown (1);
  }

  if (win->in_iconlist) {
    next = win->icon_next;
    delete_win_iconlist (win, win->manager);
  }
  else {
    changed = 1;
    next = NULL;
  }

  insert_win_iconlist (win);
  changed = (win->icon_next != next);

  ConsoleDebug ("leaving move_win_iconlist: %d\n", changed);
  return changed;
}
    
void insert_win_iconlist (WinData *win)
{
  WinData *p;
  ConsoleDebug ("in insert_win_iconlist\n");

  if (!win->complete) {
    ConsoleMessage ("Internal error in insert_win_iconlist\n");
    ShutMeDown (1);
  }

  assert (win->manager);

  if (win->in_iconlist) {
    ConsoleMessage ("Internal error in insert_win_iconlist: already there\n");
    return;
  }
  
  p = find_win_insert (win);
  insert_before (win, p);

  win->in_iconlist = 1;
  win->manager->icon_list.n++;
  ConsoleDebug ("leaving insert_win_iconlist\n");
}

void delete_win_iconlist (WinData *win, WinManager *man)
{
  WinList *list;

  ConsoleDebug ("In delete_win_iconlist\n");

  assert (man);

  if (win->in_iconlist) {
    list = &man->icon_list;
    if (win->icon_prev) 
      win->icon_prev->icon_next = win->icon_next;
    else
      list->head = win->icon_next;
    if (win->icon_next)
      win->icon_next->icon_prev = win->icon_prev;
    else
      list->tail = win->icon_prev;
    list->n--;
    win->in_iconlist = 0;
  }
  ConsoleDebug ("Exiting delete_win_iconlist\n");
}

void delete_win_hashtab (WinData *win)
{
  int entry;
  WinList *list;

  entry = win->app_id & 0xff;
  list = &hash_tab[entry];

  if (win->win_prev) 
    win->win_prev->win_next = win->win_next;
  else
    list->head = win->win_next;
  if (win->win_next)
    win->win_next->win_prev = win->win_prev;
  else
    list->tail = win->win_prev;
  list->n--;
}  

void insert_win_hashtab (WinData *win)
{
  int entry;
  WinList *list;
  WinData *p;

  entry = win->app_id & 0xff;
  list = &hash_tab[entry];

  for (p = list->head; p && win->app_id > p->app_id;
       p = p->win_next);

  if (p) {
    /* insert win before p */
    win->win_next = p;
    win->win_prev = p->win_prev;
    if (p->win_prev)
      p->win_prev->win_next = win;
    else
      list->head = win;
    p->win_prev = win;
  }
  else {
    /* put win at end of list */
    win->win_next = NULL;
    win->win_prev = list->tail;
    if (list->tail)
      list->tail->win_next = win;
    else
      list->head = win;
    list->tail = win;
  }
  list->n++;
}

WinData *find_win_hashtab (Ulong id)
{
  WinList *list;
  int entry = id & 0xff;
  WinData *p;

  list = &hash_tab[entry];

  for (p = list->head; p && p->app_id != id; p = p->win_next);

  return p;
}

void walk_hashtab (void (*func)(void *))
{
  int i;
  WinData *p;

  for (i = 0; i < 256; i++) {
    for (p = hash_tab[i].head; p; p = p->win_next)
      func (p);
  }
}
