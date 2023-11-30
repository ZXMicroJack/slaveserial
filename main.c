#include <stdio.h>
#include <stdint.h>
#include "gpio.h"
#include "fpga.h"

int main(int argc, char **argv) {

  if (argc < 2) {
    printf("Usage: fpga <bitfile> - load bitfile\n"
           "       fpga - boot from flash\n");
    return 1;
  }

  gpio_initialise();
  if (argv[1][0] == '-') {
    fpga_boot_from_flash();
  } else {
    fpga_load_bitfile(argv[1]);
  }
  gpio_kill();
  return 0;
}
