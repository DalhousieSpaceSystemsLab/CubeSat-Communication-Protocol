#include "antenna.h"

// Glocal variables
static int uartfd = -1;
static int panic_errno = 0;

/**
 * @brief Initializes the UART port for the antenna.
 * 
 * @param path Sets the device path to configure.
 * @return 0 on success, 1 on error.
 */
int antenna_init(const char* path) {
  if ((uartfd = open(path, O_RDWR | O_NOCTTY | O_SYNC)) < 0)
  {
    printf("[!] Failed to open I/O device at %s\n", path);
    return -1;
  }
  
  struct termios tty;

  if (tcgetattr(uartfd, &tty) < 0)
  {
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
  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  tty.c_oflag &= ~OPOST;

  /* fetch bytes as they become available */
  tty.c_cc[VMIN] = 1;
  tty.c_cc[VTIME] = 1;

  if (tcsetattr(uartfd, TCSANOW, &tty) != 0)
  {
    printf("Error from tcsetattr: %s\n", strerror(errno));
    return -1;
  }
}

/**
 * @brief Writes bytes to the antenna.
 * 
 * @param data Array of bytes to send.
 * @param data_len Number of bytes from data to send.
 * 
 * @return 0 on success, -1 on error
 */
int antenna_write(const char* data, size_t data_len) {
  // Make sure file desc initialized
  if(uartfd == -1) {
    printf("[!] Cannot write to unitialized fd. Make sure to run antenna_init() first\n");
    return -1;
  }

  // Write bytes to antenna
  if(write(uartfd, data, data_len) < data_len) {
    printf("[!] Failed to send data of %d bytes in length.\n", data_len);
    return -1;
  }

  // done
  return 0;
}

/**
 * @brief Writes bytes to the antenna with Reed-solomon FEC
 * 
 * @param data Array of bytes to send.
 * @param data_len Number of bytes from data to send.
 * @return 0 on success, -1 on error 
 */
int antenna_write_rs(const char* data, size_t data_len) {
  // Encode data
  int bytes_encoded = 0;
  correct_reed_solomon *encoder = correct_reed_solomon_create(correct_rs_primitive_polynomial_8_4_3_2_0, 1, 1, 2);
  if(encoder == NULL) {
    printf("[!] Failed to create RS encoder\n");
    return -1;
  }
  while(bytes_encoded < data_len) {
    // Encode block of data
    int bytes_remaining = (data_len - bytes_encoded);
    int bytes_to_encode = (bytes_remaining > MAX_READ_LEN) ? MAX_READ_LEN : bytes_remaining;
    char data_encoded[255];
    int data_encoded_len = correct_reed_solomon_encode(encoder, data, bytes_to_encode, data_encoded);
    if(data_encoded_len < 0) {
      printf("[!] Failed to encode data\n");
      return -1;
    }

    // Send block
    if(antenna_write(data_encoded, data_encoded_len) < 0) {
      printf("[!] Failed to send block of encoded data\n");
      return -1;
    }

    // Update counters
    bytes_encoded += bytes_to_encode;
  }

  // Free encoder
  correct_reed_solomon_destroy(encoder);

  // done
  return 0;
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
  if(uartfd == -1) {
    printf("[!] Cannot read to unitialized fd. Make sure to run antenna_init() first\n");
    return -1;
  }

  // Update read_len if > MAX
  if(read_len > MAX_READ_LEN) {
    read_len = MAX_READ_LEN;
  }

  // Create placeholders for reading
  size_t bytes_read = 0;
  char buffer_in[MAX_READ_LEN];

  // Cleanup buffer
  memset(buffer_in, MAX_READ_LEN, 0);

  // Check read mode
  if(read_mode == READ_MODE_UPTO) {
    if((bytes_read = read(uartfd, buffer_in, read_len)) < 0) {
      printf("[!] Failed to read from uartfd\n");
      return -1;
    }
  } else if(read_mode == READ_MODE_UNTIL) {
    while(bytes_read < read_len) {
      // Read bytes
      int new_bytes_read = -1;
      if((new_bytes_read = read(uartfd, &buffer_in[bytes_read], read_len - bytes_read)) < 0) {
        printf("[!] Failed to read from uartfd\n");
        return -1;
      }

      // Update bytes read so far
      bytes_read += new_bytes_read;
    }
  } else {
    printf("[!] Invalid read mode. Try READ_MODE_UPTO or READ_MODE_UNTIL\n");
    return -1;
  }

  // Copy internal buffer out
  memcpy(buffer, buffer_in, bytes_read);

  // done
  return bytes_read;
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
  // Reset panic
  panic_errno = 0;
  
  // Create placeholders for memory 
  char *buffer_in = NULL;
  char *decoded   = NULL;

  // Allocate memory for incoming message
  int bytes_to_read = (read_len / RS_BLOCK_LEN) * RS_BLOCK_LEN + RS_BLOCK_LEN;
  buffer_in = (char*) malloc(bytes_to_read);
  if(buffer_in == NULL) {
    printf("[!] Failed to allocate memory for incoming encoded message\n");
    return -1;
  }
  
  // Read data
  int bytes_read = -1;
  if((bytes_read = antenna_read(buffer_in, bytes_to_read, read_mode)) < 0) {
    printf("[!] Failed to read UPTO %d bytes from the antenna\n", bytes_to_read);
    panic_errno = -1;
    goto cleanup;
  }

  // Allocate memory for decoded message
  int max_decoded_len = bytes_to_read / RS_BLOCK_LEN * RS_DATA_LEN;
  decoded = (char*) malloc(max_decoded_len);
  if(decoded == NULL) {
    printf("[!] Failed to allocate memory for decoded message\n");
    panic_errno = -1;
    goto cleanup;
  }
  
  // Decode it
  int bytes_decoded = 0;
  int decoded_len   = 0;
  correct_reed_solomon *encoder = correct_reed_solomon_create(correct_rs_primitive_polynomial_8_4_3_2_0, 1, 1, 2);
  while(bytes_decoded < bytes_read) {
    // Decode block
    int bytes_to_decode = ((bytes_read - bytes_decoded) > RS_BLOCK_LEN) ? RS_BLOCK_LEN : (bytes_read - bytes_decoded); // protection against overreading in block length multiple
    int bytes_decoded_this_time = correct_reed_solomon_decode(encoder, &buffer_in[bytes_decoded], bytes_to_decode, &decoded[decoded_len]);
    if(bytes_decoded_this_time < 0) {
      printf("[!] Failed to decode block\n");
      panic_errno = -1;
      goto cleanup;
    }

    // Update counters
    bytes_decoded += bytes_to_decode;
    decoded_len += bytes_decoded_this_time;
  }

  // Copy decoded data into buffer
  int bytes_to_export = (read_len > decoded_len) ? decoded_len : read_len;
  memcpy(buffer, decoded, bytes_to_export);

  // Free encoder
  correct_reed_solomon_destroy(encoder);

  cleanup:
    // Free memory
    free(buffer_in);
    free(decoded);
    if(panic_errno) return panic_errno;
  
  // done
  return bytes_to_export;
}