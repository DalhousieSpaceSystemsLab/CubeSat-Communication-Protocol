#ifndef LORIS_ANTENNA_H
#define LORIS_ANTENNA_H

// Feature macros
#define _BSD_SOURCE

// Reed-solomon library
#include "correct.h"

// Standard C libraries
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// Settings //
// UART
#define UART_SPEED B9600
#define UART_PARITY 0

// Antenna
#define MAX_TXT_FILE_SIZE 8191
#define MAX_READ_LEN 256
#define RS_BLOCK_LEN 255
#define RS_DATA_LEN 234

// Reed-solomon
#define RS_NUM_ROOTS 8

enum { READ_MODE_UPTO, READ_MODE_UNTIL };

/**
 * @brief Initializes the UART port for the antenna.
 *
 * @param path Sets the device path to configure.
 * @return 0 on success, 1 on error.
 */
int antenna_init(const char* path);

/**
 * @brief Writes bytes to the antenna.
 *
 * @param data Array of bytes to send.
 * @param data_len Number of bytes from data to send.
 *
 * @return 0 on success, -1 on error
 */
int antenna_write(const char* data, size_t data_len);

/**
 * @brief Writes bytes to the antenna with Reed-solomon FEC
 *
 * @param data Array of bytes to send.
 * @param data_len Number of bytes from data to send.
 * @return 0 on success, -1 on error
 */
int antenna_write_rs(const char* data, size_t data_len);

/**
 * @brief Reads bytes from the antenna.
 * Note that there are 2 ways to read:
 * (1) Read UP TO read_len bytes,
 * (2) Block until read_len bytes have been read.
 *
 * @param buffer Output buffer array for incoming bytes.
 * @param read_len Read UP TO or block UNTIL this many bytes read.
 * @param read_mode Set to READ_MODE_UPTO or READ_MODE_UNTIL
 * @return number of bytes read or < 0 for error .
 */
int antenna_read(char* buffer, size_t read_len, int read_mode);

/**
 * @brief Reads Reed-solomon encoded bytes from the antenna.
 *
 * @param buffer Output buffer array for incoming bytes.
 * @param read_len Read UP TO or block UNTIL this many bytes read.
 * @param read_mode Set to READ_MODE_UPTO or READ_MODE_UNTIL
 * @return number of bytes read or < 0 for error.
 */
int antenna_read_rs(char* buffer, size_t read_len, int read_mode);

#endif