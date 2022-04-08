// Project headers
#include "antenna.h"

// Standard C libraries
#include <pthread.h>
#include <stdio.h>
#include <string.h>

// Requests
#define REQ_BASIC_TELEMETRY "A1"
#define REQ_LARGE_TELEMETRY "B2"
#define REQ_DELET_TELEMETRY "C3"
#define REQ_REBOOT_OBC "D4"
#define REQ_RESET_COMMS "E5"
#define REQ_ENABLE_RAVEN "F6"

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
    char txt_file_data[MAX_TXT_FILE_SIZE];
    char *dynamic_file_data = NULL;
    size_t bytes_to_read;

    pthread_t monitor_requests_thread;

    printf(
        "[?] Read or write (r/w) or encoded (R/W) or receive text file (f) or "
        "send text file (F) or autonomous mode (X)");
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
      // messy. move contents to a function.
      case 'f':
        file_pointer = fopen("output.txt", "w");
        if (file_pointer == NULL) {
          printf("[!] failed to open file \"output.txt\"\n");
          return -1;
        }
        printf("[?] How many bytes would you like to read: ");
        bytes_to_read = 0;
        scanf(" %lu", &bytes_to_read);
        // if (bytes_to_read > MAX_TXT_FILE_SIZE)
        //   bytes_to_read = MAX_TXT_FILE_SIZE;
        dynamic_file_data = malloc(bytes_to_read);
        printf("[!] Reading text file...\n");
        if ((data_len = antenna_read_rs(dynamic_file_data, bytes_to_read,
                                        READ_MODE_UNTIL)) < 0) {
          printf("[!] failed to antenna read upto %d bytes\n", choice_len);
          return -1;
        }
        // fputs(txt_file_data, file_pointer);
        fwrite(
            dynamic_file_data, sizeof(char), data_len,
            file_pointer);  // NOTE: using fwrite for this to work with bitmaps
        fclose(file_pointer);
        printf("\n%s\n", dynamic_file_data);
        printf("[!] text file contents written to \"output.txt\"\n");
        skip_data_dump = 1;
        free(dynamic_file_data);
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
      case 'F':
        // fileName must be <20 characters in length.
        printf("[?] Enter text file to send unencoded: ");
        // hardcoding a designated filename would make repeated testing faster.
        scanf("%s", file_name);
        file_pointer = fopen(file_name, "r");
        if (file_pointer == NULL) {
          printf("[!] failed to open file \"%s\"\n", file_name);
          return -1;
        }
        // while (fgets(data, MAX_READ_LEN, file_pointer)) {
        //   strcat(txt_file_data, data);
        // }
        // data_len = strlen(txt_file_data);
        printf("[?] How many bytes would you like to read: ");
        bytes_to_read = 0;
        scanf(" %lu", &bytes_to_read);
        // if (bytes_to_read > MAX_TXT_FILE_SIZE)
        //   bytes_to_read = MAX_TXT_FILE_SIZE;
        dynamic_file_data = malloc(bytes_to_read);
        data_len =
            fread(dynamic_file_data, sizeof(char), bytes_to_read,
                  file_pointer);  // NOTE: using fread for bitmaps to work
        fclose(file_pointer);
        if (antenna_write_rs(dynamic_file_data, data_len) < 0) {
          printf(
              "[!] failed to antenna write text file \"%s\" containing %d "
              "bytes\n",
              file_name, data_len);
          return -1;
        }
        skip_data_dump = 1;
        free(dynamic_file_data);
        break;

      case 'x':
      case 'X':
        printf("[i] Entering autonomous mode...\n");
        pthread_create(&monitor_requests_thread, NULL, monitor_requests, NULL);
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

void *monitor_requests(void *data) {
start:
  // Listen for incoming requests
  char req[2];
  if (antenna_read(req, 2, READ_MODE_UNTIL) == -1) {
    printf("[!] Failed to read request from antenna\n");
    goto start;
  }

  // Identify request
  if (strcmp(req, REQ_BASIC_TELEMETRY) == 0) {
    } else if (strcmp(req, REQ_LARGE_TELEMETRY) == 0) {
  } else if (strcmp(req, REQ_DELET_TELEMETRY) == 0) {
  } else if (strcmp(req, REQ_REBOOT_OBC) == 0) {
  } else if (strcmp(req, REQ_RESET_COMMS) == 0) {
  } else if (strcmp(req, REQ_ENABLE_RAVEN) == 0) {
  }
  goto start;
}