#CC=gcc
CC=/pitools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-gcc
objs=main.o gpio.o fpga.o bitfile.o

%.o : %.c
	$(CC) -c -o$@ $<

%.d : %.c
	$(CC) -M -o$@ $<

default: fpga

include $(objs:.o=.d)



fpga: $(objs)
	$(CC) -o$@ $(objs)

clean:
	-rm *.d *.o main


