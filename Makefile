SHELL = /bin/sh

ifndef DEBUG
   OPTS = -O3
   DLIBS =
else
   OPTS = -g -DDEBUG
   DLIBS = -lmalloc_ss
endif

ifndef CC
   CC = cc
endif
ifndef CFLAGS
   ifneq ($(CC),gcc)
      CFLAGS = -mips4
   endif
   CFLAGS += $(OPTS)
endif
ifndef BINDIR
   BINDIR = /usr/nekoware/bin
endif
ifndef MANDIR
   MANDIR = /usr/nekoware/man/man1
endif   
ifndef FTRDIR
   FTRDIR = /usr/lib/filetype/install
endif 

SRC = reximage.c geticon.c menu.c border.c iconbar.c
OBJS = $(SRC:.c=.o)

DISTFILES = $(SRC) iconbar.h iconbar.spec iconbar.idb Makefile\
            ABOUT COPYING INSTALL iconbar.groff iconbar.fti iconbar.ftr\
            iconbar_image.rgb iconbar_mask.rgb blend_image.rgb blend_mask.rgb

# Motif version
XM12 = -I/usr/Motif-1.2/include -L/usr/Motif-1.2/lib32
XM21 = -I/usr/Motif-2.1/include -L/usr/Motif-2.1/lib32

LIBS = -lm 
XLIBS = -limage -lSgm -lXsgivc -lXmu -lXext -lXm -lXt -lX11

.PHONY: all
all: iconbar

iconbar: $(OBJS) iconbar.h iconbar.1
	$(CC) $(CFLAGS) $(OBJS) -o iconbar $(XM21) $(XLIBS) $(DLIBS) $(LIBS)

mips3: $(SRC) iconbar.h iconbar.1
	rm -f ./*.o;\
	$(CC) -mips3 -c reximage.c; \
	$(CC) -mips3 -c geticon.c; \
	$(CC) -mips3 -c menu.c; \
	$(CC) -mips3 -c border.c; \
	$(CC) -mips3 -c iconbar.c; \
	$(CC) -mips3 $(OBJS) -o iconbar-mips3 $(XM12) $(XLIBS) $(LIBS);

iconbar.1: iconbar.groff
	groff -e -man -Tascii iconbar.groff > iconbar.1

reximage.o: reximage.c
geticon.o: reximage.o geticon.c iconbar.h
menu.o: menu.c iconbar.h
border.o: border.c iconbar_mask.xbm iconbar.h

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: clean   
clean:
	rm -f ./*.o
	rm -f ./iconbar
	rm -f ./iconbar-mips3
	rm -f iconbar.1

.PHONY: distclean
distclean: clean   

.PHONY: dist
dist: iconbar mips3
	tar cbf 20 iconbar.tar $(DISTFILES);\
	gzip --best iconbar.tar


TAGCMD      = tag 0x100018 $(BINDIR)/iconbar
ICONCATCMD  = /sbin/install -idb myIdbTag -F /usr/lib/desktop/iconcatalog/pages/C/Applications -lns $(BINDIR)/iconbar Iconbar 

.PHONY: install
install: install-common
	/sbin/install -v -F $(BINDIR) -src ./iconbar -O iconbar
	$(TAGCMD)
	$(ICONCATCMD)

.PHONY: install-mips3   
install-mips3: install-common
	/sbin/install -v -F $(BINDIR) -src ./iconbar-mips3 -O iconbar
	$(TAGCMD)
	$(ICONCATCMD)

.PHONY: install-common   
install-common:   
	/sbin/install -v -F $(MANDIR) -src ./iconbar.1 -O iconbar.1
	/sbin/install -m 644 -v -F $(FTRDIR) -src ./iconbar.ftr -O iconbar.ftr
	/sbin/install -m 644 -v -F $(FTRDIR)/iconlib -src ./iconbar.fti -O iconbar.fti      

.PHONY: uninstall   
uninstall:
	rm -f $(BINDIR)/iconbar
	rm -f $(MANDIR)/iconbar.1
	rm -f $(FTRDIR)/iconbar.ftr
	rm -f $(FTRDIR)/iconlib/iconbar.fti
	rm -f /usr/lib/desktop/iconcatalog/pages/C/Applications/Iconbar

