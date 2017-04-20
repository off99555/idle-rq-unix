#include <stdlib.h>
#include <stdio.h>
#include "idle-rq.h"
#include "trouble-maker.h"

int N = 0; // current sequence number

char* makeframes(char *buf, size_t len);

ssize_t mysend(int sockfile, const void *buf, size_t len, int flags) {
  char *tmp = (char*)buf;
  // make frames
  // for each frame, send to secondary, set a timer to resend I(N)
  // and wait for secondary to
  // send a frame back, test for ACK/NAK, if it's ACK(N) and not corrupted,
  // send next frame(N+1) else send current frame again
  printf("Buf:\n");
  while (*tmp) {
    /* *tmp = corrupt(*tmp); */
    printbits(*tmp);
    tmp++;
  }
  char *frames = makeframes((char*)buf, len);
  printf("Frames:\n");
  while (*frames) {
    printbits(*frames);
    frames++;
  }
  return send(sockfile, buf, len, flags);
}

ssize_t myrecv(int sockfile, void *buf, size_t len, int flags) {
  // forever try to receive frames
  // for each frame, if null break
  // else if corrupted send NAK frame,
  // else send ACK

  // join packets from frames together into *buf
  return recv(sockfile, buf, len, flags);
}

// split data into packets then make frames containing them
// 6th bit is seqNo, 7th bit is parity
char *makeframes(char *buf, size_t len) {
  char *frames = (char*) malloc(len*2); // len*2 is rough size approximation
  int fNo = 0; // current frame number that we are filling bits into
  int bufindex = 0;
  int bufbit = 0; // current bit that we are dealing with
  while (1) {
    frames[fNo] = 0;
    if (buf[bufindex] == 0 || bufindex >= len) { // if run out of buf
      return frames;
    }
    int i;
    for (i = 0; i < 6; i++) {
      if (buf[bufindex] & (1 << bufbit)) {
        frames[fNo] |= 1 << i;
      }
      bufbit++;
      if (bufbit >= 8) {
        bufbit = 0;
        bufindex++;
        if (buf[bufindex] == 0 || bufindex >= len) { // if run out of buf
          break; // go add header and leave unused bits blank
        }
      }
    }
    // add header bits
    fNo++;
  }
  return frames;
}
