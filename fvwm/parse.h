/****************************************************************************
 * This module was originally based on the twm module of the same name. 
 * Since its use and contents have changed so dramatically, I have removed
 * the original twm copyright, and inserted my own.
 *
 * by Rob Nation 
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/

/**********************************************************************
 *
 * Codes for fvwm builtins 
 *
 **********************************************************************/

#ifndef _PARSE_
#define _PARSE_

#define F_NOP			0
#define F_BEEP			1
#define F_QUIT			2
#define F_RESTART               3
#define F_REFRESH		4
#define F_TITLE			5
#define F_SCROLL                6      /* scroll the virtual desktop */
#define F_CIRCULATE_UP          7
#define F_CIRCULATE_DOWN        8
#define F_TOGGLE_PAGE           9
#define F_GOTO_PAGE             10
#define F_WINDOWLIST            11
#define F_MOVECURSOR            12
#define F_FUNCTION              13
/* #define F_WARP                  14 */
#define F_MODULE                15
#define F_DESK                  16
#define F_CHANGE_WINDOWS_DESK   17
#define F_EXEC			18	/* string */
#define F_POPUP			19	/* string */
#define F_WAIT                  20
#define F_CLOSE                 21
#define F_SET_MASK              23
#define F_ADDMENU               24
#define F_ADDFUNC               25
#define F_STYLE                 26
#define F_EDGE_SCROLL           27
#define F_PIXMAP_PATH           28
#define F_ICON_PATH             29
#define F_MODULE_PATH           30
#define F_HICOLOR               31
#define F_SETDESK               32
#define F_MOUSE                 34
#define F_KEY                   35
#define F_OPAQUE                36
#define F_XOR                   37
#define F_CLICK                 38
#define F_MENUFONT              39
#define F_ICONFONT              40
#define F_WINDOWFONT            41
#define F_EDGE_RES              42
#define F_BUTTON_STYLE          43
#define F_READ                  44
#define F_ADDMENU2              45
#define F_NEXT                  46
#define F_PREV                  47
#define F_NONE                  48
#define F_STAYSUP	        49 /* string */
#define F_RECAPTURE             50
#define F_CONFIG_LIST	        51
#define F_DESTROY_MENU	        52
#define F_ZAP	                53
#define F_QUIT_SCREEN		54
#define F_COLORMAP_FOCUS        55
#define F_TITLESTYLE            56
#define F_BORDERCOLOR		57
#define F_STICKYCOLOR		58
#define F_EXEC_SETUP            59
#define F_MENUCOLORS            60
#define F_CURRENT 		61
#define F_WINDOWID 		62
#define F_CURSOR_STYLE          63

/* Functions which require a target window */
#define F_RESIZE		100
#define F_RAISE			101
#define F_LOWER			102
#define F_DESTROY		103
#define F_DELETE		104
#define F_MOVE			105
#define F_ICONIFY		106
#define F_STICK                 107
#define F_RAISELOWER            108
#define F_MAXIMIZE              109
#define F_FOCUS                 110
#define F_WARP                  111
#define F_SEND_STRING           112
#define F_ADD_MOD               113
#define F_DESTROY_MOD           114

/* Functions for use by modules only! */
#define F_SEND_WINDOW_LIST     1000

/* Functions for internal  only! */
/* #define F_RAISE_IT              2000 */

#endif /* _PARSE_ */

