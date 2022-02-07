// Reed-solomon library
#include "correct.h"

// Standard C libraries
#include <stdio.h>

int main() {
  // Create encoder/decoder
  correct_reed_solomon * coder = correct_reed_solomon_create(correct_rs_primitive_polynomial_8_4_3_2_0, 1, 1, );
  if(coder == NULL) {
    printf("[!] Failed to create encoder/decoder\n");
    return -1;
  }

  return 0;
}