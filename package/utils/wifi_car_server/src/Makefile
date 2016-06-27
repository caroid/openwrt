CC = gcc
CFLAGS = -Wall
OBJS = wifi_car_server.o

all: wifi_car_server

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)

fbtest: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f rbcfg *.o
