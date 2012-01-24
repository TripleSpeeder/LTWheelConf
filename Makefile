OBJS=main.o wheelfunctions.o wheels.o
CCFLAGS = `pkg-config --cflags libusb-1.0`
LDFLAGS = `pkg-config --libs libusb-1.0`

all: ltwheelconf

ltwheelconf: $(OBJS)
	gcc -Wall $(CCFLAGS) -g3 -o ltwheelconf $(OBJS) $(LDFLAGS)

main.o: main.c
	gcc -Wall $(CCFLAGS) -c main.c

wheels.o: wheels.c wheels.h
	gcc -Wall $(CCFLAGS) -c wheels.c


wheelfunctions.o: wheelfunctions.c wheelfunctions.h wheels.h
	gcc -Wall $(CCFLAGS) -c wheelfunctions.c

clean:
	rm -rf ltwheelconf $(OBJS)
