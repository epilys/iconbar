
/********************************************************************************
** Iconbar - a desktop icon management utility compatible with IRIX(TM) 4dwm
** Copyright 2003 Steve Queen and Bryan Sawler
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

 
#include "iconbar.h"

Display        *dpy;
Visual	       *visual;
int	         vdepth;
Colormap       private_colormap;

XtAppContext   app;

Widget         shell, bar, w, menu = (Widget)NULL;
int            n, Nicons=0, numColumns, unit_wd,
               is_shrinking = FALSE,
               is_growing = FALSE, 
               ICONBAR_HT,
               ICONBAR_WD = 0,
               queue_shrinking = FALSE,
               GUI_UPDATE_INTERVAL=GUI_SLOW_UPDATE_INTERVAL, /*milliseconds*/
               MAG_ACTION = FALSE,
               menu_popped_up = FALSE,               
               max_clients = 25,
               QUIT_DIALOG_VISIBLE = FALSE,
               grabbed_icon_index,
               isRGBvisual;
char           **IgnoreNamesArray;
static int     shrink_countdown,
               is_hidden = FALSE;
Arg            args[10];
Dimension      scr_wd, scr_ht, max_width, max_width_popped, last_row=0;
icon_t         *IconData, *popped_icon;

Atom           xa_WM_STATE = (Atom)NULL, xa_WM_ICON_NAME = (Atom)NULL;    

Screen         *scrn;
int            scrnNum;
Window         rootWin, shellWin, barWin, quitWin=(Window)NULL, toolchestWin=(Window)NULL;
options_t      options;

/* private functions */
static void TimeoutCallback( XtPointer, XtIntervalId * );
static void growCallback( Widget w, XtPointer IconData, XEvent *event, Boolean *flag );
static void keyCallback( Widget w, XtPointer IconData, XEvent *event, Boolean *flag );
static void magnifyCallback( Widget w, XtPointer IconData, XEvent *event, Boolean *flag );
static void PackIcons(void);
static void SetIconNameByIndex( int idx );
static void SetSignalHandler(void);
static void SignalHandler( int sig, siginfo_t *sip, ucontext_t *up );
static int ErrorHandler( Display *dpy, XErrorEvent *error );
static int CheckArgs( int argc, char **argv, char *look_for );
static void DeleteIconByIndex( int idx );
static void ManageBarGeometry(void);
static Dimension CalcBarWidth(void);
static int isIgnorableApp( char *windowname );
static void parseIgnorableApps(XmString xstr);
static void AddIconEvents( icon_t * );
static void RemoveIconEvents( icon_t * );
static void MapAppWin( Widget w, XtPointer icon_data, XEvent *event, Boolean *flag );
static int FindVisual(void);
static int checkClientMachineProp( icon_t *id );

#define getHorzCenter() (max( min( ((int)( scr_wd - getWindowWidth() )/2 + options.horzshift), options.horzmax), options.horzmin ) )


/* fallbacks for added resources */
static XtResource resources[] = {
   {"animate", "AnimatePopup", XmRInt, sizeof( int ), XtOffset(options_t *, animate),
         XmRImmediate, (XtPointer)5 },
   {"border", "Border", XmRString, sizeof(char *), XtOffset(options_t *, border),
         XmRString, "default" },
   {"image", "Image", XmRString, sizeof(char *), XtOffset(options_t *, image),
         XmRString, "" },
   {"blend", "Blend", XmRBoolean, sizeof( Boolean ), XtOffset(options_t *, blend),
         XmRString, "False" },
   {"hide", "Hide", XmRBoolean, sizeof( Boolean ), XtOffset(options_t *, hide),
         XmRString, "True" },
   {"hidecount", "Hidecount", XmRInt, sizeof( int ), XtOffset(options_t *, hidecount),
         XmRImmediate, (XtPointer)10 },
   {"hideheight", "Hideheight", XmRInt, sizeof( int ), XtOffset(options_t *, hideheight),
         XmRImmediate, (XtPointer)1 },
   {"highlightFG", "HighlightForeground", XmRPixel, sizeof(Pixel), XtOffset(options_t *, highlightFG),
         XmRString, "blue" },
   {"horzshift", "HorzShift", XmRInt, sizeof( int ), XtOffset(options_t *, horzshift),
         XmRImmediate, (XtPointer)0 },
   {"horzmin", "HorzMin", XmRInt, sizeof( int ), XtOffset(options_t *, horzmin),
         XmRImmediate, (XtPointer)0 },
   {"horzmax", "HorzMax", XmRInt, sizeof( int ), XtOffset(options_t *, horzmax),
         XmRImmediate, (XtPointer)10000 },
   {"ignoreApps", "IgnoreApps", XmRString, sizeof(char *), XtOffset(options_t *, ignoreApps),
         XmRString, "ToolChest" },
   {"magnify", "Magnify", XmRBoolean, sizeof( Boolean ), XtOffset(options_t *, magnify), 
         XmRString, "True" },
   {"margin", "Margin", XmRInt, sizeof( int ), XtOffset(options_t *, margin),
         XmRImmediate, (XtPointer)1 },
   {"path", "Path", XmRString, sizeof(char *), XtOffset(options_t *, path),
         XmRString, "WMHINTS:FILES:DEFAULT:SCREEN" },         
   {"raise", "Raise", XmRBoolean, sizeof( Boolean ), XtOffset(options_t *, raise),
         XmRString, "True" },
   {"scale", "MagnifyScale", XmRFloat, sizeof( float ), XtOffset(options_t *, scale),
         XmRString, "0.75" },
   {"shape", "Shape", XmRBoolean, sizeof( Boolean ), XtOffset(options_t *, shape),
         XmRString, "True" },
   {"tint", "Tint", XmRBoolean, sizeof( Boolean ), XtOffset(options_t *, tint),
         XmRString, "True" },
   {"toolchestX", "toolchestX", XmRInt, sizeof( int ), XtOffset(options_t *, toolchestX),
         XmRImmediate, (XtPointer)1 },
   {"toolchestY", "toolchestY", XmRInt, sizeof( int ), XtOffset(options_t *, toolchestY),
         XmRImmediate, (XtPointer)1 },
   {"toolchestManage", "toolchestManage", XmRBoolean, sizeof( Boolean ), XtOffset(options_t *, toolchestManage),
         XmRString, "False" }
};

/* use these values if nothing set in /usr/lib/X11/ap-deafults, $HOME/.Xdefaults, etc. */
static String fallback_resources[] = {
   "*sgiMode: True",
   "*SgNuseEnhancedFSB: True",
   "*bar*logo*fontList: -*-Helvetica-bold-R-normal-*-12-*-*-*-*-*-iso8859-1",
   "*bar*logo*foreground: blue",         
   "*bar*background: #C4C4C4",
   "*bar*exitB*fontList: -*-Helvetica-bold-O-normal-*-12-*-*-*-*-*-iso8859-1",
   "*bar*closeB*fontList: -*-Helvetica-bold-O-normal-*-12-*-*-*-*-*-iso8859-1",
   "*bar*fontList: -*-Helvetica-medium-O-normal-*-12-*-*-*-*-*-iso8859-1",
   "*bar*iconlabel*fontList: -*-Helvetica-medium-R-normal--10-*-*-*-p-*-iso8859-1",
   "*iconpixmap*background: #C4C4C4", /* was #787878 */
   NULL };

/* command line options --> resources */
XrmOptionDescRec CLoptions[] = {
   {"-animate", "*animate", XrmoptionSepArg, NULL},
   {"-blend", "*blend", XrmoptionSepArg, NULL},
   {"-border", "*border", XrmoptionSepArg, NULL},
   {"-hide", "*hide", XrmoptionSepArg, NULL},
   {"-hidecount", "*hidecount", XrmoptionSepArg, NULL},
   {"-hideheight", "*hideheight", XrmoptionSepArg, NULL},
   {"-highlightFG", "*highlightFG", XrmoptionSepArg, NULL},
   {"-horzshift", "*horzshift", XrmoptionSepArg, NULL},
   {"-horzmin", "*horzmin", XrmoptionSepArg, NULL},
   {"-horzmax", "*horzmax", XrmoptionSepArg, NULL},
   {"-ignoreApps", "*ignoreApps", XrmoptionSepArg, NULL},
   {"-image", "*image", XrmoptionSepArg, NULL},
   {"-magnify", "*magnify", XrmoptionSepArg, NULL},
   {"-margin", "*margin", XrmoptionSepArg, NULL},
   {"-path", "*path", XrmoptionSepArg, NULL},
   {"-raise", "*raise", XrmoptionSepArg, NULL},
   {"-scale", "*scale", XrmoptionSepArg, NULL},
   {"-shape", "*shape", XrmoptionSepArg, NULL},
   {"-tint", "*tint", XrmoptionSepArg, NULL},
   {"-toolchestManage", "*toolchestManage", XrmoptionSepArg, NULL},
   {"-toolchestX", "*toolchestX", XrmoptionSepArg, NULL},   
   {"-toolchestY", "*toolchestY", XrmoptionSepArg, NULL}
};


int main ( int argc, char **argv )
{
   char              iconName[256];
   Arg               arglist[8] ;
   int               n, cc;
   XEvent            event_rtn;
   
   
       
   IconData = (icon_t *) calloc( max_clients, sizeof(icon_t) );
   
   if (DEBUG) printf("\n------------- START ---------------------------\n");
   
   /* manually perform the XtOpenApplication steps */
   /* -------------------------------------------- */
   XtToolkitInitialize();
   app = XtCreateApplicationContext();
   XtAppSetFallbackResources( app, fallback_resources );
   dpy = XtOpenDisplay( app, NULL, NULL, "Iconbar", CLoptions, XtNumber(CLoptions), &argc, argv );
   if ( dpy == NULL ) {
      fprintf(stderr, "Iconbar: Can't open display!\n");
      exit(1);
   }

   /* Verify we can find a TrueColor/24bpp visual */
   isRGBvisual = FindVisual();

   private_colormap = XCreateColormap(dpy, DefaultRootWindow(dpy), visual, AllocNone);
   
   n = 0;
   XtSetArg(arglist[n], XmNoverrideRedirect, True);  n++; 
   XtSetArg(arglist[n], XmNmwmDecorations, MWM_DECOR_ALL | MWM_DECOR_TITLE | MWM_DECOR_BORDER
          | MWM_DECOR_MENU | MWM_DECOR_RESIZEH | MWM_DECOR_MAXIMIZE | MWM_DECOR_MINIMIZE);  n++;   
   XtSetArg(arglist[n], XmNvisual, visual); n++;
   XtSetArg(arglist[n], XmNdepth, vdepth); n++;
   XtSetArg(arglist[n], XmNcolormap, private_colormap); n++;
   shell = XtAppCreateShell( NULL, "Iconbar", applicationShellWidgetClass, dpy, arglist, n);
   /* -------------------------------------------- */
   scrn = XtScreen(shell);
   scrnNum =  XScreenNumberOfScreen(scrn);
   rootWin = RootWindowOfScreen( scrn );
   
   /* Retrieve the application resources */
   /* ++++++++++++++++++++++++++++++++++ */
   XtGetApplicationResources( shell, &options, resources, XtNumber(resources), NULL, 0 );
   options.margin = max( options.margin, 0 );
   options.hideheight = max( options.hideheight, 1  );
   options.animate = max( options.animate, 1  );
   options.animate = min( options.animate, 20  );
   options.scale = min( options.scale, 1.0f  );
   options.scale = max( options.scale, 0.1f  );
   parseIgnorableApps( options.ignoreApps );
   /* ++++++++++++++++++++++++++++++++++ */

   unit_wd = (int)(ICON_WD*options.scale) + 2*options.margin;
   ICONBAR_HT = (Dimension)( ICON_HT*options.scale + BTM_TRIM + 1 );
   shrink_countdown = options.hidecount;      
   
   xa_WM_STATE = XInternAtom( dpy, "WM_STATE", False);
   xa_WM_ICON_NAME = XInternAtom( dpy, "WM_ICON_NAME", False);
   
   /* process (remaining) command line arguments */
   if ( argc > 1 ) {
      if ( CheckArgs( argc, argv, "-v" ) || CheckArgs( argc, argv, "-version" ) ||
            CheckArgs( argc, argv, "-h" ) || CheckArgs( argc, argv, "-help" ) ) {
         printf("\nIconbar (v%s)\n", VERSION);
         printf("\tCopyright (C) Steven Queen, Antony Fountain and Brian Sawler 2006.\n\tIconbar comes with ABSOLUTELY NO WARRANTY; ");
         printf("for details see the\n\tGNU Public Licence (GPL)\n\n");
         if ( CheckArgs( argc, argv, "-h" ) || CheckArgs( argc, argv, "-help" ) ) {
            printf("\tUsage: \ticonbar [-option value ...]\n\n");
            printf("\t\t-magnify\t<True>|False\n");
            printf("\t\t-hide\t\t<True>|False\n");
            printf("\t\t-raise\t\t<True>|False\n");
            printf("\t\t-animate\t1-20 (1=fastest, 5=default)\n\n");
            printf("\tSee the iconbar(1) man page for more details and options.\n\n");
         }
         return(0);
      }
   }
    
   initBorders( options.border, options.image, options.blend );

   /* uses the SGI video control extension to get the channel 0 width */
   if (1) { 
     XSGIvcChannelInfo *cinfo_return;
     
     XSGIvcQueryChannelInfo( dpy, scrnNum, 0, &cinfo_return );
     /* printf("screen %d  = %d x %d\nchannel 0 = %g x %g\n", scrnNum, scr_wd, scr_ht,
            cinfo_return->source.width, cinfo_return->source.height ); */
     scr_wd = (Dimension)cinfo_return->source.width;
     scr_ht = (Dimension)cinfo_return->source.height;
     XFree( cinfo_return );
   }
   else {
      scr_wd = (Dimension)DisplayWidth( dpy, scrnNum );
      scr_ht = (Dimension)DisplayHeight( dpy, scrnNum );
   }
   XtVaSetValues( shell, XmNwidth, scr_wd/2, XmNy, scr_ht-getWindowHeight(), XmNheight, getWindowHeight(), NULL );
   
   n = 0;            
   do {
      max_width =  ((int)(scr_wd/unit_wd) - 2*n)*unit_wd;
      max_width_popped =  max_width + ICON_WD + 2*options.margin + getBorderWidth() - unit_wd;
      n++;
   } while( max_width_popped >= scr_wd );
   
   bar = XtVaCreateManagedWidget( "bar", xmBulletinBoardWidgetClass, shell,
            XmNshadowThickness, 0,
            XmNallowOverlap, TRUE,
            XmNresizePolicy, XmRESIZE_ANY,
            XmNmarginHeight, 0,
            XmNmarginWidth, 0,
            XmNvisual, visual,
            XmNcolormap, private_colormap,
            XmNdepth, vdepth,
            NULL );       
            
   XtAddEventHandler( shell, EnterWindowMask, FALSE, growCallback, NULL );
   XtAddEventHandler( bar, KeyPressMask, FALSE, keyCallback, NULL );
   
   XtManageChild(bar);                   

   XtRealizeWidget ( shell );
   
   shellWin = XtWindow(shell);
   barWin = XtWindow(bar);
   sprintf(iconName, "iconbar");
   XSetIconName( XtDisplay(shell), shellWin, iconName); 
   
   for (n=0;n<Nicons;n++) {
      /* a pop-up/down puts the icon window into the root window hiearchy */
      XtPopup(  IconData[n].w_popup, XtGrabNone );
      XSync( dpy, FALSE);
      XtPopdown( IconData[n].w_popup );  
   }
   if ( Nicons > 0 ) {
      XtVaGetValues( IconData[0].w_label, XmNforeground, options.iconlabelFG, NULL);
   }
   SetSignalHandler();   
   
   /* add self-updating work procedure */
   XtAppAddTimeOut ( app, GUI_UPDATE_INTERVAL, TimeoutCallback, NULL );
   if (DEBUG0) XSynchronize( dpy, TRUE );
   XSetErrorHandler(ErrorHandler);
  
   CollectIcons();
/*    if ( Nicons > 0 ) {
      Pixel bg;
      XtVaGetValues( IconData[0].w_label, XmNbackground, &bg, NULL);
      printf("bg pixel = 0x%d\n",(int)bg);
   }  */    

   addBorderToWindow(ICONBAR_WD);
   XSelectInput( dpy, rootWin, SubstructureNotifyMask );  
   XSelectInput( dpy, XtWindow(bar), VisibilityChangeMask );
   /* XShapeSelectInput( dpy, XtWindow(bar), ShapeNotifyMask); */

   /* customized XtAppMainLoop() */
   /* ----------------------------- */
   while(1) {      
      
      XtAppNextEvent( app, &event_rtn );
      
      /* auto-raise (always-on-top) */
      /* -------------------------- */
      if ( event_rtn.type == VisibilityNotify 
           /* && event_rtn.xany.window == XtWindow(bar) */  ) { /*<--XSHAPE breaks!!*/
         if ( event_rtn.xvisibility.state == VisibilityFullyObscured ) {
            Window         root_win, child_win;
            int            root_x, root_y, win_x, win_y;
            unsigned int   buttons, width, height, border_width, depth;
            
            XQueryPointer( dpy, rootWin, &root_win, &child_win, &root_x, &root_y, &win_x, &win_y, &buttons );
            XGetGeometry( dpy, child_win, &root_win, &root_x, &root_y, &width, &height, &border_width, &depth );
            if ( width == scr_wd && height == scr_ht ) {
                if (DEBUG) printf("full screen app - don't raise \n");                              
            }
            else {
               if ( menu_popped_up )
                  XRaiseWindow( dpy,  XtWindow(XtParent(menu)) );
               else if ( popped_icon != (icon_t *)NULL ) 
                  XRaiseWindow( dpy,  XtWindow(popped_icon->w_popup) );
               else if ( options.raise )
                  XRaiseWindow( dpy, shellWin );
               if ( options.toolchestManage ) {
                  XRaiseWindow( dpy, toolchestWin );
                  XMoveWindow( dpy, toolchestWin, options.toolchestX, options.toolchestY );
               }
            }      
         }
         else {
            if ( menu_popped_up )
               XRaiseWindow( dpy,  XtWindow(XtParent(menu)) );
            else if ( popped_icon != (icon_t *)NULL ) 
               XRaiseWindow( dpy,  XtWindow(popped_icon->w_popup) );
            else if ( options.raise )
               XRaiseWindow( dpy, shellWin );
            if ( options.toolchestManage && event_rtn.xany.window==toolchestWin) {
                  XRaiseWindow( dpy, toolchestWin );
                  XMoveWindow( dpy, toolchestWin, options.toolchestX, options.toolchestY );
            }
         }

      }
      
      if ( !XtDispatchEvent( &event_rtn ) ) {
         
         /* delete icons */         
         /* ------------ */
         if ( event_rtn.type == DestroyNotify ) {            
            for (cc=0;cc<Nicons;cc++) {
               if ( IconData[cc].top_parent == event_rtn.xdestroywindow.window ) {
                  if (DEBUG) printf("%s window destoryed\n", IconData[cc].iconName);
                  DeleteIconByIndex( cc );
                  PackIcons();
               }
            }
         }
         /* hide icon windows on the desktop */
         /* -------------------------------- */
         else if ( event_rtn.type == UnmapNotify && event_rtn.xany.window != rootWin ) {
            for (cc=0;cc<Nicons;cc++) {
               if ( IconData[cc].top_parent == event_rtn.xunmap.window ) {
                     XUnmapWindow( dpy, IconData[cc].icon );
               }
            }
         }
         /* detect icon name changes */
         /* ------------------------ */
         else if ( event_rtn.type == PropertyNotify && event_rtn.xproperty.atom == xa_WM_ICON_NAME ) {
            for (cc=0;cc<Nicons;cc++) {
               if ( IconData[cc].top == event_rtn.xany.window ) {
                  if (DEBUG) printf("%s window property (icon name) changed\n", IconData[cc].iconName);
                  SetIconNameByIndex(cc);           
               }
            }
         }
         /* detect new shell windows */
         /* ------------------------ */
         else if ( event_rtn.xany.window == rootWin 
                     && event_rtn.type == MapNotify 
                     && event_rtn.xmap.override_redirect == False
                     && !QUIT_DIALOG_VISIBLE )
         {
            CollectIcons();
         }
      }   
   } 
   /* ----------------------------- */
   
   return(0);
}


 

static int FindVisual(void)
{
	XVisualInfo xvi;
	int xvimask = VisualClassMask|VisualRedMaskMask|VisualBlueMaskMask|VisualGreenMaskMask|VisualDepthMask;
	XVisualInfo *xvis;
	int howmany = 0;
   int color_reversed = 0;	   
   
   /* this works for O2 and Octane */
	xvi.class = TrueColor;
   xvi.red_mask = 0x000000ff;
	xvi.green_mask = 0x0000ff00;
	xvi.blue_mask = 0x00ff0000;
	xvi.bits_per_rgb = 8;
	xvi.depth = 24;
	
	xvis = XGetVisualInfo(dpy, xvimask, &xvi, &howmany);

	if (!howmany) {
      /* this works for Onyx4/UltimateVision and Onyx350/IR3 */
      howmany = 0;
      xvi.red_mask = 0xff0000;
      xvi.green_mask = 0xff00;
      xvi.blue_mask = 0xff;
      xvi.bits_per_rgb = 8;
	   xvi.depth = 24;
      xvis = XGetVisualInfo(dpy, xvimask, &xvi, &howmany);
      color_reversed = 1;
      if ( !howmany ) {
		   fprintf(stderr, "\nUnable to find a 24bit TrueColor visual\nexiting...\n\n");
		   exit(1);
      }
	}

	visual = xvis[0].visual;
	vdepth = xvis[0].depth;
   return(color_reversed);
}

/*****************************************************************************************************/

static int ErrorHandler( Display *dpy, XErrorEvent *error )
{
   int   trapped_error_code = error->error_code;
   char  err_msg[80];
   
   if (DEBUG) {
      XGetErrorText( dpy, error->error_code, err_msg, 80);
      fprintf(stderr,"!!!ErrorHandler: code =%d\t%s\n",trapped_error_code,err_msg);
      fprintf(stderr,"\t\tid:\tOx%x\n",(unsigned int)error->resourceid);
   }
   return(0);
}



Pixel GetPixelByName ( Widget w, char *colorname ) 
{
    Display *dpy  = XtDisplay( w );
    int      scr  = DefaultScreen( dpy );
    Colormap cmap = DefaultColormap( dpy, scr );
    XColor   color, ignore;

    if ( XAllocNamedColor( dpy, cmap, colorname, &color, &ignore ) ) 
        return( color.pixel );
    else
    {
        XtWarning ( "Couldn't allocate color" );
        return( BlackPixel( dpy, scr ) );
    }
}

static int CheckArgs( int argc, char **argv, char *look_for )
{
   int cc, len;
   
   len = strlen(look_for);
   for (cc=1;cc< argc;cc++) {
      if ( strncmp( argv[cc], look_for, len ) == 0 ) {
         return(cc);
      }
   }
   return(0);
}


void DeleteIconByIndex( int idx ) 
{
   RemoveIconEvents( &IconData[idx] );
   XtDestroyWidget( IconData[idx].w_label );
   XtDestroyWidget( IconData[idx].w_pix );
   XtDestroyWidget( IconData[idx].w );
   XtDestroyWidget( IconData[idx].w_popup_pix );
   XtDestroyWidget( IconData[idx].w_popup );

   XFreePixmap( dpy, IconData[idx].pixmap );
   XFreePixmap( dpy, IconData[idx].pixmap_sm );

   IconData[idx].w = NULL;
   return;
}      

static void MapAppWin( Widget w, XtPointer icon_data, XEvent *event, Boolean *flag )
{   
   icon_t         *cd = (icon_t *) icon_data;
   XButtonEvent   *pushbutts =  (XButtonEvent *)event;
   
   if (QUIT_DIALOG_VISIBLE) return;
   
   if ( pushbutts->button == 1 && (pushbutts->state & ControlMask ) ) {
      /* grab an icon for drop and drag swap */
      int idx;
      
      for (idx=0;idx<Nicons;idx++) {
         if ( (&IconData[idx] == cd) ) grabbed_icon_index = idx;
      }

   }   
   else if ( pushbutts->button == 1 ) { /* map the application window */      
      if ( cd->w != NULL ) {
         XMapWindow( dpy, cd->top );
         XRaiseWindow( dpy, cd->top );
         /* XtVaSetValues( w, XmNshadowType, XmSHADOW_ETCHED_IN, NULL ); */
      }      
   }
   else if ( pushbutts->button == 2 ) { /* hide bar */
      is_shrinking = TRUE;
      shrink_countdown = options.hidecount;
      demagnify( popped_icon, DEMAG_ALL );      
   }

   return;
}


static void AddIconEvents( icon_t *id )
{
   id->cbd = id;
   XtAddEventHandler( id->w, EnterWindowMask, FALSE, magnifyCallback, (XtPointer)id->cbd );
   XtAddEventHandler( id->w, ButtonPressMask|OwnerGrabButtonMask, FALSE, MapAppWin, (XtPointer)id->cbd );
   XtAddEventHandler( id->w, ButtonReleaseMask, FALSE, PostMenu, (XtPointer)id->cbd );
   XtAddEventHandler( id->w, KeyPressMask, FALSE, keyCallback, (XtPointer)id->cbd );    
   XtAddEventHandler( id->w_popup_pix, ButtonPressMask|OwnerGrabButtonMask, FALSE, MapAppWin, (XtPointer)id->cbd );
   XtAddEventHandler( id->w_popup_pix, ButtonReleaseMask, FALSE, PostMenu, (XtPointer)id->cbd );
   XtAddEventHandler( id->w_popup_pix, KeyPressMask, FALSE, keyCallback, (XtPointer)id->cbd );                                                                             
   return;
}

static void RemoveIconEvents( icon_t *id )
{
   XtRemoveEventHandler( id->w, EnterWindowMask, FALSE, magnifyCallback, (XtPointer)id->cbd );
   XtRemoveEventHandler( id->w, ButtonPressMask|OwnerGrabButtonMask, FALSE, MapAppWin, (XtPointer)id->cbd );
   XtRemoveEventHandler( id->w, ButtonReleaseMask, FALSE, PostMenu, (XtPointer)id->cbd );
   XtRemoveEventHandler( id->w, KeyPressMask, FALSE, keyCallback, (XtPointer)id->cbd );    
   XtRemoveEventHandler( id->w_popup_pix, ButtonPressMask|OwnerGrabButtonMask, FALSE, MapAppWin, (XtPointer)id->cbd );
   XtRemoveEventHandler( id->w_popup_pix, ButtonReleaseMask, FALSE, PostMenu, (XtPointer)id->cbd );
   XtRemoveEventHandler( id->w_popup_pix, KeyPressMask, FALSE, keyCallback, (XtPointer)id->cbd );                                                                              
   return;
}      

void SwapIcons( icon_t *icon1, icon_t *icon2 ) {

   icon_t   tmp;
   
   if (DEBUG) printf( "Swapping Icons: %s\t<--->\t%s\n",icon1->iconName, icon2->iconName );
   demagnify( popped_icon, DEMAG_ALL );
   RemoveIconEvents( icon1 );
   RemoveIconEvents( icon2 );
   tmp = *icon1;
   *icon1 = *icon2;
   *icon2 = tmp;
   AddIconEvents( icon1 );
   AddIconEvents( icon2 );
   
   return;
}


void SetIconName( icon_t *id, char *iconname )
{   
   XmString       xstr;

   if ( iconname != NULL ) {           
      if ( strncmp( iconname, id->iconName, MAX_NAME ) != 0 ) {
         strncpy(id->iconName,iconname, MAX_NAME);
         if ( strlen( iconname ) > LONG_ICON_NAME_LENGTH ) {
            xstr = XmStringCreateLtoR ( iconname , XmFONTLIST_DEFAULT_TAG );
            XtVaSetValues( id->w_label,
                  XmNlabelString, xstr,
                  XmNalignment, XmALIGNMENT_BEGINNING,
                  NULL );
            XmStringFree(xstr);
         }
         else {
            xstr = XmStringCreateLtoR ( iconname , XmFONTLIST_DEFAULT_TAG );
            XtVaSetValues( id->w_label,
                  XmNlabelString, xstr,
                  XmNalignment, XmALIGNMENT_CENTER,
                  NULL );
            XmStringFree(xstr);
         }                     
      }
   }
   
   return;
}

static void SetIconNameByIndex( int cc )
{
   
   char           *iconname;
   XmString       xstr;
   
   XGetIconName( dpy, IconData[cc].top, &iconname );
   if ( iconname == NULL ) XFetchName( dpy, IconData[cc].top, &iconname );
   if ( iconname != NULL ) {           
      if ( strncmp( iconname, IconData[cc].iconName, MAX_NAME ) != 0 ) {
         strncpy(IconData[cc].iconName,iconname, MAX_NAME);
         if ( strlen( iconname ) > LONG_ICON_NAME_LENGTH ) {
            xstr = XmStringCreateLtoR ( iconname , XmFONTLIST_DEFAULT_TAG );
            XtVaSetValues( IconData[cc].w_label,
                  XmNlabelString, xstr,
                  XmNalignment, XmALIGNMENT_BEGINNING,
                  NULL );
            XmStringFree(xstr);
         }
         else {
            xstr = XmStringCreateLtoR ( iconname , XmFONTLIST_DEFAULT_TAG );
            XtVaSetValues( IconData[cc].w_label,
                  XmNlabelString, xstr,
                  XmNalignment, XmALIGNMENT_CENTER,
                  NULL );
            XmStringFree(xstr);
         }                     
      }
   }
   
   return;
}


static void PackIcons(void)
{
   int cc, dd;   
/*    icon_t         *cbd;; */
   
   if (DEBUG) printf("packing list: were %d icons...",Nicons);
   if ( popped_icon != (icon_t *) NULL ) {
      if (DEBUG) printf("PackIcons: (packlist) popping down\n");
      demagnify( popped_icon, DEMAG_ONE );
   }
   for(cc=0;cc<Nicons;cc++){
      if ( IconData[cc].w == NULL ) {
         if (DEBUG) printf("icon %d is NULL\n",cc);       
         for(dd=cc;dd<Nicons-1;dd++) {

            if ( IconData[dd+1].w != NULL ) {
               RemoveIconEvents( &IconData[dd+1] );               
            }

            IconData[dd] = IconData[dd+1];
            if (DEBUG) printf("packing %d/%d (%s)\n",dd,Nicons,IconData[dd].iconName);
            if ( IconData[dd].w != NULL ) {
               AddIconEvents( &IconData[dd] );
            }
         }
         if (DEBUG) printf("\n");
         Nicons--;
      }
   }

   ICONBAR_HT = (Dimension) ceil( (double)(unit_wd*max(Nicons,1))/((double)scr_wd ) )*( (int)(ICON_HT*options.scale) + BTM_TRIM) + 1;
   ICONBAR_WD = CalcBarWidth();      

   XtVaSetValues( shell, XmNwidth, getWindowWidth(), XmNx, getHorzCenter(), NULL );
   ManageBarGeometry();

   addBorderToWindow(ICONBAR_WD);

   return;
}

/******************************************************************************/
/******************************************************************************/


Window         last_kid_win=(Window)NULL;
unsigned int   Nkids = 0;

void CollectIcons( void )
{
   static int     do_once = TRUE;
   unsigned int   cc, Nchildren;
   Widget         pix, label, popup, popup_pix;

   Window         root_win, parent_win, *children_win=(Window *)NULL,
                  top_win=(Window)NULL, icon_win=(Window)NULL;
   char           *windowname=NULL, *iconname=NULL;
   
   unsigned long  *property = NULL;
   unsigned long  nitems;
   unsigned long  leftover;
   Atom           actual_type, n;
   int            dd, repeat_client=FALSE, actual_format;
   Status         status;
   Arg            args[16];
   
   XQueryTree( dpy, rootWin, &root_win,
                    &parent_win, &children_win, &Nchildren);
   if (DEBUG) printf("CollectIcons: old kids = %d\tnew kids = %d\n",Nkids,Nchildren);
   if ( Nkids && Nchildren == Nkids && children_win[Nchildren-1] == last_kid_win ) {
      if (*children_win) {
         XFree ((Window *)children_win);
         *children_win=(Window)NULL;
      }  
      return;
   }
   else {
      Nkids = Nchildren;
      last_kid_win = children_win[Nchildren-1];
   }
   
   for (cc=0;cc<Nchildren;cc++) {
      status = XQueryTree( dpy, rootWin, &root_win, &parent_win, &children_win, &Nchildren);
      if ( (cc >= Nchildren) || (status == (Status)NULL) ) break;
      XFetchName( dpy, children_win[cc], &windowname );
      XGetIconName( dpy, children_win[cc], &iconname );      
      
      status = XGetWindowProperty (dpy, children_win[cc],
                  xa_WM_STATE, 0L, WM_STATE_ELEMENTS,
                  False, xa_WM_STATE, &actual_type, &actual_format,
                  &nitems, &leftover, (unsigned char **)&property);
      if ( !( (status==Success) && (actual_type==xa_WM_STATE) && (nitems==WM_STATE_ELEMENTS) ) ) {
         /*
          * The XmuClientWindow function finds a window, at or below the specified window,
          * that has a WM_STATE property. If such a window is found, it is returned; 
          * otherwise the argument window is returned.
          */ 
         top_win = XmuClientWindow(dpy, children_win[cc]);
         if ( top_win != (Window)NULL )
            repeat_client = FALSE;
         else
            repeat_client = TRUE;
         for (dd=0;dd<Nicons;dd++){
            if ( top_win == IconData[dd].top ) {
               repeat_client = TRUE;
               break;
            }
         }
         
         if ( top_win != children_win[cc]  && top_win != shellWin && repeat_client == FALSE  ) {

               XFetchName( dpy, top_win, &windowname );
               XGetIconName( dpy, top_win, &iconname );
               
               if ( iconname == NULL && windowname != NULL ) {
                  iconname = (char *) calloc( strlen(windowname), sizeof(char) );
                  strcpy( iconname, windowname );
               }
               else if ( iconname == NULL ) {
                  XClassHint  class_hints;
                                              
                  status = XGetClassHint( dpy, top_win, &class_hints );
                  if (DEBUG) printf(" 0x%lx in here - status = %d (%s)\n",
                        children_win[cc],status,class_hints.res_class);
                  if (status) {
                     windowname = (char *) calloc( strlen(class_hints.res_name), sizeof(char) );
                     snprintf( windowname, strlen(class_hints.res_name),
                           "%s",class_hints.res_name );
                     iconname = (char *) calloc( strlen(windowname), sizeof(char) );
                     strcpy( iconname, windowname );
                     XFree( class_hints.res_name );
                     XFree( class_hints.res_class );
                  }
               }
               if ( !windowname ) {
                  windowname = (char *) calloc( 5, sizeof(char) );
                  strcpy( windowname, "None");
                  iconname = (char *) calloc( strlen(windowname), sizeof(char) );
                  strcpy( iconname, windowname );
               }
               
               if (0) {
                  /* ------------------------------------------ */
                  /*    gets some info on multiple SGI desks    */
                  /*  <successfully sets property (no effect)>  */
                  /* <<also uneccessary with overrideredirect>> */
                  /* ------------------------------------------ */
                  if ( iconname != NULL && strncmp(windowname,"Desks Overview",14) == (int)NULL ) {
                     Atom  xa_SGI_DESKS_HINTS, xa_SGI_DESKS_ALWAYS_GLOBAL, actual_type;
                     int   actual_format, rtn;
                     unsigned long  nitems, leftover;
                     unsigned char  *data = NULL;
                     long  bufsize = 128;
                     char  *names_rtn, *atom_name;

                     xa_SGI_DESKS_HINTS = XInternAtom( dpy, "_SGI_DESKS_HINTS", False);
                     rtn = XGetWindowProperty(  dpy, top_win, xa_SGI_DESKS_HINTS, 0L, bufsize,
                                 FALSE, AnyPropertyType, &actual_type, &actual_format,
                                 &nitems, &leftover, &data );
                     if (rtn == Success) {
                        names_rtn = XGetAtomName(dpy, actual_type);
                        atom_name = XGetAtomName(dpy, *((Atom *)data)); 
                        printf("info on Desks Overview:\t%s(%s)[format=%d][nitems=%ld][bytesleft=%ld]\n", 
                              atom_name, names_rtn, actual_format, nitems, leftover );
                        XFree( data );
                        XFree( names_rtn );
                     }
                     xa_SGI_DESKS_ALWAYS_GLOBAL = XInternAtom( dpy, "_SGI_DESKS_ALWAYS_GLOBAL", False);
                     XChangeProperty( dpy, shellWin, xa_SGI_DESKS_HINTS, XA_ATOM, 32, PropModeReplace,
                           (unsigned char *)&xa_SGI_DESKS_ALWAYS_GLOBAL, 1 );
                  }
                  /* ----------------------------------- */
               }
               
               if ( toolchestWin == (Window)NULL && options.toolchestManage && 
                     ( strcmp(windowname,"ToolChest" )==0 || strcmp(windowname,"toolchest" )==0 ) )
               {                  
                  toolchestWin = top_win;
                  XMoveWindow( dpy, toolchestWin, options.toolchestX, options.toolchestY );
                  XRaiseWindow( dpy, toolchestWin );
                  XSelectInput( dpy, toolchestWin, VisibilityChangeMask );
                  
               }
               if ( iconname != NULL && !isIgnorableApp( windowname ) )
               {
                  status = XGetWindowProperty (dpy, top_win,
                        xa_WM_STATE, 0L, WM_STATE_ELEMENTS,
                        False, xa_WM_STATE, &actual_type, &actual_format,
                        &nitems, &leftover, (unsigned char **)&property);

                  icon_win = property[1];

                  if ( DEBUG && do_once ) {
                     printf("[top windows is really:\t\t%s\n",windowname);
                     printf("\t\ticonname:\t%s\n",iconname);
                     if ( status == Success && actual_type == xa_WM_STATE && nitems == WM_STATE_ELEMENTS ) {
                        if ( property[0] == IconicState) printf("\t\tWM_STATE:\ticonic\n");
                        else if ( property[0] == NormalState) printf("\t\tWM_STATE:\tnormal\n");
                        else if ( property[0] == WithdrawnState) printf("\t\tWM_STATE:\twithdrawn\n");
                     }               
                     printf("\t\ticon win: Ox%lx",property[1]);
                     printf("\n");               
                  }

                  if ( status == Success && icon_win && property[0] != WithdrawnState ) {

                     /**** create the icon widgets ****/
                     /* XSelectInput( dpy, top_win, StructureNotifyMask ); */ 
                     XSelectInput( dpy, children_win[cc], StructureNotifyMask ); 
                     XSelectInput( dpy, top_win, PropertyChangeMask );

                     w = XtVaCreateManagedWidget( iconname, xmFormWidgetClass, bar,                           
                              XmNresizable, TRUE,  
                              XmNresizePolicy, XmRESIZE_ANY,
                              XmNrubberPositioning, FALSE,
                              XmNrow, 0, XmNcolumn, Nicons+1, /* exitB in slot 0 */
                              XmNmarginWidth, options.margin,
                              XmNwidth, unit_wd,
                              XmNheight, ICON_HT,
                              XmNy, 0,
                              XmNx, (Nicons)*unit_wd,
                              XmNvisual, visual,
			                     XmNdepth, vdepth,
			                     XmNcolormap, private_colormap,
			                     NULL);
                     
                     pix = XtVaCreateManagedWidget( "iconpixmap", widgetClass, w,
                              XmNborderWidth, 0,                                 
                              XmNheight, (Dimension)(ICON_HT*options.scale),
                              XmNwidth, (Dimension)(ICON_WD*options.scale),                           
                              XmNtopAttachment, XmATTACH_FORM,
                              XmNleftAttachment, XmATTACH_FORM,
                              XmNrightAttachment, XmATTACH_NONE,
                              XmNbottomAttachment, XmATTACH_NONE,
                              XmNvisual, visual,
                              XmNdepth, vdepth,
                              XmNcolormap, private_colormap,
                              NULL);                                 
                                 
                     label = XtVaCreateManagedWidget( "iconlabel", xmLabelWidgetClass, w,
                              XmNalignment, XmALIGNMENT_CENTER,
                              XmNmarginHeight, 0,
                              XmNmarginRight, 4,
                              XmNmarginLeft, 4,
                              XmNrecomputeSize, FALSE,
                              XmNheight, (Dimension)(BTM_TRIM),
                              XmNwidth, (Dimension)(ICON_WD*options.scale),
                              XmNpushButtonEnabled, FALSE,
                              XmNshadowType, XmSHADOW_ETCHED_OUT,
                              XmNlabelString, XmStringCreateLtoR ( iconname , XmFONTLIST_DEFAULT_TAG ) ,
                              XmNtopAttachment, XmATTACH_WIDGET,
                              XmNbottomAttachment, XmATTACH_FORM,                                 
                              XmNleftAttachment, XmATTACH_FORM,
                              XmNrightAttachment, XmATTACH_FORM,
                              XmNtopWidget, pix,
                              XmNtraversalOn, FALSE,
			                     NULL); 
                                                      
                     if ( strlen( iconname ) > LONG_ICON_NAME_LENGTH ) {
                        XtVaSetValues( label, XmNalignment, XmALIGNMENT_BEGINNING, NULL );
                     }
                     else {
                        XtVaSetValues( label, XmNalignment, XmALIGNMENT_CENTER, NULL );
                     }                                
                     n = 0;
                     XtSetArg( args[n], XmNmwmDecorations, (long) NULL );n++;
                     XtSetArg( args[n], XmNmwmFunctions, (long) NULL );n++;
                     XtSetArg( args[n], XmNoverrideRedirect, TRUE);  n++;
                     XtSetArg( args[n], XmNborderWidth, 0 ); n++;
                     XtSetArg( args[n], XmNcolormap, private_colormap ); n++;
                     XtSetArg( args[n], XmNvisual, visual ); n++;
                     XtSetArg( args[n], XmNdepth, vdepth ); n++;
                     popup = XtCreatePopupShell( "iconpopup", overrideShellWidgetClass, bar, args, n );
                     
                     popup_pix = XtVaCreateManagedWidget( "iconpopupPixmap", widgetClass, popup,
                                 XmNborderWidth, 1,
                                 XmNheight, (Dimension)(ICON_HT),
                                 XmNwidth, (Dimension)(ICON_WD),
                                 XmNvisual, visual,
                                 XmNdepth, vdepth,
                                 XmNcolormap, private_colormap,
                                 NULL);                     
                     XtRealizeWidget(popup);
                     

                     /* store data in structure array */                 
                     IconData[Nicons].top = top_win;
                     IconData[Nicons].top_parent = children_win[cc];
                     IconData[Nicons].icon = icon_win;
                     IconData[Nicons].state = property[0];
                     IconData[Nicons].w = w;
                     IconData[Nicons].w_pix = pix;
                     IconData[Nicons].w_label = label;
                     IconData[Nicons].w_popup = popup;
                     IconData[Nicons].w_popup_pix = popup_pix;
                     strncpy(IconData[Nicons].winName,windowname, MAX_NAME);
                     strncpy(IconData[Nicons].iconName,iconname, MAX_NAME);
                     IconData[Nicons].cbd = &IconData[Nicons];
                     IconData[Nicons].popped = FALSE;
                     IconData[Nicons].x = 0;
                     IconData[Nicons].y = 0;
                     
                     AddIconEvents( &IconData[Nicons] );
                     IconData[Nicons].remote = checkClientMachineProp(&IconData[Nicons]);
                     geticon( &IconData[Nicons] );
                     
                     /* set icon pixmap in bar and popup widgets */                   
                     XtVaSetValues( pix, XmNbackgroundPixmap, IconData[Nicons].pixmap_sm, NULL );
                     XtVaSetValues( popup_pix, XmNbackgroundPixmap, IconData[Nicons].pixmap, NULL );                                       
                                          
                     Nicons++;
                     
                     /* resize icon data array */
                     /* ---------------------- */
                     if ( Nicons >= max_clients ) {
                        
                        XSync(dpy, FALSE);
                        max_clients += 25;
                        IconData = (icon_t *) realloc( IconData, max_clients*sizeof(icon_t) );
                        if ( IconData == (icon_t *)NULL ) {
                           fprintf(stderr,"CollectIcons: error while enlarging icon data array...exiting\n");
                           NiceExit();
                        }
                        popped_icon = (icon_t *)NULL;
                        for (cc=0;cc<Nicons;cc++) {
                           RemoveIconEvents( &IconData[cc] );
                           AddIconEvents( &IconData[cc] );
                        }
                        
                     }
                     /* ---------------------- */                     
                     if ( property[0] == IconicState ) 
                        XUnmapWindow( dpy, icon_win );
                                  
                     icon_win = (Window)NULL;
                                           
                     if ( popped_icon ) {
                        demagnify( popped_icon, DEMAG_ONE );
                     }
                     
                     /* pop down menu to prevent accidental grab lockout */
                     if ( menu_popped_up ) {
                        XtUnmanageChild(menu);
                        menu_popped_up = FALSE;
                     }
                  }            
               }

            }
            if (property && nitems) {
               XFree(property);
               property = NULL;
            }
            if (windowname) {
               XFree(windowname);
               windowname = NULL;
            }
            if (iconname) {
               XFree(iconname);
               iconname = NULL;
            }
            if (*children_win) {
               XFree (children_win);
               *children_win=(Window)NULL;
            }
      }            
      
   }
   ICONBAR_HT = (Dimension) ceil( (double)(unit_wd*Nicons)/(double)scr_wd )*( (int)(ICON_HT*options.scale) + BTM_TRIM ) + 1;
   ICONBAR_WD = CalcBarWidth();
   
   XtVaSetValues( shell, XmNwidth, getWindowWidth(), XmNx, getHorzCenter(), NULL );
   ManageBarGeometry();
   addBorderToWindow(ICONBAR_WD);

   do_once = FALSE;
   return;
}

/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/



static void TimeoutCallback( XtPointer icon_data, XtIntervalId *id )
{
/*    Widget         fw; */
   Dimension      ht, new_ht;   
   Window         root_win, child_win;
   int            root_x, root_y, win_x, win_y;
   unsigned int   buttons;
   
   if ( Nicons < 1 ) {
      ICONBAR_WD = unit_wd;
      XtVaSetValues( shell, XmNwidth, getWindowWidth(), XmNx, getHorzCenter(), NULL );
   }
   
   if ( !is_growing && !is_shrinking ) {      
      
      /* delay autohide/shrink of window */
      if ( !is_hidden && !menu_popped_up ) {
         XQueryPointer( dpy, shellWin, &root_win, &child_win, &root_x, &root_y, &win_x, &win_y, &buttons );
         if ( child_win == None ) {
             
            if ( popped_icon != (icon_t *)NULL ) {
               XQueryPointer( dpy, XtWindow(popped_icon->w_popup), &root_win, &child_win, &root_x, &root_y,
                  &win_x, &win_y, &buttons );
            }
            if ( child_win == None ) {
               if (DEBUG) printf("%d\n",shrink_countdown);
               if ( popped_icon != (icon_t *)NULL ) {
                  
                  /***************************************************************/
                  /* (only) this demagnifies an icon when the pointer completely */
                  /*               leaves the iconbar shell window               */
                  /***************************************************************/                  
                  demagnify( popped_icon, DEMAG_ALL );
	       }
               if ( shrink_countdown-- < 0 && !QUIT_DIALOG_VISIBLE ) {
                  is_shrinking = TRUE;                  
                  shrink_countdown = options.hidecount;
               }
            }
            else if (DEBUG) printf(">Focus window = %lx (%d)\n", child_win, shrink_countdown);
         }
         else if (DEBUG) printf("Focus window = %lx (%d)\n", child_win, shrink_countdown);
      }
            
   }      
   else if ( is_shrinking ) {
      
      if ( options.hide ) { /* Hide Bar enabled */  
         XtVaGetValues( shell, XmNheight, &ht, NULL );
         if ( ht > options.hideheight ); {
            new_ht = max( ht - getWindowHeight()/options.animate, options.hideheight );      
            XtVaSetValues( shell, XmNheight, new_ht, XmNy, scr_ht-new_ht, NULL );
         }
         if ( new_ht > options.hideheight ) {
            is_shrinking = TRUE;
            is_hidden = FALSE;
            GUI_UPDATE_INTERVAL=GUI_FAST_UPDATE_INTERVAL;
         }
         else {
            is_shrinking = FALSE;
            is_hidden = TRUE;
            GUI_UPDATE_INTERVAL=GUI_SLOW_UPDATE_INTERVAL;
         }
      }
      else { /* Hide Bar disabled */
         is_shrinking = FALSE;
      }
   }   
   else if ( is_growing ) {
      is_hidden = FALSE;
      XtVaGetValues( shell, XmNheight, &ht, NULL );      
      if ( ht < ICONBAR_HT*(Dimension)ceil(Nicons*ICON_WD/(scr_wd/2)) ); {
         new_ht = min( ht + getWindowHeight()/options.animate, getWindowHeight() );      
         XtVaSetValues( shell, XmNheight, new_ht, XmNy, scr_ht-new_ht, NULL );
	 /* XSync( dpy, TRUE ); */
      }
      if ( new_ht < ICONBAR_HT ) {
         is_growing = TRUE;
         GUI_UPDATE_INTERVAL=GUI_FAST_UPDATE_INTERVAL;
      }
      else {
         is_growing = FALSE;
         GUI_UPDATE_INTERVAL=GUI_SLOW_UPDATE_INTERVAL;
      }
   }
  
   MAG_ACTION = FALSE;
   
   /* re-register the Timeout */   
   XtAppAddTimeOut ( app, GUI_UPDATE_INTERVAL, TimeoutCallback, NULL );
   
   return;
}


static void growCallback( Widget w, XtPointer IconData, XEvent *event, Boolean *flag )
{      
   Dimension ht;
   
   XSetInputFocus( dpy, XtWindow(bar), RevertToParent, CurrentTime);
   
   if ( !is_growing && !is_shrinking ) {
      XtVaGetValues( shell, XmNheight, &ht, NULL );
      if ( ht < ICONBAR_HT && Nicons > 0 ) {
         is_growing = TRUE;
      }
   }
   return;
}

static void keyCallback( Widget w, XtPointer IconData, XEvent *event, Boolean *flag )
{      
   XKeyEvent   *key_event = (XKeyEvent *) event;
   KeySym      keysym_rtn;
   Modifiers   mods_rtn;
   
   XtTranslateKeycode( dpy, key_event->keycode, key_event->state, &mods_rtn, &keysym_rtn ); 
   
   if (DEBUG) printf("keypress event : keycode(0x%x) keysym(0x%x)\n",(unsigned int)key_event->keycode, (unsigned int)keysym_rtn);
   
   if ( key_event->keycode == 0x10 || keysym_rtn == XK_space ) {
      is_shrinking = TRUE;
      shrink_countdown = options.hidecount;
      demagnify( popped_icon, DEMAG_ALL );      
   }
   
   return;
}

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

static void magnifyCallback( Widget w, XtPointer icon_data, XEvent *event, Boolean *flag)
{
   icon_t        *cd = (icon_t *) icon_data;
   int            x,y, /* cc, */ root_x, root_y, child_x, child_y;
   unsigned int   mask_rtn;
   Bool           rtn;
   Window         child_win_rtn, root_win, child_win;
         
   if (QUIT_DIALOG_VISIBLE) return;   
   /* check to see if the pointer is still in the window that triggered this event */ 
   /* ---------------------------------------------------------------------------- */     
   if ( XQueryPointer( dpy, barWin, &root_win, &child_win, &root_x, &root_y, &child_x, &child_y, &mask_rtn ) ) {
      if ( cd->w != NULL && child_win != XtWindow(cd->w) ) {
	      return;
      }
   }
   else
      return;
   /* ---------------------------------------------------------------------------- */   
   
   if ( popped_icon && (popped_icon->w_popup != cd->w_popup) ) {      
      demagnify( popped_icon, DEMAG_ONE );
   }
   
   if (!options.magnify) {
      /* don't magnify the icon, just highlight the label */
      if ( cd->w != NULL )
         popped_icon = cd; 
      XtVaSetValues( cd->w_label, XmNforeground, options.highlightFG, NULL);
      return;
   }
   
   
   if (  !is_growing && !is_shrinking ) {     
       
      popped_icon = cd;      
      ICONBAR_WD = CalcBarWidth();
      XtVaSetValues( shell, XmNwidth, getWindowWidth(), XmNx, getHorzCenter(), NULL );
      addBorderToWindow(ICONBAR_WD);

      if ( cd->w != NULL ) {

         Dimension   width, height;
         
         /* resize the icon(s) */
         height =  (Dimension)(ICON_HT*options.scale + BTM_TRIM);
         width = (Dimension)(ICON_WD + 2*options.margin);
         XtResizeWidget( cd->w, width, height, (Dimension)0 );
         XtVaSetValues( cd->w_label, XmNforeground, options.highlightFG, NULL);       
            
         /* popup enlarged icon at proper coordinates */
         cd->popped = TRUE;
         ManageBarGeometry();
         rtn = XTranslateCoordinates( dpy, XtWindow(cd->w), rootWin, 0, 0, &x, &y, &child_win_rtn );
         y -= (Dimension)ceil( (double)ICON_HT - (double)ICON_HT*(double)options.scale);  
         if ( cd->w && rtn ) {
            XtVaSetValues( cd->w_popup, XmNx, x+options.margin, XmNy, y, NULL);
         } 
         else
            printf("magnifyCallback: huh?\n");
         
         XtPopup( cd->w_popup, XtGrabNonexclusive );         
         XSetInputFocus( dpy, XtWindow(cd->w_popup), RevertToParent, CurrentTime);
         XFlush( dpy ); 
      }
    
   }
 


   if (DEBUG2) printf("\nleaving magnify callback <<<<<<<<\n");     
   return;
}


void demagnify( icon_t *cd, int type )
{
   if (DEBUG2) printf("\n***entered demagnify, type %d\n", type);
   
   if ( cd != NULL ) {
      if ( cd->w != NULL ) {
         Dimension   width, height;

         height =  (Dimension)( (int)(ICON_HT*options.scale) + BTM_TRIM );
         width = (Dimension)( (int)(ICON_WD*options.scale) + 2*options.margin );
         XtResizeWidget( cd->w, width, height, (Dimension)0 );
         XtVaSetValues( cd->w_label, XmNforeground, options.iconlabelFG, NULL); 
         XtPopdown( cd->w_popup );
         cd->popped = FALSE;
      }
   }
   popped_icon = (icon_t *) NULL;
   if ( type == DEMAG_ALL ) {
      ICONBAR_WD =  CalcBarWidth();
      XtVaSetValues( shell, 
            XmNwidth, getWindowWidth(),
            XmNx, getHorzCenter(),
            NULL );      
      ManageBarGeometry();      
      addBorderToWindow(ICONBAR_WD);
      XUngrabKeyboard( dpy, CurrentTime );
   }

   if (DEBUG2) printf("\nleft demagnify***\n");

   return;
}


void SetSignalHandler(void)
{
   int   signum;
   char  errmsg[128];
   
   struct sigaction most_sa = {SA_SIGINFO,SignalHandler}; 

   for (signum=1;signum<33;signum++){
      switch (signum) {
         case SIGSTOP:
         case SIGKILL:
         case SIGSEGV:
         case SIGBUS:   
            break;
         default:
            if ((int)sigaction( signum, &most_sa, NULL ) == -1) {
               sprintf(errmsg,"[setup_signals]: Error setting most_sighandler (%i)",signum);
		         perror(errmsg);
		         exit(1);
	         }
      }
   }
   return;
}

void SignalHandler( int sig, siginfo_t *sip, ucontext_t *up )
{  
   fprintf(stderr,"\nSignalHandler: signal number %i caught!\a\n\n",sig);
   if ( sig == SIGINT || sig == SIGHUP ) NiceExit();
   return;
}

void NiceExit(void) {
	//int cc;

	fprintf(stderr,"iconbar aborting...\n\n");
/*    for (cc=0;cc<Nicons;cc++) {
      if ( IconData[cc].state == IconicState ) {
         XSync( dpy, FALSE );
         XMapWindow( dpy, IconData[cc].top );
         XSync( dpy, FALSE );
         XIconifyWindow( dpy, IconData[cc].top, scrnNum );
         XSync( dpy, FALSE );
      }         
   }  */

   system("tellwm restart");
   exit(2);
}

void ExitCallback( Widget w, XtPointer IconData, XtPointer callData)
{
   NiceExit();
}


static void ManageBarGeometry(void)
{
   int         cc, row=0, col=0;
   Dimension   x=0,y=0;
   int border = getBorderWidth();

   x = border;
   y = getBorderHeight();

   for (cc=0;cc<Nicons;cc++) {
      if ( IconData[cc].w != NULL ) {
         if ( IconData[cc].x != x || IconData[cc].y != y ) {
            XtMoveWidget( IconData[cc].w, x, y );
            IconData[cc].x = x;
            IconData[cc].y = y;
            IconData[cc].row = row;
            IconData[cc].col = col++;
         }
         if ( IconData[cc].popped )
            x += ICON_WD + 2*options.margin;      
         else
            x += unit_wd;
        
         if ( x > max_width_popped - unit_wd ) {
            /* next row */
            x = (Dimension)0;
            y += (Dimension)( ICON_HT*options.scale + BTM_TRIM + 1 );
            col = 0;
            row++;
            last_row = row;
         }
      }
   }

   return;
}

static Dimension CalcBarWidth(void)
{
  
   if ( Nicons < 1 ) {
      return((Dimension)unit_wd);
   }
   if ( popped_icon && (popped_icon->row != last_row || last_row == 0) ) {
      ICONBAR_WD =  min( (unit_wd*(Nicons-1) + ICON_WD + 2*options.margin), max_width_popped );
   }
   else {
      ICONBAR_WD = min( unit_wd*Nicons, max_width );
   }
  
   return(ICONBAR_WD);
}

static void parseIgnorableApps(XmString xstr)
{
   int   cc = 0;
   char  *text, *token;
      
   /* XmStringGetLtoR( options.ignoreApps, XmFONTLIST_DEFAULT_TAG, &text ); */
   
   /* this is a kludge but, XmStringGetLtoR was bombing */
   text = (char *) calloc(  strlen( (char *)xstr )+2, sizeof( char ) );
   snprintf( text, strlen( (char *)xstr )+1, "%s", (char *)xstr );   
   if (DEBUG) printf("parseIgnorableApps: ignoreApps = %s\n",text);
   
   token = strtok( text, ":" );
   while ( token ) {
      if (DEBUG) printf("%s\n",token);
      IgnoreNamesArray = (char **) realloc( IgnoreNamesArray, (cc+1)*sizeof( char * ) );
      IgnoreNamesArray[cc] = (char *) calloc(  strlen( (char *)token )+1, sizeof( char ) );
      strcpy( IgnoreNamesArray[cc], token );
      token = strtok( NULL, ":");
      cc++;
   }
   IgnoreNamesArray = (char **) realloc( IgnoreNamesArray, (cc+1)*sizeof( char * ) );
   IgnoreNamesArray[cc] = (char *)NULL;
   
   return;
}

static int isIgnorableApp( char *windowname )
{
   int cc=0;
   
   while ( IgnoreNamesArray[cc] != NULL ) {
      if ( strcmp( IgnoreNamesArray[cc], windowname) == 0 ) {
         if (DEBUG) printf("isIgnorableApp: ignoring %s\n",windowname);
         return(TRUE);
      }
      cc++;
   }
   return(FALSE);
}

static int checkClientMachineProp( icon_t *id )
{
   int   actual_format, rtn;
   unsigned long  nitems, leftover;
   unsigned char  *data = NULL;
   long  bufsize = 128;
   Atom  xa_WM_CLIENT_MACHINE = (Atom)NULL, actual_type;
   char host_id[1024];

   if (!options.tint)
      return(0);
   
   if( gethostname(host_id, 1023) ) {
	   return(0);
   }
 
   xa_WM_CLIENT_MACHINE = XInternAtom( dpy, "WM_CLIENT_MACHINE", False);
   rtn = XGetWindowProperty(  dpy, id->top, xa_WM_CLIENT_MACHINE, 0L, bufsize,
                                 FALSE, AnyPropertyType, &actual_type, &actual_format,
                                 &nitems, &leftover, &data );
   if (rtn == Success) {
      /* printf("info on Client Machine:\t%s[format=%d][nitems=%ld][bytesleft=%ld]\n", 
            data, actual_format, nitems, leftover ); */
      if ( strncmp( (const char *)data, host_id, nitems ) == 0 ) {
         /* this is a local application */
         XFree( data );
         return(0);
      }
      else { /* this is a remote application */
         char remote_name[MAX_NAME+1];
         
         snprintf( remote_name, MAX_NAME, "%s@%s", id->iconName, (const char *)data );
         SetIconName(id, remote_name);
         XFree( data );
         
         return(1);
      }
   }  
   return(0);
}
