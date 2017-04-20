#include "idle-rq.h"
#include "trouble-maker.h"

ssize_t mysend(int sockfile, const void *buf, size_t len, int flags) {
  char *tmp = (char*)buf;
  while (*tmp) {
    *tmp = corrupt(*tmp);
    tmp++;
  }
  return send(sockfile, buf, len, flags);
}
