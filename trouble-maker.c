#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include "trouble-maker.h"
#include "stdio.h"
#include "stdbool.h"

short corrupt(short frame) {
  /* printbits(frame); */
  int bit = rand_lim(16);
  return frame ^ (1 << bit);
}

void mightsend(int sockfile, short frame) {
  int random = rand_lim(100);
  if (random <= 50) { // chance to send and not get lost on the way
    random = rand_lim(100);
    if (random <= 30) { // chance to corrupt
      frame = corrupt(frame);
      printf("-- TROUBLE MADE: Corrupted Frame\n");
    }
    send(sockfile, &frame, 2, 0);
  } else {
    printf("-- TROUBLE MADE: Frame get lost\n");
  }
}

void printbytebits(char byte) {
  int i;
  for (i = 0; i < 8; i++) {
    bool on = testbit(byte, i);
    printf("%d", on);
  }
  printf("\n");
}

void printbits(short frame) {
  int i;
  for (i = 0; i < 16; i++) {
    bool on = testbit(frame, i);
    printf("%d", on);
    if (i == 7) printf(" ");
  }
  printf("\n");
}

// see http://stackoverflow.com/a/2999130/2593810 for more information
int rand_lim(int limit) {
  /* return a random number uniformly between 0 and limit inclusive.
  */
  int divisor = RAND_MAX/(limit+1);
  int retval;
  do { 
    retval = rand() / divisor;
  } while (retval > limit);
  return retval;
}

int testbit(short frame, int bitorder) {
  return (frame >> bitorder) & 1;
}

void setbit(short *frame, int bitorder, int value) {
  *frame ^= (-value ^ *frame) & (1 << bitorder);
}
