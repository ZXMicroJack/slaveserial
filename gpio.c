//#include <sysdep.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "gpio.h"

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

void gpio_delay(int i) {
	usleep(i);
}

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(g_gpio_map+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(g_gpio_map+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(g_gpio_map+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(g_gpio_map+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(g_gpio_map+10) // clears bits which are 1 ignores bits which are 0

#define GET_GPIO(g) (*(g_gpio_map+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH

#define GPIO_PULL *(g_gpio_map+37) // Pull up/pull down
#define GPIO_PULLCLK0 *(g_gpio_map+38) // Pull up/pull down clock

static uint8_t pinLut[] = {/*TDO*/9, /*TDI*/10, /*TCK*/11, /*TMS*/25};

volatile uint32_t *g_gpio_map = NULL;

int gpio_initialise(void) {
  int  mem_fd;
  void *gpio_map;

   /* open /dev/mem */
   if ((mem_fd = open("/dev/gpiomem", O_RDWR|O_SYNC) ) < 0) {
      printf("can't open /dev/mem \n");
      return -1;
   }

   /* mmap GPIO */
   gpio_map = mmap(
      NULL,             //Any adddress in our space will do
      0xb4,       //Map length
      PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
      MAP_SHARED,       //Shared with other processes
      mem_fd,           //File to map
      0         //Offset to GPIO peripheral
   );

   close(mem_fd); //No need to keep mem_fd open after mmap

   if (gpio_map == MAP_FAILED) {
      printf("mmap error %d\n", (int)gpio_map);//errno also set!
      return -1;
   }

   g_gpio_map = (volatile void *)gpio_map;
   printf("Info: g_gpio_map = %08X\n", g_gpio_map);
   return 0;
}

void gpio_write(int pinx, int state) {
  uint8_t pin = pinLut[pinx];
	if (state) GPIO_SET = 1 << pin;
	else       GPIO_CLR = 1 << pin;
}

int gpio_read(int pinx) {
  uint8_t pin = pinLut[pinx];
	return GET_GPIO(pin) ? 1 : 0;
}

void gpio_setup(int pinx, int out) {
  uint8_t pin = pinLut[pinx];
	INP_GPIO(pin);
	if (out) OUT_GPIO(pin);
}

void gpio_kill(void) {
	munmap((void *)g_gpio_map, 0xb4);
}
