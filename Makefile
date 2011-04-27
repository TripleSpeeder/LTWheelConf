all : ltwheelconf

ltwheelconf: ltwheelconf.c
	gcc -Wall -lusb -g3 -o ltwheelconf ltwheelconf.c

clean:
	rm -rf ltwheelconf
