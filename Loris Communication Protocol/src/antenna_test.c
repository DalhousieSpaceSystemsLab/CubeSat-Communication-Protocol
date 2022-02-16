// Project headers
#include "antenna.h"

// Standard C libraries
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Check argc
  if(argc != 2) {
    printf("[!] Invalid number of arguments. Try ./antenna_test PATH\n");
    return -1;
  }

  // Init
  if(antenna_init(argv[1]) < 0) {
    printf("[!] Failed to init antenna\n");
    return -1;
  }

  // Main loop
  int done = 0;
  while(!done) {
    char choice;
    char encoded;
    int choice_len = 0;
    char data[MAX_READ_LEN];
    int data_len = -1;
    printf("[?] Read or write (r/w) or encoded (R/W): ");
    scanf(" %c", &choice);

    switch(choice) {
      case 'r':
        printf("[i] Reading UPTO %d bytes\n", MAX_READ_LEN);
        if((data_len = antenna_read(data, MAX_READ_LEN, READ_MODE_UPTO)) < 0) {
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
        
        if(choice == 'u') {
          if(encoded == 'y') {
            printf("[i] Reading UNTIL %d bytes ENCODED...\n", choice_len);
            if((data_len = antenna_read_rs(data, choice_len, READ_MODE_UNTIL)) < 0) {
              printf("[!] failed to antenna read until %d bytes\n", choice_len);
              return -1;
            }
          } else {
            printf("[i] Reading UNTIL %d bytes...\n", choice_len);
            if((data_len = antenna_read(data, choice_len, READ_MODE_UNTIL)) < 0) {
              printf("[!] failed to antenna read until %d bytes\n", choice_len);
              return -1;
            }
          }
        } else {
          if(encoded == 'y') {
            printf("[i] Reading UPTO %d bytes ENCODED...\n", choice_len);
            if((data_len = antenna_read_rs(data, choice_len, READ_MODE_UPTO)) < 0) {
              printf("[!] failed to antenna read upto %d bytes\n", choice_len);
              return -1;
            }
          } else {
            printf("[i] Reading UPTO %d bytes...\n", choice_len);
            if((data_len = antenna_read(data, choice_len, READ_MODE_UPTO)) < 0) {
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
        if(antenna_write(data, data_len) < 0) {
          printf("[!] failed to antenna write \"%s\" containing %d bytes\n", data, data_len);
          return -1;
        }
        break;
      case 'W':
        printf("[?] Enter message to ENCODE & send: ");
        scanf(" %s", data);
        data_len = strlen(data);
        if(antenna_write_rs(data, data_len) < 0) {
          printf("[!] failed to antenna write \"%s\" containing %d bytes\n", data, data_len);
          return -1;
        }
        break;
      default:
        printf("[!] %c is not a valid option\n", choice);
        continue;
    }

    // Print results
    printf("[i] Normal string print (may cause undefined output): %s\n", data);
    printf("[i] Data size = %d\n", data_len);
    printf("-- DATA DUMP --\n");
    for(int x = 0; x < data_len; x++) {
      printf("%.2x=\'%c\' ", data[x], data[x]);
    }    
    printf("\n-- END DUMP --\n");
  }

  // done
  return 0;
}