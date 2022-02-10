#include "antenna.h"

// Glocal variables
static int uartfd = -1;

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

  // Check read mode
  size_t bytes_read = 0;
  char buffer_in[MAX_READ_LEN];
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