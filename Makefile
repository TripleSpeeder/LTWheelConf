OBJS=main.o wheelfunctions.o
LIBS=usb-1.0

all: ltwheelconf

ltwheelconf: $(OBJS)
	gcc -Wall -l$(LIBS) -g3 -o ltwheelconf $(OBJS)

main.o: main.c
	gcc -Wall -c main.c

wheelfunctions.o: wheelfunctions.c wheelfunctions.h wheels.h
	gcc -Wall -c wheelfunctions.c

clean:
	rm -rf ltwheelconf $(OBJS)
