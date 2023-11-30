#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include "gpio.h"
#include "fpga.h"
#include "bitfile.h"
#include "debug.h"

#define true  1
#define false 0

#define RESET_TIMEOUT 100000

uint8_t inited = 0;

#define GPIO_FPGA_M1M2    6
#define GPIO_FPGA_RESET   13
#define GPIO_FPGA_FCLK    5
#define GPIO_FPGA_FMISO   27
#define GPIO_FPGA_INITB   17


int fpga_init() {
  if (inited) return 0;
  
  gpio_init(GPIO_FPGA_M1M2);
  gpio_put(GPIO_FPGA_M1M2, 0);
  gpio_set_dir(GPIO_FPGA_M1M2, GPIO_OUT);
  gpio_put(GPIO_FPGA_RESET, 1);
  
  inited = 1;

  return 0;
}

int fpga_claim(uint8_t claim) {
  gpio_init(GPIO_FPGA_M1M2);
  gpio_put(GPIO_FPGA_M1M2, claim ? 1 : 0);
  gpio_set_dir(GPIO_FPGA_M1M2, GPIO_OUT);
  return 0;
}

uint64_t time_us_64() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return 1000000 * tv.tv_sec + tv.tv_usec;
}

#define tight_loop_contents() usleep(1)

int fpga_reset() {
  uint64_t timeout;

  gpio_init(GPIO_FPGA_INITB);
  gpio_init(GPIO_FPGA_RESET);
  gpio_set_dir(GPIO_FPGA_RESET, GPIO_OUT);

  gpio_put(GPIO_FPGA_RESET, 0);
  timeout = time_us_64() + RESET_TIMEOUT;
  while (gpio_get(GPIO_FPGA_INITB) && time_us_64() < timeout)
    tight_loop_contents();
  
  gpio_put(GPIO_FPGA_RESET, 1);
  if (time_us_64() >= timeout) {
    gpio_init(GPIO_FPGA_INITB);
    gpio_init(GPIO_FPGA_RESET);
    return 1;
  }

  timeout = time_us_64() + RESET_TIMEOUT;
  while (!gpio_get(GPIO_FPGA_INITB) && time_us_64() < timeout)
    tight_loop_contents();

  if (time_us_64() >= timeout) {
    gpio_init(GPIO_FPGA_INITB);
    gpio_init(GPIO_FPGA_RESET);
    return 2;
  }
  
  gpio_init(GPIO_FPGA_INITB);
  gpio_init(GPIO_FPGA_RESET);
  return 0;
}

#define CRC32_POLY 0x04c11db7   /* AUTODIN II, Ethernet, & FDDI */
#define CRC32(crc, byte) \
        crc = (crc << 8) ^ CRC32_lut[(crc >> 24) ^ byte]

static uint32_t CRC32_lut[256] = {0};
static void initCRC(void) {
  int i, j;
  unsigned long c;
	if (CRC32_lut[0]) return;

  for (i = 0; i < 256; ++i) {
    for (c = i << 24, j = 8; j > 0; --j)
      c = c & 0x80000000 ? (c << 1) ^ CRC32_POLY : (c << 1);
    CRC32_lut[i] = c;
  }
}


static uint32_t crc32(uint32_t crc, uint8_t *blk, uint32_t len) {
  while (len--) {
    CRC32(crc, *blk);
    blk++;
  }
  return crc;
}

void fpga_clock_byte(uint8_t data, int first) {
  if (first) {
    gpio_put(GPIO_FPGA_FMISO, data >> 7);
    usleep(1);
    gpio_put(GPIO_FPGA_FCLK, 1);
    usleep(1);
    data <<= 1;

    for (int i=1; i<8; i++) {
      gpio_put(GPIO_FPGA_FMISO, data >> 7);
      usleep(1);
      gpio_put(GPIO_FPGA_FCLK, 0);
      usleep(1);
      gpio_put(GPIO_FPGA_FCLK, 1);
      data <<= 1;
    }
  } else {
    for (int i=0; i<8; i++) {
      gpio_put(GPIO_FPGA_FMISO, data >> 7);
      usleep(1);
      gpio_put(GPIO_FPGA_FCLK, 0);
      usleep(1);
      gpio_put(GPIO_FPGA_FCLK, 1);
      data <<= 1;
    }
  }
}

int fpga_configure(void *user_data, uint8_t (*next_block)(void *, uint8_t *), uint32_t assumelength) {
  uint8_t bits[512];
  uint32_t data;
  uint32_t len;
  uint32_t thislen;
  int j;

  initCRC();
  uint32_t crc = 0xffffffff;

  if (!next_block(user_data, bits)) {
    // couldn't read data
    return 1;
  }

  uint32_t totallen = bitfile_get_length(bits, assumelength);
  len = totallen == 0xffffffff ? assumelength : totallen;
  debug(("fpga_get_length: returns %u\n", len));
  if (!len) {
    // couldn't find data length
    return 1;
  }

  int firstbyte = 1;
  gpio_init(GPIO_FPGA_FMISO);
  gpio_init(GPIO_FPGA_FCLK);
  gpio_set_dir(GPIO_FPGA_FMISO, GPIO_OUT);
  gpio_set_dir(GPIO_FPGA_FCLK, GPIO_OUT);
  do {
    thislen = len > 512 ? 512 : len;
    crc = crc32(crc, bits, thislen);

    // clock data out
    for (int i=0; i<thislen; i++) {
      fpga_clock_byte(bits[i], firstbyte);
      firstbyte = 0;
    }

    len -= thislen;
    debug(("fpga_configure: remaining %u / %u %u %08X %08X\n", len, totallen, gpio_get(GPIO_FPGA_INITB),
           crc, crc32(0xffffffff, bits, thislen)));
  } while (len > 0 && next_block(user_data, bits) && gpio_get(GPIO_FPGA_INITB));

  gpio_init(GPIO_FPGA_FMISO);
  gpio_init(GPIO_FPGA_FCLK);

  debug(("fpga_configure: crc %08X %d\n", crc, gpio_get(GPIO_FPGA_INITB)));
  return 0;
}


void fpga_boot_from_flash() {
  gpio_init(GPIO_FPGA_FCLK);
  gpio_init(GPIO_FPGA_FMISO);

  fpga_claim(true);
  fpga_reset();
}


static uint8_t readblock(void *f, uint8_t *data) {
  uint32_t len = fread(data, 1, 512, (FILE *)f);
  return len > 0;
}

#define LATE_RESET
void fpga_load_bitfile(char *fn) {
  FILE *f;
  int r;
  /* initialise fpga */
  fpga_init();
  fpga_claim(true);

  /* now configure */
#ifndef LATE_RESET
  int r = fpga_reset();
  if (r) {
    printf("Failed: FPGA reset returns %d\n", r);
    return;
  }
#endif

  /* now open and read file */
  f = fopen(fn, "rb");
  if (!f) {
    printf("Cannot find file\n");
    fpga_claim(false);
    return;
  }

  /* set up SD card read */
  uint32_t filesize;
  fseek(f, 0, SEEK_END);
  filesize = ftell(f);
  rewind(f);

#ifdef LATE_RESET
  /* now configure */
  r = fpga_reset();
  if (r) {
    printf("Failed: FPGA reset returns %d\n", r);
    return;
  }
#endif

  fpga_configure(f, readblock, filesize);
  fpga_claim(false);
}
