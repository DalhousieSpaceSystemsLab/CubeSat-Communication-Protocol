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

// Settings
#define SERVER_ADDR 1
#define PORT 10
#define MAX_PEND_CONN 5
#define CONN_TIMEOUT 1000 // in ms
#define PACKET_TIMEOUT 10 // in ms
#define BUF_SIZE 10000
#define CSP_PACKET_SIZE 256
#define ROUTE_WORD_STACK 500
#define ROUTE_OS_PRIORITY 1

#define UART_SPEED B9600
#define UART_PARITY 0

// Program flags
#define DEBUG 0

// Glocal variables
static int io;
static int fifo_i, fifo_o;
static bool FIFO = false;

// Function to be used by the CSP library to route packets to the named pipe (FIFO)
static int fifo_tx(const csp_route_t *ifroute, csp_packet_t *packet);

// Function to be used by the CSP library to route packets from the named pipe (FIFO)
// into the CSP internal network
static void *fifo_rx(void *parameters);

// Define I/O device interface
static csp_iface_t csp_if_fifo = {
    .name = "fifo",
    .nexthop = fifo_tx,
    .mtu = CSP_PACKET_SIZE,
};

int main(int argc, char *argv[])
{
  // Check argc
  if (argc != 2 && argc != 1)
  {
    printf("[!] Invalid number of arguments. Try specifying I/O device path (e.g.: /dev/uart0)\n");
    return -1;
  }

  // Enter FIFO mode if no device path specified
  if (argc == 1)
  {
    FIFO = true;
  }

  // Create CSP settings struct
  csp_conf_t conf;
  csp_conf_get_defaults(&conf);

  // Overwrite default settings
  conf.address = SERVER_ADDR;
  conf.buffer_data_size = BUF_SIZE;

  // Init CSP
  if (csp_init(&conf) != CSP_ERR_NONE)
  {
    printf("[!] Failed to init csp\n");
    return -1;
  }

  // Open I/O device
  if (!FIFO)
  {
    // DEBUG
    if (DEBUG)
      printf("[i] Entering UART mode...\n");

    if ((io = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC)) < 0)
    {
      printf("[!] Failed to open I/O device at %s\n", argv[1]);
      return -1;
    }

    struct termios tty;
    if (tcgetattr(io, &tty) != 0)
    {
      printf("error %d from tcgetattr", errno);
      return -1;
    }

    cfsetospeed(&tty, UART_SPEED);
    cfsetispeed(&tty, UART_SPEED);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK; // disable break processing
    tty.c_lflag = 0;        // no signaling chars, no echo,
                            // no canonical processing
    tty.c_oflag = 0;        // no remapping, no delays
    tty.c_cc[VMIN] = 0;     // read doesn't block
    tty.c_cc[VTIME] = 5;    // 0.5 seconds read timeout

    // tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
    tty.c_iflag |= (IXON);

    tty.c_cflag |= (CLOCAL | CREAD);   // ignore modem controls,
                                       // enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
    tty.c_cflag |= UART_PARITY;
    tty.c_cflag &= ~CSTOPB;
    // tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(io, TCSANOW, &tty) != 0)
    {
      printf("error %d from tcsetattr", errno);
      return -1;
    }

    // DEBUG
    if (DEBUG)
      printf("[i] Done.\n\n");
  }
  else
  {
    // DEBUG
    if (DEBUG)
      printf("[i] Entering FIFO mode...\n");

    if ((fifo_i = open("fifo_o", O_RDWR)) < 0)
    {
      printf("[!] Failed to open fifo_i\n");
      return -1;
    }
    if ((fifo_o = open("fifo_i", O_RDWR)) < 0)
    {
      printf("[!] Failed to open fifo_o\n");
      return -1;
    }

    // DEBUG
    if (DEBUG)
      printf("[i] Done.\n\n");
  }

  // DEBUG
  printf("[i] Starting to route incoming packets into CSP network...\n");

  // Start routing incoming packets into CSP network
  pthread_t rx_thread;
  pthread_create(&rx_thread, NULL, fifo_rx, NULL);

  // DEBUG
  printf("[i] Done.\n\n");

  // DEBUG
  printf("[i] Setting default CSP route\n");

  // Configure default CSP route
  csp_route_set(CSP_DEFAULT_ROUTE, &csp_if_fifo, CSP_NODE_MAC);
  csp_route_start_task(ROUTE_WORD_STACK, ROUTE_OS_PRIORITY);

  // DEBUG
  printf("[i] Done.\n\n");

  // DEBUG
  printf("[i] Configuring server socket...\n");

  // Listen for incoming connections
  csp_socket_t *socket = csp_socket(CSP_SO_NONE);
  csp_bind(socket, PORT);
  csp_listen(socket, MAX_PEND_CONN);

  // DEBUG
  printf("[i] Done.\n\n");

  // Main loop
  for (;;)
  {
    // DEBUG
    if (DEBUG)
      printf("[i] Listening for incoming connections...\n");

    // Proces incoming connection
    csp_conn_t *conn;
    if ((conn = csp_accept(socket, CONN_TIMEOUT)))
    {
      // DEBUG
      if (DEBUG)
        printf("[i] Done.\n\n");
      // DEBUG
      if (DEBUG)
        printf("[i] Connection established. Parsing incoming packets...\n");

      // Process incoming packets
      csp_packet_t *packet;
      while ((packet = csp_read(conn, PACKET_TIMEOUT)) != NULL)
      {
        printf("%s", packet->data);
      }

      // Free packet buffer
      csp_buffer_free(packet);

      // DEBUG
      if (DEBUG)
        printf("[i] Done.\n\n");
    }

    // DEBUG
    if (DEBUG)
      printf("[i] Closing connection...\n");

    // Close connection
    csp_close(conn);

    // DEBUG
    if (DEBUG)
      printf("[i] Done.\n\n");
  }

  return 0;
}

// Function to be used by the CSP library to route packets to the named pipe(FIFO)
static int fifo_tx(const csp_route_t *ifroute, csp_packet_t *packet)
{
  // Write packets to output device
  if (!FIFO)
  {
    if (write(io, &packet->length, packet->length + sizeof(uint32_t) + sizeof(uint16_t)) < 0)
    {
      printf("[!] Failed to write data packet to output device. SKIPPING.\n");
    }
  }
  else
  {
    if (write(fifo_o, &packet->length, packet->length + sizeof(uint32_t) + sizeof(uint16_t)) < 0)
    {
      printf("[!] Failed to write to fifo_o. SKIPPING.\n");
    }
  }

  // Free packet buffer
  csp_buffer_free(packet);

  // Return default CSP return value
  return CSP_ERR_NONE;
}

// Function to be used by the CSP library to route packets from the named pipe (FIFO)
// into the CSP internal network
static void *fifo_rx(void *parameters)
{
  // Allocate buffer for incoming packet
  csp_packet_t *packet = csp_buffer_get(CSP_PACKET_SIZE);
  if (!packet)
  {
    printf("[!] Failed to allocate packet buffer for incoming data from input device. SKIPPING.\n");
    return NULL;
  }

  // Wait for incoming packet from input device
  if (!FIFO)
  {
    while (read(io, (void *)&packet->length, CSP_PACKET_SIZE) >= 0)
    {
      // Inject received packet into CSP network
      csp_qfifo_write(packet, &csp_if_fifo, NULL);
      packet = csp_buffer_get(CSP_PACKET_SIZE);
    }
  }
  else
  {
    while (read(fifo_i, (void *)&packet->length, CSP_PACKET_SIZE) >= 0)
    {
      // Inject received packet into CSP network
      csp_qfifo_write(packet, &csp_if_fifo, NULL);
      packet = csp_buffer_get(CSP_PACKET_SIZE);
    }
  }

  // done
  return NULL;
}