/**
 * @file client.c
 * @author Alex Amellal (loris@alexamellal.com)
 * @brief 
 * @version 0.1
 * @date 2021-10-07
 * 
 * @copyright Dalhousie Space Systems Lab (c) 2021 
 * 
 */

// Project headers
#include "settings.h"

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
#include <time.h>
#include <stdbool.h>

// Cubesat space protocol
#include "csp/csp.h"
#include "csp/csp_interface.h"

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
static csp_iface_t csp_io_dev = {
    .name = "fifo",
    .nexthop = fifo_tx,
    .mtu = CSP_PACKET_SIZE,
};

// Thread which listens for incoming messages on the CSP network and prints them to screen
static void *listen(void *parameters);

int main(int argc, char *argv[])
{
  // Check argc
  if (argc != 2 && argc != 1)
  {
    printf("[!] Invalid number of arguments. Try specifying I/O device path (e.g.: /dev/uart0)\n");
    return -1;
  }

  if (argc == 1)
  {
    FIFO = true;
  }

  // Create CSP settings struct
  csp_conf_t conf;
  csp_conf_get_defaults(&conf);

  // Overwrite default settings
  conf.address = CLIENT_ADDR;
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
      printf("[i] Entering FIFO mode...\n");

    if ((io = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC)) < 0)
    {
      printf("[!] Failed to open I/O device at %s\n", argv[1]);
      return -1;
    }

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
  }
  else
  {
    // DEBUG
    if (DEBUG)
      printf("[i] Entering FIFO mode...\n");

    if ((fifo_i = open("fifo_i", O_RDWR)) < 0)
    {
      printf("[!] Failed to open fifo_i\n");
      return -1;
    }
    if ((fifo_o = open("fifo_o", O_RDWR)) < 0)
    {
      printf("[!] Failed to open fifo_o\n");
      return -1;
    }

    // DEBUG
    if (DEBUG)
      printf("[i] Done.\n\n");
  }

  // DEBUG
  if (DEBUG)
    printf("[i] Starting to route incoming packets into CSP network...\n");

  // Start routing incoming packets into CSP network
  pthread_t rx_thread;
  pthread_create(&rx_thread, NULL, fifo_rx, NULL);

  // DEBUG
  if (DEBUG)
    printf("[i] Done.\n\n");

  // Configure default CSP route
  csp_route_set(CSP_DEFAULT_ROUTE, &csp_io_dev, CSP_NODE_MAC);
  csp_route_start_task(ROUTE_WORD_STACK, ROUTE_OS_PRIORITY);

  // Main loop
  for (;;)
  {
    printf("[i] Establishing connection to server...\n");

    // Connect to server with 1000ms timeout
    csp_conn_t *conn = csp_connect(CSP_PRIO_NORM, SERVER_ADDR, PORT, CONN_TIMEOUT, CSP_O_NONE);

    if (conn)
    {
      printf("[i] Connection established.\n\n");
    }
    else
    {
      printf(".");
      struct timespec ts = {
          .tv_sec = 0,
          .tv_nsec = 500000000,
      };
      continue;
    }

    // Start listening thread
    // pthread_t thread_listen;
    // pthread_create(&thread_listen, NULL, listen, (void *)conn);

    for (;;)
    {
      // Get message from user
      char msg[BUF_SIZE];
      printf("[?] Enter a message to send to the server: ");
      fgets(msg, BUF_SIZE, stdin);

      // DEBUG
      if (DEBUG)
        printf("[i] Allocating buffer for packet...\n");

      // Allocate buffer for new packet
      csp_packet_t *packet = csp_buffer_get(strlen(msg));
      if (!packet)
      {
        printf("[!] Failed to allocate packet buffer for message %s. SKIPPING.\n", msg);
        continue;
      }

      // DEBUG
      if (DEBUG)
        printf("[i] Done.\n\n");

      // Copy string into packet data
      strcpy((char *)packet->data, msg);

      // Set packet length
      packet->length = strlen(msg);

      // DEBUG
      if (DEBUG)
        printf("[i] Done.\n\n");

      if (DEBUG)
        printf("Sending: %s\r\n", msg);

      if (!csp_send(conn, packet, 1000)) // csp_send() failed
      {
        printf("[!] Failed to send packet to server\n");
        break;
      }
      // DEBUG
      if (DEBUG)
        printf("[i] Done.\n\n");
    }

    printf("[i] Closing connection...\n");

    // Stop listen thread
    // pthread_cancel(thread_listen);

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

    // TEST : add newline character
    write(io, "\n", 1);
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
    while (read(io, (void *)&packet->length, CSP_PACKET_SIZE) > 0)
    {
      // Inject received packet into CSP network
      csp_qfifo_write(packet, &csp_io_dev, NULL);
      packet = csp_buffer_get(CSP_PACKET_SIZE);
    }
  }
  else
  {
    while (read(fifo_i, (void *)&packet->length, CSP_PACKET_SIZE) > 0)
    {
      // Inject received packet into CSP network
      csp_qfifo_write(packet, &csp_io_dev, NULL);
      packet = csp_buffer_get(CSP_PACKET_SIZE);
    }
  }

  // done
  return NULL;
}

static void *listen(void *parameters)
{
  csp_conn_t *conn = (csp_conn_t *)parameters;

  for (;;)
  {
    csp_packet_t *packet = csp_read(conn, CONN_TIMEOUT);

    if (packet->length > 0)
    {
      // Print message
      printf("\rserver >> %.*s\n", packet->length, packet->data);
    }

    csp_buffer_free(packet);
  }
}