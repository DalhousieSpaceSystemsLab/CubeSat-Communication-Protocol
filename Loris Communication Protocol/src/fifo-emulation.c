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
#include <pthread.h>

// Settings
#define MAX_MSG_LEN 256

void * check_rx(void * data) {
  start:
    char incoming[MAX_MSG_LEN];
    int bytes_read = 0;
    if((bytes_read = antenna_read_fd(fifo_get_rx(), incoming, MAX_MSG_LEN, READ_MODE_UPTO)) < 0) {
      printf("[!] Failed to read data from antenna\n");
      return NULL;
    } else if(bytes_read == 0) {
      goto start;
    } else {
      printf("\r[*] Incoming message: %.*s\n\r>> ", bytes_read, incoming);
    }
  goto start;
}

void * check_rx_file(void * data) {
  start:
    if(antenna_fread_fd(fifo_get_rx(), "incoming.txt") == -1) {
      printf("[!] Failed to fread file from antenna\n");
      goto start;
    }
  goto start;
}

int main(int argc, char *argv[]) {
  // Check argc
  if(argc != 4) {
    printf("[!] Invalid number of arguments. Please try ./fifo <address#> rx_path tx_path\n");
    return -1;
  }

  printf("[DEBUG] Starting fifo...\n");
  // Init fifo
  if(fifo_init(atoi(argv[1]) ,argv[2], argv[3]) == -1) {
    printf("[!] Failed to initialize fifo module\n");
    return -1;
  }
  printf("[DEBUG] Done!\n");

  // Start listening
  // printf("[DEBUG] Start listening for messages...\n");
  // pthread_t rx_thread;
  // pthread_create(&rx_thread, NULL, check_rx, NULL);
  // printf("[DEBUG] Done!\n");
  
  printf("[DEBUG] Start listening for files...\n");
  pthread_t rx_file_thread;
  pthread_create(&rx_file_thread, NULL, check_rx_file, NULL);
  printf("[DEBUG] Done!\n");

  for(;;) {
    printf(">> ");
    char command[MAX_MSG_LEN];
    fgets(command, MAX_MSG_LEN-2, stdin);
    if(antenna_write_fd(fifo_get_tx(), command, strlen(command)) < 0) {
      printf("[!] Failed to send message.\n");
      continue;
    } else {
      printf("[∂] Successfully sent\n");
    }
  }

  // done
  return 0;
}