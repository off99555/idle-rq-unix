#include "idle-rq.h"
#include "trouble-maker.h"

int N = 0; // current sequence number

ssize_t mysend(int sockfile, const void *buf, size_t len, int flags) {
  char *tmp = (char*)buf;
  // make frames
  // for each frame, send to secondary, set a timer to resend I(N)
  // and wait for secondary to
  // send a frame back, test for ACK/NAK, if it's ACK(N) and not corrupted,
  // send next frame(N+1) else send current frame again
  while (*tmp) {
    *tmp = corrupt(*tmp);
    tmp++;
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
