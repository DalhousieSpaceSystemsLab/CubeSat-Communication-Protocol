/* Example use of Reed-Solomon library 
 *
 * (C) Universal Access Inc. 1996
 *
 * This same code demonstrates the use of the encodier and 
 * decoder/error-correction routines. 
 *
 * We are assuming we have at least four bytes of parity (NPAR >= 4).
 * 
 * This gives us the ability to correct up to two errors, or 
 * four erasures. 
 *
 * In general, with E errors, and K erasures, you will need
 * 2E + K bytes of parity to be able to correct the codeword
 * back to recover the original message data.
 *
 * You could say that each error 'consumes' two bytes of the parity,
 * whereas each erasure 'consumes' one byte.
 *
 * Thus, as demonstrated below, we can inject one error (location unknown)
 * and two erasures (with their locations specified) and the 
 * error-correction routine will be able to correct the codeword
 * back to the original message.
 * */
 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdlib.h>
#include "ecc.h"

//change data length!!
#define DATA_LEN 28

unsigned char msg[DATA_LEN];

                       
unsigned char codeword[256];
 
/* Some debugging routines to introduce errors or erasures
   into a codeword. 
   */

/* Introduce a byte error at LOC */
void
byte_err (int err, int loc, unsigned char *dst)
{
  printf("Adding Error at loc %d, data %#x\n", loc, dst[loc-1]);
  dst[loc-1] ^= err;
}

/* Pass in location of error (first byte position is
   labeled starting at 1, not 0), and the codeword.
*/
void
byte_erasure (int loc, unsigned char dst[], int cwsize, int erasures[]) 
{
  printf("Erasure at loc %d, data %#x\n", loc, dst[loc-1]);
  dst[loc-1] = 0;
}


void
print_word (int n, unsigned char *data) {
  int i;
  for (i=0; i < n; i++) {
    printf ("%02c ", data[i]);
  }
  printf("\n");
}

int main (int argc, char *argv[]){
	//unsigned char testmsg[DATA_LEN];
	printf("opening text.txt ...\n");
	
	FILE *file = fopen("text.txt", "r");
	
	if(file == NULL){
		printf("\nSpecified file path is NULL");
	}
	printf("read file\n");
	
	int bytesRead = fread(msg, sizeof(char), DATA_LEN, file);
	
	printf("successfully read form file.");
	printf("bytes read: %d\nThe Unencoded message is: %s\n", bytesRead, msg);
	
 
  int erasures[16];
  int nerasures = 0;

  /* Initialization the ECC library */
 
  initialize_ecc();
 
  /* ************** */
 
#define ML (sizeof (msg) + NPAR)



  /* Add one error and two erasures */


  /* We need to indicate the position of the erasures.  Eraseure
     positions are indexed (1 based) from the end of the message... */

  /* 
     erasures[nerasures++] = ML-17;
     erasures[nerasures++] = ML-19;
  */
 
  /* Now decode -- encoded codeword size must be passed */
  printf("unencoded codeword: %s\n", codeword);
  decode_data(codeword, ML);
  

	
  srand(time(NULL));   // Initialization, should only be called once.

  int fails = 0;
  //for (int i = 0; i < 10; i++) {
      //int j;
      // make random msg data
      
      //populates msg with random data. 
      /*
      for (j=0; j<sizeof(msg); j++) {
          msg[j] = rand() % 256;
      }
      */
      printf("msg =");
      print_word(28, msg);
      encode_data(msg, sizeof(msg), codeword);
      printf("encoded data:\n");
      print_word(ML, codeword);

      // add two random errors to codeword
      unsigned char r = rand() % 256; 
      int rloc = rand() % ML; 
      codeword[rloc] = r;

      unsigned char r2 = rand() % 256; 
      int rloc2 = rand() % ML; 
      codeword[rloc2] = r2;

      printf("encoded data with two errors: %d @ loc %d, %d @ loc %d\n", r, rloc, r2, rloc2);
      print_word(ML, codeword);

      /* Now decode -- encoded codeword size must be passed */
      decode_data(codeword, ML);

      { 
          int syndrome = check_syndrome ();
          printf("syndrome = %d\n",syndrome);
          /* check if syndrome is all zeros */
          if (syndrome == 0) {
              // no errs detected, codeword payload should match message  
              for (int k=0; k < sizeof(msg); k++) {
                  if (msg[k] != codeword[k]) {
                      printf("#### FAILURE TO DETECT ERROR @ %d: %d != %d\n", k, msg[k], codeword[k]);
                      fails++;
                  }
              }

          } else {
              printf("nonzero syndrome, attempting correcting errors\n");
              int result = 0;
              result =correct_errors_erasures (codeword, 
                                               ML,
                                               nerasures, 
                                               erasures);
              printf("correct_errors_erasures = %d\n", result);
              print_word(28, codeword);
              int k;
              for (k=0; k < sizeof(msg); k++) {
                  if (msg[k] != codeword[k]) {
                      printf("##### FAILURE TO CORRECT ERROR @ %d: %d != %d\n", k, msg[k], codeword[k]);
                      fails++;
                  }
              }
          }
      }
  //}

  if (fails == 0) {
      printf("\n\n All Tests Passed: No failures to correct codeword\n");
  } else {
      printf("### ERROR Algorithm failed to correct codeword %d times!!!\n", fails);
  }

  exit(0);
}

