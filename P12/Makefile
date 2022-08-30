CC     = gcc
CFLAGS = -Wall -g
CLIBS = -lm

PROG = pingpong-mqueue
OBJS = ppos_core.o queue.o

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
