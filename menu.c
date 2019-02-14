
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

#include "iconbar.h"
#include <Xm/ToggleB.h>
#include <Xm/Separator.h>
#include <Xm/MessageB.h>


extern Visual *visual;

static Widget  exitB, hideB, magB, raiseB, toolchestB, blendB, leftB, rightB, closeB, remoteB;
static icon_t  *target_icon;

static void ToggleStateCallback( Widget w, XtPointer IconData, XtPointer callData);
static void RemoteStateCallback( Widget w, XtPointer clientData, XtPointer callData);
static void CreateMenu( Widget parent );
static void popDownCallback( Widget w, XtPointer clientData, XtPointer callData);
static void CloseAppCallback( Widget w, XtPointer clientData, XtPointer callData);
static void RaiseAppCallback( Widget w, XtPointer clientData, XtPointer callData);
static void LowerAppCallback( Widget w, XtPointer clientData, XtPointer callData);
static void RestoreAppCallback( Widget w, XtPointer clientData, XtPointer callData);
static void MinimizeAppCallback( Widget w, XtPointer clientData, XtPointer callData);
static void MinimizeAllCallback( Widget w, XtPointer clientData, XtPointer callData);

void PostMenu( Widget w, XtPointer icon_data, XEvent *event, Boolean *flag )
{   
   char           str[21];
   unsigned int   k;
   XmString       xstr;
   XButtonEvent   *pushbutts =  (XButtonEvent *)event;
   
   target_icon = (icon_t *) icon_data;
   
   if (QUIT_DIALOG_VISIBLE) return;

   if ( pushbutts->button == Button1 && (pushbutts->state & ControlMask ) ) {
      /* drop an icon for drop and drag swap */
      SwapIcons( target_icon, &IconData[grabbed_icon_index] ); 
   }
   if ( pushbutts->button == Button3  ) {
      /* pop-up menu */
      
      if ( menu == NULL ) CreateMenu( bar );
      
      /* position menu over cursor */
      event->xbutton.y_root -= 320; /* move the menu up a bit */
      event->xbutton.x_root -= 5;
      XmMenuPosition( menu, ( XButtonPressedEvent * ) event );      
      
      if ( w == IconData[0].w_popup_pix || w == IconData[0].w_pix || w == IconData[0].w_label ) 
         XtVaSetValues( leftB, XmNsensitive, FALSE, NULL );
      else XtVaSetValues( leftB, XmNsensitive, TRUE, NULL );
      
      k = Nicons-1;
      if ( w == IconData[k].w_popup_pix || w == IconData[k].w_pix || w == IconData[k].w_label )
         XtVaSetValues( rightB, XmNsensitive, FALSE, NULL );
      else XtVaSetValues( rightB, XmNsensitive, TRUE, NULL );
      
      snprintf( str, 17, "Close %s", target_icon->iconName);
      if ( strlen(target_icon->iconName) > 11 ) strcat( str, "..." );      
      xstr = XmStringCreateLtoR ( str, XmFONTLIST_DEFAULT_TAG );
      XtVaSetValues( closeB, XmNlabelString, xstr, NULL );
      XmStringFree(xstr);
      
      /* remote toggle */
      XtVaSetValues( remoteB, XmNset, target_icon->remote, NULL );
      
      XtManageChild( menu );
      menu_popped_up = TRUE;
   }
   
   return;
}

static void CancelCallback( Widget w, XtPointer clientData, XtPointer callData)
{
   Widget dialog_shell = (Widget)clientData;

   XtUnmanageChild( dialog_shell );
   QUIT_DIALOG_VISIBLE = FALSE;
   CollectIcons(); 
   return;
} 


void QuitDialogCallback( Widget w, XtPointer clientData, XtPointer callData)
{
   static Widget  dialog_shell = NULL;
   Widget         dialog_form, message, cancel, ok;
   char           QuitMessage[]={"Really Exit Iconbar?"},
                  title[] = {"Verify exit"};
   
   if ( dialog_shell == NULL ) {
      int   n, wd = 187, ht = 70;
      Arg   args[16];
      XmString       xstr;
      
      n = 0;
      XtSetArg( args[n], XmNmwmFunctions, (long) NULL );n++;
      XtSetArg( args[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL ); n++;
      XtSetArg( args[n], XmNy, (Dimension)(scr_ht-(last_row+3)*ICON_HT ) ); n++;
      XtSetArg( args[n], XmNx, (Dimension)(scr_wd/2) ); n++;
      XtSetArg( args[n], XmNwidth, wd ); n++;
      XtSetArg( args[n], XmNheight, ht ); n++;
      dialog_shell = XtCreatePopupShell( title, transientShellWidgetClass, bar, args, n );
      dialog_form = XtVaCreateManagedWidget( "quitDialogForm", xmFormWidgetClass, dialog_shell,
                        XmNwidth, wd,
                        XmNheight, ht,
                        XmNvisual, visual, NULL );
      xstr = XmStringCreateLtoR ( QuitMessage, XmFONTLIST_DEFAULT_TAG );
      message = XtVaCreateManagedWidget( "quitDialogMessage", xmLabelWidgetClass, dialog_form,
                        XmNlabelString, xstr,
                        XmNalignment, XmALIGNMENT_CENTER,
                        XmNheight, 30,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNrightAttachment, XmATTACH_FORM,            
                        XmNbottomAttachment, XmATTACH_NONE,
                        XmNvisual, visual, NULL );      
      XmStringFree(xstr);
      xstr = XmStringCreateLtoR ( "Cancel", XmFONTLIST_DEFAULT_TAG );
      cancel = XtVaCreateManagedWidget( "canelDialogButton", xmPushButtonWidgetClass, dialog_form,
                        XmNlabelString, xstr,
                        XmNalignment, XmALIGNMENT_CENTER,            
                        XmNtopAttachment, XmATTACH_WIDGET,
                        XmNtopWidget, message,
                        XmNleftAttachment, XmATTACH_POSITION,
                        XmNleftPosition, 55,
                        XmNrightAttachment, XmATTACH_POSITION,
                        XmNrightPosition, 85,                                    
                        XmNbottomAttachment, XmATTACH_POSITION,
                        XmNbottomPosition, 85,
                        XmNmarginHeight, 10,
                        XmNmarginWidth, 10,
                        XmNrecomputeSize, TRUE,            
                        XmNvisual, visual, NULL );
      XtAddCallback( cancel, XmNactivateCallback, CancelCallback, (XtPointer)dialog_shell );
      XmStringFree(xstr);
      xstr = XmStringCreateLtoR ( "OK", XmFONTLIST_DEFAULT_TAG );
      ok = XtVaCreateManagedWidget( "okDialogButton", xmPushButtonWidgetClass, dialog_form,
                        XmNlabelString, xstr,
                        XmNalignment, XmALIGNMENT_CENTER,            
                        XmNtopAttachment, XmATTACH_WIDGET,
                        XmNtopWidget, message,
                        XmNleftAttachment, XmATTACH_POSITION,
                        XmNleftPosition, 15,
                        XmNrightAttachment, XmATTACH_POSITION,
                        XmNrightPosition, 45,
                        XmNbottomAttachment, XmATTACH_POSITION,
                        XmNbottomPosition, 85,
                        XmNmarginHeight, 10,
                        XmNmarginWidth, 10,
                        XmNrecomputeSize, TRUE,
                        XmNvisual, visual, NULL );
      XtAddCallback( ok, XmNactivateCallback, ExitCallback, (XtPointer)dialog_shell );
      XmStringFree(xstr);  
   }
   XtManageChild( dialog_shell );
   QUIT_DIALOG_VISIBLE = TRUE;
   return;
}  

icon_t *WidgetToIcon( Widget w ) {
   int      cc;
   icon_t   *id = target_icon;
   
   for ( cc=0; cc<Nicons; cc++ ) {
      id = &IconData[cc];
      if ( w == id->w_popup_pix || w == id->w ) {
          return(id);
      }
      printf("w = 0x%lx\n",XtWindow(w));
   }
   
   fprintf(stderr,"WidgetToIcon: couldn't find icondata for widget\n");
   return(NULL);
}

static void FileCancelCallback( Widget w, XtPointer clientData, XtPointer callData)
{
   XtUnmanageChild(w);
   QUIT_DIALOG_VISIBLE = FALSE;
   return;
} 

static void FileOKCallback( Widget w, XtPointer clientData, XtPointer callData)
{
   XmFileSelectionBoxCallbackStruct *cbs = (XmFileSelectionBoxCallbackStruct *) callData;
   
   char *filename=NULL;
   
   XtUnmanageChild(w);
   QUIT_DIALOG_VISIBLE = FALSE;
         
   if ( clientData == options.border ) {
      XmString    xstr, xstr2;
      Widget      dialog;
      Dimension   scr_ht, ht;
         
      /* replace border mask file */
      XmStringGetLtoR( cbs->value, XmFONTLIST_DEFAULT_TAG, &filename );
      /* XmStringFree(options.border); */
      options.border = (XmString)filename; /* kludge! */
      /* options.border = XmStringCreateLtoR ( filename, XmFONTLIST_DEFAULT_TAG ); *//* <-- didn't work ! */
      
      xstr = XmStringCreateLtoR ( "*image.rgb", XmFONTLIST_DEFAULT_TAG );
      xstr2 = XmStringCreateLtoR ( "Select RGB file for border (image)", XmFONTLIST_DEFAULT_TAG );
      dialog = XmCreateFileSelectionDialog( w, "openImageFile", NULL, 0 );
      XtVaSetValues( dialog, 
            SgNviewerFilter, xstr,
            XmNpattern, xstr,
            SgNbrowserFileMask, SgFILE_NOT_HIDDEN,
            SgNviewerMode, SgVIEWER_AUTOMATIC,
            XmNfileListLabelString, xstr2,
            NULL );
      XmStringFree(xstr);
      XmStringFree(xstr2);
      XtAddCallback(dialog, XmNokCallback, FileOKCallback, (XtPointer)options.image );
      XtAddCallback(dialog, XmNcancelCallback, FileCancelCallback, NULL);
      XtManageChild(dialog);
      scr_ht = (Dimension)DisplayHeight( dpy, scrnNum );
      XtVaGetValues( dialog, XmNheight, &ht, NULL );
      XtVaSetValues( dialog, XmNy, (scr_ht - getWindowHeight() - ht - 10), NULL );
      QUIT_DIALOG_VISIBLE = TRUE;      
   }
   else {
      /* replace border image file */
      XmStringGetLtoR( cbs->value, XmFONTLIST_DEFAULT_TAG, &filename );
      XmStringFree(options.image);
      options.image = (XmString)filename; /* kludge! */
      /* options.border = XmStringCreateLtoR ( filename, XmFONTLIST_DEFAULT_TAG ); *//* <-- didn't work ! */
   }
   
     
   
   initBorders( options.border, options.image, options.blend );
   return;
} 


static void ToggleStateCallback( Widget w, XtPointer clientData, XtPointer callData)
{
   /* callback for pop-up menu toggle buttons */
   
   icon_t    *id = target_icon;
   
   if ( DEBUG ) printf( "ToggleStateCallback: %s\n", id->iconName );
   
   if ( w == magB ) {
      options.magnify = XmToggleButtonGetState( w );
   }
   else if ( w == hideB ) {
      options.hide = XmToggleButtonGetState( w );
   }
   else if ( w == raiseB ) {
      options.raise = XmToggleButtonGetState( w );
   }
   else if ( w == toolchestB ) {
      options.toolchestManage = XmToggleButtonGetState( w );
   }
   else if ( w == blendB ) {
      
      
      options.blend = XmToggleButtonGetState( w );
      
      if ( options.blend ) {
         XmString xstr, xstr2;
         Widget dialog;
         Dimension   scr_ht, ht;
        
         xstr = XmStringCreateLtoR ( "*mask.rgb", XmFONTLIST_DEFAULT_TAG );
         xstr2 = XmStringCreateLtoR ( "Select RGB file for border (mask)", XmFONTLIST_DEFAULT_TAG );
         dialog = XmCreateFileSelectionDialog( w, "openImageFile", NULL, 0 );
         XtVaSetValues( dialog, 
               SgNviewerFilter, xstr,
               XmNpattern, xstr,
               SgNbrowserFileMask, SgFILE_NOT_HIDDEN,
               SgNviewerMode, SgVIEWER_AUTOMATIC,
               XmNfileListLabelString, xstr2,
               XmNy, 200,
               NULL );
         XmStringFree(xstr);
         XmStringFree(xstr2);
         XtAddCallback(dialog, XmNokCallback, FileOKCallback, (XtPointer)options.border );
         XtAddCallback(dialog, XmNcancelCallback, FileCancelCallback, NULL);
         XtManageChild(dialog);
         scr_ht = (Dimension)DisplayHeight( dpy, scrnNum );
         XtVaGetValues( dialog, XmNheight, &ht, NULL );
         XtVaSetValues( dialog, XmNy, (scr_ht - getWindowHeight() - ht - 10), NULL );
         QUIT_DIALOG_VISIBLE = TRUE;
      }
      
      initBorders( options.border, options.image, options.blend );

   }
   else if ( w == leftB ) {
      int cc;

      for (cc=0;cc<Nicons;cc++) {
         demagnify( popped_icon, DEMAG_ALL );
         if ( &IconData[cc] == id ) SwapIcons( &IconData[cc], &IconData[cc-1] ); 
      }
   }
   else if ( w == rightB ) {
      int cc;
      
      for (cc=0;cc<Nicons;cc++) {
         demagnify( popped_icon, DEMAG_ALL );
         if ( &IconData[cc] == id ) SwapIcons( &IconData[cc], &IconData[cc+1] ); 
      }
   }
   
   return;
}

static void RemoteStateCallback( Widget w, XtPointer clientData, XtPointer callData)
{
   icon_t *id;
   
   id = target_icon;
   if ( id ) {
      id->remote = XmToggleButtonGetState( w );
      XFreePixmap( dpy, id->pixmap );
      XFreePixmap( dpy, id->pixmap_sm );
      geticon( id );
      XtVaSetValues( id->w_pix, XmNbackgroundPixmap, id->pixmap_sm, NULL );
      XtVaSetValues( id->w_popup_pix, XmNbackgroundPixmap, id->pixmap, NULL );
   }
   else {
      printf("%s : couldn't find widget on list\n", __func__);
   }
   return;
}

static void popDownCallback( Widget w, XtPointer clientData, XtPointer callData)
{
   menu_popped_up = FALSE;
   return;
} 


static void CreateMenu( Widget parent )
{   /* create a pop-up menu */
   XmString    xstr;
   Widget      raiseAppB, lowerB, minB, restoreB, minallB;
   
   menu = XmCreatePopupMenu( parent, "menu", NULL, 0 );
   
   XtAddCallback( XtParent(menu), XmNpopdownCallback, popDownCallback, (XtPointer)NULL );
   
   XtVaCreateManagedWidget( "logo", xmLabelWidgetClass, menu,
         XmNlabelString,  XmStringCreateLtoR ( "I C O N B A R" , XmFONTLIST_DEFAULT_TAG ),
         NULL);
   XtVaCreateManagedWidget( "separator", xmSeparatorWidgetClass, menu,
         XmNseparatorType, XmDOUBLE_LINE,
         NULL);
   
   xstr = XmStringCreateLtoR ( "Exit iconbar", XmFONTLIST_DEFAULT_TAG );
   exitB = XtVaCreateManagedWidget( "exitB", xmPushButtonWidgetClass, menu, 
            XmNlabelString, xstr,
            NULL);
   XmStringFree(xstr);
   XtAddCallback( exitB, XmNactivateCallback, QuitDialogCallback, NULL );
   
   XtCreateManagedWidget( "separator", xmSeparatorWidgetClass, menu, NULL, 0);
   
   magB = XtVaCreateManagedWidget( "Magnify Icon", xmToggleButtonWidgetClass, menu,
            XmNset, options.magnify,
            NULL);
   XtAddCallback( magB, XmNvalueChangedCallback, ToggleStateCallback, NULL );
   
   hideB = XtVaCreateManagedWidget( "Hide Bar", xmToggleButtonWidgetClass, menu,
         XmNset, options.hide,
         NULL);
   XtAddCallback( hideB, XmNvalueChangedCallback, ToggleStateCallback, NULL );
   
   raiseB = XtVaCreateManagedWidget( "Raise Bar", xmToggleButtonWidgetClass, menu,
         XmNset, options.raise,
         NULL);
   XtAddCallback( raiseB, XmNvalueChangedCallback, ToggleStateCallback, NULL );
   
   toolchestB = XtVaCreateManagedWidget( "Manage Toolchest", xmToggleButtonWidgetClass, menu,
         XmNset, options.toolchestManage,
         NULL);
   XtAddCallback( toolchestB, XmNvalueChangedCallback, ToggleStateCallback, NULL );
   
   blendB = XtVaCreateManagedWidget( "Blend Border", xmToggleButtonWidgetClass, menu,
         XmNset, options.blend,
         NULL);
   XtAddCallback( blendB, XmNvalueChangedCallback, ToggleStateCallback, NULL );
   
   XtCreateManagedWidget( "separator", xmSeparatorWidgetClass, menu, NULL, 0);      
   
   leftB = XtVaCreateManagedWidget( "Shuffle Icon Left", xmPushButtonWidgetClass, menu,
         NULL);
   XtAddCallback( leftB, XmNactivateCallback, ToggleStateCallback, NULL );
   
   
   rightB = XtVaCreateManagedWidget( "Shuffle Icon Right", xmPushButtonWidgetClass, menu,
         NULL);
   XtAddCallback( rightB, XmNactivateCallback, ToggleStateCallback, NULL );
   
   remoteB = XtVaCreateManagedWidget( "Remote Host Tint", xmToggleButtonWidgetClass, menu,
         XmNset, 0,
         NULL);
   XtAddCallback( remoteB, XmNvalueChangedCallback, RemoteStateCallback, NULL );
   
   XtCreateManagedWidget( "separator", xmSeparatorWidgetClass, menu, NULL, 0);
   
   minallB = XtCreateManagedWidget( "Minimize All", xmPushButtonWidgetClass, menu, NULL, 0);
   XtAddCallback( minallB, XmNactivateCallback, MinimizeAllCallback, NULL );
   
   XtCreateManagedWidget( "separator", xmSeparatorWidgetClass, menu, NULL, 0);
   
   minB = XtCreateManagedWidget( "Minimize", xmPushButtonWidgetClass, menu, NULL, 0);
   XtAddCallback( minB, XmNactivateCallback, MinimizeAppCallback, NULL );
   
   restoreB = XtCreateManagedWidget( "Restore", xmPushButtonWidgetClass, menu, NULL, 0);
   XtAddCallback( restoreB, XmNactivateCallback, RestoreAppCallback, NULL );
   
   raiseAppB = XtCreateManagedWidget( "Raise", xmPushButtonWidgetClass, menu, NULL, 0);
   XtAddCallback( raiseAppB, XmNactivateCallback, RaiseAppCallback, NULL );
   
   lowerB = XtCreateManagedWidget( "Lower", xmPushButtonWidgetClass, menu, NULL, 0);     
   XtAddCallback( lowerB, XmNactivateCallback, LowerAppCallback, NULL );
      
   XtCreateManagedWidget( "separator", xmSeparatorWidgetClass, menu, NULL, 0);
    
   closeB = XtCreateManagedWidget( "closeB", xmPushButtonWidgetClass, menu, NULL, 0);
   XtAddCallback( closeB, XmNactivateCallback, CloseAppCallback, NULL );
   
   return;
}

static void CloseAppCallback( Widget w, XtPointer clientData, XtPointer callData)
{
   Atom                          WM_DELETE_WINDOW, WM_PROTOCOLS;   
   static XClientMessageEvent    DeleteMsgEvent;
   
   WM_DELETE_WINDOW = XInternAtom( XtDisplay(shell), "WM_DELETE_WINDOW", True );
   WM_PROTOCOLS = XInternAtom( XtDisplay(shell), "WM_PROTOCOLS", True );
   
   if ( WM_DELETE_WINDOW == None || WM_PROTOCOLS == None ) {
      printf("CloseAppCallback: no delete atom protocol\n");
      return;
   }
   
   DeleteMsgEvent.type = ClientMessage;
   DeleteMsgEvent.serial = (unsigned long)0;
   DeleteMsgEvent.send_event = True;
   DeleteMsgEvent.display = dpy;
   DeleteMsgEvent.window = target_icon->top;
   DeleteMsgEvent.message_type = WM_PROTOCOLS;
   DeleteMsgEvent.format = 32;
   DeleteMsgEvent.data.l[0] = WM_DELETE_WINDOW;
   XSendEvent( XtDisplay(shell), target_icon->top, False, 0L, (XEvent*)&DeleteMsgEvent ); 

}

static void RaiseAppCallback( Widget w, XtPointer clientData, XtPointer callData)
{
   XRaiseWindow( dpy, target_icon->top );
   return;
}

static void LowerAppCallback( Widget w, XtPointer clientData, XtPointer callData)
{
   XLowerWindow( dpy, target_icon->top );
   return;
}

static void RestoreAppCallback( Widget w, XtPointer clientData, XtPointer callData)
{
   XMapWindow( dpy, target_icon->top );
   XRaiseWindow( dpy, target_icon->top );
   return;
}

static void MinimizeAppCallback( Widget w, XtPointer clientData, XtPointer callData)
{
   XIconifyWindow( dpy, target_icon->top, scrnNum );
   XSync(dpy, FALSE);
   XUnmapWindow( dpy, target_icon->icon );
   return;
}

static void MinimizeAllCallback( Widget w, XtPointer clientData, XtPointer callData)
{
   int cc;
   
   for ( cc=0;cc<Nicons;cc++) {
      XIconifyWindow( dpy, IconData[cc].top, scrnNum );
      XSync(dpy, FALSE);
      XSync(dpy, FALSE);
      XSync(dpy, FALSE);
      XSync(dpy, FALSE);
      XUnmapWindow( dpy, IconData[cc].icon );
      XSync(dpy, FALSE);
   }
   return;
}
