CFLAGS= -DMAXSPLITPARTS=500

spl: spl.c util.c util.h Makefile json.o ini.o
	$(CC) $(CFLAGS) -o spl spl.c util.c json.o ini.o
