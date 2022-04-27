#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#define MAX_NAME 100
#define MAX_DATA 1024

enum messageType{
    LOGIN,
    LO_ACK,
    LO_NAK,
    EXIT,
    JOIN,
    JN_ACK,
    JN_NAK,
    LEAVE_SESS,
    NEW_SESS,
    NS_ACK,
    MESSAGE,
    QUERY,
    QU_ACK,
    PRIVATE,
    PRI_NAK,
    REGISTER,
    RE_ACK,
    RE_NCK
};

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];  //Treated as string
    unsigned char data[MAX_DATA];
};
typedef struct message Message;

typedef struct MsgBuf { 
    uint8_t buf[4096]; // Note: must be larger than MAX_PAYLOAD
    int len;   // due to circular buffer, full when len - 1
    int in;
    int out;

}MsgBuf;

MsgBuf* msgBuf_init();
MsgBuf* msgBuf_destroy(MsgBuf* mBuf);


void message_send(int sockfd, Message* msg);
//int message_receive(int sockfd, Message* msg);
int message_receive(int sockfd, Message* msg, MsgBuf* mBuf);

uint32_t message_findSerializedLength(Message* msg);
int message_serialize(uint8_t payload[], Message* msg);
int message_deserialize(uint8_t payload[], Message* msg);



#endif