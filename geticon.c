
/********************************************************************************
** Iconbar - a desktop icon management utility compatible with the IRIX(TM) 4dwm
** Copyright 2003 Steven Queen and Bryan Sawler
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
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/extensions/shape.h>

#include <gl/image.h>
/* function protottypes absent from gl/image.h */
extern int iclose(IMAGE *image);
extern int getrow(IMAGE *image, unsigned short *buffer, unsigned int y, unsigned int z);


#ifndef MAXFILENAME
#define MAXFILENAME  80
#endif

extern Visual *visual;
extern int vdepth;

static GC                  gc;   
static Pixel               icon_background_pixel;
static unsigned int        depth;
static unsigned int        icon_w, icon_h;
static XWindowAttributes   attributes;

static int tryWMHint( XImage **ximg_ptr, icon_t *id );
static int tryRGBFile( XImage **ximg_ptr, icon_t *id, int use_default );
static int useScreenCopy( XImage **ximg_ptr, icon_t *id );


void geticon( icon_t *id )
{
   int                  success = 0;
   Pixmap               icon_pixmap, icon_pixmap_sm;
   unsigned int         rbg, gbg, bbg;
   XGCValues            gcvalues;           
   XImage               *ximg = NULL, *ximg_sm, *ximg_lg;
   char                 *text, *token;
   /* long                 remote_pixel_tint = 0x003b00; */
   
   XtVaGetValues( id->w, XmNdepth, &depth, NULL );
   icon_pixmap = XCreatePixmap( dpy, rootWin, ICON_WD, ICON_HT, depth ); 
   icon_pixmap_sm = XCreatePixmap( dpy, rootWin,
                        (unsigned int)(ICON_WD*options.scale),
                        (unsigned int)(ICON_HT*options.scale),
                        depth );
   gc = XtGetGC( id->w, GCForeground | GCBackground, &gcvalues );
   XtVaGetValues( id->w_pix, XmNbackground, &icon_background_pixel, NULL );
   rbg = (unsigned int)(icon_background_pixel & 0x0000ff);
   gbg = (unsigned int)(icon_background_pixel & 0x00ff00)>>8;
   bbg = (unsigned int)(icon_background_pixel & 0xff0000)>>16;
   
   XGetWindowAttributes( dpy, id->icon, &attributes );
   
   
   text = (char *) calloc(  strlen( (char *)options.path )+2, sizeof( char ) );
   snprintf( text, strlen( (char *)options.path )+1, "%s", (char *)options.path );   
   
   token = strtok( text, ":" );
   while ( token && !success ) {
      if ( strcmp( token, "WMHINTS" ) == 0 ) {
         /* try method 1 - window manager hints */
         success = tryWMHint( &ximg, id );
      }
      else if ( strcmp( token, "FILES" ) == 0 ) {
         /* try method 2a - instance / class icon file */
         success = tryRGBFile( &ximg, id, FALSE );
      }
      else if ( strcmp( token, "DEFAULT" ) == 0 ) {
         /* try method 2b - default icon file */
         success = tryRGBFile( &ximg, id, TRUE );
      }
      else if ( strcmp( token, "SCREEN" ) == 0 ) {
         /* use method 3 - copy from screen - always works (badly) */
         success = useScreenCopy( &ximg, id );
      }
      token = strtok( NULL, ":");
   } 
   
   if ( !success ) {
      static int once = FALSE;
      
      if ( !once ) {
         fprintf(stderr, "ICONBAR: geticon: most likely a bad path resource...using DEFAULT:SCREEN\n");
         once = TRUE;
      }
      success = tryRGBFile( &ximg, id, TRUE );
      if ( !success ) success = useScreenCopy( &ximg, id );
   }
      
   if ( ximg != (XImage *) NULL ) {
      int      x_off = 0, y_off = 0, x_off_sm = 0, y_off_sm = 0,
               offset, bitmap_pad, bytes_per_line, 
               zbufsize = ICON_BUF_SIZE;
      int      x, y, z;
      XImage   *bg_ximg, *bg_ximg_sm;
      char     *zbuf;
      double   scale_sm=0.0, scale_lg=0.0;
      
      /* if ( !id->remote ) remote_pixel_tint = 0x0; */
      
      /* ++++++++++++++ shrink image ++++++++++++++ */
      if ( icon_w <= (int)(ICON_WD*options.scale)
            && icon_h <=  (int)(ICON_HT*options.scale) )
      {/* if image is smaller than small icon -- use it directly on the small icon */
         ximg_sm = ximg;
         ximg_lg = reXimage( dpy, shellWin, ximg, 1.0/options.scale, 1.0/options.scale );
      }
      else { /* put the image on the full-sized (large) icon */         
         
         scale_sm = (double)min( (ICON_WD*options.scale)/icon_w, (ICON_HT*options.scale)/icon_h );         
         ximg_sm = reXimage( dpy, shellWin, ximg, scale_sm, scale_sm );
         
         if ( icon_w > ICON_WD || icon_h > ICON_HT ) {
            scale_lg = (double)min( (double)ICON_WD/(double)icon_w, (double)ICON_HT/(double)icon_h );
            ximg_lg = reXimage( dpy, shellWin, ximg, scale_lg, scale_lg );
         }
         else { 
            ximg_lg = ximg;
         }

      }
      icon_w =  ximg_lg->width;
      icon_h =  ximg_lg->height;
      /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
      
      /* center undersized images */
      if ( icon_w < ICON_WD ) {
         x_off = (ICON_WD - icon_w)/2;
         x_off = max(0, x_off);
         if ( ximg_sm->width >=  (int)(ICON_WD*options.scale) ) {
            x_off_sm = (unsigned int)0;
         }
         else {
            x_off_sm = (unsigned int)( (int)(ICON_WD*options.scale) - ximg_sm->width )/2;
         }
      }
      if ( icon_h < ICON_HT ) {
         y_off = (int)ceil( (ICON_HT - icon_h)/2.0 );
         y_off = max(0, y_off);
         if ( ximg_sm->height >=  (int)(ICON_HT*options.scale) ) {
            y_off_sm = (unsigned int)0;
         }
         else {
            y_off_sm = (unsigned int)( (int)(ICON_HT*options.scale) - ximg_sm->height )/2;
         }
      }
      
      icon_w = min( ICON_WD, icon_w);
      icon_h = min( ICON_HT, icon_h);
      
      /* full size (large) pixmap creation */
      /* --------------------------------- */
      if ( x_off || y_off || icon_h < ICON_HT ) {
                  
         Pixel          pixel;
         
         /* install the background color */         
         offset = 0;
         bitmap_pad = 32;
         bytes_per_line = 0;
         zbuf = (char *)malloc(zbufsize*sizeof(short));
         for (z=0;z<zbufsize;z++) {
               zbuf[z++] = rbg;
               zbuf[z++] = gbg;
               zbuf[z++] = bbg;
               zbuf[z] = 255;
         }
                   
         /* install the image over background color */                
         bg_ximg = XCreateImage( dpy, visual, vdepth, ZPixmap,
                     offset, zbuf, ICON_WD, ICON_HT, bitmap_pad, bytes_per_line );
         bg_ximg->byte_order = LSBFirst;
         
         for (y=0;y<ximg_lg->height;y++) { 
            for (x=0;x<ximg_lg->width;x++) {
               pixel = XGetPixel( ximg_lg, x, y );
               XPutPixel( bg_ximg, x+x_off, y+y_off, pixel );
            }
         }
         
         
         /* install new image into widget's pixmap */
         XPutImage( dpy, icon_pixmap, gc, bg_ximg, 0, 0, 0, 0, ICON_WD, ICON_HT );
         
         XDestroyImage( bg_ximg );
      }
      else {     
         /* install original image into widget's pixmap */
         XPutImage( dpy, icon_pixmap, gc, ximg_lg, 0, 0, 
            (unsigned int)x_off, (unsigned int)y_off, ICON_WD, ICON_HT ); 
      }
      
      /* demagnified (small) pixmap creation */
      /* --------------------------------- */
      if ( (x_off_sm || y_off_sm) ) {
         
         Pixel pixel;

         /* install the background color */
         offset = 0;
         bitmap_pad = 32;
         bytes_per_line = 0;
         zbuf = (char *)malloc(zbufsize*sizeof(short));
         for (z=0;z<zbufsize;z++) {
               zbuf[z++] = rbg;
               zbuf[z++] = gbg;
               zbuf[z++] = bbg;
               zbuf[z] = 255;
         }
                         
         bg_ximg_sm = XCreateImage( dpy, visual, vdepth, ZPixmap,
                     offset, zbuf, (int)(ICON_WD*options.scale), 
                     (int)(ICON_HT*options.scale),
                     bitmap_pad, bytes_per_line );
         bg_ximg_sm->byte_order = LSBFirst;
         
         /* install the small image over the background */
         for (y=0;y<(int)ximg_sm->height;y++) {
            for (x=0;x<(int)ximg_sm->width;x++) { 
               pixel = XGetPixel( ximg_sm, (unsigned int)x, (unsigned int)y ); 
               XPutPixel( bg_ximg_sm, (unsigned int)(x+x_off_sm), (unsigned int)(y+y_off_sm), pixel);
            }
         }
         
         
         XPutImage( dpy, icon_pixmap_sm, gc, bg_ximg_sm, 0, 0, 0, 0, 
            (unsigned int)(bg_ximg_sm->width),
            (unsigned int)(bg_ximg_sm->height) );

         XDestroyImage( bg_ximg_sm );
      } 
      else {
   
         /* install demagnified image into widget's pixmap */
         XPutImage( dpy, icon_pixmap_sm, gc, ximg_sm, 0, 0, 
            (unsigned int)x_off_sm, (unsigned int)y_off_sm,  
            (unsigned int)(ximg_sm->width),
            (unsigned int)(ximg_sm->height) );
      }

      
             
      /* --- tint if a remote client --- */
      if (id->remote) {
         XImage   *ximg_tmp;
         Pixel    pixel;
         int      sm_wd, sm_ht;
         float    a;
         Pixel    r, g, b;
         
         sm_wd = (int)(ICON_WD*options.scale);
         sm_ht = (int)(ICON_HT*options.scale);
         /* colorize the small icon */
         ximg_tmp = XGetImage( dpy, icon_pixmap_sm, 0, 0, sm_wd, sm_ht, AllPlanes, ZPixmap );
         
         for (y=0;y<sm_ht;y++) { 
            for (x=0;x<sm_wd;x++) { 
               pixel =  XGetPixel( ximg_tmp, x, y );
               
               /* darken */
              /*  if ( pixel==icon_background_pixel ) pixel = 0x888888; 
               else pixel &= 0xdddddd; */
               
               a = 0.5f;
               r = (Pixel)(a*(pixel & 0xff0000))&0xff0000;
               g = (Pixel)(a*(pixel & 0x00ff00))&0x00ff00 + (Pixel)((1.0f-a)*0x00ff00);
               g &= 0x00ff00;
               b = (Pixel)(a*(pixel & 0x0000ff))&0x0000ff;
               pixel = r | g | b;
               
               XPutPixel( ximg_tmp, x, y, pixel );
            }
         }
         XPutImage( dpy, icon_pixmap_sm, gc, ximg_tmp, 0, 0, 0, 0, sm_wd, sm_ht );
         XDestroyImage( ximg_tmp );
         
      } /* --- end remote tint --- */

            
      /*********************************************
      ** use the background color as a SHAPE mask **
      ** for the top of the large icon window     **
      *********************************************/     
      if (options.shape) {
         int            x, y;
         Pixel          pixel;
         XImage         *ximg_mask;
	      Pixmap	    mask;
         GC             mask_gc;


         ximg_mask = XGetImage( dpy, icon_pixmap, 0, 0, ICON_WD, ICON_HT, 
                                   AllPlanes, ZPixmap );
         mask = XCreatePixmap( dpy, rootWin, ICON_WD, ICON_HT, 1 );
         mask_gc = XCreateGC( dpy, mask, 0, 0);

/* 		XSetForeground( dpy, mask_gc, WhitePixel(dpy, scrnNum) );
		XFillRectangle( dpy, mask, mask_gc, x, y, ICON_WD, ICON_HT); */
         
         for (y=0;y<ximg_mask->height;y++) { 
            for (x=0;x<ximg_mask->width;x++) {
               pixel = XGetPixel(ximg_mask, x, y);
		         if( (pixel==icon_background_pixel) && ( y<(ICON_HT-(int)(ICON_HT*options.scale)) ) ){
                  XSetForeground( dpy, mask_gc, BlackPixel(dpy, scrnNum) );
                  XFillRectangle( dpy, mask, mask_gc, x, y, 1, 1);                  
		         }
               else {
                  XSetForeground( dpy, mask_gc, WhitePixel(dpy, scrnNum) );
                  XFillRectangle( dpy, mask, mask_gc, x, y, 1, 1);
               }
            }
         }

         XShapeCombineMask( XtDisplay(id->w_popup), XtWindow(id->w_popup),
                            ShapeBounding, 0, 0, mask, ShapeSet );

         XDestroyImage( ximg_mask );
         XFreePixmap( dpy, mask );
         XFreeGC( dpy, mask_gc );

      }
      /***********************************************************************/

      XDestroyImage( ximg );
      XDestroyImage( ximg_sm );
      XDestroyImage( ximg_lg );
   }
   

   /* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
   
   
   
   id->pixmap = icon_pixmap;
   id->pixmap_sm = icon_pixmap_sm;
   XtReleaseGC( id->w, gc );
   return;
}


   /* ------------------- METHOD 1 - window manger hints -------------------- */

static int tryWMHint( XImage **ximg_ptr, icon_t *id )
{
   int            icon_x, icon_y;
   unsigned int   icon_bdr, icon_depth;
   XWMHints       *wmhints;
   Window         root;
   Pixmap         tmp_pixmap;

   XImage	      *icon_mask;
   Pixel          pixel = (Pixel)0;
   int            xloop, yloop;

   if ( ( wmhints = XGetWMHints(dpy, id->top) ) ) {
      if ( (wmhints->flags & IconWindowHint) && DEBUG4 ) {
         printf("geticon: there's an icon window hint for %s \n", id->iconName);
         printf("geticon: can't do anything with it for now...\n");
      }
      if ( wmhints->flags&IconPixmapHint ) {
         if ( ! XGetGeometry( dpy, wmhints->icon_pixmap, &root, &icon_x, &icon_y,
                             &icon_w, &icon_h, &icon_bdr, &icon_depth) ) {
            fprintf( stderr, "geticon: client passed invalid icon pixmap\n");               
         }
         else {
            if ( icon_depth == depth ) {
               /* TrueColor pixmap, depth 24 */
               *ximg_ptr = XGetImage( dpy, wmhints->icon_pixmap, 0, 0, 
                        icon_w, icon_h, AllPlanes, ZPixmap );
               

	            if(wmhints->flags&IconMaskHint){
                  icon_mask = XGetImage(dpy, wmhints->icon_mask, 0, 0, icon_w, icon_h, AllPlanes, XYPixmap);
		            if(icon_mask){
                     
		               for(yloop = 0; yloop < icon_h; yloop++){
		                  for(xloop = 0; xloop < icon_w; xloop++){
			                  pixel = XGetPixel(icon_mask, xloop, yloop);
		   	               if(!pixel){
			                     pixel = icon_background_pixel;
			                     XPutPixel(*ximg_ptr, xloop, yloop, pixel);
			                  }
                        }
		               }

		               XDestroyImage(icon_mask);
		            } else if(DEBUG4){
	                   printf("geticon: unable to get icon mask\n");
	               }
               }
               
               return(1);
               
            } /*end if depth = 24 */
            else if ( icon_depth == 1 ) {
               /* Bitmap (black & white) */
               tmp_pixmap = XCreatePixmap( dpy, rootWin, ICON_WD, ICON_HT, depth );
               XSetForeground( dpy, gc, BlackPixel( dpy, 0 ) );                  
               XSetBackground( dpy, gc, icon_background_pixel );
               XCopyPlane( dpy, wmhints->icon_pixmap, tmp_pixmap, gc, 
                     0, 0, icon_w, icon_h, 0, 0, 1);

               *ximg_ptr = XGetImage( dpy, tmp_pixmap, 0, 0, 
                        icon_w, icon_h, AllPlanes, ZPixmap );
               
	            XFreePixmap( dpy, tmp_pixmap );
               return(1);
               
            } /*end if depth = 1 */
            else if ( icon_depth == 30 ) {
               /******************************************************
               ** This is the VPro/gtk+ blank/garbled icon error... **
               ** it selects the best visual depth assuming 24      **
               ** but get something greater instead                 **
               ******************************************************/               
               /* red_mask =  0x3ff;
               green_mask =  0xffc00;
               blue_mask =  0x3ff00000; */
               *ximg_ptr = XGetImage( dpy, wmhints->icon_pixmap, 0, 0, 
                        icon_w, icon_h, AllPlanes, ZPixmap );
               (*ximg_ptr)->depth = 24;
               /* for (y=0;y<tmp_ximg->height;y++) {
                  for (x=0;x<tmp_ximg->width;x++) {

                  }
               } */
               return(1); /* <-- doesn't work yet so might want to return 0 */
               
            } /* endif depth = 30 */
            else {
               printf("geticon:tryWMHints: unexpected icon depth=%u (screen=%u)\n", depth, icon_depth);
            }
         } /* endif got geometry */
      } /* endif got window manager pixmap hint */      
   } /* endif got window manager hints */

   
   return(0);
}
   /* ----------------------------------------------------------------------- */





   /* --------------------- METHOD 2 - image from file ---------------------- */
   /* ----------------------------------------------------------------------- */
static int tryRGBFile( XImage **ximg_ptr, icon_t *id, int use_default )
{
   int            cc, x,y,z, offset, bitmap_pad, bytes_per_line,
                  found_file = FALSE, zbufsize = ICON_BUF_SIZE;
   XClassHint     class_hints;
   Status         status;
   struct stat    buf;
   char           fname[MAXFILENAME+1];
   IMAGE          *sgi_image = (IMAGE *)NULL;
   unsigned short *rbuf, *gbuf, *bbuf, *abuf;
   char           *zbuf;
   static char    *home_dir = NULL;

   if ( home_dir == NULL ){
      home_dir = getenv("HOME");
   }
   
   status = XGetClassHint( dpy, id->top, &class_hints );
   if (!status) 
      use_default = TRUE;   

   if ( use_default ) {
      
      /* search for deafult.icon */
      /* ----------------------- */
      if ( !found_file ) {
         snprintf( fname, MAXFILENAME, "%s/.icons/default.icon", home_dir );
         if ( stat( fname, &buf ) == 0 ) {
            if ( DEBUG4 ) printf("no icon file - trying $HOME/default.icon\n");
            if ( ( sgi_image = iopen(fname,"r") ) ) found_file = TRUE;
         }
      }
      if ( !found_file ) {
         if ( stat( "/usr/lib/images/default.icon", &buf ) == 0 ) {
            if ( ( sgi_image = iopen("/usr/lib/images/default.icon","r") ) ) found_file = TRUE;
         }
      }
      
   } /* end if use_default icon file */
   else {

      /* search by instance name (res_name)... */
      /* ------------------------------------- */
      if ( !found_file ) {
         snprintf( fname, MAXFILENAME, "%s/.icons/%s.icon", home_dir, class_hints.res_name );
         if ( stat( fname, &buf ) == 0 ) {
            if ( ( sgi_image = iopen(fname,"r") ) ) found_file = TRUE;           
         }
      }
      if ( !found_file ) {
         snprintf( fname, MAXFILENAME, "/usr/lib/images/%s.icon", class_hints.res_name );
         if ( stat( fname, &buf ) == 0 ) {
            if ( ( sgi_image = iopen(fname,"r") ) ) found_file = TRUE;          
         }
      }


      /* search by class name (res_name)... */
      /* ---------------------------------- */
      if ( !found_file ) {
         snprintf( fname, MAXFILENAME, "%s/.icons/%s.icon", home_dir, class_hints.res_class ); /*<--SIGSEGV here in snprintf*/
         if ( stat( fname, &buf ) == 0 ) {
            if ( ( sgi_image = iopen(fname,"r") ) ) found_file = TRUE;           
         }
      }
      if ( !found_file ) {
         snprintf( fname, MAXFILENAME, "/usr/lib/images/%s.icon", class_hints.res_class );
         if ( stat( fname, &buf ) == 0 ) {
            if ( ( sgi_image = iopen(fname,"r") ) ) found_file = TRUE; 
         }
      }
   
   } /* end if-else don't use_default icon file */

   if (status) {
      XFree( class_hints.res_name );
      XFree( class_hints.res_class );
   }

   if ( found_file ) {

      /* NOTE: this assumes zsize is 3 (RGB) but it could be 1 (B&W) or 4 (RGBA) */
      zbuf = (char *)malloc(zbufsize*sizeof(short));                 /* <---SIGSEGV here when running a remote X win !!! */
      if ( zbuf == NULL ) fprintf( stderr, "geticon: zbuf alloc error, zbufsize = %d\n",zbufsize);
      for (cc=0;cc<zbufsize;cc++) {
         zbuf[cc] = 198;
      }
      rbuf = (unsigned short *)malloc(sgi_image->xsize*sizeof(short));
      if ( rbuf == NULL ) fprintf( stderr, "geticon: rbuf alloc error, xsize = %d\n",sgi_image->xsize);
      gbuf = (unsigned short *)malloc(sgi_image->xsize*sizeof(short));
      if ( gbuf == NULL ) fprintf( stderr, "geticon: gbuf alloc error, xsize = %d\n",sgi_image->xsize);
      bbuf = (unsigned short *)malloc(sgi_image->xsize*sizeof(short));
      if ( bbuf == NULL ) fprintf( stderr, "geticon: bbuf alloc error, xsize = %d\n",sgi_image->xsize);
      abuf = (unsigned short *)malloc(sgi_image->xsize*sizeof(short));
      if ( abuf == NULL ) fprintf( stderr, "geticon: abuf alloc error, xsize = %d\n",sgi_image->xsize);
      z = 0;
      /* if (sgi_image->zsize == 4) printf("%s got alpha!\n",fname); */
      for (y=sgi_image->ysize-1;y>=0;y--) {         
         getrow(sgi_image,rbuf,y,0);
         getrow(sgi_image,gbuf,y,1);
         getrow(sgi_image,bbuf,y,2);
         if (sgi_image->zsize == 4) {            
            getrow(sgi_image,abuf,y,2);
         }
         for (x=0;x<sgi_image->xsize;x++) {
            if ( isRGBvisual ) {
               zbuf[z++] = (char)bbuf[x];       /* have to swap these for the Onyx4 (XFree86) and IR3 */
               zbuf[z++] = (char)gbuf[x];
               zbuf[z++] = (char)rbuf[x];       /* have to swap these for the Onyx4 (XFree86) and IR3 */
            }
            else {               
               zbuf[z++] = (char)rbuf[x];       /* this for O2 and Octane */
               zbuf[z++] = (char)gbuf[x];
               zbuf[z++] = (char)bbuf[x];       /* this for O2 and Octane */
            }
            /* alpha */
            if (sgi_image->zsize == 4)
               zbuf[z++] = (char)abuf[x]; 
            else
               zbuf[z++] = 255;
         }
      }
      offset = 0;
      bitmap_pad = 32;
      bytes_per_line = 0;         
      /* NOTE: this will not work if the default visual is not 24bit !! */
      icon_w = sgi_image->xsize; /* max(ICON_WD,sgi_image->xsize); */
      icon_h = sgi_image->ysize; /* max(ICON_HT,sgi_image->ysize); */                 
      *ximg_ptr = XCreateImage( dpy, visual, vdepth, ZPixmap,
                  offset, zbuf, icon_w, icon_h, bitmap_pad, bytes_per_line );

      (*ximg_ptr)->byte_order = LSBFirst;          
           
      iclose(sgi_image);                            
      return(1);
   }
   else
      return(0); /* search for icon file failed */
}
   /* ----------------------------------------------------------------------- */
   /* ----------------------------------------------------------------------- */



   /* ----------------------------------------------------------------------- */
   /* ----------------- METHOD 3 - copy pixmap from screen ------------------ */
   /* ----------------------------------------------------------------------- */
static int useScreenCopy( XImage **ximg_ptr, icon_t *id )
{
   int               sl;
      
   XGetWindowAttributes( dpy, id->icon, &attributes );

   /* iconify window */
   if ( id->state != IconicState ) {
      XMapWindow( dpy, id->top );
      XSync( dpy, FALSE );
      XIconifyWindow( dpy, id->top, (int)NULL );  /* <-- NULL = Screen number? */
      XSync( dpy, FALSE );                        
   }
   XRaiseWindow( dpy, id->icon );
   for (sl=0;sl<SYNC_LOOPS;sl++) {
      XSync( dpy, FALSE );
   }
   XSync( dpy, FALSE );

   XMapWindow( dpy, id->icon );      

   icon_w = ICON_WD;
   icon_h = ICON_HT;
   *ximg_ptr = XGetImage( dpy, id->icon, 6, 6, icon_w, icon_h, AllPlanes, ZPixmap );   

   /* restore window to previous state */
   if ( id->state != IconicState ) {
      XMapWindow( dpy, id->top );
   }
   else {
      XUnmapWindow( dpy, id->icon );
   }
   return(1);
}         

   /* ----------------------------------------------------------------------- */
   /* ----------------------------------------------------------------------- */
   /* ----------------------------------------------------------------------- */   




