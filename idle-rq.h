#include <sys/types.h>
#include <sys/socket.h>

ssize_t mysend(int sockfile, const void *buf, size_t len, int flags);
