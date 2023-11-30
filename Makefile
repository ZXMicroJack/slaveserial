CC=gcc
objs=main.o gpio.o fpga.o fpgasupp.o bitfile.o

%.o : %.c
	$(CC) -c -o$@ $<

%.d : %.c
	$(CC) -M -o$@ $<

default: main

include $(objs:.o=.d)



main: $(objs)
	$(CC) -o$@ $(objs)

clean:
	-rm *.d *.o main


