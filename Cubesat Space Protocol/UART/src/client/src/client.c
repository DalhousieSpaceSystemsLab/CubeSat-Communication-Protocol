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

// Cubesat space protocol
#include "csp/csp.h"
#include "csp/csp_interface.h"

// Settings
#define SERVER_ADDR 1
#define CLIENT_ADDR 2
#define PORT 10
#define BUF_SIZE 256
#define CONN_TIMEOUT 1000 // in ms

// Glocal variables
static int io;

// Function to be used by the CSP library to route packets to the named pipe (FIFO)
static int fifo_tx(const csp_route_t *ifroute, csp_packet_t *packet);

// Function to be used by the CSP library to route packets from the named pipe (FIFO)
// into the CSP internal network
static void *fifo_rx(void *parameters);

// Define I/O device interface
static csp_iface_t csp_io_dev = {
    .name = "io",
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
  int io;
  if ((io = open(argv[1], O_RDWR)) < 0)
  {
    printf("[!] Failed to open I/O device at %s\n", argv[1]);
    return -1;
  }

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

    // Close connection
    csp_close(conn);

    // Impose 1ms delay between each packet to give CSP time to free buffers
    // sleep(1);
  }

  return 0;
}

// Function to be used by the CSP library to route packets to the named pipe(FIFO)
static int fifo_tx(const csp_route_t *ifroute, csp_packet_t *packet)
{
  // Write packets to output device
  if (write(io, &packet->length, packet->length + sizeof(uint32_t) + sizeof(uint16_t)) < 0)
  {
    printf("[!] Failed to write data packet to output device. SKIPPING.\n");
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
  while (read(io, (void *)&packet->length, BUF_SIZE) > 0)
  {
    // Inject received packet into CSP network
    csp_qfifo_write(packet, &csp_io_dev, NULL);
    packet = csp_buffer_get(BUF_SIZE);
  }

  // done
  return NULL;
}