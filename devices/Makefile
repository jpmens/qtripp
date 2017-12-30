CFLAGS=-I.. -Wall -Werror

D2C=./d2c.py
DEPS= devices.yml
LIBDEV=../libdev.a

.DELETE_ON_ERROR:

OBJS=$(LIBDEV)(reports.o) \
	$(LIBDEV)(models.o) \
	$(LIBDEV)(devices.o) \
	$(LIBDEV)(ignores.o)

all: $(OBJS)

%.i: %.j2 $(D2C) $(DEPS)
	python2 $(D2C) $* devices.yml > $@.tmp && mv $@.tmp $@

reports.o: reports.c reports.h reports.i
models.o: models.c models.h models.i
devices.o: devices.c devices.h devices.i
ignores.o: ignores.c ignores.h ignores.i

clean:
	rm -f *.o *.i *.tmp
clobber: clean
	rm -f $(LIBDEV)
