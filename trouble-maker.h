
// this is a header file for the troublesome communication channel
// it randomly corrupts messages and sometime discards them all
// but it should not keep the message and delay the transmission
// by sending it after a new message, that would mimic network layer's
// premature timeout problem, but we want to mimic link layer.
// idle RQ does not work if premature timeout problem is found

// corrupt a frame, 5th bit is for last frame indicator,
// 6th bit is for seqNo, 7th bit is for parity
char corrupt(char frame);
void mightsend(int sockfile, char frame);
void printbits(char frame);
