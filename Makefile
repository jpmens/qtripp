
# Point BSC at the directory into which you've built beanstalk-client
BSC=/Users/jpm/tmp/python/beanstalk/c/beanstalk-client
BEANSTALK=yes
#
CC=gcc
CFLAGS= -DMAXSPLITPARTS=500 -Idevices/ -I. -I/usr/local/include -Wall -Werror
LDFLAGS=-L /usr/local/lib -lmosquitto -lm

OBJS=	util.o \
	json.o \
	ini.o \
	conf.o \
	mongoose.o \
	tline.o

ifeq ($(BEANSTALK),yes)
	OBJS += bean.o
	CFLAGS += -DWITH_BEAN -I$(BSC)
	LDFLAGS += -L $(BSC) -lbeanstalk
endif

LIBDEV=libdev.a

all: libdev qtripp


qtripp: qtripp.o Makefile $(OBJS) $(LIBDEV)
	$(CC) $(CFLAGS) -o qtripp qtripp.o $(OBJS) $(LIBDEV) $(LDFLAGS)
	if test -r codesign.sh; then /bin/sh codesign.sh; fi


conf.o: conf.c conf.h udata.h
tline.o: tline.c tline.h util.h json.h ini.h devices/devices.h devices/models.h devices/reports.h udata.h bean.h
qtripp.o: qtripp.c conf.h util.h json.h ini.h devices/devices.h devices/models.h devices/reports.h udata.h tline.h
util.o: util.c util.h json.h udata.h
bean.o: bean.c udata.h

.PHONY: libdev

libdev:
	$(MAKE) -C devices

clean:
	rm -f *.o
	$(MAKE) -C devices clean

clobber: clean
	rm -f qtripp
	$(MAKE) -C devices clobber
