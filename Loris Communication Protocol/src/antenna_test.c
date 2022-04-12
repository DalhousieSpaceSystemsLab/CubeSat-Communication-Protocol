// Project headers
#include "antenna.h"
#include "fifo.h"

// Standard C libraries
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

// Requests
#define REQ_BASIC_TELEMETRY "A1"
#define REQ_LARGE_TELEMETRY "B2"
#define REQ_DELET_TELEMETRY "C3"
#define REQ_REBOOT_OBC "D4"
#define REQ_SHUTDOWN_OBC "E5"
#define REQ_RESET_COMMS "F6"
#define REQ_ENABLE_RAVEN "G7"
#define REQ_FWD_COMMAND "CC"
#define REQ_LISTEN_FILE "FF"

// Files
#define FILE_BASIC_TELEMETRY "telemetry.txt"
#define FILE_INCOMING "incoming.txt"

void *monitor_requests(void *data);

int main(int argc, char *argv[]) {
  // Check argc
  if (argc != 2) {
    printf("[!] Invalid number of arguments. Try ./antenna_test PATH\n");
    return -1;
  }

  // Init
  if (antenna_init(argv[1]) < 0) {
    printf("[!] Failed to init antenna\n");
    return -1;
  }

  // Main loop
  int done = 0;
  while (!done) {
    char choice;
    char encoded;
    int choice_len = 0;
    char file_name[20];
    char data[MAX_TXT_FILE_SIZE];
    int skip_data_dump = 0;
    int data_len = -1;

    FILE *file_pointer;
    char file_path[256];
    char *dynamic_file_data = NULL;
    size_t bytes_to_read;

    pthread_t monitor_requests_thread;
    char req[2];
    char userreq[2];

    printf(
        "[?] Read or write (r/w) or encoded (R/W) or make a file request (f/F) "
        "or autonomous mode (x/X) or start the groundstation "
        "daemon (Z)");
    scanf(" %c", &choice);

    switch (choice) {
      case 'r':
        printf("[i] Reading UPTO %d bytes\n", MAX_READ_LEN);
        if ((data_len = antenna_read(data, MAX_READ_LEN, READ_MODE_UPTO)) < 0) {
          printf("[!] failed to antenna read UPTO %d bytes\n", MAX_READ_LEN);
          return -1;
        }
        break;
      case 'R':
        printf("[?] Will this be encoded (y/n): ");
        scanf(" %c", &encoded);
        printf("[?] Read until (u) or UPTO (U): ");
        scanf(" %c", &choice);
        printf("[?] How many: ");
        scanf("%d", &choice_len);
        if (choice == 'u') {
          if (encoded == 'y') {
            printf("[i] Reading UNTIL %d bytes ENCODED...\n", choice_len);
            if ((data_len =
                     antenna_read_rs(data, choice_len, READ_MODE_UNTIL)) < 0) {
              printf("[!] failed to antenna read until %d bytes\n", choice_len);
              return -1;
            }
          } else {
            printf("[i] Reading UNTIL %d bytes...\n", choice_len);
            if ((data_len = antenna_read(data, choice_len, READ_MODE_UNTIL)) <
                0) {
              printf("[!] failed to antenna read until %d bytes\n", choice_len);
              return -1;
            }
          }
        } else {
          if (encoded == 'y') {
            printf("[i] Reading UPTO %d bytes ENCODED...\n", choice_len);
            if ((data_len = antenna_read_rs(data, choice_len, READ_MODE_UPTO)) <
                0) {
              printf("[!] failed to antenna read upto %d bytes\n", choice_len);
              return -1;
            }
          } else {
            printf("[i] Reading UPTO %d bytes...\n", choice_len);
            if ((data_len = antenna_read(data, choice_len, READ_MODE_UPTO)) <
                0) {
              printf("[!] failed to antenna read upto %d bytes\n", choice_len);
              return -1;
            }
          }
        }
        break;
      case 'w':
        printf("[?] Enter message to send: ");
        scanf(" %s", data);
        data_len = strlen(data);
        if (antenna_write(data, data_len) < 0) {
          printf("[!] failed to antenna write \"%s\" containing %d bytes\n",
                 data, data_len);
          return -1;
        }
        break;
      case 'W':
        printf("[?] Enter message to ENCODE & send: ");
        scanf(" %s", data);
        data_len = strlen(data);
        if (antenna_write_rs(data, data_len) < 0) {
          printf("[!] failed to antenna write \"%s\" containing %d bytes\n",
                 data, data_len);
          return -1;
        }
        break;

      // messy. move contents to a function.
      case 'f':
        printf("[?] Enter the PLAINTEXT request you would like to make: ");

        scanf(" %s", req);

        if (strncmp(req, REQ_BASIC_TELEMETRY, 2) == 0) {
          if (antenna_write(REQ_BASIC_TELEMETRY, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }

          if (antenna_fread(FILE_INCOMING) == -1) {
            printf("[!] failed to fread incoming file\n");
            continue;
          }
        } else if (strncmp(req, REQ_LISTEN_FILE, 2) == 0) {
          // Make request
          if (antenna_write(REQ_LISTEN_FILE, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }

          // Ask user to file to send
          printf("[?] Enter the path of the file you'd like to send: ");
          // fgets(file_path, 256, stdin);
          scanf(" %s", file_path);

          // Send file
          if (antenna_fwrite(file_path) == -1) {
            printf("[!] Failed to send plaintext file over the antenna\n");
            continue;
          }
        } else {
          printf("[!] {%c%c} is not a recognized request\n", req[0], req[1]);
          break;
        }

        break;
      case 'F':
        printf("[?] Enter the ENCODED request you would like to make: ");

        scanf(" %s", req);

        if (strncmp(req, REQ_BASIC_TELEMETRY, 2) == 0) {
          if (antenna_write_rs(REQ_BASIC_TELEMETRY, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }

          if (antenna_fread_rs(FILE_INCOMING) == -1) {
            printf("[!] failed to fread incoming file\n");
            continue;
          }
        } else if (strncmp(req, REQ_LISTEN_FILE, 2) == 0) {
          // Make request
          if (antenna_write_rs(REQ_LISTEN_FILE, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }

          // Ask user to file to send
          printf("[?] Enter the path of the file you'd like to send: ");
          // fgets(file_path, 256, stdin);
          scanf(" %s", file_path);

          // Send file
          if (antenna_fwrite_rs(file_path) == -1) {
            printf("[!] Failed to send plaintext file over the antenna\n");
            continue;
          }
        } else {
          printf("[!] {%c%c} is not a recognized request\n", req[0], req[1]);
          break;
        }

        break;

      case 'x':
        printf("[i] Entering autonomous PLAINTEXT mode...\n");

        for (;;) {
          // Listen for incoming requests
          printf("[*] Awaiting next request...");
          if (antenna_read(userreq, 2, READ_MODE_UNTIL) == -1) {
            printf("[!] Failed to read request from antenna\n");
            continue;
          }

          // Identify request
          if (strncmp(userreq, REQ_BASIC_TELEMETRY, 2) == 0) {
            // Send telemetry file
            printf("[i] Basic telemetry request received!\n");
            if (antenna_fwrite(FILE_BASIC_TELEMETRY) == -1) {
              printf("[!] Failed to send telemetry file to fulfill request\n");
              continue;
            }
          } else if (strncmp(userreq, REQ_LARGE_TELEMETRY, 2) == 0) {
          } else if (strncmp(userreq, REQ_DELET_TELEMETRY, 2) == 0) {
          } else if (strncmp(userreq, REQ_REBOOT_OBC, 2) == 0) {
            //Reboot the system
            FILE *console_command;
            console_command = popen("reboot", "w");
            pclose(console_command);
          } else if (strncmp(userreq, REQ_SHUTDOWN_OBC, 2) == 0) {
            //Shutdown the system. The watchdog will kick in in 10 minutes and boot it back up again.
            FILE *console_command;
            console_command = popen("shutdown now", "w");
            pclose(console_command);
          } else if (strncmp(userreq, REQ_RESET_COMMS, 2) == 0) {
            // Cut off power to COMMS for 5 seconds.
            set_gpio(127,0);
            usleep(50000000);
            set_gpio(127,1);
            //Should include some sort of safety in case gpio doesn't turn back on, reboot OBC which
            //should reset all the pins to their default states?
          } else if (strncmp(userreq, REQ_ENABLE_RAVEN, 2) == 0) {
          } else if (strncmp(userreq, REQ_FWD_COMMAND, 2) == 0) {
          } else if (strncmp(userreq, REQ_LISTEN_FILE, 2) == 0) {
            // Listen for incoming file
            printf("[i] File listen request received!\n");
            if (antenna_fread("incoming.txt") == -1) {
              printf("[!] Failed to read incoming file over the antenna\n");
              continue;
            }
          } else {
            printf("[:/] Could not process request [%c%c]\n", userreq[0],
                   userreq[1]);
          }
        }
        break;
      case 'X':
        printf("[i] Entering autonomous ENCODED mode...\n");

        for (;;) {
          // Listen for incoming requests
          printf("[*] Awaiting next request...");
          if (antenna_read_rs(userreq, 2, READ_MODE_UNTIL) == -1) {
            printf("[!] Failed to read request from antenna\n");
            continue;
          }

          // Identify request
          if (strncmp(userreq, REQ_BASIC_TELEMETRY, 2) == 0) {
            // Send telemetry file
            printf("[i] Basic telemetry request received!\n");
            if (antenna_fwrite_rs(FILE_BASIC_TELEMETRY) == -1) {
              printf("[!] Failed to send telemetry file to fulfill request\n");
              continue;
            }
          } else if (strncmp(userreq, REQ_LARGE_TELEMETRY, 2) == 0) {
          } else if (strncmp(userreq, REQ_DELET_TELEMETRY, 2) == 0) {
          } else if (strncmp(userreq, REQ_REBOOT_OBC, 2) == 0) {
          } else if (strncmp(userreq, REQ_RESET_COMMS, 2) == 0) {
          } else if (strncmp(userreq, REQ_ENABLE_RAVEN, 2) == 0) {
          } else if (strncmp(userreq, REQ_FWD_COMMAND, 2) == 0) {
          } else if (strncmp(userreq, REQ_LISTEN_FILE, 2) == 0) {
            // Listen for incoming file
            printf("[i] File listen request received!\n");
            if (antenna_fread_rs("incoming.txt") == -1) {
              printf("[!] Failed to read incoming file over the antenna\n");
              continue;
            }
          } else {
            printf("[:/] Could not process request [%c%c]\n", userreq[0],
                   userreq[1]);
          }
        }
        break;

      case 'z':
      case 'Z':
        printf("Starting the groundstation FIFO daemon...\n");
        // fifo_init(1, )
        break;

      default:
        printf("[!] %c is not a valid option\n", choice);
        continue;
    }

    // skips data dump when dealing with txt files.
    if (skip_data_dump) {
      continue;
    }

    // Print results
    printf("[i] Normal string print (may cause undefined output): ");
    for (int x = 0; x < data_len; x++) printf("%c", data[x]);
    printf("\n");
    printf("[i] Data size = %d\n", data_len);
    printf("-- DATA DUMP --\n");
    for (int x = 0; x < data_len; x++) {
      printf("%.2x=\'%c\' ", data[x], data[x]);
    }
    printf("\n-- END DUMP --\n");
  }

  // done
  return 0;
}

//This function streamlines altering the logic level of a specified pin.
void set_gpio(int gpio, int state){
    char gpio_s[3];
    char state_s[3];
    sprintf(gpio_s,"%d",gpio);
    sprintf(state_s,"%d",state);
    printf("Setting GPIO.\n");
    printf("GPIO:%s, STATE: %s\n", gpio_s,state_s);
    int fd;
    char* gpio_directory = (char*)malloc(sizeof (char) * 100);
    sprintf(gpio_directory,"/sys/class/gpio/gpio%s/value", gpio_s);
    fd = open(gpio_directory, O_WRONLY | O_SYNC);
    write(fd, state_s, 1);
    close(fd);
    printf("Output GPIO %s set to %s.\n",gpio_s,state_s);
    free(gpio_directory);
}
// void *monitor_requests(void *data) {
//   char req[2];
// start:
//   // Listen for incoming requests
//   if (antenna_read(req, 2, READ_MODE_UNTIL) == -1) {
//     printf("[!] Failed to read request from antenna\n");
//     goto start;
//   }

//   // Identify request
//   if (strncmp(req, REQ_BASIC_TELEMETRY, 2) == 0) {
//     // Send telemetry file
//     printf("[i] Basic telemetry request received!\n");
//     if (antenna_fwrite(FILE_BASIC_TELEMETRY) == -1) {
//       printf("[!] Failed to send telemetry file to fulfill request\n");
//       goto start;
//     }
//   } else if (strncmp(req, REQ_LARGE_TELEMETRY, 2) == 0) {
//   } else if (strncmp(req, REQ_DELET_TELEMETRY, 2) == 0) {
//   } else if (strncmp(req, REQ_REBOOT_OBC, 2) == 0) {
//   } else if (strncmp(req, REQ_RESET_COMMS, 2) == 0) {
//   } else if (strncmp(req, REQ_ENABLE_RAVEN, 2) == 0) {
//   } else {
//     printf("[:/] Could not process request [%c%c]\n", req[0], req[1]);
//   }
//   goto start;
// }