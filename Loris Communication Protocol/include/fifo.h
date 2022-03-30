/**
 * @file fifo.h
 * @author Alex Amellal (loris@alexamellal.com)
 * @brief Library which uses a named pipe to relay information
 * @version 0.1
 * @date 2022-03-21
 * 
 * @copyright Dalhousie Space Systems Lab (c) 2022
 * 
 */

#ifndef LORIS_INCLUDE_FIFO_H
#define LORIS_INCLUDE_FIFO_H

// Standard C libraries
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

/**
 * @brief Initialize fifo module.
 * @param address Desired address for this thread. Odd for tx first, even for rx first.
 * @param path_rx Path to the RX named pipe (relative to self).
 * @param path_tx Path to the TX named pipe (relative to self).
 * @return 0 = OK, -1 = ERR
 */
int fifo_init(const int address, const char* path_rx, const char* path_tx);

/**
 * @brief Writes data to the fifo TX pipe.
 * 
 * @param data Array of data to send.
 * @param data_len Length of data to send in bytes.
 * @return 0 = OK, -1 = ERR.
 */
int fifo_write(char *data, size_t data_len);

/**
 * @brief Reads incoming data from RX pipe.
 * 
 * @param data_out Incoming data buffer.
 * @param max_len Maximum read length into data_out.
 * @return int 
 */
int fifo_read(char *data_out, size_t max_len);

/**
 * @brief Returns the RX file descriptor.
 * 
 * @return rx file descriptor.
 */
int fifo_get_rx();

/**
 * @brief Returns the TX file descriptor.
 * 
 * @return tx file descriptor.
 */
int fifo_get_tx();

#endif