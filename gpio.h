#ifndef _GPIO_H
#define _GPIO_H

int gpio_initialise(void);
void gpio_kill(void);
void gpio_setup(int pin, int out);
int gpio_read(int pin);
void gpio_write(int pin, int state);
void gpio_ussleep(uint32_t sleep_us);
void gpio_delay(int us);

#endif
