CC = gcc
CFLAGS = -Wall
OBJS = wd_operation.o

all: wd_operation

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

wd_operation: $(OBJS)
	$(CC) -o $@ $(OBJS)

clean:
	rm -f rbcfg *.o
