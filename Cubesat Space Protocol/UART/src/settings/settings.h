/**
 * @file settings.h
 * @author Alex Amellal (loris@alexamellal.com)
 * @brief Global settings file for server & client
 * @version 0.1
 * @date 2021-10-19
 * 
 * @copyright Dalhousie Space Systems Lab (c) 2021 
 * 
 */

#ifndef CUBESATCOMM_UART_SRC_SETTINGS_SETTINGS_H
#define CUBESATCOMM_UART_SRC_SETTINGS_SETTINGS_H

// Settings
#define SERVER_ADDR 1
#define CLIENT_ADDR 2
#define PORT 10
#define MAX_PEND_CONN 5
#define CONN_TIMEOUT 1000 // in ms
#define PACKET_TIMEOUT 10 // in ms
#define BUF_SIZE 10000
#define CSP_PACKET_SIZE 16
#define ROUTE_WORD_STACK 500
#define ROUTE_OS_PRIORITY 1

#define UART_SPEED B9600
#define UART_PARITY 0

#endif