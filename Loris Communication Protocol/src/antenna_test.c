// Project headers
#include "antenna.h"

// Standard C libraries
#include <stdio.h>
#include <string.h>

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

    printf(
        "[?] Read or write (r/w) or encoded (R/W) or receive text file (f) or "
        "send text file (F)");
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
        int bytes_to_read = 0;
        scanf(" %d", &bytes_to_read);
        if (bytes_to_read > MAX_TXT_FILE_SIZE)
          bytes_to_read = MAX_TXT_FILE_SIZE;
        printf("[!] Reading text file...\n");
        if ((data_len = antenna_read_rs(txt_file_data, bytes_to_read,
                                        READ_MODE_UNTIL)) < 0) {
          printf("[!] failed to antenna read upto %d bytes\n", choice_len);
          return -1;
        }
        // fputs(txt_file_data, file_pointer);
        fwrite(txt_file_data, sizeof(char), data_len, file_pointer); // NOTE: using fwrite for this to work with bitmaps
        printf("\n%s\n", txt_file_data);
        printf("[!] text file contents written to \"output.txt\"\n");
        skip_data_dump = 1;
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
        data_len = fread(data, sizeof(char), MAX_TXT_FILE_SIZE, file_pointer); // NOTE: using fread for bitmaps to work
        if (antenna_write_rs(txt_file_data, data_len) < 0) {
          printf(
              "[!] failed to antenna write text file \"%s\" containing %d "
              "bytes\n",
              file_name, data_len);
          return -1;
        }
        skip_data_dump = 1;
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