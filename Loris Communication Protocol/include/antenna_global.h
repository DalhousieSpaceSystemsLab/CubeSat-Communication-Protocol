/**
 * @file antenna_global.h
 * @author Alex Amellal (loris@alexamellal.com)
 * @brief Global header for antenna related sources
 * @version 0.1
 * @date 2022-05-13
 *
 * @copyright Dalhousie Space Systems Lab (c) 2022
 *
 */

#ifndef CUBESAT_COMMUNICATION_PROTOCOL_LORIS_COMMUNICATION_PROTOCOL_INCLUDE_ANTENNA_GLOBAL_H
#define CUBESAT_COMMUNICATION_PROTOCOL_LORIS_COMMUNICATION_PROTOCOL_INCLUDE_ANTENNA_GLOBAL_H

// Settings
#define PATH_TO_ANTENNA_DEV "/dev/colibri-uartb"

// Requests
#define REQ_BASIC_TELEMETRY "A1"
#define REQ_LARGE_TELEMETRY "B2"
#define REQ_DELET_TELEMETRY "C3"
#define REQ_REBOOT_OBC "D4"
#define REQ_RESET_COMMS "E5"
#define REQ_ENABLE_RAVEN "F6"
#define REQ_FWD_COMMAND "CC"
#define REQ_LISTEN_FILE "FF"
#define REQ_GET_FILE "FR"
#define REQ_GET_LS "LS"
#define REQ_TAKE_PICTURE "TP"
#define REQ_ENCODE_FILE "EF"
#define REQ_DECODE_FILE "DF"
#define REQ_SHUTDOWN "SD"
#define REQ_REBOOT "RB"
#define REQ_REMOVE "RM"

// Files
#define FILE_BASIC_TELEMETRY "telemetry.txt"
#define FILE_INCOMING "incoming.txt"
#define FILE_LS_INDEX "ls_index.txt"
#define MAX_FILENAME_LEN 64
#define FILE_NOT_FOUND "!!FNF!!"

#endif