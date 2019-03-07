CFLAGS += -I.. -Wall -Werror
PYTHON=python

# Are we running next to a virtualenv called `venv/`?
# If so, use that Python binary
ifeq ($(shell test -e venv/bin/python && echo -n yes),yes)
	PYTHON_BIN=venv/bin/python
else
	PYTHON_BIN=$(PYTHON)
endif

# echo "PYTHON=python2 > .make"
-include .make

D2C=./d2c.py
DEPS= devices.yml
LIBDEV=../libdev.a
ARFLAGS=rcs

.DELETE_ON_ERROR:

OBJS=reports.o \
	models.o \
	devices.o \
	ignores.o

all: $(LIBDEV)

$(LIBDEV): $(OBJS)
	$(AR) rcs $@ $(OBJS)


%.i: %.j2 $(D2C) $(DEPS)
	$(PYTHON_BIN) $(D2C) $* devices.yml > $@.tmp && mv $@.tmp $@

reports.o: reports.c reports.h reports.i
models.o: models.c models.h models.i
devices.o: devices.c devices.h devices.i
ignores.o: ignores.c ignores.h ignores.i

clean:
	rm -f *.o *.i *.tmp
clobber: clean
	rm -f $(LIBDEV)
