
/********************************************************************************
** Iconbar - a desktop icon management utility compatible with IRIX(TM) 4dwm
** Copyright 2003 Bryan Sawler
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
#include "gl/image.h"
#include "iconbar_mask.xbm"
#include <X11/Xatom.h>
/* #include <X11/SGIFastAtom.h> */
#define SGI_XA_PIXEL ((Atom)83)

int		last_width = 0;
int		no_border = 0;
int		use_blend = 0;

enum {
	MODE_REPEAT,
	MODE_TILE,
};

Pixmap borderImage;
Pixmap borderMask;

typedef struct {
	int tl_X, tl_Y, tl_W, tl_H;
	int t_X, t_Y, t_W, t_H, t_WT, t_WL, t_WB;
	int tr_X, tr_Y, tr_W, tr_H;

	int l_X, l_Y, l_W, l_H, l_HT, l_HL, l_HB;
	int c_X, c_Y, c_W, c_H;
	int r_X, r_Y, r_W, r_H, r_HT, r_HL, r_HB;

	int bl_X, bl_Y, bl_W, bl_H;
	int b_X, b_Y, b_W, b_H, b_WT, b_WL, b_WB;
	int br_X, br_Y, br_W, br_H;
} BORDER_SPEC;

BORDER_SPEC borderSpec;

static BORDER_SPEC defSpec = {
	0, 0, 8, 8,
	9, 0, 8, 8, 0, 8, 0,
	18, 0, 8, 8,

	0, 9, 8, 8, 0, 8, 0,
	9, 9, 8, 8,
	18, 9, 8, 8, 0, 8, 0,

	0, 18, 8, 8,
	9, 18, 8, 8, 0, 8, 0,
	18, 18, 8, 8,
};

extern Dimension scr_wd, scr_ht;

void loadBorderFiles(char *imagefile, char *maskfile, BORDER_SPEC *spec)
{
	IMAGE *image = (IMAGE *)NULL, *mask = (IMAGE *)NULL;
	XImage *mimg;
	XImage *iimg;
	short *mask_r, *mask_g, *mask_b, *mask_data;
	GC gc;
	XGCValues values;

	char *idata;
	char *mdata;
	
	int yloop, xloop;

	int haveimage;

	borderImage = 0;
	borderMask = 0;

	haveimage = 0;
	if(imagefile){
		image = iopen(imagefile, "r");
		if(image) haveimage = 1;
	}
	mask = iopen(maskfile, "r");
	if(!mask){
		if(image) iclose(image);
		printf("Error opening border mask file: %s or %s.\n", imagefile, maskfile);
		return;
	}

	if(!mask->xsize){
		printf("0-width mask?  Ignoring.\n");
		if(image) iclose(image);
		iclose(mask);
		return;
	}

	if(haveimage){
		mask_r = (short *)malloc((mask->xsize+1)*sizeof(short));
		mask_g = (short *)malloc((mask->xsize+1)*sizeof(short));
		mask_b = (short *)malloc((mask->xsize+1)*sizeof(short));
	}
	mask_data = (short *)malloc((mask->xsize+1)*sizeof(short));

	if(haveimage){
		borderImage = XCreatePixmap(dpy, rootWin, mask->xsize, mask->ysize, 24);
		idata = (char *)malloc(mask->xsize*mask->ysize*4);
	}

	borderMask = XCreatePixmap(dpy, rootWin, mask->xsize, mask->ysize, 1);
	if(!borderMask){
		printf("Error creating border-mask pixmap!\n");
		return;
	}

	mdata = (char *)malloc(mask->xsize*mask->ysize);

	if(haveimage){
		iimg = XCreateImage(dpy, DefaultVisual(dpy, scrnNum), 24, ZPixmap, 0, idata, mask->xsize, mask->ysize, 16, 0);
		XInitImage(iimg);
	}
	mimg = XCreateImage(dpy, DefaultVisual(dpy, scrnNum), 1, ZPixmap, 0, mdata, mask->xsize, mask->ysize, 16, 0);
	if(!mimg){
		printf("Error creating temporary border-mask image!\n");
		return;
	}
	XInitImage(mimg);

	for(yloop = 0; yloop < mask->ysize; yloop++){
		int y = ((mask->ysize-1)-yloop);
		if(haveimage){
			getrow(image, mask_r, y, 0);
			getrow(image, mask_g, y, 1);
			getrow(image, mask_b, y, 2);
		}
		getrow(mask, mask_data, y, 0);

		for(xloop = 0; xloop < mask->xsize; xloop++){
			if(mask_data[xloop]){
				if(haveimage) XPutPixel(iimg, xloop, yloop, 0);
				XPutPixel(mimg, xloop, yloop, 0); 
			} else {
				if(haveimage) XPutPixel(iimg, xloop, yloop, (mask_r[xloop]<<16)|(mask_g[xloop]<<8)|(mask_b[xloop]));
				XPutPixel(mimg, xloop, yloop, 1); 
			}
		}
	}

	if(haveimage){
		gc = XCreateGC(dpy, borderImage, 0, &values);
		if(!gc){
			printf("Error creating left GC.\n");
			return;
		}
		if(XPutImage(dpy, borderImage, gc, iimg, 0, 0, 0, 0, mask->xsize, mask->ysize))
			printf("Error in XPutImage for borderImage?\n");
		XFreeGC(dpy, gc);
	}

	gc = XCreateGC(dpy, borderMask, 0, &values);
	if(!gc){
		printf("Error creating right GC.\n");
		return;
	}
	if(XPutImage(dpy, borderMask, gc, mimg, 0, 0, 0, 0, mask->xsize, mask->ysize))
		printf("Error in XPutImage for borderMask!\n");
	XFreeGC(dpy, gc);

	iclose(mask);
	free(mask_data);
	if(haveimage){	
		iclose(image);
		free(mask_r);
		free(mask_g);
		free(mask_b);
	}

	if(haveimage){
		XDestroyImage(iimg);
		free(idata);
	}
	XDestroyImage(mimg);
	free(mdata);

	if(spec){
		memcpy(&borderSpec, spec, sizeof(BORDER_SPEC));
	}
}

void initBorders(XmString borderXstr, XmString imageXstr, int blend)
{
   char  *borderfile, *imagefile;
   
	borderImage = borderMask = 0;
	memset(&borderSpec, 0, sizeof(BORDER_SPEC));
   
   /* convert XmStrings to char */
   borderfile = (char *) calloc( strlen( (char*)borderXstr )+2, sizeof(char) );
   snprintf( borderfile, strlen( (char*)borderXstr )+1, "%s", (char *)borderXstr );
   imagefile = (char *) calloc( strlen( (char*)imageXstr )+2, sizeof(char) );
   snprintf( imagefile, strlen( (char*)imageXstr )+1, "%s", (char *)imageXstr );   

	/* Set filename to "none" to disable border shapes */
	if ( !strcasecmp(borderfile, "none") ) {
		no_border = 1;
		return;
	}

#if 1
	if ( !strcasecmp(borderfile, "default") ) {
		/* ----------------------------------------------------------------------- **
		**     use an included bitmap file instead of loading an SGI rgb file      **
		** ----------------------------------------------------------------------- */
		borderMask = XCreateBitmapFromData(dpy, rootWin, (char *)iconbar_mask_bits, iconbar_mask_width, iconbar_mask_height);
		memcpy(&borderSpec, &defSpec, sizeof(BORDER_SPEC));
		return;
	} 
#endif

	use_blend = blend;
	if ( strlen(imagefile) > 1 ) {
		loadBorderFiles(imagefile, borderfile, &defSpec);
	}
	else {
		loadBorderFiles(NULL, borderfile, &defSpec);
	}

   free( borderfile );
   free( imagefile );
   
	return;
}

int getBorderWidth(void){
	if(no_border) return 0;
	return borderSpec.l_W;
}
int getBorderHeight(void){
	if(no_border) return 0;
	return borderSpec.t_H;
}

int getWindowWidth(void){
	if(no_border) return ICONBAR_WD;
	return ICONBAR_WD+borderSpec.l_W+borderSpec.r_W;
}
int getWindowHeight(void){
	if(no_border) return ICONBAR_HT;
	return ICONBAR_HT+borderSpec.t_H+borderSpec.b_H;
}



Pixmap getRoot(void){
	Pixmap rpixmap = None;
	Atom rpatom = XInternAtom(dpy, "_SGI_DESKS_BACKGROUND", True);

	if( rpatom != None ){
		Atom type;
		int format;
		unsigned long nitems, bytes_after;
		unsigned char *prop = NULL;

		XGetWindowProperty(dpy, rootWin, rpatom, 0, 1, False, XA_PIXMAP, &type, &format, &nitems, &bytes_after, &prop);
		if (prop && (type == XA_PIXMAP)){
			rpixmap = *((Pixmap *)prop);
			XFree(prop);
		} else {
			if(DEBUG) printf("prop = None\n");
		}
	} else {
		printf("rpatom = None\n");
	}

	return rpixmap;
}
Pixel getRootPixel(void)
{
	Pixmap rpixel = 0;
	Atom rpatom = XInternAtom(dpy, "_SGI_DESKS_BACKGROUND", True);

	if( rpatom != None ){
		Atom type;
		int format;
		unsigned long nitems, bytes_after;
		unsigned char *prop = NULL;

		XGetWindowProperty(dpy, rootWin, rpatom, 0, 1, False, SGI_XA_PIXEL, &type, &format, &nitems, &bytes_after, &prop);
		if (prop){
			rpixel = *((Pixel *)prop);
			XFree(prop);
		} else {
			if (DEBUG) printf("prop = None\n");
		}
	}
	else
	{
		if (DEBUG) printf("rpatom = None\n");
	}

	return rpixel;
}

void blendImageToWindow(int width){
	Pixmap mask_pixmap, root_pixmap;
	Pixel root_pixel;  
	GC gc;
	XGCValues values;
	int i;
	XImage *fromroot=NULL, *frommask=NULL, *blended=NULL;
	int newwidth, newheight;
	char *idata;
	Pixel a,b;
	if(!borderImage) return;

	newwidth = ICONBAR_WD+borderSpec.l_W+borderSpec.r_W;
	newheight = ICONBAR_HT+borderSpec.t_H+borderSpec.b_H;

	mask_pixmap = XCreatePixmap(dpy, rootWin, newwidth+1, newheight+1, 24);
	if(!mask_pixmap){
		printf("Error making temporary mask_pixmap\n");
	}
	gc = XCreateGC(dpy, mask_pixmap, 0, &values);
	XSetForeground(dpy, gc, WhitePixel(dpy, scrnNum));
	XFillRectangle(dpy, mask_pixmap, gc, 0, 0, newwidth, newheight);

	/* Sides first... */
	if(borderSpec.l_W){
			if(borderSpec.l_HT){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.l_X, borderSpec.l_Y, borderSpec.l_W, borderSpec.l_HT, 0, borderSpec.t_H);
			}
			if(borderSpec.l_HL){
				for(i = borderSpec.l_HT; i < ICONBAR_HT-borderSpec.l_HB; i += borderSpec.l_HL){
					XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.l_X, borderSpec.l_Y+borderSpec.l_HT, borderSpec.l_W, borderSpec.l_HL, 0, borderSpec.t_H+i);
				}
			}
			if(borderSpec.l_HB){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.l_X, borderSpec.l_Y+borderSpec.l_HT+borderSpec.l_HL, borderSpec.l_W, borderSpec.l_HB, 0, borderSpec.t_H+ICONBAR_HT-borderSpec.l_HB);
			}
	}
	if(borderSpec.r_W){
			if(borderSpec.r_HT){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.r_X, borderSpec.r_Y, borderSpec.r_W, borderSpec.r_HT, borderSpec.l_W+ICONBAR_WD, borderSpec.t_H);
			}
			if(borderSpec.r_HL){
				for(i = borderSpec.r_HT; i < ICONBAR_HT-borderSpec.r_HB; i += borderSpec.r_HL){
					XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.r_X, borderSpec.r_Y+borderSpec.r_HT, borderSpec.r_W, borderSpec.r_HL, borderSpec.l_W+ICONBAR_WD, borderSpec.t_H+i);
				}
			}
			if(borderSpec.r_HB){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.r_X, borderSpec.r_Y+borderSpec.r_HT+borderSpec.r_HL, borderSpec.r_W, borderSpec.r_HB, borderSpec.l_W+ICONBAR_WD, borderSpec.t_H+ICONBAR_HT-borderSpec.r_HB);
			}
	}

	/* Top next */
	if(borderSpec.tl_W && borderSpec.tl_H){
		XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.tl_X, borderSpec.tl_Y, borderSpec.tl_W, borderSpec.tl_H, 0, 0);
	}
	if(borderSpec.t_W && borderSpec.t_H){
			if(borderSpec.t_WT){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.t_X, borderSpec.t_Y, borderSpec.t_WT, borderSpec.t_H, borderSpec.tl_W, 0);
			}
			if(borderSpec.t_WL){
				for(i = borderSpec.t_WT; i < ICONBAR_WD-borderSpec.t_WB; i += borderSpec.t_WL){
					XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.t_X+borderSpec.t_WT, borderSpec.t_Y, borderSpec.t_WL, borderSpec.t_H, borderSpec.tl_W+i, 0);
				}
			}
			if(borderSpec.t_WB){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.t_X+borderSpec.t_WT+borderSpec.t_WL, borderSpec.t_Y, borderSpec.t_WB, borderSpec.t_H, borderSpec.tl_W+ICONBAR_WD-borderSpec.t_WB, 0);
			}
	}
	if(borderSpec.tr_W && borderSpec.tr_H){
		XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.tr_X, borderSpec.tr_Y, borderSpec.tr_W, borderSpec.tr_H, borderSpec.l_W+ICONBAR_WD, 0);
	}

	/* Finally bottom */
	if(borderSpec.bl_W && borderSpec.bl_H){
		XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.bl_X, borderSpec.bl_Y, borderSpec.bl_W, borderSpec.bl_H, 0, borderSpec.t_H+ICONBAR_HT);
	}
	if(borderSpec.b_W && borderSpec.b_H){
			if(borderSpec.b_WT){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.b_X, borderSpec.b_Y, borderSpec.b_WT, borderSpec.b_H, borderSpec.bl_W, borderSpec.t_H+ICONBAR_HT);
			}
			if(borderSpec.b_WL){
				for(i = borderSpec.b_WT; i < ICONBAR_WD-borderSpec.b_WB; i += borderSpec.b_WL){
					XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.b_X+borderSpec.b_WT, borderSpec.b_Y, borderSpec.b_WL, borderSpec.b_H, borderSpec.bl_W+i, borderSpec.t_H+ICONBAR_HT);
				}
			}
			if(borderSpec.b_WB){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.b_X+borderSpec.b_WT+borderSpec.b_WL, borderSpec.b_Y, borderSpec.b_WB, borderSpec.b_H, borderSpec.bl_W+ICONBAR_WD-borderSpec.b_WB, borderSpec.t_H+ICONBAR_HT);
			}
	}
	if(borderSpec.br_W && borderSpec.br_H){
		XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.br_X, borderSpec.br_Y, borderSpec.br_W, borderSpec.br_H, borderSpec.l_W+ICONBAR_WD, borderSpec.t_H+ICONBAR_HT);
	}


	idata = (char *)malloc(newheight*newwidth*4);
	root_pixmap = getRoot();
	if(root_pixmap != None){
		fromroot = XGetImage(dpy, root_pixmap, (scr_wd-newwidth)>>1, (scr_ht-newheight), newwidth, newheight, 0xffffffff, ZPixmap);

		frommask = XGetImage(dpy, mask_pixmap, 0, 0, newwidth, newheight, 0xffffffff, ZPixmap);
		blended = XCreateImage(dpy, DefaultVisual(dpy, scrnNum), 24, ZPixmap, 0, idata, newwidth, newheight, 16, 0);
      if ( !fromroot || !frommask ) return;
		XInitImage(blended);
		if (DEBUG) {
			printf("fromroot: %d frommask: %d\n", fromroot->bits_per_pixel, frommask->bits_per_pixel);
		}
		if(fromroot->bits_per_pixel == 32 && frommask->bits_per_pixel == 32)
		{
			int xloop, yloop;
         
			for(yloop = 0; yloop < newheight; yloop++){
				for(xloop = 0; xloop < newwidth; xloop++){
					a = XGetPixel(fromroot, xloop, yloop);
					b = XGetPixel(frommask, xloop, yloop);

					if(b){
						a &= 0xfefefefe;
						b &= 0xfefefefe;
						a >>= 1; b >>= 1;
						a += b;
					} else {
						a = 0;
					}
					XPutPixel(blended, xloop, yloop, a);
				}
			}
		}
		XPutImage(dpy, mask_pixmap, gc, blended, 0, 0, 0, 0, newwidth, newheight);

		XDestroyImage(fromroot);
		XDestroyImage(frommask);
		XDestroyImage(blended);
	}
	else
	{
		root_pixel = getRootPixel();

		frommask = XGetImage(dpy, mask_pixmap, 0, 0, newwidth, newheight, 0xffffffff, ZPixmap);
		blended = XCreateImage(dpy, DefaultVisual(dpy, scrnNum), 24, ZPixmap, 0, idata, newwidth, newheight, 16, 0);
		XInitImage(blended);
		if (DEBUG) printf("frommask: %d\n", frommask->bits_per_pixel);
		if(frommask->bits_per_pixel == 32){
			int xloop, yloop;
         
			for(yloop = 0; yloop < newheight; yloop++){
				for(xloop = 0; xloop < newwidth; xloop++){
					a = root_pixel;
					b = XGetPixel(frommask, xloop, yloop);

					if(b){
						a &= 0xfefefefe;
						b &= 0xfefefefe;
						a >>= 1; b >>= 1;
						a += b;
					} else {
						a = 0;
					}
					XPutPixel(blended, xloop, yloop, a);
				}
			}
		}
		XPutImage(dpy, mask_pixmap, gc, blended, 0, 0, 0, 0, newwidth, newheight);

		XDestroyImage(frommask);
		XDestroyImage(blended);
	}

	XFreeGC(dpy, gc);
	gc = XCreateGC(dpy, XtWindow(bar), 0, &values);
	XCopyArea(dpy, mask_pixmap, XtWindow(bar), gc, 0, 0, ICONBAR_WD+borderSpec.l_W+borderSpec.r_W, ICONBAR_HT+borderSpec.t_H+borderSpec.b_H, 0, 0);
	XSetWindowBackgroundPixmap(dpy, XtWindow(bar), mask_pixmap);

	XFreeGC(dpy, gc);
	XFreePixmap(dpy, mask_pixmap);
   
   return;
}

void addImageToWindow(int width){
	Pixmap mask_pixmap;
	GC gc;
	XGCValues values;
	int i;

	if(!borderImage) return;

	mask_pixmap = XCreatePixmap(dpy, rootWin, ICONBAR_WD+borderSpec.l_W+borderSpec.r_W+1, ICONBAR_HT+borderSpec.t_H+borderSpec.b_H+1, 24);
	if(!mask_pixmap){
		printf("Error making temporary mask_pixmap\n");
	}
	gc = XCreateGC(dpy, mask_pixmap, 0, &values);
	XSetForeground(dpy, gc, WhitePixel(dpy, scrnNum));
	XFillRectangle(dpy, mask_pixmap, gc, 0, 0, ICONBAR_WD+borderSpec.l_W+borderSpec.r_W, ICONBAR_HT+borderSpec.t_H+borderSpec.b_H);

	/* Sides first... */
	if(borderSpec.l_W){
			if(borderSpec.l_HT){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.l_X, borderSpec.l_Y, borderSpec.l_W, borderSpec.l_HT, 0, borderSpec.t_H);
			}
			if(borderSpec.l_HL){
				for(i = borderSpec.l_HT; i < ICONBAR_HT-borderSpec.l_HB; i += borderSpec.l_HL){
					XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.l_X, borderSpec.l_Y+borderSpec.l_HT, borderSpec.l_W, borderSpec.l_HL, 0, borderSpec.t_H+i);
				}
			}
			if(borderSpec.l_HB){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.l_X, borderSpec.l_Y+borderSpec.l_HT+borderSpec.l_HL, borderSpec.l_W, borderSpec.l_HB, 0, borderSpec.t_H+ICONBAR_HT-borderSpec.l_HB);
			}
	}
	if(borderSpec.r_W){
			if(borderSpec.r_HT){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.r_X, borderSpec.r_Y, borderSpec.r_W, borderSpec.r_HT, borderSpec.l_W+ICONBAR_WD, borderSpec.t_H);
			}
			if(borderSpec.r_HL){
				for(i = borderSpec.r_HT; i < ICONBAR_HT-borderSpec.r_HB; i += borderSpec.r_HL){
					XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.r_X, borderSpec.r_Y+borderSpec.r_HT, borderSpec.r_W, borderSpec.r_HL, borderSpec.l_W+ICONBAR_WD, borderSpec.t_H+i);
				}
			}
			if(borderSpec.r_HB){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.r_X, borderSpec.r_Y+borderSpec.r_HT+borderSpec.r_HL, borderSpec.r_W, borderSpec.r_HB, borderSpec.l_W+ICONBAR_WD, borderSpec.t_H+ICONBAR_HT-borderSpec.r_HB);
			}
	}

	/* Top next */
	if(borderSpec.tl_W && borderSpec.tl_H){
		XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.tl_X, borderSpec.tl_Y, borderSpec.tl_W, borderSpec.tl_H, 0, 0);
	}
	if(borderSpec.t_W && borderSpec.t_H){
			if(borderSpec.t_WT){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.t_X, borderSpec.t_Y, borderSpec.t_WT, borderSpec.t_H, borderSpec.tl_W, 0);
			}
			if(borderSpec.t_WL){
				for(i = borderSpec.t_WT; i < ICONBAR_WD-borderSpec.t_WB; i += borderSpec.t_WL){
					XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.t_X+borderSpec.t_WT, borderSpec.t_Y, borderSpec.t_WL, borderSpec.t_H, borderSpec.tl_W+i, 0);
				}
			}
			if(borderSpec.t_WB){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.t_X+borderSpec.t_WT+borderSpec.t_WL, borderSpec.t_Y, borderSpec.t_WB, borderSpec.t_H, borderSpec.tl_W+ICONBAR_WD-borderSpec.t_WB, 0);
			}
	}
	if(borderSpec.tr_W && borderSpec.tr_H){
		XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.tr_X, borderSpec.tr_Y, borderSpec.tr_W, borderSpec.tr_H, borderSpec.l_W+ICONBAR_WD, 0);
	}

	/* Finally bottom */
	if(borderSpec.bl_W && borderSpec.bl_H){
		XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.bl_X, borderSpec.bl_Y, borderSpec.bl_W, borderSpec.bl_H, 0, borderSpec.t_H+ICONBAR_HT);
	}
	if(borderSpec.b_W && borderSpec.b_H){
			if(borderSpec.b_WT){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.b_X, borderSpec.b_Y, borderSpec.b_WT, borderSpec.b_H, borderSpec.bl_W, borderSpec.t_H+ICONBAR_HT);
			}
			if(borderSpec.b_WL){
				for(i = borderSpec.b_WT; i < ICONBAR_WD-borderSpec.b_WB; i += borderSpec.b_WL){
					XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.b_X+borderSpec.b_WT, borderSpec.b_Y, borderSpec.b_WL, borderSpec.b_H, borderSpec.bl_W+i, borderSpec.t_H+ICONBAR_HT);
				}
			}
			if(borderSpec.b_WB){
				XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.b_X+borderSpec.b_WT+borderSpec.b_WL, borderSpec.b_Y, borderSpec.b_WB, borderSpec.b_H, borderSpec.bl_W+ICONBAR_WD-borderSpec.b_WB, borderSpec.t_H+ICONBAR_HT);
			}
	}
	if(borderSpec.br_W && borderSpec.br_H){
		XCopyArea(dpy, borderImage, mask_pixmap, gc, borderSpec.br_X, borderSpec.br_Y, borderSpec.br_W, borderSpec.br_H, borderSpec.l_W+ICONBAR_WD, borderSpec.t_H+ICONBAR_HT);
	}


	XFreeGC(dpy, gc);

	gc = XCreateGC(dpy, XtWindow(bar), 0, &values);
	XCopyArea(dpy, mask_pixmap, XtWindow(bar), gc, 0, 0, ICONBAR_WD+borderSpec.l_W+borderSpec.r_W, ICONBAR_HT+borderSpec.t_H+borderSpec.b_H, 0, 0);
	XSetWindowBackgroundPixmap(dpy, XtWindow(bar), mask_pixmap);

	XFreeGC(dpy, gc);
	XFreePixmap(dpy, mask_pixmap);
	return;
}



void addBorderToWindow(int width){
	Pixmap mask_pixmap;
	GC gc;
	XGCValues values;
	int i;

	if(no_border) return;

	if(!width) return;
	if(width == last_width) return;

	mask_pixmap = XCreatePixmap(dpy, rootWin, ICONBAR_WD+borderSpec.l_W+borderSpec.r_W+1, ICONBAR_HT+borderSpec.t_H+borderSpec.b_H+1, 1);
	if(!mask_pixmap){
		printf("Error making temporary mask_pixmap\n");
	}
	gc = XCreateGC(dpy, mask_pixmap, 0, &values);
	XSetForeground(dpy, gc, WhitePixel(dpy, scrnNum));
	XFillRectangle(dpy, mask_pixmap, gc, 0, 0, ICONBAR_WD+borderSpec.l_W+borderSpec.r_W, ICONBAR_HT+borderSpec.t_H+borderSpec.b_H);


	/* Sides first... */
	if(borderSpec.l_W){
			if(borderSpec.l_HT){
				XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.l_X, borderSpec.l_Y, borderSpec.l_W, borderSpec.l_HT, 0, borderSpec.t_H);
			}
			if(borderSpec.l_HL){
				for(i = borderSpec.l_HT; i < ICONBAR_HT-borderSpec.l_HB; i += borderSpec.l_HL){
					XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.l_X, borderSpec.l_Y+borderSpec.l_HT, borderSpec.l_W, borderSpec.l_HL, 0, borderSpec.t_H+i);
				}
			}
			if(borderSpec.l_HB){
				XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.l_X, borderSpec.l_Y+borderSpec.l_HT+borderSpec.l_HL, borderSpec.l_W, borderSpec.l_HB, 0, borderSpec.t_H+ICONBAR_HT-borderSpec.l_HB);
			}
	}
	if(borderSpec.r_W){
			if(borderSpec.r_HT){
				XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.r_X, borderSpec.r_Y, borderSpec.r_W, borderSpec.r_HT, borderSpec.l_W+ICONBAR_WD, borderSpec.t_H);
			}
			if(borderSpec.r_HL){
				for(i = borderSpec.r_HT; i < ICONBAR_HT-borderSpec.r_HB; i += borderSpec.r_HL){
					XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.r_X, borderSpec.r_Y+borderSpec.r_HT, borderSpec.r_W, borderSpec.r_HL, borderSpec.l_W+ICONBAR_WD, borderSpec.t_H+i);
				}
			}
			if(borderSpec.r_HB){
				XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.r_X, borderSpec.r_Y+borderSpec.r_HT+borderSpec.r_HL, borderSpec.r_W, borderSpec.r_HB, borderSpec.l_W+ICONBAR_WD, borderSpec.t_H+ICONBAR_HT-borderSpec.r_HB);
			}
	}

	/* Top next */
	if(borderSpec.tl_W && borderSpec.tl_H){
		XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.tl_X, borderSpec.tl_Y, borderSpec.tl_W, borderSpec.tl_H, 0, 0);
	}
	if(borderSpec.t_W && borderSpec.t_H){
			if(borderSpec.t_WT){
				XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.t_X, borderSpec.t_Y, borderSpec.t_WT, borderSpec.t_H, borderSpec.tl_W, 0);
			}
			if(borderSpec.t_WL){
				for(i = borderSpec.t_WT; i < ICONBAR_WD-borderSpec.t_WB; i += borderSpec.t_WL){
					XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.t_X+borderSpec.t_WT, borderSpec.t_Y, borderSpec.t_WL, borderSpec.t_H, borderSpec.tl_W+i, 0);
				}
			}
			if(borderSpec.t_WB){
				XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.t_X+borderSpec.t_WT+borderSpec.t_WL, borderSpec.t_Y, borderSpec.t_WB, borderSpec.t_H, borderSpec.tl_W+ICONBAR_WD-borderSpec.t_WB, 0);
			}
	}
	if(borderSpec.tr_W && borderSpec.tr_H){
		XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.tr_X, borderSpec.tr_Y, borderSpec.tr_W, borderSpec.tr_H, borderSpec.l_W+ICONBAR_WD, 0);
	}

	/* Finally bottom */
	if(borderSpec.bl_W && borderSpec.bl_H){
		XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.bl_X, borderSpec.bl_Y, borderSpec.bl_W, borderSpec.bl_H, 0, borderSpec.t_H+ICONBAR_HT);
	}
	if(borderSpec.b_W && borderSpec.b_H){
			if(borderSpec.b_WT){
				XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.b_X, borderSpec.b_Y, borderSpec.b_WT, borderSpec.b_H, borderSpec.bl_W, borderSpec.t_H+ICONBAR_HT);
			}
			if(borderSpec.b_WL){
				for(i = borderSpec.b_WT; i < ICONBAR_WD-borderSpec.b_WB; i += borderSpec.b_WL){
					XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.b_X+borderSpec.b_WT, borderSpec.b_Y, borderSpec.b_WL, borderSpec.b_H, borderSpec.bl_W+i, borderSpec.t_H+ICONBAR_HT);
				}
			}
			if(borderSpec.b_WB){
				XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.b_X+borderSpec.b_WT+borderSpec.b_WL, borderSpec.b_Y, borderSpec.b_WB, borderSpec.b_H, borderSpec.bl_W+ICONBAR_WD-borderSpec.b_WB, borderSpec.t_H+ICONBAR_HT);
			}
	}
	if(borderSpec.br_W && borderSpec.br_H){
		XCopyArea(dpy, borderMask, mask_pixmap, gc, borderSpec.br_X, borderSpec.br_Y, borderSpec.br_W, borderSpec.br_H, borderSpec.l_W+ICONBAR_WD, borderSpec.t_H+ICONBAR_HT);
	}

	XShapeCombineMask( dpy, shellWin, ShapeBounding, 0, 0, mask_pixmap, ShapeSet);
	XSelectInput( dpy, XtWindow(bar), VisibilityChangeMask );   

	XFreePixmap(dpy, mask_pixmap);
	XFreeGC(dpy, gc);
	last_width = width;

	if(use_blend)
	{
		blendImageToWindow(width);
	}
	else
	{
		addImageToWindow(width);
	}
}
