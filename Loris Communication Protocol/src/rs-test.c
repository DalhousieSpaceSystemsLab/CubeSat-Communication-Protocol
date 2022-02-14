// Reed-solomon library
#include "correct.h"

// Standard C libraries
#include <stdio.h>
#include <string.h>

int main() {
  // Create encoder/decoder
  correct_reed_solomon * coder = correct_reed_solomon_create(correct_rs_primitive_polynomial_8_4_3_2_0, 1, 1, 2);

  if(coder == NULL) {
    printf("[!] Failed to create encoder/decoder\n");
    return -1;
  }

  // Encode a message
  unsigned char msg[] = "hey this is encoded";
  unsigned char decoded_msg[255];
  unsigned char encoded[255];

  printf("[i] Message length = %d\n", strlen(msg));
  printf("[i] Message to encode is: ");
  for(int x = 0; x < strlen(msg); x++) {
    printf("%.2x ", msg[x]);
  }
  printf("\n");

  int encoded_len = correct_reed_solomon_encode(coder, msg, strlen(msg), encoded);

  printf("[i] Encoded length = %d\n", encoded_len);
  printf("[i] Encoded message is: ");
  for(int x = 0; x < encoded_len; x++) {
    printf("%.2x ", encoded[x]);
  }
  printf("\n");

  int decoded_msg_len = correct_reed_solomon_decode(coder, encoded, encoded_len, decoded_msg);
  if(decoded_msg_len < 0) {
    printf("[!] Failed to decode message\n");
    return -1;
  }
  printf("\n");

  printf("[i] Decoded message length = %d\n", decoded_msg_len);
  printf("[i] Decoded message: ");
  // for(int x = 0; x < decoded_msg_len; x++) {
    for(int x = 0; x < strlen(msg); x++) {
    printf("%c", decoded_msg[x]);
  }
  printf("\n");


  return 0;
}