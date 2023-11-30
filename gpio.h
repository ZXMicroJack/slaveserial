#ifndef _GPIO_H
#define _GPIO_H

int gpio_initialise(void);
void gpio_kill(void);
// void gpio_setup(int pin, int out);
// int gpio_read(int pin);
// void gpio_write(int pin, int state);
// void gpio_ussleep(uint32_t sleep_us);
// void gpio_delay(int us);


#define GPIO_OUT 0
#define GPIO_IN 1

void gpio_init(int pin);
void gpio_put(int pin, int state);
void gpio_set_dir(int pin, int state);
int gpio_get(int pin);

#endif
