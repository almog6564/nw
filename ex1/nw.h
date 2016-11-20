#ifndef NW_H_
#define NW_H_


#define LENGTH_SIZE sizeof(short)

#define HOST    "localhost"
#define PORT    6423
#define MAXSIZE 65536
#define WELLCOME_MSG    "Welcome! I am simple-mail-server."
#define WELLCOME_SIZE   34

typedef int SOCKET;

typedef struct {
    short  length;
    char   msg[MAXSIZE];
} MSG;












#endif