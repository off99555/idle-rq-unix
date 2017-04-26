#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "idle-rq.h"
#include "trouble-maker.h"

int N = 0; // current sequence number
const int PARITY_BIT = 15;
const int SEQ_BIT = 14;
const int LAST_INDICATOR_BIT = 13;
const int ACK_BIT = 12;

void joinframes(short *frames, char *buf, int len);
short* makeframes(char *buf, size_t len);
int parity(short frame);
int corrupted(short frame);
void printstat(short frame);

ssize_t mysend(int sockfile, const void *buf, size_t len, int flags) {
  char *tmp = (char*)buf;
  printf("Buf (%zu):\n", len);
  while (*tmp) {
    printbytebits(*tmp);
    tmp++;
  }
  // make frames
  short *frames = makeframes((char*)buf, len);
  // for each frame, send to secondary, set a timer to resend I(N)
  // and wait for secondary to
  // send a frame back, test for ACK/NAK, if it's ACK(N) and not corrupted,
  // send next frame(N+1) else send current frame again
  size_t n = len;
  printf("Frames (%zu):\n", n);
  int i;
  for (i = 0; i < n; i++) {
    printf("Sending I-frame %d: ", i);
    printbits(frames[i]);
    printstat(frames[i]);
    mightsend(sockfile, frames[i]);
    // TODO: set a timer
    // WTACK state: wait for Secondary to respond
    short ack; // we prefer ACK_BIT bit to mean ACK, if it's 1 or NAK if 0
    ssize_t status = recv(sockfile, &ack, 2, 0); // receiving the ACK frame
    if (status == 0) {
      // the Secondary has closed the socket
      printf("Secondary has closed connection, indicating proper transmission. ACK frame not needed. Primary process is terminating.\n");
      break;
    }
    int isack = testbit(ack, ACK_BIT);
    int corrup = corrupted(ack);
    printf("Receiving %s frame: ", corrup ? "a corrupted" : isack ? "ACK" : "NAK");
    printbits(ack);
    printstat(ack);
    int wanted = N == testbit(ack, SEQ_BIT);
    if (!corrup && !wanted) { // show msg only when the frame is not corrupted
      printf("Its order is %s\n", wanted ? "valid" : "NOT valid");
      printf("Expected N=%d, got %d\n", N, testbit(ack, SEQ_BIT));
    }
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
  short frames[len+1], frame;
  short ack = 0;
  int i = 0;
  while (1) {
    ssize_t status = recv(sockfile, &frame, 2, 0);
    if (status == 0) {
      fprintf(stderr, "Primary has closed connection, unexpected behavior!\n");
    }
    int corrup = corrupted(frame);
    int last = testbit(frame, LAST_INDICATOR_BIT);
    int wanted = N == testbit(frame, SEQ_BIT);
    if (!corrup && !wanted) {
      i--;
    }
    printf("Receiving %sI-frame %d: ", corrup ? "a corrupted " : last ? "the last " : "", i);
    printbits(frame);
    printstat(frame);
    if (!corrup && !wanted) {
      fprintf(stderr, "The I-frame order is invalid. Duplicate detected.\n");
      printf("Expected N=%d, got %d\n", N, testbit(frame, SEQ_BIT));
    }
    setbit(&ack, ACK_BIT, !corrup);

    // add seqNo bit
    if (wanted && N || (!wanted && testbit(frame, SEQ_BIT))) { // if wanting odd seqNo
      setbit(&ack, SEQ_BIT, 1);
    }

    // add parity bit
    setbit(&ack, PARITY_BIT, parity(ack));
    printf("Sending %s frame: ", testbit(ack, ACK_BIT) ? "ACK" : "NAK");
    printbits(ack);
    printstat(ack);
    mightsend(sockfile, ack);

    if (testbit(ack, ACK_BIT)) {
      N = !N;
      frames[i] = frame;
      i++;
    }
    // check for last frame
    if (last) {
      /* printf("This is the last I-frame.\n"); */
      //TODO: try to not send the last ACK frame back and let Primary notice
      // that you got the last I-frame by closing the socket
      // Primary should check if the socket is closed that mean you are done
      // and it should stop resending the last I-frame to you and stop waiting
      // for the ack from you
      break;
    }
  }

  // join packets from frames together into *buf
  printf("Joining frames ...\n");
  joinframes(frames, buf, i);
  return strlen(buf);
}

// join frames into buffer data
void joinframes(short *frames, char* buf, int len) {
  size_t i, j;
  for (i = 0; i < len; i++) { // for each buffer
    buf[i] = 0;
    for (j = 0; j < 8; j++) { // for each bit
      if (testbit(frames[i], j)) {
        buf[i] |= 1 << j;
      }
    }
  }
  buf[i] = 0;
}

// split data into packets then make frames containing them
// 4th bit is nothing, 5th bit is last frame, 6th bit is seqNo, 7th bit is parity
short *makeframes(char *buf, size_t len) {
  short *frames = (short*) malloc(len+1);
  int fNo = 0; // current frame number that we are filling bits into
  int done = 0;
  while (!done) {
    frames[fNo] = 0;
    int i;
    for (i = 0; i < 8; i++) {
      if (buf[fNo] & (1 << i)) {
        frames[fNo] |= 1 << i;
      }
      if (buf[fNo] == 0 || fNo >= len) { // if run out of buf
        setbit(frames+fNo, LAST_INDICATOR_BIT, 1); // last frame indicator
        done = 1;
        break; // go add header and leave unused bits blank
      }
    }

    // add seqNo bit
    if (fNo % 2) { // if current frame sequence number is odd
      setbit(frames+fNo, SEQ_BIT, 1); // turn on the seqNo bit
    }

    // add parity bit
    if (parity(frames[fNo])) {
      setbit(frames+fNo, PARITY_BIT, 1);
    }
    fNo++;
  }
  return frames;
}

// return parity function of 0th to 7th bit
// currently, the parity function is just XOR
int parity(short frame) {
  int result = 0, i;
  for (i = 0; i < PARITY_BIT; i++) {
    result ^= (frame >> i) & 1;
  }
  return result;
}

int corrupted(short frame) {
  return parity(frame) != ((frame >> PARITY_BIT) & 1);
}

void printstat(short frame) {
  printf("ACK: %d\n", testbit(frame, ACK_BIT));
  printf("Last: %d\n", testbit(frame, LAST_INDICATOR_BIT));
  printf("SEQ: %d\n", testbit(frame, SEQ_BIT));
  printf("Parity: %d\n", testbit(frame, PARITY_BIT));
}
