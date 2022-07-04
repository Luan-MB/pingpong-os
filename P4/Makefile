CC     = gcc
CFLAGS = -Wall

PROG = pingpong-tasks1 pingpong-tasks2 pingpong-tasks3
OBJS = ppos_core.o queue.o

.PHONY: clean purge all

all: $(PROG)

%.o: %.c
	$(CC) -c $(CFLAGS) $<

$(PROG) : % :  $(OBJS) %.o
	$(CC) -o $@ $^

clean:
	@rm -f *.o

purge:   
	@rm -f $(PROG)
