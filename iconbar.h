
/********************************************************************************
** Iconbar - a desktop icon management utility compatible with the IRIX(TM) 4dwm
** Copyright 2003 Steven Queen
** 
** This file is part of Iconbar.
** 
** Iconbar is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** Iconbar is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with Iconbar; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*******************************************************************************/ 
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <signal.h>
 
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/IntrinsicP.h>
#include <X11/extensions/XScreenSaver.h>
#include <X11/extensions/shape.h>
#include <Xm/Xm.h>
#include <Xm/MwmUtil.h>
#include <Xm/Protocols.h>
#include <Xm/RowColumn.h>
#include <Xm/DrawnB.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Sgm/Grid.h>
#include <Xm/FileSB.h>

#include <sys/signal.h>
/* #include <sys/ddi.h> */

#include <X11/Intrinsic.h>
#include <X11/xpm.h>
#include <X11/Xmu/WinUtil.h>
#include <X11/extensions/XSGIvc.h>

#ifndef __ICONBAR_H__
#define __ICONBAR_H__

#ifndef DEBUG
#define DEBUG  FALSE
#else
#undef DEBUG
#define DEBUG TRUE
#endif

#define DEBUG0 FALSE
#define DEBUG2 FALSE
#define DEBUG3 FALSE
#define DEBUG4 FALSE

#define VERSION "0.8"

/* customizable */
/* ------------------------------------ */
#define GUI_SLOW_UPDATE_INTERVAL 200
#define GUI_FAST_UPDATE_INTERVAL 20
#define SYNC_LOOPS      10
#define BAR_CENTERING   TRUE
#define ICON_SHRINK_FACTOR 0.75
#define LONG_ICON_NAME_LENGTH 11
/* ------------------------------------ */

#ifndef max
#define max(A, B) ((A) > (B) ? (A) : (B))
#define min(A, B) ((A) < (B) ? (A) : (B))
#endif

#define ICON_WD            85
#define ICON_HT            67
#define WM_STATE_ELEMENTS  2
#define REPARENT_LOOPS     20
#define MAX_NAME           80
#define BTM_TRIM           15
#define LABEL_MARGIN       5
#define ICON_BUF_SIZE      100*100*4

#define DEMAG_ALL 0
#define DEMAG_ONE 1

typedef struct {
   Boolean  magnify;
   Boolean  hide;
   Boolean  raise;
   Boolean  blend;
   int      margin;
   float    scale;
   int      hideheight;
   int      hidecount;
   int      horzshift;
   int      horzmin;
   int      horzmax;
   int      animate;
   XmString ignoreApps;
   Pixel    highlightFG;
   Pixel    iconlabelFG;
   XmString path;
   Boolean  shape;
   Boolean  tint;
   XmString border;
   XmString image;
   int      toolchestX, toolchestY;
   Boolean  toolchestManage;
} options_t;

typedef struct iconDataStructure{
   Window      top, top_parent;
   Window      icon;
   int         state;
   Widget      w, w_pix, w_label, w_popup, w_popup_pix;
   char        winName[MAX_NAME];
   char        iconName[MAX_NAME];
   Pixmap      pixmap, pixmap_sm;
   void        *cbd;
   Dimension   x,y;
   int         popped;
   int         row, col;
   int         remote;
} icon_t;

extern Display       *dpy;
extern XtAppContext  app;
extern Screen        *scrn;
extern int           scrnNum;
extern Window        rootWin, shellWin, barWin, toolchestWin;
extern Dimension     scr_wd, scr_ht, last_row;

extern int        Magnify_Icon, Hide_Bar, Raise_Bar, menu_popped;
extern int        Nicons, menu_popped_up, max_clients, kb_grab, scrn_saver_on;
extern int        QUIT_DIALOG_VISIBLE, grabbed_icon_index, isRGBvisual;
extern icon_t     *IconData, *popped_icon;
extern Widget     shell, bar, menu;
extern options_t  options;

extern int	ICONBAR_WD, ICONBAR_HT;

extern Pixel GetPixelByName ( Widget w, char *colorname );
extern void NiceExit( void );
extern void ExitCallback( Widget w, XtPointer IconData, XtPointer callData);
extern void CollectIcons( void );

extern void geticon( icon_t *id );

extern void PostMenu( Widget w, XtPointer clientData, XEvent *event, Boolean *flag );
extern void QuitDialogCallback( Widget w, XtPointer IconData, XtPointer callData);
extern void SwapIcons( icon_t *icon1, icon_t *icon2 );
extern void SetIconName( icon_t *id, char *iconname );
extern void demagnify( icon_t *, int type );

/* reximage */
extern XImage *reXimage(Display *display, Window window, XImage *source, 
                        double widthScale, double heightScale );

/* borders */
extern void initBorders(XmString border, XmString image, int blend);
extern int getWindowWidth(void);
extern void addBorderToWindow(int width);

#endif /* __ICONBAR_H__ */
