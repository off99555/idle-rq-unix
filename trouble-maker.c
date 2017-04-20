#include "trouble-maker.h"
#include "stdio.h"
#include "stdbool.h"

/* void corrupt(char) */
char corrupt(char frame) {
  /* printbits(frame); */
  return frame ^ 1;
}

void printbits(char frame) {
  int i;
  for (i = 0; i < 8; i++) {
    bool on = frame & (1 << i);
    printf("%d", on);
  }
}
