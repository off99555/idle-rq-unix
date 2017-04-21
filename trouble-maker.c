#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include "trouble-maker.h"
#include "stdio.h"
#include "stdbool.h"

char corrupt(char frame) {
  /* printbits(frame); */
  int bit = rand_lim(7);
  return frame ^ (1 << bit);
}

void mightsend(int sockfile, char frame) {
  int random = rand_lim(100);
  if (random < 30) { // chance to corrupt
    frame = corrupt(frame);
  }
  send(sockfile, &frame, 1, 0);
}

void printbits(char frame) {
  int i;
  for (i = 0; i < 8; i++) {
    bool on = (frame >> i) & 1;
    printf("%d", on);
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
