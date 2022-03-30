/**
 * @file fifo.c
 * @author Alex Amellal (loris@alexamellal.com)
 * @brief Library which uses a named pipe to relay information
 * @version 0.1
 * @date 2022-03-21
 * 
 * @copyright Dalhousie Space Systems Lab (c) 2022
 * 
 */

#include "fifo.h"

static int fifo_rx = -1;
static int fifo_tx = -1;

static int fifo_is_init() {
  if(fifo_rx == -1 || fifo_tx == -1) {
    return 0;
  } else {
    return 1;
  }
}

/**
 * @brief Initialize fifo module.
 * @param address Desired address for this thread. Odd for tx first, even for rx first.
 * @param path_rx Path to the RX named pipe (relative to self).
 * @param path_tx Path to the TX named pipe (relative to self).
 * @return 0 = OK, -1 = ERR
 */
int fifo_init(const int address, const char* path_rx, const char* path_tx) {
  if(address % 2 != 0) {
    goto tx;
  }

  rx:
    if(fifo_rx = open(path_rx, O_RDONLY) == -1) {
      perror("failed to initialize fifo_rx");
      return -1;
    }

    if(address % 2 != 0) {
      goto end;
    }

  tx:
    if(fifo_tx = open(path_tx, O_WRONLY) == -1) {
      perror("failed to initialize fifo_tx");
      return -1;
    }

    if(address % 2 != 0) {
      goto rx;
    }

  end:

  // done
  return 0;
}

/**
 * @brief Writes data to the fifo TX pipe.
 * 
 * @param data Array of data to send.
 * @param data_len Length of data to send in bytes.
 * @return 0 = OK, -1 = ERR.
 */
int fifo_write(char *data, size_t data_len) {
  if(!fifo_is_init()) {
    printf("[!] Fifo must be initialized before reading or writing\n");
    return -1;
  }

  if(write(fifo_tx, data, data_len) == -1) {
    perror("Failed to write data to TX pipe");
    return -1;
  }

  // done
  return 0;
}

/**
 * @brief Reads incoming data from RX pipe.
 * 
 * @param data_out Incoming data buffer.
 * @param max_len Maximum read length into data_out.
 * @return int 
 */
int fifo_read(char *data_out, size_t max_len) {
  if(!fifo_is_init()) {
    printf("[!] Fifo must be initialized before reading or writing\n");
    return -1;
  }

  int bytes_read = -1;
  if((bytes_read = read(fifo_rx, data_out, max_len)) == -1) {
    printf("[!] Failed to read data from RX pipe\n");
    return -1;
  }

  // done
  return bytes_read;
}

/**
 * @brief Returns the RX file descriptor.
 * 
 * @return rx file descriptor.
 */
int fifo_get_rx() {
  return fifo_rx;
}

/**
 * @brief Returns the TX file descriptor.
 * 
 * @return tx file descriptor.
 */
int fifo_get_tx() {
  return fifo_tx;
}