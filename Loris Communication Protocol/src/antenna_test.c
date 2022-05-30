// Project headers
#include "antenna.h"
#include "antenna_encoding.h"
#include "antenna_global.h"
#include "fifo.h"

// Standard C libraries
#include <pthread.h>
#include <stdio.h>
#include <string.h>

void *monitor_requests(void *data);

main(int argc, char *argv[]) {
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
    char filename[MAX_FILENAME_LEN];
    char filename_local[MAX_FILENAME_LEN];
    char filename_dest[MAX_FILENAME_LEN];
    FILE *ls_fp = NULL;

    printf(
        "[?] Read or write (r/w) or encoded (R/W) or make a file request (f/F) "
        "or autonomous mode (x/X) or encode/decode a file (e/d): ");
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

          if (antenna_fread(FILE_BASIC_TELEMETRY) == -1) {
            printf("[!] failed to fread incoming file\n");
            continue;
          }

          // Display file contents
          ls_fp = fopen(FILE_BASIC_TELEMETRY, "r");
          if (!ls_fp) {
            printf("[!] Failed to open small telemetry file for reading.\n");
            continue;
          }
          char *nextline = NULL;
          int n = 64;
          while (getline(&nextline, &n, ls_fp) != -1) {
            printf("%s", nextline);
          }
          fclose(ls_fp);
          free(nextline);
        } else if (strncmp(req, REQ_LARGE_TELEMETRY, 2) == 0) {
          if (antenna_write(REQ_LARGE_TELEMETRY, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }

          if (antenna_fread(FILE_LARGE_TELEMETRY) == -1) {
            printf("[!] failed to fread incoming file\n");
            continue;
          }

          // Display file contents
          ls_fp = fopen(FILE_LARGE_TELEMETRY, "r");
          if (!ls_fp) {
            printf("[!] Failed to open large telemetry for reading.\n");
            continue;
          }
          char *nextline = NULL;
          int n = 64;
          while (getline(&nextline, &n, ls_fp) != -1) {
            printf("%s", nextline);
          }
          fclose(ls_fp);
          free(nextline);

        } else if (strncmp(req, REQ_FWD_COMMAND, 2) == 0) {
          // Get command to forward
          printf("[?] Enter the command to forward (remote): ");
          scanf(" %[^\n]%*c", filename);

          // Make request
          if (antenna_write(REQ_FWD_COMMAND, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }

          // Send desired filename
          if (antenna_write(filename, MAX_FILENAME_LEN) == -1) {
            printf("[!] Failed to forward command\n");
            continue;
          }
        } else if (strncmp(req, REQ_LISTEN_FILE, 2) == 0) {
          // Ask user to file to send
          printf("[?] Enter the path of the file you'd like to send (local): ");
          // fgets(file_path, 256, stdin);
          scanf(" %s", filename_local);
          printf("[?] Enter the target filename to save (remote): ");
          // fgets(file_path, 256, stdin);
          scanf(" %s", filename);

          // Make request
          if (antenna_write(REQ_LISTEN_FILE, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }

          // Send filename
          if (antenna_write(filename, MAX_FILENAME_LEN) == -1) {
            printf("[!] Failed to send remote filename\n");
            continue;
          }

          // Send file
          if (antenna_fwrite(filename_local) == -1) {
            printf("[!] Failed to send plaintext file over the antenna\n");
            continue;
          }
        } else if (strncmp(req, REQ_GET_FILE, 2) == 0) {
          // Get desired filename
          printf(
              "[?] Enter the filename or path you would like to request "
              "(remote): ");
          scanf(" %s", filename);
          printf("[?] Enter the filename to save (local): ");
          scanf(" %s", filename_local);

          // Make request
          if (antenna_write(REQ_GET_FILE, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }

          // Send desired filename
          if (antenna_write(filename, MAX_FILENAME_LEN) == -1) {
            printf("[!] Failed to forward filename\n");
            continue;
          }

          // Listen for incoming file
          if (antenna_fread(filename_local) == -1) {
            printf("[!] Failed to read incoming file\n");
            continue;
          }
        } else if (strncmp(req, REQ_GET_LS, 2) == 0) {
          // Get desired directory
          printf(
              "[?] Enter the directory you would like to LS "
              "(remote): ");
          scanf(" %s", filename);

          // Send request
          if (antenna_write(REQ_GET_LS, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }

          // Send directory
          if (antenna_write(filename, MAX_FILENAME_LEN) == -1) {
            printf("[!] Failed to pass directory\n");
            continue;
          }

          // Listen for incoming file
          if (antenna_fread(FILE_LS_INDEX) == -1) {
            printf("[!] Failed to read incoming file\n");
            continue;
          }

          // Display file contents
          ls_fp = fopen(FILE_LS_INDEX, "r");
          if (!ls_fp) {
            printf("[!] Failed to open ls index for reading.\n");
            continue;
          }
          char *nextline = NULL;
          int n = 64;
          while (getline(&nextline, &n, ls_fp) != -1) {
            printf("%s", nextline);
          }
          fclose(ls_fp);
          free(nextline);

        } else if (strncmp(req, REQ_TAKE_PICTURE, 2) == 0) {
          // Send request
          if (antenna_write(REQ_TAKE_PICTURE, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }
        } else if (strncmp(req, REQ_ENCODE_FILE, 2) == 0) {
          // Get desired filename
          printf(
              "[?] Enter the filename or path you would like to encode "
              "(remote): ");
          scanf(" %s", filename);

          // Send request
          if (antenna_write(REQ_ENCODE_FILE, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }

          // Send filename
          if (antenna_write(filename, MAX_FILENAME_LEN) == -1) {
            printf("[!] Failed to forward filename\n");
            continue;
          }
        } else if (strncmp(req, REQ_DECODE_FILE, 2) == 0) {
          // Get desired filename
          printf(
              "[?] Enter the filename or path you would like to decode "
              "(remote): ");
          scanf(" %s", filename);

          // Send request
          if (antenna_write(REQ_DECODE_FILE, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }

          // Send filename
          if (antenna_write(filename, MAX_FILENAME_LEN) == -1) {
            printf("[!] Failed to forward filename\n");
            continue;
          }
        } else if (strncmp(req, REQ_SHUTDOWN, 2) == 0) {
          // Send request
          if (antenna_write(REQ_SHUTDOWN, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          } else {
            printf("[i] Request sent!\n");
          }
        } else if (strncmp(req, REQ_REBOOT, 2) == 0) {
          // Send request
          if (antenna_write(REQ_REBOOT, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          } else {
            printf("[i] Request sent!\n");
          }
        } else if (strncmp(req, REQ_REMOVE, 2) == 0) {
          // Get desired filename
          printf(
              "[?] Enter the filename or path you would like to remove "
              "(remote): ");
          scanf(" %s", filename);

          // Warning
          printf("!!!!!!!!!!! ARE YOU SURE? !!!!!!!!!!!!\n");
          printf(
              "It would be much better to move stuff (cant guarantee the "
              "absolutely correct filename will be passed).\n");
          printf("(y/n): ");
          char iamsure;
          scanf(" %c", &iamsure);
          if (iamsure == 'y') {
            // Send request
            if (antenna_write(REQ_REMOVE, 2) == -1) {
              printf("[!] Failed to make request\n");
              continue;
            }

            // Send filename
            if (antenna_write(filename, MAX_FILENAME_LEN) == -1) {
              printf("[!] Failed to forward filename\n");
              continue;
            }
          } else {
            continue;
          }
        } else if (strncmp(req, REQ_MOVE, 2) == 0) {
          // Get desired filename
          printf(
              "[?] Enter the filename source "
              "(remote): ");
          scanf(" %s", filename);
          printf(
              "[?] Enter the filename destination "
              "(remote): ");
          scanf(" %s", filename_dest);

          // Send request
          if (antenna_write(REQ_MOVE, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }

          // Send filenames
          if (antenna_write(filename, MAX_FILENAME_LEN) == -1) {
            printf("[!] Failed to forward filename\n");
            continue;
          }
          if (antenna_write(filename_dest, MAX_FILENAME_LEN) == -1) {
            printf("[!] Failed to forward filename\n");
            continue;
          }
        } else if (strncmp(req, REQ_BURNWIRE, 2) == 0) {
          // Send request
          if (antenna_write(REQ_BURNWIRE, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }

          printf("[i] Burnwire request successfully sent!\n\n");
        } else if (strncmp(req, REQ_ENABLE_ACS, 2) == 0) {
          // Send request
          if (antenna_write(REQ_ENABLE_ACS, 2) == -1) {
            printf("[!] Failed to make request\n");
            continue;
          }

          printf("[i] ACS enable request successfully sent!\n\n");

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
          } else if (strncmp(userreq, REQ_RESET_COMMS, 2) == 0) {
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

      case 'e':
      case 'E':
        // Get desired filename
        printf("[?] Enter the filename or path you would like to encode: ");
        scanf(" %s", filename);

        // Encode file
        if (antenna_encode_file(filename) == -1) {
          printf("[!] Failed to encode file at %s\n", filename);
          return -1;
        } else {
          printf("[i] Successfully encoded file!\n");
        }
        break;
      case 'd':
      case 'D':
        // Get desired filename
        printf("[?] Enter the filename or path you would like to decode: ");
        scanf(" %s", filename);

        // Encode file
        if (antenna_decode_file(filename) == -1) {
          printf("[!] Failed to decode file at %s\n", filename);
          return -1;
        } else {
          printf("[i] Successfully decoded file!\n");
        }
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