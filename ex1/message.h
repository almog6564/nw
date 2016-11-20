#ifndef MESSAGE_H_
#define MESSAGE_H_
#include "nw.h"
#include <string.h>

int getMessage(SOCKET s, MSG* message);

int sendMessage(SOCKET s, MSG* message);






#endif