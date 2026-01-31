
#
# Makefile for exif library
#

LIBS := ../baselib/baselib.a
DIRS := -I ../baselib
DEBUG := -g
OPTIMIZE := #-O3
CFLAGS := -Wall -std=c99 -pedantic $(OPTIMIZE) $(PROFILE) $(DEBUG) $(DIRS)
DEP := ../baselib/baselib.a
CC := gcc $(GDEFS)

all:    exiflib.a xexif prexif injexif

clean:
	   rm *.o exiflib.a xexif prexif injexif

exiflib.a:  exif.o parse.o print.o $(LIBS)
	   /usr/bin/ar csr $@ $^

prexif:   prexif.o exiflib.a $(LIBS)
	   $(CC) $(CFLAGS) -o $@ $^

xexif: xexif.o exiflib.a $(LIBS)
	   $(CC) $(CFLAGS) -o $@ $^

injexif:   injexif.o exiflib.a $(LIBS)
	   $(CC) $(CFLAGS) -o $@ $^

exif.o:     exif.c exif.h parse.h $(DEP)

parse.o:    parse.c exif.h parse.h

print.o:    print.c exif.h print.h

prexif.o:   prexif.c exif.h

xexif.o:    xexif.c exif.h

injxif.o:   injexif.c exif.h

