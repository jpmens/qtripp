UNAME_S := $(shell uname -s)

# Point BSC at the directory into which you've built beanstalk-client
BSC=/usr/local/src/beanstalk-client

# Optionally override make variables in a file called $(hostname).make
#
ifeq ($(host-name),)
	host-name := $(shell hostname)
endif
LOCAL_CONF = $(wildcard $(host-name).make)

-include $(LOCAL_CONF)

BEANSTALK=no
STATSD = yes

#
CC=gcc -g
CFLAGS += -DMAXSPLITPARTS=500 -Idevices/ -I. -I/usr/local/include -Wall -Werror
ifdef LOGFILE_SIZE
CFLAGS += -DLOGFILE_SIZE=$(LOGFILE_SIZE)
endif
ifeq ($(UNAME_S),Darwin)
	CFLAGS += -I/opt/homebrew/include
endif

LDFLAGS=-L /usr/local/lib -lmosquitto -lm -lcdb
ifeq ($(UNAME_S),Darwin)
	LDFLAGS+= -L /opt/homebrew/lib
endif


OBJS=	util.o \
	json.o \
	ini.o \
	conf.o \
	mongoose.o \
	tline.o \
	constfile.o \
	statsd/statsd-client.o

ifeq ($(BEANSTALK),yes)
	OBJS += bean.o
	CFLAGS += -DWITH_BEAN -I$(BSC)
	LDFLAGS += -L $(BSC) -lbeanstalk
endif

ifeq ($(STATSD),yes)
	CFLAGS += -DSTATSD
endif

LIBDEV=libdev.a

all: libdev qtripp qlog


qtripp: qtripp.o Makefile $(OBJS) $(LIBDEV)
	$(CC) $(CFLAGS) -o qtripp qtripp.o $(OBJS) $(LIBDEV) $(LDFLAGS)
	if test -r codesign.sh; then /bin/sh codesign.sh; fi

qlog: qlog.o Makefile mongoose.o
	$(CC) $(CFLAGS) -o qlog qlog.o mongoose.o $(LDFLAGS)
	if test -r codesign.sh; then /bin/sh codesign.sh; fi

conf.o: conf.c conf.h udata.h constfile.h
tline.o: tline.c tline.h util.h json.h ini.h devices/devices.h devices/models.h devices/reports.h udata.h bean.h constfile.h
qtripp.o: qtripp.c conf.h util.h json.h ini.h devices/devices.h devices/models.h devices/reports.h udata.h tline.h constfile.h
util.o: util.c util.h json.h udata.h constfile.h
bean.o: bean.c udata.h constfile.h
constfile.o: constfile.c constfile.h
mongoose.o: mongoose.c mongoose.h

.PHONY: libdev

libdev:
	$(MAKE) -C devices

clean:
	rm -f *.o
	$(MAKE) -C devices clean

clobber: clean
	rm -f qtripp qlog
	$(MAKE) -C devices clobber
