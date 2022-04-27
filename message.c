#include "message.h"
#include <string.h>
#include "connection.h"

#define MAX_PAYLOAD 4096


// Note: kind of arbituary choice here, but must ensure it call store everything
#define START 135

void message_send(int sockfd, Message* msg) {
    int payloadLen = message_findSerializedLength(msg);
    uint8_t* payload = (uint8_t*)malloc(sizeof(uint8_t) * payloadLen); // Note: must free() later
    int serializedLen = message_serialize(payload, msg);
    if (serializedLen != payloadLen) {
        printf("SerializedPacket(): incorrect length serialized\n"
                "serialized length: %d   payloadLen  %d\n", serializedLen, payloadLen);
        exit(1);
    }
    // Not doing sendAll since receiving side would be difficult to manage
    //int len = payloadLen;
    /*if (tcp_sendAll(sockfd, payload, &len) == -1) {
        perror("tcp_sendAll(): ");
        printf("We only sent %d bytes because of the error!\n", len);
    }*/
    int bytesSent = send(sockfd, payload, payloadLen, 0);
    if (bytesSent == -1) {
        perror("send(): ");
        exit(1);
    }
    if (bytesSent != payloadLen) printf("send(): incorrect length sent");
    free(payload);
}


uint32_t message_findSerializedLength(Message* msg) {
    uint32_t headerBytes = 1 + 4; //START + msg Length
    uint32_t numOfSeperators = 5; //number of colons used to seperate each field
    return headerBytes + numOfSeperators + sizeof(unsigned int)*2 + strlen(msg->source)+1 + msg->size;
    // Note we use strlen()+1 for source since we shouldn't transmit MAX_NAME when unnecessary
    // We only transmit the size of the content of the string (including null)
}


MsgBuf* msgBuf_init() {
    MsgBuf* mBuf = (MsgBuf*)malloc(sizeof(MsgBuf));
    mBuf->len = 4096;
    mBuf->in = 0;
    mBuf->out = 0;
    return mBuf;
}

MsgBuf* msgBuf_destroy(MsgBuf* mBuf) {
    free(mBuf);
    return NULL;
}

static int mBufEmpty(MsgBuf* m) {
    return (m->in == m->out);
}

static int mBufFull(MsgBuf* m) {
    return ((m->in - m->out + m->len) % (m->len) == m->len - 1);
}


// Returns 1 if successfully received message
int message_receive(int sockfd, Message* msg, MsgBuf* mBuf) {
    // 1. Try to read from DataBuf
    // Loop until START is found
    uint8_t rawBuf[MAX_PAYLOAD];
    int nbytes = 0;
    int startFound = 0;
    do {
        while (!mBufEmpty(mBuf)) {
            int s = (int)mBuf->buf[mBuf->out];
            if (s == START) {
                startFound = 1;
                break; // Note we dont update out. So, out will index start
            }
            mBuf->out = (mBuf->out + 1) % mBuf->len;
        }   
        if (!startFound) {
            nbytes = recv(sockfd, rawBuf, MAX_PAYLOAD, 0);
            if (nbytes == -1) {
                perror("recv(): ");
                exit(1);
            }
            if (nbytes == 0) {
                // Remote side has closed the connection
                return 0; // Let caller handle this
            }
            for (int i = 0; i < nbytes; i++) {
                if (mBufFull(mBuf)) break;
                // put rawBuf into mBuf
                mBuf->buf[mBuf->in] = rawBuf[i];
                mBuf->in = (mBuf->in + 1) % mBuf->len;
            }
        }
    } while (!startFound);
         
    // while (space after START + ':' is less than 4 bytes) -> meaning less than 6 in buf
        // call recv and append recvBuf into buf
    while ((mBuf->in - mBuf->out + mBuf->len) % mBuf->len < 6) {  //the + 1 includes ':'
        nbytes = recv(sockfd, rawBuf, MAX_PAYLOAD, 0);
        if (nbytes == -1) {
            perror("recv(): ");
            exit(1);
        }
        if (nbytes == 0) {
            // Remote side has closed the connection
            mBuf->out = (mBuf->out + 1) % mBuf->len; //So out is no longer START
            return 0; // Let caller handle this
        }
        for (int i = 0; i < nbytes; i++) {
            // put rawBuf into mBuf
            mBuf->buf[mBuf->in] = rawBuf[i];
            mBuf->in = (mBuf->in + 1) % mBuf->len;
        }
    }
    // Get length of msg from position 0 to 3 from out+2
    uint32_t expectedLen = ntohl(*((uint32_t*)&mBuf->buf[mBuf->out + 2]));
    // while space after msgLength: is less than msgLength-(5bytes+2bytes)
        // call recv and append recvBuf into buf 
    while ((mBuf->in - mBuf->out + mBuf->len) % mBuf->len < expectedLen) {
        nbytes = recv(sockfd, rawBuf, MAX_PAYLOAD, 0);
        if (nbytes == -1) {
            perror("recv(): ");
            exit(1);
        }
        if (nbytes == 0) {
            // Remote side has closed the connection
            mBuf->out = (mBuf->out + 1) % mBuf->len; //So out is no longer START
            return 0; // Let caller handle this
        }
        for (int i = 0; i < nbytes; i++) {
            // put rawBuf into mBuf
            mBuf->buf[mBuf->in] = rawBuf[i];
            mBuf->in = (mBuf->in + 1) % mBuf->len;
        }
    }
    // Now our buf should be large enough to be deserialized into a Message object 
    // call Message deserialization on the dataBuf->buf
    message_deserialize(&(mBuf->buf[mBuf->out]), msg);
        

    // Remove serialized parts from the buf
    for (int i = 0; i < expectedLen; i++) {
        mBuf->out = (mBuf->out + 1) % mBuf->len;
    }
    // Return message object (already returned since msg passed as pointer)

    return 1; // success
}

// Convert struct message into payload[]
// Returns the number of bytes that has been serialized
// payload = START:SERIALIZED_LENGTH:TYPE:DATASIZE:SOURCE:DATA
int message_serialize(uint8_t payload[], Message* msg) {
    int pos = 0;    
    payload[pos] = START;
    pos += sizeof(uint8_t);
    payload[pos++] = ':';   
    
     
    *((uint32_t*)&payload[pos]) = htonl(message_findSerializedLength(msg));
    pos += sizeof(uint32_t);
    payload[pos++] = ':';    
    
    *((uint32_t*)&payload[pos]) = htonl(msg->type);
    pos += sizeof(uint32_t);
    payload[pos++] = ':';
    
    *((uint32_t*)&payload[pos]) = htonl(msg->size);
    pos += sizeof(uint32_t);
    payload[pos++] = ':';
    
    // Note: '\0' is transmitted
    for (int i = 0; msg->source[i] != '\0'; i++) {
        if (i == MAX_NAME) {
            printf("message_serialize error: source is not a string!");
            exit(1);
        }
        payload[pos++] = msg->source[i];
    }
    payload[pos++] = '\0';
    payload[pos++] = ':';
    
    for (int i = 0; i < msg->size; i++) {
        payload[pos++] = msg->data[i];
    }
    
    
    return pos;
}

// returns -1 if error/message dropped
// returns length deserialized otherwise
int message_deserialize(uint8_t payload[], Message* msg){
    /*for (int i = 0; i < 100; i++) {
        printf("%c", payload[i]);
    }*/

    uint32_t expectedLen = ntohl(*((uint32_t*)&payload[2]));


    int pos = 7; //since pos 0 to 1 stores START and ":" and 2 to 6 the length of payload and ":"
    msg->type = ntohl(*((uint32_t*)&payload[pos]));
    pos += sizeof(uint32_t);
    pos++; //to skip the ':'
    
    msg->size = ntohl(*((uint32_t*)&payload[pos]));
    pos += sizeof(uint32_t);
    pos++; //to skip the ':'
   
    
    for (int i = 0; payload[pos] != ':'; i++) {
        if (pos >= expectedLen) {
            puts("message_deserialize(): expected length exceeded, dropping message");
            return -1;
        }
        msg->source[i] = payload[pos++];
    }

    pos++; //to skip the ':'
    
    //printf("%u\n", msg->size);
    // Finally, load into filedata
    for (int i = 0; i < msg->size; i++) {
        if (pos >= expectedLen) {
            puts("message_deserialize(): expected length exceeded, dropping message");
            return -1;
        }
        msg->data[i] = payload[pos++];
        
    }
    return pos;
}