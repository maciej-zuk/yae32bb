#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <hidapi/hidapi.h>

int main(int argc, char *argv[]) {
  int res;
  unsigned char buf[16];
  wchar_t wstr[255];
  hid_device *handle;
  hid_init();

  handle = hid_open(0x0, 0x0, NULL);
  if (!handle) {
    printf("Bad handle :(\n");
    hid_exit();
    return 1;
  }

  // Read the Manufacturer String
  res = hid_get_manufacturer_string(handle, wstr, 255);
  printf("Manufacturer String: %ls\n", wstr);

  // Read the Product String
  res = hid_get_product_string(handle, wstr, 255);
  printf("Product String: %ls\n", wstr);

  // Read the Serial Number String
  res = hid_get_serial_number_string(handle, wstr, 255);
  printf("Serial Number String: %ls\n", wstr);

  for (int i = 0; i < 16; i++) {
    buf[i] = 0;
  }

  if (argc == 2) {
    if (strcmp(argv[1], "--prog") == 0) {
      printf("Entering program download mode.\n");
      buf[0] = 0x10;
      buf[4] = 0;
      buf[6] = (1 << 3) | (1 << 4);
      buf[9] = 0x67;
      res = hid_write(handle, buf, 16);

      buf[0] = 0x8;
      buf[11] = 0;
      buf[12] = (1 << 3) | (1 << 4);
      res = hid_write(handle, buf, 16);

      usleep(150000);

      buf[11] = (1 << 3);
      buf[12] = (1 << 4);
      res = hid_write(handle, buf, 16);

      usleep(150000);

      buf[11] = (1 << 3) | (1 << 4);
      buf[12] = 0;
      res = hid_write(handle, buf, 16);

      buf[0] = 0x10;
      buf[4] = (1 << 3) | (1 << 4);
      buf[6] = (1 << 3) | (1 << 4);
      buf[9] = 0x67;
      res = hid_write(handle, buf, 16);
    } else if (strcmp(argv[1], "--reset") == 0) {
      printf("Resetting.\n");
      buf[0] = 0x10;
      buf[4] = 0;
      buf[6] = (1 << 3) | (1 << 4);
      buf[9] = 0x67;
      res = hid_write(handle, buf, 16);

      buf[0] = 0x8;
      buf[11] = (1 << 4);
      buf[12] = (1 << 3);
      res = hid_write(handle, buf, 16);

      usleep(150000);

      buf[11] = (1 << 3) | (1 << 4);
      buf[12] = 0;
      res = hid_write(handle, buf, 16);

      buf[0] = 0x10;
      buf[4] = (1 << 3) | (1 << 4);
      buf[6] = (1 << 3) | (1 << 4);
      buf[9] = 0x67;
      res = hid_write(handle, buf, 16);
    }
  }
  hid_close(handle);
  hid_exit();
  return 0;
}