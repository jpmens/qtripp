CFLAGS=-I..

all: reports.o models.o 

reports.o: reports.c reports.h data_reports.i

data_reports.i: reports.j2 devices.yml
	./d2c.py reports devices.yml > data_reports.i

models.o: models.c models.h data_models.i

data_models.i: models.j2 devices.yml
	./d2c.py models devices.yml > data_models.i

clean:
	rm -f data_reports.i data_models.i
