#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include <fvwm/fvwmlib.h>

/****************************************************************************
 *
 * Find the specified icon file somewhere along the given path.
 *
 * There is a possible race condition here:  We check the file and later
 * do something with it.  By then, the file might not be accessible.
 * Oh well.
 *
 ****************************************************************************/
char *findIconFile(char *icon, char *pathlist, int type)
{
  char *path;
  char *dir_end;
  int l1,l2;

  if(icon != NULL)
    l1 = strlen(icon);
  else 
    l1 = 0;

  if(pathlist != NULL)
    l2 = strlen(pathlist);
  else
    l2 = 0;

  path = safemalloc(l1 + l2 + 10);
  *path = '\0';
  if (icon != NULL)
    if (*icon == '/') 
      {
        /* No search if icon begins with a slash */
        strcpy(path, icon);
        return path;
      }
     
  if ((pathlist == NULL) || (*pathlist == '\0')) 
    {
      /* No search if pathlist is empty */
      if (icon != NULL) strcpy(path, icon);
      return path;
    }
 
  /* Search each element of the pathlist for the icon file */
  while ((pathlist)&&(*pathlist))
    { 
      dir_end = strchr(pathlist, ':');
      if (dir_end != NULL)
	{
	  strncpy(path, pathlist, dir_end - pathlist);
	  path[dir_end - pathlist] = 0;
	}
      else 
	strcpy(path, pathlist);

      strcat(path, "/");
      if (icon != NULL) strcat(path, icon);
      if (access(path, type) == 0) 
	return path;
      strcat(path, ".gz");
      if (access(path, type) == 0) 
	return path;

      /* Point to next element of the path */
      if(dir_end == NULL)
	pathlist = NULL;
      else
	pathlist = dir_end + 1;
    }
  /* Hmm, couldn't find the file.  Return NULL */
  free(path);
  return NULL;
}
 

