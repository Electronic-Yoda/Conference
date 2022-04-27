#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include "string.h"
#include "pthread.h"
#include "sys/select.h"
#include "stdlib.h"

unsigned long hash(char *str, long tableLen)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % tableLen;
}

// Individual client node
enum CLIENTSTATUS {
    
    LOGGEDOUT,
    LOGGEDIN,
    INSESSION,
};
typedef struct client {
    int status;
    char* clientID;  
    char* password;
    int sockfd;
    char* sessionID;
}Client;

void client_free(Client* client){
    free(client->clientID);
    free(client->password);
    free(client);
}
Client* client_init(char* clientID) {
    if (clientID == NULL) {
        puts("client_init: clientID is null");
        return NULL;
    }
    Client* client = (Client*)malloc(sizeof(Client));
    client->clientID = (char*)malloc(strlen(clientID)+1);
    strncpy(client->clientID, clientID, strlen(clientID)+1);

    return client;
}

// Set sessionID to NULL if not in a session
Client* client_initAll(
        int status, 
        char* clientID, 
        char* password, 
        int sockfd,
        char* sessionID) {
    
    Client* client = (Client*)malloc(sizeof(Client));
    
    client->status = status;
    client->clientID = (char*)malloc(strlen(clientID)+1);
    strncpy(client->clientID, clientID, strlen(clientID)+1);
    int a = strlen(password);
    client->password = (char*)malloc(strlen(password)+1);
    strncpy(client->password, password, strlen(password)+1);
    
    client->sockfd = sockfd;
    
    if (sessionID != NULL) {
        client->sessionID = (char*)malloc(strlen(sessionID)+1);
        strncpy(client->sessionID, sessionID, strlen(sessionID)+1);
    } else {
        client->sessionID = NULL;
    }
    
}

// Set sessionID to NULL if not in a session
void client_setSessionID(Client* client, char* sessionID) {
    if (client->sessionID != NULL) free(client->sessionID);
    client->sessionID = NULL;
    if (sessionID != NULL) {
        client->sessionID = (char*)malloc(strlen(sessionID)+1);
        strncpy(client->sessionID, sessionID, strlen(sessionID)+1);
    } else {
        client->sessionID = NULL;
    }
}

typedef struct ClientHashNode {
    Client* client;
    struct ClientHashNode* next;
}ClientHashNode;

ClientHashNode* clientHashNode_init() {
    ClientHashNode* hashNode = (ClientHashNode*)malloc(sizeof(ClientHashNode));
    return hashNode;
    hashNode->client = NULL;
    hashNode->next = NULL;
    return hashNode;
}
/*
unsigned long hash(char *str, long tableLen)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % tableLen;
}*/

int client_hash_table_insert(ClientHashNode* node, ClientHashNode** hashTable, int tableLen) {
    if (node == NULL) return 0;
    unsigned long idx = hash(node->client->clientID, tableLen);
    if (idx < 0 || idx >= tableLen) {
        printf("Hash idx out of bounds\n");
        return -1;
    }
    node->next = hashTable[idx];
    hashTable[idx] = node;
    return 1;
}


// returns 1 if deleted, 0 if not found or error
int client_hash_table_delete_Node(char* name, ClientHashNode** hashTable, int tableLen) {
    unsigned long idx = hash(name, tableLen);
    if (idx < 0 || idx >= tableLen) {
        printf("Hash idx out of bounds\n");
        return 0;
    }
    //ClientHashNode* head = hashTable[idx];
    //printf("tableLen is: %ld\n", tableLen);
    //printf("hash index is: %ld\n", idx);

    if (hashTable[idx] == NULL) return 0;
    if (strcmp(hashTable[idx]->client->clientID, name) == 0) {
      if (hashTable[idx]->next == NULL) {
          client_free(hashTable[idx]->client);
          free(hashTable[idx]);
          hashTable[idx] = NULL;
          return 1;
      }
      else {
          //found is head but next is not NULL
          ClientHashNode* tmp = hashTable[idx];
          hashTable[idx] = hashTable[idx]->next;
          client_free(tmp->client);
          free(tmp);
          tmp = NULL;
          return 1;
      }
    }
    
    ClientHashNode* cur = hashTable[idx]->next;
    ClientHashNode* prev = hashTable[idx];
    for (; cur!= NULL; prev = cur, cur = cur->next) {
        if (strcmp(cur->client->clientID, name) == 0) {
            prev->next = cur->next;
            client_free(cur->client);
            free(cur);
            cur = NULL;
            return 1;
        }
    }
    return 0; // not found in linked list
}

// Returns null if not found, else returns the hashNode
ClientHashNode* client_hash_table_find(char* word, ClientHashNode** hashTable, int tableLen){
    unsigned long idx = hash(word, tableLen);
    if (idx < 0 || idx >= tableLen) {
        printf("Hash idx out of bounds\n");
    }
    ClientHashNode* tmp = hashTable[idx];
    //printf("tableLen is: %ld\n", tableLen);
    //printf("hash index is: %ld\n", idx);

    if (tmp == NULL) return tmp;
    //printf("finding word: %s\n", tmp->word);
    
    //client_hash_table_print(hashTable, tableLen);
    while (tmp != NULL && strcmp(tmp->client->clientID, word) != 0) {
        tmp = tmp->next;

    }
    return tmp;
}

void client_hash_table_print(ClientHashNode** hashTable, int tableLen) {
    printf("Start\n");
    for (long i = 0; i < tableLen; i++) {
        if (hashTable[i] == NULL) {
            printf("\t%li\t---\n", i);
        }
        else {
            printf("\t%li\t", i);
            ClientHashNode* tmp = hashTable[i];
            while (tmp != NULL ) {
                //if (tmp->file != NULL && tmp->file->data != NULL && tmp->file->data->file_name != NULL) 
                    
                printf("%s - ", tmp->client->clientID);
                //else 
                    //printf("Data Null??? - ");
                tmp = tmp->next;
            }
            //if (tmp != NULL) printf("%s - ", tmp->file->data->file_name);
            printf("\n");
        }
    }
    printf("End\n");
}

// returns current index of ary
int client_hash_table_get_full_str(ClientHashNode** hashTable, int tableLen, char* ary, int aryLen) {
    //printf("Start\n");
    int count = 0;
    strncpy(ary+count, "Connected clients:\n", aryLen-count);
    count += strlen("Connected clients:\n");
    for (long i = 0; i < tableLen; i++) {
        if (hashTable[i] == NULL) {
            //printf("\t%li\t---\n", i);
        }
        else {
            //printf("\t%li\t", i);
            ClientHashNode* tmp = hashTable[i];
            while (tmp != NULL ) {
                //if (tmp->file != NULL && tmp->file->data != NULL && tmp->file->data->file_name != NULL) 
                    
                //printf("%s - ", tmp->client->clientID);
                
                if (tmp->client->status != LOGGEDOUT) {
                    strncpy(ary+count, "\t", aryLen-count);
                    count += strlen("\t");
                    strncpy(ary+count, tmp->client->clientID, aryLen-count);
                    count += strlen(tmp->client->clientID);
                    strncpy(ary+count, "\n", aryLen-count);
                    count += strlen("\n");  //Note we dont include null terminator
                }
                tmp = tmp->next;
            }
            
        }
    }
    strncpy(ary+count, "\n", aryLen-count);
    count += strlen("\n"); 
    return count;
}

typedef struct clientTable{
     ClientHashNode** hashTable; //ptr to an array where each index is a ptr to ClientHashNode
     int tableLen;
     pthread_mutex_t tableLock;
}ClientTable;

ClientTable* clientTable_init(int maxClients) {
    ClientTable* cTable = (ClientTable*)malloc(sizeof(ClientTable));
    
    int tableLen = ((maxClients+1) * 2);
    cTable->tableLen = tableLen;
    cTable->hashTable = (ClientHashNode**)malloc(sizeof(ClientHashNode*) * tableLen);
    for (int i = 0; i < tableLen; i++) {
        cTable->hashTable[i] = NULL;
    }
    pthread_mutex_init(&cTable->tableLock, NULL);
    return cTable;
    
}
void clientTable_addEmptyClient(ClientTable* cTable, char *clientID) {
    Client* client = client_init(clientID);
    ClientHashNode* hashNode = clientHashNode_init();
    hashNode->client = client;
    
    //pthread_mutex_lock(&cTable->tableLock);
    client_hash_table_insert(hashNode, cTable->hashTable, cTable->tableLen);
    //pthread_mutex_unlock(&cTable->tableLock);
    
}

void clientTable_addClient(ClientTable* cTable, Client* client) {
    ClientHashNode* hashNode = clientHashNode_init();
    hashNode->client = client;
    
    //pthread_mutex_lock(&cTable->tableLock);
    client_hash_table_insert(hashNode, cTable->hashTable, cTable->tableLen);
}



void clientTable_removeClient(ClientTable* cTable, char *clientID) {
    
    //pthread_mutex_lock(&cTable->tableLock);
    client_hash_table_delete_Node(clientID, cTable->hashTable, cTable->tableLen);
    //pthread_mutex_unlock(&cTable->tableLock);
    
}

// Returns null if not found
Client* clientTable_getClient(ClientTable* cTable, char *clientID) {
    ClientHashNode* hashNode = client_hash_table_find(clientID,cTable->hashTable, cTable->tableLen);
    if (hashNode == NULL) return NULL;
    Client* client = hashNode->client;
    return client;
}

Client* clientTable_getClientByFd(ClientTable* cTable, int sockfd) {
    for (long i = 0; i < cTable->tableLen; i++) {
        if (cTable->hashTable[i] != NULL) {
            //puts("a");
            ClientHashNode* tmp = cTable->hashTable[i];
            while (tmp != NULL ) {
                //puts("b");
                if (tmp->client->sockfd == sockfd) {
                    //puts("c");
                    return tmp->client;
                }    
                
                tmp = tmp->next;
            }
            
        }
    }
    return NULL;
}



void clientTable_print(ClientTable* cTable){
    puts("ClientTable");
    //pthread_mutex_lock(&cTable->tableLock);
    client_hash_table_print(cTable->hashTable, cTable->tableLen);
    //pthread_mutex_unlock(&cTable->tableLock);
}

int clientTable_getConnectedClients(ClientTable* cTable, char* ary, int aryLen) {
    return client_hash_table_get_full_str(cTable->hashTable, cTable->tableLen, ary, aryLen);
}


#endif


