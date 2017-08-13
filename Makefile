CC=gcc
CFLAGS= -DMAXSPLITPARTS=500 -Idevices/ -I. -I/usr/local/include -Wall -Werror
LDFLAGS=-L /usr/local/lib -lmosquitto

LIBDEV=libdev.a

all: libdev qtripp


qtripp: qtripp.o util.o Makefile json.o ini.o conf.o mongoose.o tline.o $(LIBDEV)
	$(CC) $(CFLAGS) -o qtripp qtripp.o util.o json.o ini.o conf.o mongoose.o tline.o $(LIBDEV) $(LDFLAGS)
	if test -r codesign.sh; then /bin/sh codesign.sh; fi


conf.o: conf.c conf.h
tline.o: tline.c tline.h util.h json.h ini.h devices/devices.h devices/models.h devices/reports.h udata.h
qtripp.o: qtripp.c conf.h util.h json.h ini.h devices/devices.h devices/models.h devices/reports.h udata.h tline.h
util.o: util.c util.h json.h udata.h

.PHONY: libdev

libdev:
	$(MAKE) -C devices

clean:
	rm -f *.o
	$(MAKE) -C devices clean

clobber: clean
	rm -f qtripp
	$(MAKE) -C devices clobber
