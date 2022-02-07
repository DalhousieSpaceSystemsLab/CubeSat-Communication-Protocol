// Reed-solomon library
#include "correct.h"

// Standard C libraries
#include <stdio.h>
#include <string.h>

int main() {
  // Create encoder/decoder
  correct_reed_solomon * coder = correct_reed_solomon_create(correct_rs_primitive_polynomial_8_4_3_2_0, 1, 1, 16);

  if(coder == NULL) {
    printf("[!] Failed to create encoder/decoder\n");
    return -1;
  }

  // Encode a message
  unsigned char msg[] = "hey this is encoded";
  unsigned char decoded_msg[255];
  unsigned char encoded[255];
  int encoded_len = correct_reed_solomon_encode(coder, msg, strlen(msg), encoded);

  correct_reed_solomon_decode(coder, encoded, encoded_len, decoded_msg);

  printf("[i] Decoded message is: %s\n", decoded_msg);


  return 0;
}