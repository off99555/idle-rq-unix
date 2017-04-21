#include <sys/types.h>
#include <sys/socket.h>
#include "trouble-maker.h"
#include "stdio.h"
#include "stdbool.h"

char corrupt(char frame) {
  /* printbits(frame); */
  return frame ^ 1;
}

void mightsend(int sockfile, char frame) {
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
