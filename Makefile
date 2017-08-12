CC=gcc
CFLAGS= -DMAXSPLITPARTS=500 -Idevices/ -I. -Wall -Werror
LIBDEV=libdev.a

qtripp: qtripp.o util.o Makefile json.o ini.o conf.o $(LIBDEV)
	$(CC) $(CFLAGS) -o qtripp qtripp.o util.o json.o ini.o conf.o $(LIBDEV)

conf.o: conf.c conf.h
qtripp.o: qtripp.c conf.h util.h json.h ini.h devices/devices.h devices/models.h devices/reports.h udata.h
util.o: util.c util.h json.h udata.h

clean:
	rm -f *.o
