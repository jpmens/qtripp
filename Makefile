CFLAGS= -DMAXSPLITPARTS=500 -Idevices/ -I.
LIBDEV=libdev.a

spl: spl.o util.o Makefile json.o ini.o conf.o $(LIBDEV)
	$(CC) $(CFLAGS) -o spl spl.o util.o json.o ini.o conf.o $(LIBDEV)

conf.o: conf.c conf.h
spl.o: spl.c conf.h
util.o: util.c util.h json.h
