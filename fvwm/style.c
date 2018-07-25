/****************************************************************************
 * This module is all original code 
 * by Rob Nation 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 *
 * code for parsing the fvwm style command
 *
 ***********************************************************************/
#include <FVWMconfig.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include <fvwm/version.h>

void ProcessNewStyle(XEvent *eventp,
                     Window w,
                     FvwmWindow *tmp_win,
                     unsigned long context,
                     char *text,
                     int *Module)
{
  char *name, *line;
  char *ticon_name = NULL;
  char *restofline,*tmp;
  char *icon_name = NULL;
  char *forecolor = NULL;
  char *backcolor = NULL;
  unsigned long off_buttons=0;
  unsigned long on_buttons=0;
  name_list *nptr;
  int butt;
  int BoxFillMethod = 0;
  int IconBox[4];
  int num,i;

  int len,desknumber = 0,bw=0, nobw = 0;
  unsigned long off_flags = 0;
  unsigned long on_flags = 0;
  
  IconBox[0] = -1;
  IconBox[1] = -1;
  IconBox[2] = Scr.MyDisplayWidth;
  IconBox[3] = Scr.MyDisplayHeight;

  /* Without this we get NO borders */
  off_flags = MWM_BUTTON_FLAG | MWM_BORDER_FLAG;

  restofline = GetNextToken(text,&name);
  /* in case there was no argument! */
  if((name == NULL)||(restofline == NULL))
    return;

  while(isspace((unsigned char)*restofline)&&(*restofline != 0))restofline++;
  line = restofline;

  if(restofline == NULL)return;
  while((*restofline != 0)&&(*restofline != '\n'))
  {
    while(isspace((unsigned char)*restofline)) restofline++;
    switch (tolower(restofline[0]))
    {
      case 'a':
        if(strncasecmp(restofline,"ActivePlacement",15)==0)
        {
          restofline +=15;
          on_flags |= RANDOM_PLACE_FLAG;
        }
        break;
      case 'b':
        if(strncasecmp(restofline,"BackColor",9)==0)
        {
          restofline +=9;
          while(isspace((unsigned char)*restofline))restofline++;
          tmp = restofline;
          len = 0;
          while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&&
                (*tmp != '\n')&&(!isspace((unsigned char)*tmp)))
          {
            tmp++;
            len++;
          }
          if(len > 0)
          {
            backcolor = safemalloc(len+1);
            strncpy(backcolor,restofline,len);
            backcolor[len] = 0;
            off_flags |= BACK_COLOR_FLAG;
          }
          restofline = tmp;
        }
        else if (strncasecmp(restofline,"Button",6)==0)
        {
          restofline +=6;
	  
          sscanf(restofline,"%d",&butt);
          while(isspace((unsigned char)*restofline))restofline++;
          while((!isspace((unsigned char)*restofline))&&(*restofline!= 0)&&
                (*restofline != ',')&&(*restofline != '\n'))
            restofline++;
          while(isspace((unsigned char)*restofline))restofline++;
	  
          on_buttons |= (1<<(butt-1));        
        }
        else if(strncasecmp(restofline,"BorderWidth",11)==0)
        {
          restofline +=11;
          off_flags |= BW_FLAG;
          sscanf(restofline,"%d",&bw);
          while(isspace((unsigned char)*restofline))restofline++;
          while((!isspace((unsigned char)*restofline))&&(*restofline!= 0)&&
                (*restofline != ',')&&(*restofline != '\n'))
            restofline++;
          while(isspace((unsigned char)*restofline))restofline++;
        }
        break;
      case 'c':
        if(strncasecmp(restofline,"Color",5)==0)
        {
          restofline +=5;
          while(isspace((unsigned char)*restofline))restofline++;
          tmp = restofline;
          len = 0;
          while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&&
                (*tmp != '\n')&&(*tmp != '/')&&(!isspace((unsigned char)*tmp)))
          {
            tmp++;
            len++;
          }
          if(len > 0)
          {
            forecolor = safemalloc(len+1);
            strncpy(forecolor,restofline,len);
            forecolor[len] = 0;
            off_flags |= FORE_COLOR_FLAG;
          }
          
          while(isspace((unsigned char)*tmp))tmp++;
          if(*tmp == '/')
          {
            tmp++;
            while(isspace((unsigned char)*tmp))tmp++;
            restofline = tmp;
            len = 0;
            while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&&
                  (*tmp != '\n')&&(*tmp != '/')&&(!isspace((unsigned char)*tmp)))
            {
              tmp++;
              len++;
            }
            if(len > 0)
            {
              backcolor = safemalloc(len+1);
              strncpy(backcolor,restofline,len);
              backcolor[len] = 0;
              off_flags |= BACK_COLOR_FLAG;
            }
          }
          restofline = tmp;
        }
        else if(strncasecmp(restofline,"CirculateSkipIcon",17)==0)
        {
          restofline +=17;
          off_flags |= CIRCULATE_SKIP_ICON_FLAG;
        }
        else if(strncasecmp(restofline,"CirculateHitIcon",16)==0)
        {
          restofline +=16;
          on_flags |= CIRCULATE_SKIP_ICON_FLAG;
        }
        else if(strncasecmp(restofline,"ClickToFocus",12)==0)
        {
          restofline +=12;
          off_flags |= CLICK_FOCUS_FLAG;
          on_flags |= SLOPPY_FOCUS_FLAG;
        }
        else if(strncasecmp(restofline,"CirculateSkip",13)==0)
        {
          restofline +=13;
          off_flags |= CIRCULATESKIP_FLAG;
        }
        else if(strncasecmp(restofline,"CirculateHit",12)==0)
        {
          restofline +=12;
          on_flags |= CIRCULATESKIP_FLAG;
        }
        break;
      case 'd':
        if(strncasecmp(restofline,"DecorateTransient",17)==0)
        {
          restofline +=17;
          off_flags |= DECORATE_TRANSIENT_FLAG;
        }
        else if(strncasecmp(restofline,"DumbPlacement",13)==0)
        {
          restofline +=13;
          on_flags |= SMART_PLACE_FLAG;
        }
        break;
      case 'e':
        break;
      case 'f':
        if(strncasecmp(restofline,"ForeColor",9)==0)
        {
          restofline +=9;
          while(isspace((unsigned char)*restofline))restofline++;
          tmp = restofline;
          len = 0;
          while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&&
                (*tmp != '\n')&&(!isspace((unsigned char)*tmp)))
          {
            tmp++;
            len++;
          }
          if(len > 0)
          {
            forecolor = safemalloc(len+1);
            strncpy(forecolor,restofline,len);
            forecolor[len] = 0;
            off_flags |= FORE_COLOR_FLAG;
          }
          restofline = tmp;
        }
        else if(strncasecmp(restofline,"FocusFollowsMouse",17)==0)
        {
          restofline +=17;
          on_flags |= CLICK_FOCUS_FLAG;
          on_flags |= SLOPPY_FOCUS_FLAG;
        }
        break;
      case 'g':
        break;
      case 'h':
        if(strncasecmp(restofline,"HintOverride",12)==0)
        {
          restofline +=12;
          off_flags |= MWM_OVERRIDE_FLAG;
        }
        else if(strncasecmp(restofline,"Handles",7)==0)
        {
          restofline +=7;
          on_flags |= NOBORDER_FLAG;
        }
        else if(strncasecmp(restofline,"HandleWidth",11)==0)
        {
          restofline +=11;
          off_flags |= NOBW_FLAG;
          sscanf(restofline,"%d",&nobw);
          while(isspace((unsigned char)*restofline))restofline++;
          while((!isspace((unsigned char)*restofline))&&(*restofline!= 0)&&
                (*restofline != ',')&&(*restofline != '\n'))
            restofline++;
          while(isspace((unsigned char)*restofline))restofline++;
        }
        break;
      case 'i':
        if(strncasecmp(restofline,"IconTitle",9)==0)
        {
          on_flags |= NOICON_TITLE_FLAG;
          restofline +=9;
        }
        else if(strncasecmp(restofline,"IconBox",7) == 0)
        {
          restofline +=7;
          /* Standard X11 geometry string */
          num = sscanf(restofline,"%d%d%d%d",&IconBox[0], &IconBox[1],
                       &IconBox[2],&IconBox[3]);
          for(i=0;i<num;i++)
          {
            while(isspace((unsigned char)*restofline))restofline++;
            while((!isspace((unsigned char)*restofline))&&(*restofline!= 0)&&
                  (*restofline != ',')&&(*restofline != '\n'))
              restofline++;
          }
          if(num !=4)
            fvwm_msg(ERR,"ProcessNewStyle",
                     "IconBox style requires 4 arguments!");
          else
          {
            /* check for negative locations */
            if(IconBox[0] < 0)
              IconBox[0] += Scr.MyDisplayWidth;
            if(IconBox[1] < 0)
              IconBox[1] += Scr.MyDisplayHeight;
            if(IconBox[2] < 0)
              IconBox[2] += Scr.MyDisplayWidth;
            if(IconBox[3] < 0)
              IconBox[3] += Scr.MyDisplayHeight;
          }
        }
        else if(strncasecmp(restofline,"Icon",4)==0)
        {
          restofline +=4;
          while(isspace((unsigned char)*restofline))restofline++;
          tmp = restofline;
          len = 0;
          while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&&(*tmp != '\n'))
          {
            tmp++;
            len++;
          }
          if(len > 0)
          {
            icon_name = safemalloc(len+1);
            strncpy(icon_name,restofline,len);
            icon_name[len] = 0;
            off_flags |= ICON_FLAG;
            on_flags |= SUPPRESSICON_FLAG;
          }
          else
            on_flags |= SUPPRESSICON_FLAG;	    
          restofline = tmp;
        }
        break;
      case 'j':
        break;
      case 'k':
        break;
      case 'l':
        if(strncasecmp(restofline,"Lenience",8)==0)
        {
          restofline += 8;
          off_flags |= LENIENCE_FLAG;
        }
        break;
      case 'm':
        if(strncasecmp(restofline,"MiniIcon",8)==0)
          {
          restofline +=9;
          while(isspace((unsigned char)*restofline))restofline++;
          tmp = restofline;
          len = 0;
          while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&&(*tmp != '\n'))
            {
            tmp++;
            len++;
            }
          if(len > 0)
            {
            ticon_name = safemalloc(len+1);
            strncpy(ticon_name,restofline,len);
            ticon_name[len] = 0;
            }
          restofline = tmp;
          }
        else if(strncasecmp(restofline,"MWMDecor",8)==0)
        {
          restofline +=8;
          off_flags |= MWM_DECOR_FLAG;
        }
        else if(strncasecmp(restofline,"MWMFunctions",12)==0)
        {
          restofline +=12;
          off_flags |= MWM_FUNCTIONS_FLAG;
        }
        else if(strncasecmp(restofline,"MouseFocus",10)==0)
        {
          restofline +=10;
          on_flags |= CLICK_FOCUS_FLAG;
          on_flags |= SLOPPY_FOCUS_FLAG;
        }
        break;
      case 'n':
        if(strncasecmp(restofline,"NoIconTitle",11)==0)
        {
          off_flags |= NOICON_TITLE_FLAG;
          restofline +=11;
        }
        else if(strncasecmp(restofline,"NoIcon",6)==0)
        {
          restofline +=6;
          off_flags |= SUPPRESSICON_FLAG;
        }
        else if(strncasecmp(restofline,"NoTitle",7)==0)
        {
          restofline +=7;
          off_flags |= NOTITLE_FLAG;
        }
        else if(strncasecmp(restofline,"NoPPosition",11)==0)
        {
          restofline +=11;
          off_flags |= NO_PPOSITION_FLAG;
        }
        else if(strncasecmp(restofline,"NakedTransient",14)==0)
        {
          restofline +=14;
          on_flags |= DECORATE_TRANSIENT_FLAG;
        }
        else if(strncasecmp(restofline,"NoDecorHint",11)==0)
        {
          restofline +=11;
          on_flags |= MWM_DECOR_FLAG;
        }
        else if(strncasecmp(restofline,"NoFuncHint",10)==0)
        {
          restofline +=10;
          on_flags |= MWM_FUNCTIONS_FLAG;
        }
        else if(strncasecmp(restofline,"NoOverride",10)==0)
        {
          restofline +=10;
          on_flags |= MWM_OVERRIDE_FLAG;
        }
        else if(strncasecmp(restofline,"NoHandles",9)==0)
        {
          restofline +=9;
          off_flags |= NOBORDER_FLAG;
        }
        else if(strncasecmp(restofline,"NoLenience",10)==0)
        {
          restofline += 10;
          on_flags |= LENIENCE_FLAG;
        }
        else if (strncasecmp(restofline,"NoButton",8)==0)
        {
          restofline +=8;
	  
          sscanf(restofline,"%d",&butt);
          while(isspace((unsigned char)*restofline))restofline++;
          while((!isspace((unsigned char)*restofline))&&(*restofline!= 0)&&
                (*restofline != ',')&&(*restofline != '\n'))
            restofline++;
          while(isspace((unsigned char)*restofline))restofline++;
	  
          off_buttons |= (1<<(butt-1));
        }
        else if(strncasecmp(restofline,"NoOLDecor",9)==0)
        {
          restofline += 9;
          on_flags |= OL_DECOR_FLAG;
        }
        break;
      case 'o':
        if(strncasecmp(restofline,"OLDecor",7)==0)
        {
          restofline += 7;
          off_flags |= OL_DECOR_FLAG;
        }
        break;
      case 'p':
        break;
      case 'q':
        break;
      case 'r':
        if(strncasecmp(restofline,"RandomPlacement",15)==0)
        {
          restofline +=15;
          off_flags |= RANDOM_PLACE_FLAG;
        }
        break;
      case 's':
        if(strncasecmp(restofline,"SmartPlacement",14)==0)
        {
          restofline +=14;
          off_flags |= SMART_PLACE_FLAG;
        }
        else if(strncasecmp(restofline,"SkipMapping",11)==0)
        {
          restofline +=11;
          off_flags |= SHOW_MAPPING;
        }
        else if(strncasecmp(restofline,"ShowMapping",11)==0)
        {
          restofline +=12;
          on_flags |= SHOW_MAPPING;
        }
        else if(strncasecmp(restofline,"StickyIcon",10)==0)
        {
          restofline +=10;
          off_flags |= STICKY_ICON_FLAG;
        }
        else if(strncasecmp(restofline,"SlipperyIcon",12)==0)
        {
          restofline +=12;
          on_flags |= STICKY_ICON_FLAG;
        }
        else if(strncasecmp(restofline,"SloppyFocus",11)==0)
        {
          restofline +=11;
          on_flags |= CLICK_FOCUS_FLAG;
          off_flags |= SLOPPY_FOCUS_FLAG;
        }
        else if(strncasecmp(restofline,"StartIconic",11)==0)
        {
          restofline +=11;
          off_flags |= START_ICONIC_FLAG;
        }
        else if(strncasecmp(restofline,"StartNormal",11)==0)
        {
          restofline +=11;
          on_flags |= START_ICONIC_FLAG;
        }
        else if(strncasecmp(restofline,"StaysOnTop",10)==0)
        {
          restofline +=10;
          off_flags |= STAYSONTOP_FLAG;	  
        }
        else if(strncasecmp(restofline,"StaysPut",8)==0)
        {
          restofline +=8;
          on_flags |= STAYSONTOP_FLAG;	  
        }
        else if(strncasecmp(restofline,"Sticky",6)==0)
        {
          off_flags |= STICKY_FLAG;	  
          restofline +=6;
        }
        else if(strncasecmp(restofline,"Slippery",8)==0)
        {
          on_flags |= STICKY_FLAG;	  
          restofline +=8;
        }
        else if(strncasecmp(restofline,"StartsOnDesk",12)==0)
        {
          restofline +=12;
          off_flags |= STARTSONDESK_FLAG;
          sscanf(restofline,"%d",&desknumber);
          while(isspace((unsigned char)*restofline))restofline++;
          while((!isspace((unsigned char)*restofline))&&(*restofline!= 0)&&
                (*restofline != ',')&&(*restofline != '\n'))
            restofline++;
          while(isspace((unsigned char)*restofline))restofline++;
        }
        else if(strncasecmp(restofline,"StartsAnywhere",14)==0)
        {
          restofline +=14;
          on_flags |= STARTSONDESK_FLAG;
        }
        break;
      case 't':
        if(strncasecmp(restofline,"TitleIcon",9)==0)
          {
          restofline +=9;
          while(isspace((unsigned char)*restofline))restofline++;
          tmp = restofline;
          len = 0;
          while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&&(*tmp != '\n'))
            {
            tmp++;
            len++;
            }
          if(len > 0)
            {
            ticon_name = safemalloc(len+1);
            strncpy(ticon_name,restofline,len);
            ticon_name[len] = 0;
            }
          restofline = tmp;
          }
        else if(strncasecmp(restofline,"Title",5)==0)
          {
          restofline +=5;
          on_flags |= NOTITLE_FLAG;
          }
        break;
      case 'u':
        if(strncasecmp(restofline,"UsePPosition",12)==0)
        {
          restofline +=12;
          on_flags |= NO_PPOSITION_FLAG;
        }
        else if(strncasecmp(restofline,"UseStyle",8)==0)
        {
          restofline +=8;
          while(isspace((unsigned char)*restofline))restofline++;
          tmp = restofline;
          while( tmp && *tmp &&(*tmp != ',')&&
                 (*tmp != '\n')&&(!isspace((unsigned char)*tmp)))
            tmp++;
          if((len = tmp - restofline) > 0)
          {
            nptr = Scr.TheList;
            while(nptr && strncasecmp(restofline,nptr->name,len) )
              nptr = nptr->next;
            if (nptr&& !strncasecmp(restofline,nptr->name,len))
            {
              on_flags      = nptr->on_flags;
              off_flags     = nptr->off_flags;
              icon_name     = nptr->value;
              ticon_name    = nptr->tvalue;
              desknumber    = nptr->Desk;
              bw            = nptr->border_width;
              nobw          = nptr->resize_width;
              forecolor     = nptr->ForeColor;
              backcolor     = nptr->BackColor;
              BoxFillMethod = nptr->BoxFillMethod;
              IconBox[0]    = nptr->IconBox[0];
              IconBox[1]    = nptr->IconBox[1];
              IconBox[2]    = nptr->IconBox[2];
              IconBox[3]    = nptr->IconBox[3];
              off_buttons   = nptr->off_buttons;
              on_buttons    = nptr->on_buttons;
              restofline = tmp;
            }
            else 
            {
              restofline = tmp;
              tmp=safemalloc(500);
              strncat(tmp,restofline-len,len);
              fvwm_msg(ERR,"ProcessNewStyle","UseStyle: `%s' style not found",tmp);
              free(tmp);
            }
          }
          while(isspace((unsigned char)*restofline)) restofline++;
        }
        break;
      case 'v':
        break;
      case 'w':
        if(strncasecmp(restofline,"WindowListSkip",14)==0)
        {
          restofline +=14;
          off_flags |= LISTSKIP_FLAG;
        }
        else if(strncasecmp(restofline,"WindowListHit",13)==0)
        {
          restofline +=13;
          on_flags |= LISTSKIP_FLAG;
        }
        break;
      case 'x':
        break;
      case 'y':
        break;
      case 'z':
        break;
      default:
        break;
    }

    while(isspace((unsigned char)*restofline))restofline++;
    if(*restofline == ',')
      restofline++;
    else if((*restofline != 0)&&(*restofline != '\n'))
    {
      fvwm_msg(ERR,"ProcessNewStyle",
               "bad style command: %s", restofline);
      return;
    }
  }

  /* capture default icons */
  if(strcmp(name,"*") == 0)
  {
    if(off_flags & ICON_FLAG)
      Scr.DefaultIcon = icon_name;
    off_flags &= ~ICON_FLAG;
    icon_name = NULL;
  }

  AddToList(name,icon_name,ticon_name,off_flags,on_flags,desknumber,bw,nobw,
	    forecolor,backcolor,off_buttons,on_buttons,IconBox,BoxFillMethod);
}

void AddToList(char *name,
               char *icon_name,
               char *ticon_name,
               unsigned long off_flags, 
	       unsigned long on_flags,
               int desk,
               int bw,
               int nobw,
	       char *forecolor,
               char *backcolor,
               unsigned long off_buttons,
               unsigned long on_buttons,
	       int *IconBox,
               int BoxFillMethod)
{
  name_list *nptr,*lastptr = NULL;

  if((name == NULL)||((off_flags == 0)&&(on_flags == 0)&&(on_buttons == 0)&&
		      (off_buttons == 0)&(IconBox[0] < 0)))
  {
    if(name)
      free(name);
    if(icon_name)
      free(icon_name);
    return;
  }

  /* used to merge duplicate entries, but that is no longer
   * appropriate since conficting styles are possible, and the
   * last match should win! */
  for (nptr = Scr.TheList; nptr != NULL; nptr = nptr->next)
  {
    lastptr=nptr;
  }

  nptr = (name_list *)safemalloc(sizeof(name_list));
  nptr->next = NULL;
  nptr->name = name;
  nptr->on_flags = on_flags;
  nptr->off_flags = off_flags;
  nptr->value = icon_name;
  nptr->tvalue = ticon_name;
  nptr->Desk = desk;
  nptr->border_width = bw;
  nptr->resize_width = nobw;
  nptr->ForeColor = forecolor;
  nptr->BackColor = backcolor;
  nptr->BoxFillMethod = BoxFillMethod;
  nptr->IconBox[0] = IconBox[0];
  nptr->IconBox[1] = IconBox[1];
  nptr->IconBox[2] = IconBox[2];
  nptr->IconBox[3] = IconBox[3];
  nptr->off_buttons = off_buttons;
  nptr->on_buttons = on_buttons;

  if(lastptr != NULL)
    lastptr->next = nptr;
  else
    Scr.TheList = nptr;
}

