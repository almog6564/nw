#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "nw.h"


int getBuffer(SOCKET s,void* buff,int len){
    
    //printf("got getBuffer\n");

    int recvSize;
    int tot = 0;
    int leftToRead = len;

    while(tot<leftToRead){
         
         recvSize = recv(s,(void*) ( (long) buff + tot ),leftToRead,0);
         //printf("got recv\n");
         switch(recvSize){
             case -1:
             printf("error on recv\n");
             return -1;

             case 0:
             printf("connection ended on recv\n");
             return 0;

             default:
             tot        += recvSize;
             leftToRead -= recvSize;
             break;

         }
    }
    
    return tot;
}


int getMessage(SOCKET s, MSG* message){

    int chk = getBuffer(s,&message->opcode,LENGTH_SIZE);
     if(chk <= 0)
    {
        printf("error getBuffer\n");
        return chk;
    }
        chk = getBuffer(s,&message->length,LENGTH_SIZE);
     if(chk <= 0)
    {
        printf("error getBuffer\n");
        return chk;
    }
   
    message->opcode = ntohs(message->opcode);
    message->length = ntohs(message->length);   //so it would be BIG endian
    //printf("message length = %d\n", message->length);

    //TODO check if message is gonna be too long

    chk = getBuffer(s,message->msg,message->length);

    return chk;

}


int sendMessage(SOCKET s, MSG* message){

    int sendSize   = 2*LENGTH_SIZE + message->length;
    int leftToSend = sendSize;

    int tot        = 0;
    int sent;

    MSG copy;
    copy.opcode =  htons(message->opcode);
    copy.length =  htons(message->length);
    memcpy(copy.msg,message->msg,message->length);

    message->length = htons(message->length);
    message->opcode = htons(message->opcode);

    while(tot<sendSize){
         
         sent = send(s,&copy + tot,leftToSend,0);

         switch(sent){
             case -1:
             printf("send error");
             return -1;

             case 0:
             printf("send - connection eneded");
             return 0;

             default:
             tot        += sent;
             leftToSend -= sent;
             break;

         }
    }
    return tot;
    
}

