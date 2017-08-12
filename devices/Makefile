CFLAGS=-I..

LIBDEV=../libdev.a

OBJS=$(LIBDEV)(reports.o) \
	$(LIBDEV)(models.o) \
	$(LIBDEV)(devices.o) 

all: $(OBJS)

reports.o: reports.c reports.h data_reports.i

data_reports.i: reports.j2 devices.yml d2c.py
	./d2c.py reports devices.yml > data_reports.i

models.o: models.c models.h data_models.i

data_models.i: models.j2 devices.yml d2c.py
	./d2c.py models devices.yml > data_models.i

devices.o: devices.c devices.h data_devices.i

data_devices.i: devices.j2 devices.yml d2c.py
	./d2c.py devices devices.yml > data_devices.i

clean:
	rm -f *.o data_reports.i data_models.i data_devices.i
clobber: clean
	rm -f $(LIBDEV)
