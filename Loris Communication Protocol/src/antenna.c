#include "antenna.h"

// Glocal variables
static int uartfd = -1;

/**
 * @brief Initializes the UART port for the antenna.
 *
 * @param path Sets the device path to configure.
 * @return 0 on success, 1 on error.
 */
int antenna_init(const char *path) {
  if ((uartfd = open(path, O_RDWR | O_NOCTTY | O_SYNC)) < 0) {
    printf("[!] Failed to open I/O device at %s\n", path);
    return -1;
  }

  struct termios tty;

  if (tcgetattr(uartfd, &tty) < 0) {
    printf("Error from tcgetattr: %s\n", strerror(errno));
    return -1;
  }

  cfsetospeed(&tty, (speed_t)UART_SPEED);
  cfsetispeed(&tty, (speed_t)UART_SPEED);

  tty.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls */
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;      /* 8-bit characters */
  tty.c_cflag &= ~PARENB;  /* no parity bit */
  tty.c_cflag &= ~CSTOPB;  /* only need 1 stop bit */
  tty.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */

  /* setup for non-canonical mode */
  tty.c_iflag &=
      ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  tty.c_oflag &= ~OPOST;

  /* fetch bytes as they become available */
  tty.c_cc[VMIN] = 1;
  tty.c_cc[VTIME] = 1;

  if (tcsetattr(uartfd, TCSANOW, &tty) != 0) {
    printf("Error from tcsetattr: %s\n", strerror(errno));
    return -1;
  }
}

/**
 * @brief Identical to antenna_write, but allows a custom file descriptor to be specified.
 * 
 * @param fd File descriptor to use.
 * @param data Array of bytes to send.
 * @param data_len Number of bytes from data to send.
 * @return 0 on success, -1 on error
 */
int antenna_write_fd(int fd, const char* data, size_t data_len) {
  // Prepare packet
  struct antenna_packet p;
  if (antenna_packet_new(&p) < 0) {
    printf("[!] Failed to create new packet\n");
    return -1;
  }

  // Write bytes to antenna
  if (write(fd, data, data_len) < data_len) {
    printf("[!] Failed to send data of %d bytes in length.\n", data_len);
    return -1;
  }

  // done
  return 0;
}

/**
 * @brief Writes bytes to the antenna.
 *
 * @param data Array of bytes to send.
 * @param data_len Number of bytes from data to send.
 *
 * @return 0 on success, -1 on error
 */
int antenna_write(const char *data, size_t data_len) {
  // Make sure file desc initialized
  if (uartfd == -1) {
    printf(
        "[!] Cannot write to unitialized fd. Make sure to run antenna_init() "
        "first\n");
    return -1;
  }

  return antenna_write_fd(uartfd, data, data_len);
}

/**
 * @brief Writes bytes to the antenna with Reed-solomon FEC but allows a custom file descriptor to be specified.
 *
 * @param fd File descriptor to use.
 * @param data Array of bytes to send.
 * @param data_len Number of bytes from data to send.
 * @return 0 on success, -1 on error
 */
int antenna_write_rs_fd(int fd, const char* data, size_t data_len) {
  // Return status
  int status = 0;

  // Encode data
  int bytes_encoded = 0;
  correct_reed_solomon *encoder = correct_reed_solomon_create(
      correct_rs_primitive_polynomial_8_4_3_2_0, 1, 1, RS_NUM_ROOTS);
  if (encoder == NULL) {
    printf("[!] Failed to create RS encoder\n");
    status = -1;
    goto cleanup;
  }
  while (bytes_encoded < data_len) {
    // Encode block of data
    int bytes_remaining = (data_len - bytes_encoded);
    int bytes_to_encode =
        (bytes_remaining > RS_DATA_LEN) ? RS_DATA_LEN : bytes_remaining;
    char data_encoded[255];
    int data_encoded_len = correct_reed_solomon_encode(
        encoder, &data[bytes_encoded], bytes_to_encode, data_encoded);
    if (data_encoded_len < 0) {
      printf("[!] Failed to encode data\n");
      status = -1;
      goto cleanup;
    }

    // Send block
    if (antenna_write_fd(fd, data_encoded, data_encoded_len) < 0) {
      printf("[!] Failed to send block of encoded data\n");
      status = -1;
      goto cleanup;
    }

    // Update counters
    bytes_encoded += bytes_to_encode;
  }

cleanup:
  // Free encoder
  correct_reed_solomon_destroy(encoder);

  // done
  return status;
}

/**
 * @brief Writes bytes to the antenna with Reed-solomon FEC
 *
 * @param data Array of bytes to send.
 * @param data_len Number of bytes from data to send.
 * @return 0 on success, -1 on error
 */
int antenna_write_rs(const char *data, size_t data_len) {
  return antenna_write_rs_fd(uartfd, data, data_len);
}

/**
 * @brief Reads bytes from the antenna but allows a custom file descriptor to be specified. 
 * Note that there are 2 ways to read:
 * (1) Read UP TO read_len bytes,
 * (2) Block until read_len bytes have been read.
 *
 * @param fd File descriptor to use.
 * @param buffer Output buffer array for incoming bytes.
 * @param read_len Read UP TO or block UNTIL this many bytes read.
 * @param read_mode Set to READ_MODE_UPTO or READ_MODE_UNTIL
 * @return number of bytes read or < 0 for error .
 */
int antenna_read_fd(int fd, char* buffer, size_t read_len, int read_mode) {
  // Create placeholders for reading
  size_t bytes_read = 0;

  // Check read mode
  if (read_mode == READ_MODE_UPTO) {
    if ((bytes_read = read(uartfd, buffer, read_len)) < 0) {
      printf("[!] Failed to read from uartfd\n");
      return -1;
    }
  } else if (read_mode == READ_MODE_UNTIL) {
    while (bytes_read < read_len) {
      // Read bytes
      int new_bytes_read = -1;
      if ((new_bytes_read =
               read(fd, &buffer[bytes_read], read_len - bytes_read)) < 0) {
        printf("[!] Failed to read from fd\n");
        return -1;
      }

      // Update bytes read so far
      bytes_read += new_bytes_read;
    }
  } else {
    printf("[!] Invalid read mode. Try READ_MODE_UPTO or READ_MODE_UNTIL\n");
    return -1;
  }

  // done
  return bytes_read;
}

/**
 * @brief Reads bytes from the antenna.
 * Note that there are 2 ways to read:
 * (1) Read UP TO read_len bytes,
 * (2) Block until read_len bytes have been read.
 *
 * @param buffer Output buffer array for incoming bytes.
 * @param read_len Read UP TO or block UNTIL this many bytes read.
 * @param read_mode Set to READ_MODE_UPTO or READ_MODE_UNTIL
 * @return number of bytes read or < 0 for error.
 */
int antenna_read(char *buffer, size_t read_len, int read_mode) {
  // Make sure file desc initialized
  if (uartfd == -1) {
    printf(
        "[!] Cannot read to unitialized fd. Make sure to run antenna_init() "
        "first\n");
    return -1;
  }

  return antenna_read_fd(uartfd, buffer, read_len, read_mode);
}

/**
 * @brief Reads Reed-solomon encoded bytes from the antenna but allows a custom file descriptor to be specified. 
 *
 * @param fd File descriptor to use.
 * @param buffer Output buffer array for incoming bytes.
 * @param read_len Read UP TO or block UNTIL this many bytes read.
 * @param read_mode Set to READ_MODE_UPTO or READ_MODE_UNTIL
 * @return number of bytes read or < 0 for error.
 */
int antenna_read_rs_fd(int fd, char* buffer, size_t read_len, int read_mode) {
  // Return status
  int status = 0;

  // Create encoder
  correct_reed_solomon *encoder = correct_reed_solomon_create(
      correct_rs_primitive_polynomial_8_4_3_2_0, 1, 1, RS_NUM_ROOTS);
  if (encoder == NULL) {
    printf("[!] Failed to create RS encoder\n");
    status = -1;
    goto cleanup;
  }

  // Create placeholders
  int bytes_decoded = 0;
  char data_in[RS_BLOCK_LEN];

  // Parse incoming blocks until length satisfied
  do {
    // Clear incoming buffer
    memset(data_in, 0, RS_BLOCK_LEN);

    // Read block
    int bytes_read = -1;
    if ((bytes_read = antenna_read_fd(fd, data_in, RS_BLOCK_LEN, read_mode)) < 0) {
      printf("[!] Failed to read encoded block from antenna\n");
      status = -1;
      goto cleanup;
    }

    // Placeholder for decoded bytes
    char decoded[RS_DATA_LEN];

    // Decode it
    int new_bytes_decoded = -1;
    if ((new_bytes_decoded = correct_reed_solomon_decode(
             encoder, data_in, bytes_read, decoded)) < 0) {
      printf("[!] Failed to decode incoming block\n");
      status = -1;
      goto cleanup;
    }

    // Copy decoded bytes into buffer
    int bytes_to_copy = (read_len - bytes_decoded) > new_bytes_decoded
                            ? new_bytes_decoded
                            : (read_len - bytes_decoded);
    memcpy(&buffer[bytes_decoded], decoded, bytes_to_copy);

    // Update counters
    bytes_decoded += bytes_to_copy;
  } while (bytes_decoded < read_len && read_mode == READ_MODE_UNTIL);

cleanup:
  // Free encoder
  correct_reed_solomon_destroy(encoder);

  // done
  if (status)
    return status;
  else
    return bytes_decoded;
}

/**
 * @brief Reads Reed-solomon encoded bytes from the antenna.
 *
 * @param buffer Output buffer array for incoming bytes.
 * @param read_len Read UP TO or block UNTIL this many bytes read.
 * @param read_mode Set to READ_MODE_UPTO or READ_MODE_UNTIL
 * @return number of bytes read or < 0 for error.
 */
int antenna_read_rs(char *buffer, size_t read_len, int read_mode) {
  return antenna_read_rs_fd(uartfd, buffer, read_len, read_mode);
}