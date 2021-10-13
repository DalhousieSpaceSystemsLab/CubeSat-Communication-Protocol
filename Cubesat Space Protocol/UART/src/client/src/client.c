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

// Cubesat space protocol
#include "csp/csp.h"
#include "csp/csp_interface.h"

// Settings
#define SERVER_ADDR 1
#define CLIENT_ADDR 2
#define PORT 10
#define BUF_SIZE 250
#define CONN_TIMEOUT 1000 // in ms
#define ROUTE_WORD_STACK 500
#define ROUTE_OS_PRIORITY 1

#define UART_SPEED B115200
#define UART_PARITY 0

#define FIFO 0

// Glocal variables
static int io;
static int fifo_i, fifo_o;

// Function to be used by the CSP library to route packets to the named pipe (FIFO)
static int fifo_tx(const csp_route_t *ifroute, csp_packet_t *packet);

// Function to be used by the CSP library to route packets from the named pipe (FIFO)
// into the CSP internal network
static void *fifo_rx(void *parameters);

// Define I/O device interface
static csp_iface_t csp_io_dev = {
    .name = "fifo",
    .nexthop = fifo_tx,
    .mtu = BUF_SIZE,
};

int main(int argc, char *argv[])
{
  // Check argc
  if (argc != 2)
  {
    printf("[!] Invalid number of arguments. Try specifying I/O device path (e.g.: /dev/uart0)\n");
    return -1;
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

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

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
  }
  else
  {
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
  }

  // Start routing incoming packets into CSP network
  pthread_t rx_thread;
  pthread_create(&rx_thread, NULL, fifo_rx, NULL);

  // Configure default CSP route
  csp_route_set(CSP_DEFAULT_ROUTE, &csp_io_dev, CSP_NODE_MAC);
  csp_route_start_task(ROUTE_WORD_STACK, ROUTE_OS_PRIORITY);

  // Main loop
  for (;;)
  {
    // Get message from user
    char msg[BUF_SIZE];
    printf("[?] Enter a message to send to the server: ");
    fgets(msg, BUF_SIZE, stdin);

    // Allocate buffer for new packet
    csp_packet_t *packet = csp_buffer_get(strlen(msg));
    if (!packet)
    {
      printf("[!] Failed to allocate packet buffer for message %s. SKIPPING.\n", msg);
      continue;
    }

    // Copy string into packet data
    strcpy((char *)packet->data, msg);

    // Set packet length
    packet->length = strlen(msg);

    // Connect to server with 1000ms timeout
    csp_conn_t *conn = csp_connect(CSP_PRIO_NORM, SERVER_ADDR, PORT, CONN_TIMEOUT, CSP_O_NONE);

    printf("Sending: %s\r\n", msg);
    if (!csp_send(conn, packet, 1000)) // csp_send() failed
    {
      printf("[!] Failed to send packet to server\n");
      return -1;
    }
    // Impose 1ms delay between each packet to give CSP time to free buffers
    sleep(1);

    // Close connection
    csp_close(conn);
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
  csp_packet_t *packet = csp_buffer_get(BUF_SIZE);
  if (!packet)
  {
    printf("[!] Failed to allocate packet buffer for incoming data from input device. SKIPPING.\n");
    return NULL;
  }

  // Wait for incoming packet from input device
  if (!FIFO)
  {
    while (read(io, (void *)&packet->length, BUF_SIZE) >= 0)
    {
      // Inject received packet into CSP network
      csp_qfifo_write(packet, &csp_io_dev, NULL);
      packet = csp_buffer_get(BUF_SIZE);
    }
  }
  else
  {
    while (read(fifo_i, (void *)&packet->length, BUF_SIZE) >= 0)
    {
      // Inject received packet into CSP network
      csp_qfifo_write(packet, &csp_io_dev, NULL);
      packet = csp_buffer_get(BUF_SIZE);
    }
  }

  // done
  return NULL;
}