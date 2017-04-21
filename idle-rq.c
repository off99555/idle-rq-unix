#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "idle-rq.h"
#include "trouble-maker.h"

#define BUFSIZE 10000

int N = 0; // current sequence number

char* makeframes(char *buf, size_t len);
int parity(char frame);
int corrupted(char frame);

ssize_t mysend(int sockfile, const void *buf, size_t len, int flags) {
  char *tmp = (char*)buf;
  printf("Buf (%zu):\n", len);
  while (*tmp) {
    printbits(*tmp);
    tmp++;
  }
  // make frames
  char *frames = makeframes((char*)buf, len);
  // for each frame, send to secondary, set a timer to resend I(N)
  // and wait for secondary to
  // send a frame back, test for ACK/NAK, if it's ACK(N) and not corrupted,
  // send next frame(N+1) else send current frame again
  size_t n = strlen(frames);
  printf("Frames (%zu):\n", n);
  int i;
  for (i = 0; i < n; i++) {
    printf("Sending I-frame %d: ", i);
    printbits(frames[i]);
    mightsend(sockfile, frames[i]);
    // TODO: set a timer
    // wait for S to respond
    char ack; // we prefer first bit to say ACK, if it's 1 or NAK if 0
    recv(sockfile, &ack, 1, 0);
    printf("Receiving ACK frame: ");
    printbits(ack);
    int isack = ack & 1;
    if (!isack)
      printf("It's %s\n", isack ? "ACK" : "NAK");
    int wanted = N == ((ack >> 6) & 1);
    if (!wanted) {
      printf("Its order is %s\n", wanted ? "valid" : "NOT valid");
      printf("Expected N=%d, got %d\n", N, ((ack >> 6) & 1));
    }
    int corrup = corrupted(ack);
    if (isack // if the 1st bit is ack
        && wanted // if seqNo is valid
        && !corrup) { // if isn't corrupted
      /* printf("Go send next frame.\n"); */
      N = !N;
    } else {
      printf("Resend this I-frame again.\n");
      i--;
    }
  }
  return len;
}

ssize_t myrecv(int sockfile, void *buf, size_t len, int flags) {
  // forever try to receive frames
  // for each frame, if corrupted or not proper order send NAK frame,
  // else send ACK and if last frame break
  char frames[BUFSIZE];
  char ack;
  int i = 0;
  while (1) {
    recv(sockfile, frames+i, 1, 0);
    printf("Receiving I-frame %d: ", i);
    printbits(frames[i]);
    int corrup = corrupted(frames[i]);
    if (corrup) {
      fprintf(stderr, "The I-frame is corrupted.\n");
    }
    int wanted = N == ((frames[i] >> 6) & 1);
    if (!wanted) {
      fprintf(stderr, "The I-frame order is invalid.\n");
      printf("Expected N=%d, got %d\n", N, ((frames[i] >> 6) & 1));
    }
    if (corrup || !wanted) {
      ack = 0;
    } else {
      ack = 1;
    }

    // add seqNo bit
    if (N) { // if wanting odd seqNo
      ack |= 1 << 6; // turn on the seqNo bit
    }

    // add parity bit
    if (parity(ack)) {
      ack |= 1 << 7;
    }
    printf("Sending ACK frame: ");
    printbits(ack);
    mightsend(sockfile, ack);
    if (ack & 1) {
      N = !N;
    }

    // check for last frame
    if ((ack >> 5) & 1) {
      break;
    }
    i++;
  }

  // join packets from frames together into *buf
  return len;
}

// split data into packets then make frames containing them
// 5th bit is last frame, 6th bit is seqNo, 7th bit is parity
char *makeframes(char *buf, size_t len) {
  char *frames = (char*) malloc(len*2); // len*2 is rough size approximation
  int fNo = 0; // current frame number that we are filling bits into
  int bufindex = 0;
  int bufbit = 0; // current bit that we are dealing with
  int done = 0;
  while (!done) {
    frames[fNo] = 0;
    int i;
    for (i = 0; i < 5; i++) {
      if (buf[bufindex] & (1 << bufbit)) {
        frames[fNo] |= 1 << i;
      }
      bufbit++;
      if (bufbit >= 8) {
        bufbit = 0;
        bufindex++;
        if (buf[bufindex] == 0 || bufindex >= len) { // if run out of buf
          frames[fNo] |= 1 << 5; // last frame indicator
          done = 1;
          break; // go add header and leave unused bits blank
        }
      }
    }

    // add seqNo bit
    if (fNo % 2) { // if current frame sequence number is odd
      frames[fNo] |= 1 << 6; // turn on the seqNo bit
    }

    // add parity bit
    if (parity(frames[fNo])) {
      frames[fNo] |= 1 << 7;
    }
    fNo++;
  }
  return frames;
}

// return parity function of 0th to 7th bit
// currently, the parity function is just XOR
int parity(char frame) {
  int result = 0, i;
  for (i = 0; i < 7; i++) {
    result ^= (frame >> i) & 1;
  }
  return result;
}

int corrupted(char frame) {
  return parity(frame) != ((frame >> 7) & 1);
}
