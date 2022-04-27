#ifndef SESSIONS_H
#define SESSIONS_H
#include "string.h"
#include "pthread.h"
#include "sys/select.h"
#include "clientInfo.h"
#include "linkedList.h"


// Individual session node
typedef struct Session {
    char* sessionID;
    List* list;
    //fd_set sessionfds;
    pthread_mutex_t fdLock;
    pthread_mutex_t idLock;

}Session;

void session_free(Session* session){
    free(session->sessionID);
    list_delete(&session->list);
    free(session);
}
Session* session_init(char* sessionID) {
    if (sessionID == NULL) {
        puts("session_init: sessionID is null");
        return NULL;
    }
    Session* session = (Session*)malloc(sizeof(Session));
    session->sessionID = (char*)malloc(strlen(sessionID)+1);
    strncpy(session->sessionID, sessionID, strlen(sessionID)+1);
    session->list = list_init();
    //pthread_mutex_init(&session->fdLock, NULL);
    //pthread_mutex_init(&session->idLock, NULL);
    return session;
}


typedef struct HashNode {
    Session* session;
    struct HashNode* next;
}HashNode;

HashNode* hashNode_init() {
    HashNode* hashNode = (HashNode*)malloc(sizeof(HashNode));
    return hashNode;
    hashNode->session = NULL;
    hashNode->next = NULL;
    return hashNode;
}



int hash_table_insert(HashNode* node, HashNode** hashTable, int tableLen) {
    if (node == NULL) return 0;
    unsigned long idx = hash(node->session->sessionID, tableLen);
    if (idx < 0 || idx >= tableLen) {
        printf("Hash idx out of bounds\n");
        return -1;
    }
    node->next = hashTable[idx];
    hashTable[idx] = node;
    return 1;
}


// returns 1 if deleted, 0 if not found or error
int hash_table_delete_Node(char* name, HashNode** hashTable, int tableLen) {
    unsigned long idx = hash(name, tableLen);
    if (idx < 0 || idx >= tableLen) {
        printf("Hash idx out of bounds\n");
        return 0;
    }
    //HashNode* head = hashTable[idx];
    //printf("tableLen is: %ld\n", tableLen);
    //printf("hash index is: %ld\n", idx);

    if (hashTable[idx] == NULL) return 0;
    if (strcmp(hashTable[idx]->session->sessionID, name) == 0) {
      if (hashTable[idx]->next == NULL) {
          session_free(hashTable[idx]->session);
          free(hashTable[idx]);
          hashTable[idx] = NULL;
          return 1;
      }
      else {
          //found is head but next is not NULL
          HashNode* tmp = hashTable[idx];
          hashTable[idx] = hashTable[idx]->next;
          session_free(tmp->session);
          free(tmp);
          tmp = NULL;
          return 1;
      }
    }
    
    HashNode* cur = hashTable[idx]->next;
    HashNode* prev = hashTable[idx];
    for (; cur!= NULL; prev = cur, cur = cur->next) {
        if (strcmp(cur->session->sessionID, name) == 0) {
            prev->next = cur->next;
            session_free(cur->session);
            free(cur);
            cur = NULL;
            return 1;
        }
    }
    return 0; // not found in linked list
}

// Returns null if not found, else returns the hashNode
HashNode* hash_table_find(char* word, HashNode** hashTable, int tableLen){
    unsigned long idx = hash(word, tableLen);
    if (idx < 0 || idx >= tableLen) {
        printf("Hash idx out of bounds\n");
    }
    HashNode* tmp = hashTable[idx];
    //printf("tableLen is: %ld\n", tableLen);
    //printf("hash index is: %ld\n", idx);

    if (tmp == NULL) return tmp;
    //printf("finding word: %s\n", tmp->word);
    
    //hash_table_print(hashTable, tableLen);
    while (tmp != NULL && strcmp(tmp->session->sessionID, word) != 0) {
        tmp = tmp->next;

    }
    return tmp;
}

void hash_table_print(HashNode** hashTable, int tableLen) {
    printf("Start\n");
    for (long i = 0; i < tableLen; i++) {
        if (hashTable[i] == NULL) {
            printf("\t%li\t---\n", i);
        }
        else {
            printf("\t%li\t", i);
            HashNode* tmp = hashTable[i];
            while (tmp != NULL ) {
                //if (tmp->file != NULL && tmp->file->data != NULL && tmp->file->data->file_name != NULL) 
                    
                printf("%s - ", tmp->session->sessionID);
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

// returns current index of ary, fills ary with str
int hash_table_get_full_str(HashNode** hashTable, int tableLen, char* ary, int aryLen) {
    //printf("Start\n");
    int count = 0;
    strncpy(ary+count, "Available sessions:\n", aryLen-count);
    count += strlen("Available sessions:\n");
    for (long i = 0; i < tableLen; i++) {
        if (hashTable[i] == NULL) {
            //printf("\t%li\t---\n", i);
        }
        else {
            //printf("\t%li\t", i);
            HashNode* tmp = hashTable[i];
            while (tmp != NULL ) {
                //if (tmp->file != NULL && tmp->file->data != NULL && tmp->file->data->file_name != NULL) 
                    
                //printf("%s - ", tmp->client->clientID);
                strncpy(ary+count, "\t", aryLen-count);
                count += strlen("\t");
                strncpy(ary+count, tmp->session->sessionID, aryLen-count);
                count += strlen(tmp->session->sessionID);
                
                int numSpace = 20 - strlen(tmp->session->sessionID);
                for (int a = 0; a < numSpace; a++){
                    ary[count++] = ' ';
                }
                //strncpy(ary+count, "        ", aryLen-count);
                //count += strlen("       ");
                
                ListNode* listNode = tmp->session->list->head;
                while (listNode != NULL) {
                    
                    strncpy(ary+count, listNode->str, aryLen-count);
                    count += strlen(listNode->str);
                    
                    strncpy(ary+count, " - ", aryLen-count);
                    count += strlen(" - ");
                    
                    listNode = listNode->next;
                    
                }
                strncpy(ary+count, "\n", aryLen-count);
                count += strlen("\n");  //Note we dont include null terminator

                tmp = tmp->next;
            }
            
        }
    }
    strncpy(ary+count, "\n", aryLen-count);
    count += strlen("\n"); 
    return count;
}

typedef struct SessionTable{
     HashNode** hashTable; //ptr to an array where each index is a ptr to HashNode
     int tableLen;
     pthread_mutex_t tableLock;
}SessionTable;

SessionTable* sessionTable_init(int maxClients, int avgClientsPerSession) {
    SessionTable* sTable = (SessionTable*)malloc(sizeof(SessionTable));
    
    int tableLen = (maxClients/avgClientsPerSession + 1) * 4;
    //int tableLen = 1;
    sTable->tableLen = tableLen;
    sTable->hashTable = (HashNode**)malloc(sizeof(HashNode*) * tableLen);
    
    for (int i = 0; i < tableLen; i++) {
        sTable->hashTable[i] = NULL;
    }
    
    pthread_mutex_init(&sTable->tableLock, NULL);
    return sTable;
    
}
void sessionTable_addEmptySession(SessionTable* sTable, char *sessionID) {
    Session* session = session_init(sessionID);
    HashNode* hashNode = hashNode_init();
    hashNode->session = session;
    
    //pthread_mutex_lock(&sTable->tableLock);
    hash_table_insert(hashNode, sTable->hashTable, sTable->tableLen);
    //pthread_mutex_unlock(&sTable->tableLock);
    
}

void sessionTable_addSession(SessionTable* sTable, Session* session) {
    HashNode* hashNode = hashNode_init();
    hashNode->session = session;
    
    hash_table_insert(hashNode, sTable->hashTable, sTable->tableLen);
}


void sessionTable_removeSession(SessionTable* sTable, char *sessionID) {
    
    //pthread_mutex_lock(&sTable->tableLock);
    hash_table_delete_Node(sessionID, sTable->hashTable, sTable->tableLen);
    //pthread_mutex_unlock(&sTable->tableLock);
    
}

// Returns null if not found, or if sessionID passed in is NULL (which means client is not in session)
Session* sessionTable_getSession(SessionTable* sTable, char *sessionID) {
    if (sessionID == NULL) return NULL;
    HashNode* hashNode = hash_table_find(sessionID,sTable->hashTable, sTable->tableLen);
    if (hashNode == NULL) return NULL;
    Session* session = hashNode->session;
    return session;
}

/*The function FD_IS_ANY_SET returns 0 
 * if *fdset contains at least one file descriptor, otherwise 1
 */
int fdSetEmpty(fd_set const *fdset)
{
    static fd_set empty;     // initialized to 0 -> empty
    return (memcmp(fdset, &empty, sizeof(fd_set)) == 0);
}

// Removes sessionfd from the session indicated by sessionID
// Deletes the session if it becomes empty
// Returns 1 if fd is removed, but session not deleted
// Returns 0 if session is also deleted
// Returns -1 if session is not found
/*int sessionTable_removeSessionFd(SessionTable* sTable, char *sessionID, int fd){
    Session* session = sessionTable_getSession(sTable, sessionID);
    if (session == NULL) return -1;
    FD_CLR(fd, &session->sessionfds);
    if (fdSetEmpty(&session->sessionfds)) {
        sessionTable_removeSession(sTable, sessionID);
        return 0;
    }
    return 1;
}*/

void sessionTable_print(SessionTable* sTable){
    puts("SessionTable:");
    //pthread_mutex_lock(&sTable->tableLock);
    hash_table_print(sTable->hashTable, sTable->tableLen);
    //pthread_mutex_unlock(&sTable->tableLock);
}

int sessionTable_getAvailableSessions(SessionTable* sTable, char* ary, int aryLen) {
    return hash_table_get_full_str(sTable->hashTable, sTable->tableLen, ary, aryLen);
}


#endif