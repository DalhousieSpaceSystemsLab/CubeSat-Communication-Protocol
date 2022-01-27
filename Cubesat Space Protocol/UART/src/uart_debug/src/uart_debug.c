/**
 * @file server.c
 * @author Alex Amellal (loris@alexamellal.com)
 * @brief 
 * @version 0.1
 * @date 2021-10-07
 * 
 * @copyright Dalhousie Space Systems Lab (c) 2021 
 * 
 */

// Standard C libraries
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <pthread.h>
#include <stdbool.h>

// Cubesat space protocol
#include "csp/csp.h"
#include "csp/csp_interface.h"

#define STR(x) #x
#define XSTR(x) #x

// Settings
#define SERVER_ADDR 1
#define PORT 10
#define MAX_PEND_CONN 5
#define CONN_TIMEOUT 1000 // in ms
#define PACKET_TIMEOUT 10 // in ms
#define ROUTE_WORD_STACK 500
#define ROUTE_OS_PRIORITY 1

#define UART_SPEED B9600
#define UART_PARITY 0

#define BUF_SIZE 10000

// Program flags
#define DEBUG 0

int main(int argc, char *argv[])
{
  // Check argc
  if (argc != 2 && argc != 1)
  {
    printf("[!] Invalid number of arguments. Try specifying I/O device path (e.g.: /dev/uart0)\n");
    return -1;
  }

  // DEBUG
  if (DEBUG)
    printf("[i] Entering UART mode...\n");

  // File descriptor for uart device
  int io;

  if ((io = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC)) < 0)
  {
    printf("[!] Failed to open I/O device at %s\n", argv[1]);
    return -1;
  }

  /**
   * OLD UART serial port setup
   * 
   */
  // struct termios tty;
  // if (tcgetattr(io, &tty) != 0)
  // {
  //   printf("error %d from tcgetattr", errno);
  //   return -1;
  // }

  // cfsetospeed(&tty, UART_SPEED);
  // cfsetispeed(&tty, UART_SPEED);

  // tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
  // // disable IGNBRK for mismatched speed tests; otherwise receive break
  // // as \000 chars
  // tty.c_iflag &= ~IGNBRK; // disable break processing
  // tty.c_lflag = 0;        // no signaling chars, no echo,
  //                         // no canonical processing
  // tty.c_oflag = 0;        // no remapping, no delays
  // tty.c_cc[VMIN] = 0;     // read doesn't block
  // tty.c_cc[VTIME] = 5;    // 0.5 seconds read timeout

  // // tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
  // tty.c_iflag |= (IXON);

  // tty.c_cflag |= (CLOCAL | CREAD);   // ignore modem controls,
  //                                    // enable reading
  // tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
  // tty.c_cflag |= UART_PARITY;
  // tty.c_cflag &= ~CSTOPB;
  // // tty.c_cflag &= ~CRTSCTS;

  // if (tcsetattr(io, TCSANOW, &tty) != 0)
  // {
  //   printf("error %d from tcsetattr", errno);
  //   return -1;
  // }

  struct termios tty;

  if (tcgetattr(io, &tty) < 0)
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

  if (tcsetattr(io, TCSANOW, &tty) != 0)
  {
    printf("Error from tcsetattr: %s\n", strerror(errno));
    return -1;
  }

  // DEBUG
  if (DEBUG)
    printf("[i] Done.\n\n");

  // Main loop
  for (;;)
  {
    // Get user selection
    printf("\n[i] R => receive data ; S => send data ;\n");
    printf("[?] What would you like to do? >> ");
    char sel[3];
    fgets(sel, 3, stdin);

    // Read mode
    if (sel[0] == 'r' || sel[0] == 'R')
    {
      printf("[?] How many bytes would you like to read? (256 max) >> ");
      int n_bytes;
      scanf("%d", &n_bytes);

      printf("[i] Reading %d bytes from interface...\n", n_bytes);
      for (;;)
      {
        char buf[256];
        int bytes_read = -1;
        if ((bytes_read = read(io, buf, n_bytes)) < 0)
        {
          printf("[!] Failed to read data from interface. SKIPPING.\n");
          continue;
        }
        // printf("[i] Done. %d bytes read = {", bytes_read);

        for (int x = 0; x < bytes_read; x++)
        {
          // printf("%02x ", buf[x]);
          printf("%c", buf[x]);
        }
        // printf("}\n\n");
      }
    }
    // Write mode
    else
    {
      printf("[?] Enter the sequence of bytes to send (" XSTR(BUF_SIZE) " bytes max): ");
      char buf[BUF_SIZE];
      fgets(buf, BUF_SIZE, stdin);

      printf("[i] Sending %d bytes (%s) to interface...\n", strlen(buf), buf);
      int bytes_written = -1;
      if ((bytes_written = write(io, buf, strlen(buf))) < strlen(buf))
      {
        printf("[!] Failed to fully transmit data to interface. SKIPPING.\n\n");
        continue;
      }
      printf("[i] Done.\n\n");
    }
  }

  // done
  return 0;
}
