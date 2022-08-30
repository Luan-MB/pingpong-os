CC     = gcc
CFLAGS = -Wall
CLIBS = -lrt

PROG = pingpong-disco1 pingpong-disco2 pingpong-disco-novo
OBJS = disk.o ppos_disk.o ppos_core.o queue.o

.PHONY: clean purge all

all: $(PROG)

%.o: %.c
	$(CC) -c $(CFLAGS) $<

$(PROG) : % :  $(OBJS) %.o
	$(CC) -o $@ $^ $(CLIBS)

clean:
	@rm -f *.o

purge:   
	@rm -f $(PROG)
