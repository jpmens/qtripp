CFLAGS=-I/Users/jpm/syncthing/tiggr/libs/beanstalk-client/
LDFLAGS=-L /Users/jpm/syncthing/tiggr/libs/beanstalk-client/ -l beanstalk

all: tt

tt: tt.c
	gcc -Wall -Werror $(CFLAGS) -o tt tt.c -lcurl $(LDFLAGS)
