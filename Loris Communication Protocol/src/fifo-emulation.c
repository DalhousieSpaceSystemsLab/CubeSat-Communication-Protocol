/**
 * @file fifo-emulation.c
 * @author Alex Amellal (loris@alexamellal.com)
 * @brief Main source for fifo manipulation program (antenna emulation)
 * @version 0.1
 * @date 2022-03-21
 * 
 * @copyright Dalhousie Space Systems Lab (c) 2022
 * 
 */

// Project headers
#include "fifo.h"
#include "antenna.h"

// Standard C libraries
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Check argc
  if(argc != 3) {
    printf("[!] Invalid number of arguments. Please try ./fifo rx_path tx_path\n");
    return -1;
  }

  // Init fifo
  if(fifo_init(argv[1], argv[2]) == -1) {
    printf("[!] Failed to initialize fifo module\n");
    return -1;
  }

  // Override antenna lib
  char data[] = "waddup";
  antenna_write_fd(fifo_get_tx(), data, strlen(data));

  // done
  return 0;
}