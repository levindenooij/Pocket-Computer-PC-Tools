#
# Make Casio utilities
#
CC=gcc
CCOPTS=-o $@ -D__unix__	-Wno-format
CCOPTS+=-Wno-logical-op-parentheses -Wno-pointer-sign -Wno-switch \
        -Wno-int-to-void-pointer-cast -Wno-void-pointer-to-int-cast
	
os:=$(shell	uname)
TARGET=unix
OWNER=root:root
DIST=/usr/local/bin
ifeq ($(os),Linux)
  TARGET=linux
  scripts=$(TARGET)/*.sh
else
  ifeq ($(os),Darwin)
	TARGET=osx
	OWNER=root:wheel
	#CCOPTS+=-arch i386 -arch ppc -flat_namespace
	#CCOPTS+=-arch i386 -flat_namespace
  endif
endif
files:=	$(TARGET)/wav2raw $(TARGET)/wav2wav	\
	$(TARGET)/wave730 \
	$(TARGET)/wave850 $(TARGET)/waveX07	\
	$(TARGET)/list702 $(TARGET)/list730	\
	$(TARGET)/list850 $(TARGET)/list8000	\
	$(TARGET)/listX07 $(TARGET)/list74  	\
	$(TARGET)/bas702  $(TARGET)/bas730	\
	$(TARGET)/bas850  $(TARGET)/basX07	\
	$(TARGET)/md100
dist_files:=$(foreach f,$(files) $(scripts),$(DIST)/$(notdir $f))

all:	$(files)

clean:
	rm $(TARGET)/*.o

install: $(files) $(scripts)
	cp $(files)	$(scripts) $(DIST)
	chmod 755 $(dist_files)
	chown $(OWNER) $(dist_files)

zip:
	touch casutil.zip
	rm casutil.zip
	zip	casutil.zip	-r *.c *.h COPYING INSTALL Makefile	*.bat *.cmd	\
		doc	linux osx dos os2 win32	pb-dump	wince \
		-x */.*	mk*.c x*.c */wave.o	win32/debug/ win32/debug/* \
		doc/tmp/ doc/tmp/* \
		wince/*.vcl wince/*.vco wince/ARMDeb wince/ARMDeb/* \
		wince/ARMRel/*.obj wince/ARMRel/*.pch \
		wince/libce/ARMDeb wince/libce/ARMDeb/* \
		wince/libce/ARMRel/*.obj wince/libce/ARMRel/*.pch

$(TARGET)/wave.o:	wave.c wave.h
	$(CC) $(CCOPTS)	-c $<

$(TARGET)/wav2raw:	wav2raw.c $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	wav2raw.c $(TARGET)/wave.o

$(TARGET)/wav2wav:	wav2wav.c $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	wav2wav.c $(TARGET)/wave.o

$(TARGET)/wave730:	wave730.c $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	wave730.c $(TARGET)/wave.o

$(TARGET)/wave850:	wave850.c $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	wave850.c $(TARGET)/wave.o

$(TARGET)/waveX07:	waveX07.c $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	waveX07.c $(TARGET)/wave.o

$(TARGET)/list702:	list702.c $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	list702.c $(TARGET)/wave.o

$(TARGET)/list730:	list730.c $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	list730.c $(TARGET)/wave.o

$(TARGET)/list850:	list850.c $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	list850.c $(TARGET)/wave.o

$(TARGET)/list8000:	list8000.c $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	list8000.c $(TARGET)/wave.o

$(TARGET)/listX07:	listX07.c $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	listX07.c $(TARGET)/wave.o

$(TARGET)/list74:	list74.c  $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	list74.c  $(TARGET)/wave.o

$(TARGET)/bas702:	bas702.c  $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	bas702.c  $(TARGET)/wave.o

$(TARGET)/bas730:	bas730.c  $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	bas730.c  $(TARGET)/wave.o

$(TARGET)/bas850:	bas850.c  $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	bas850.c  $(TARGET)/wave.o

$(TARGET)/basX07:	basX07.c  $(TARGET)/wave.o wave.h
	$(CC) $(CCOPTS)	basX07.c  $(TARGET)/wave.o

$(TARGET)/md100:	md100.c
	$(CC) $(CCOPTS)	md100.c


