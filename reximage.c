
/********************************************************************************
** Iconbar - a desktop icon management utility compatible with the IRIX(TM) 4dwm
** Copyright 2003 Antony Fountain, Steven Queen, and Bryan Sawler
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

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

extern Visual *visual;

XImage *reXimage(Display *display, Window window, XImage *source, double widthScale, double heightScale )
{
	XImage        *destination  = (XImage *)0; 
	double         scaleX, scaleY;
	int	       xloop, yloop;

	Pixel          pixel        = (Pixel)0;
	Pixel          pixelx       = (Pixel)0;
	Pixel          pixely       = (Pixel)0;
	Boolean        zPixmap8Bit  = False;
	
	unsigned int   oldWidth, oldHeight;
	int            newWidth, newHeight;
  	unsigned char  r, g, b, a;
  	unsigned char  rx, gx, bx, ax;
  	unsigned char  ry, gy, by, ay;
	double         distx, disty;

	if ( ( display == (Display *)NULL ) || ( source == (XImage *)NULL ) ) {
		return (XImage *)NULL;
	}
	
	newWidth = (Dimension)(double)source->width*widthScale;
	newHeight = (Dimension)(double)source->height*heightScale;
	oldWidth = source->width;
	oldHeight = source->height;
   /* fprintf(stderr,"new %dx%d old %dx%d\n", newWidth, newHeight, oldWidth, oldHeight); */
	if ((newWidth == oldWidth) && (newHeight == oldHeight)) {
		return source ;
	}

	scaleX = (double)oldWidth / (double)newWidth;
	scaleY = (double)oldHeight / (double)newHeight;


	destination       = XCreateImage(display, visual, source->depth, source->format, 0, NULL, newWidth, newHeight, source->bitmap_pad, 0) ;
   /* fprintf(stderr,"size = %d\n", destination->bytes_per_line * newHeight * 4 ); */
	destination->data = XtMalloc((unsigned) (destination->bytes_per_line * newHeight * 4)) ;

	if ((source->format == ZPixmap)   &&
	    (source->depth == 8)          && 
	    (source->bits_per_pixel == 8) &&
	    (destination->bits_per_pixel == 8))
   {
		zPixmap8Bit = True ;
	}

	for (yloop = 0; yloop < newHeight; yloop++) {
		for (xloop = 0; xloop < newWidth; xloop++) {
			if (zPixmap8Bit == True) {
				pixel = ((unsigned char *)source->data)[((int)(yloop*scaleY)*source->bytes_per_line) + (int)(xloop*scaleX)];
				*(unsigned char *)(destination->data+(destination->bytes_per_line*yloop)+xloop) = pixel;
			} else {
				if(source->bits_per_pixel < 24 || destination->bits_per_pixel < 24 || source->depth < 24){
					/* Standard scaling */
					pixel = XGetPixel(source, (int)xloop*scaleX, (int)yloop*scaleY);
					XPutPixel(destination, xloop, yloop, pixel);
				} else {
					/* Smooth Scaling */
					pixel = XGetPixel(source, (int)xloop*scaleX, (int)yloop*scaleY);
					pixelx = XGetPixel(source, ((int)xloop*scaleX)+1, (int)yloop*scaleY);
					pixely = XGetPixel(source, (int)xloop*scaleX, ((int)yloop*scaleY)+1);

					/* Calculate "distance" into X and Y pixels */
					distx = ((double)xloop*scaleX) - (int)((double)xloop*scaleX);
					disty = ((double)yloop*scaleY) - (int)((double)yloop*scaleY);

					/* Break up pixel into RGBA */
					r = (pixel & 0xff); g = ((pixel>>8) & 0xff); b = ((pixel>>16) & 0xff); a = ((pixel>>24) & 0xff);
					rx = (pixelx & 0xff); gx = ((pixelx>>8) & 0xff); bx = ((pixelx>>16) & 0xff); ax = ((pixelx>>24) & 0xff);
					ry = (pixely & 0xff); gy = ((pixely>>8) & 0xff); by = ((pixely>>16) & 0xff); ay = ((pixely>>24) & 0xff);

					/* Interpolate between X and X+1 */
					if(xloop >= (newWidth-1)){
						rx = r; gx = g; bx = b; ax = a;
					} else {
						rx = (rx*distx) + (r*(1.0 - distx));
						gx = (gx*distx) + (g*(1.0 - distx));
						bx = (bx*distx) + (b*(1.0 - distx));
						ax = (ax*distx) + (a*(1.0 - distx));
					}

					/* Interpolate between Y and Y+1 */
					if(yloop >= (newHeight-1)){
						ry = r; gy = g; by = b; ay = a;
					} else {
						ry = (ry*disty) + (r*(1.0 - disty));
						gy = (gy*disty) + (g*(1.0 - disty));
						by = (by*disty) + (b*(1.0 - disty));
						ay = (ay*disty) + (a*(1.0 - disty));
					}

					/* Average X and Y interpolants */
					r = (rx>>1) + (ry>>1);
					g = (gx>>1) + (gy>>1);
					b = (bx>>1) + (by>>1);
					a = (ax>>1) + (ay>>1);

					pixel = (r&0xff) | ((g&0xff)<<8) | ((b&0xff)<<16) | ((a&0xff)<<24);
					XPutPixel(destination, xloop, yloop, pixel);
				}
			}
		}
	}

	return(destination);
}

