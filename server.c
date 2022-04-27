#include "connection.h"
#include "message.h"
#include "string.h"
#include "sessions.h"
#include "clientInfo.h"
#include "fileReadWrite.h"

typedef struct Server{
    char* port;
    int listener; //sockfd of server used to listen for incoming connections
    struct sockaddr_storage remoteaddr; // client address info
    socklen_t addrlen;
    char remoteIP[INET6_ADDRSTRLEN]; 
    fd_set master;

}Server;


/* ====== Globals Start ==========*/
MsgBuf* mBuf;
Server server;
ClientTable* cTable;
SessionTable* sTable;
//ClientTable* cTable;
#define HARDCODEDCLIENTS 10
char* clientPassword[HARDCODEDCLIENTS][2] = {
    {"Tiger", "ps0"},
    {"Lion", "ps1"},
    {"King", "ps2"},
    {"Zeus", "ps3"},
    {"Athena", "ps4"},
    {"Hera","ps5"},
    {"Achilles","ps6"},
    {"Zombie","ps7"},
    {"FBI","ps8"},
    {"CIA","ps9"},
};  

/*======== Globals End =============*/
void server_registerAck(int sockfd) {
    Message msg;
    msg.type = RE_ACK;
    msg.size = 0;
    strncpy(msg.source, "server", MAX_NAME);
    
    message_send(sockfd, &msg);
}
void server_registerNack(int sockfd) {
    Message msg;
    msg.type = RE_NCK;
    msg.size = 0;
    strncpy(msg.source, "server", MAX_NAME);
    
    message_send(sockfd, &msg);
}

void server_loginAck(int sockfd) {
    Message msg;
    msg.type = LO_ACK;
    msg.size = 0;
    strncpy(msg.source, "server", MAX_NAME);
    
    message_send(sockfd, &msg);
}
void server_loginNack(int sockfd, char* reason) {
    Message msg;
    msg.type = LO_NAK;
    msg.size = strlen(reason) + 1;
    strncpy(msg.source, "server", MAX_NAME);
    strncpy(msg.data, reason, MAX_DATA);
    message_send(sockfd, &msg);
}

void server_joinSessionAck(int sockfd, char* sessionID) {
    Message msg;
    msg.type = JN_ACK;
    msg.size = strlen(sessionID) + 1;
    strncpy(msg.source, "server", MAX_NAME);
    strncpy(msg.data, sessionID, MAX_DATA);
    message_send(sockfd, &msg);
}

void server_joinSessionNack(int sockfd, char* sessionID, char* reason) {
    Message msg;
    msg.type = JN_NAK;
    msg.size = strlen(sessionID) + strlen(", ") + strlen(reason) + 1;
    strncpy(msg.source, "server", MAX_NAME);
    strncpy(msg.data, sessionID, MAX_DATA);
    msg.data[strlen(sessionID)] = ',';
    msg.data[strlen(sessionID)+1] = ' ';
    strncpy(msg.data+strlen(sessionID)+strlen(", "), reason, MAX_DATA);
    message_send(sockfd, &msg);
}


void server_newSessionAck(int sockfd, char* sessionID) {
    Message msg;
    msg.type = NS_ACK;
    msg.size = strlen(sessionID) + 1;
    strncpy(msg.source, "server", MAX_NAME);
    strncpy(msg.data, sessionID, MAX_DATA);
    message_send(sockfd, &msg);
}

void server_queryAck(int sockfd, char* list) {
    Message msg;
    msg.type = QU_ACK;
    msg.size = strlen(list) + 1;
    strncpy(msg.source, "server", MAX_NAME);
    strncpy(msg.data, list, MAX_DATA);
    message_send(sockfd, &msg);
}

void server_privateNack(int sockfd) {
    Message msg;
    msg.type = PRI_NAK;
    msg.size = 0;
    strncpy(msg.source, "server", MAX_NAME);
    
    message_send(sockfd, &msg);
}

void server_closeConnectionFd(Server* server, int sockfd) {
    close(sockfd); // bye!
    FD_CLR(sockfd, &server->master); // remove from master set

    Client* client = clientTable_getClientByFd(cTable, sockfd);
    if (client != NULL) { // Client is logged In 
      
        // Quit from session if client is in session
        if (client->status == INSESSION) {
            puts("InSession and Quitting");
            printf("Client %s has left session %s\n", client->clientID, client->sessionID);

            Session* session = sessionTable_getSession(sTable, client->sessionID);
            if (session == NULL) {
                puts("Error: Session not found");
            } else{
                list_deleteNode(session->list, client->clientID);
                puts("deleting from list");
                if (session->list->head == NULL){
                    sessionTable_removeSession(sTable, session->sessionID);
                }
            }
        }
        printf("client %s has exited\n", client->clientID);
        client->status = LOGGEDOUT;
        client->sockfd = -1;
        client_setSessionID(client, NULL);
    }
    
}

void server_closeConnection(Server* server, Client* client) {
    if (client == NULL) {
        puts ("server_closeConnection(): client is NULL error");
    }
    close(client->sockfd); // bye!
    FD_CLR(client->sockfd, &server->master); // remove from master set
    
    // Already checked before
    //if (client != NULL) { // Client is logged In
        // Quit from session if client is in session
    if (client->status == INSESSION) {
        printf("Client %s has left session %s\n", client->clientID, client->sessionID);
        Session* session = sessionTable_getSession(sTable, client->sessionID);
        if (session == NULL) {
            puts("Error: Session not found");
        } else{
            list_deleteNode(session->list, client->clientID);
            if (session->list->head == NULL){
                sessionTable_removeSession(sTable, session->sessionID);
            }
        }
    }
        
    //}
    client->status = LOGGEDOUT;
    client->sockfd = -1;
    client_setSessionID(client, NULL);
}

void server_leaveSession(Server* server, Client* client, Session* session) {
    if (client->status == INSESSION) {
        if (session == NULL) {
            puts("Error: Session is NULL. Not found");
        } else{
            list_deleteNode(session->list, client->clientID);
            if (session->list->head == NULL){
                sessionTable_removeSession(sTable, session->sessionID);
            }
        }
        client->status = LOGGEDIN;
        client_setSessionID(client, NULL);
    }
}

// handle data from a client
void server_handleClientMessage(int i) { //i is sockfd of the client
    Message msg;
    
    if (message_receive(i, &msg, mBuf) == 0) {
        // connection closed
        printf("server: socket %d hung up\n", i);
        server_closeConnectionFd(&server, i);
    } else {
        
        // we got some data from a client
        // First check if client wants to register
        if (msg.type == REGISTER) {
            
            Client* client = clientTable_getClient(cTable, msg.source);
            if (client == NULL) {

                printf("Registering client %s with password %s\n", msg.source, msg.data);
                //int a = strlen(arguments[1]);
                Client* new = client_initAll(
                        LOGGEDIN,      // status
                        msg.source, // Username
                        msg.data, // Password
                        i,        //sockfd set to i
                        NULL    // sessionID set to NULL (not in a session)
                    );
                clientTable_addClient(cTable, new); 
                //puts(new->clientID); puts(new->password);

                char *usernameAndPassword = malloc(strlen(msg.source) + strlen(msg.data) + 2); // +2 for whitespace and null-terminator
                strcpy(usernameAndPassword, msg.source);
                usernameAndPassword[strlen(msg.source)] = ' ';
                strcpy(usernameAndPassword+strlen(msg.source)+1, msg.data);
                file_insert_lines(usernameAndPassword);
                server_registerAck(i);
                free(usernameAndPassword);
            } else {
                // client with this user name already exists
                printf("Client register rejected: client with username %s already exists\n", msg.source);
                server_registerNack(i);
            }
            return;
        }

        Client* client = clientTable_getClient(cTable, msg.source);
        if (client == NULL) {
            puts("Invalid clientID. Reject client message");
            if (msg.type == LOGIN) {
                server_loginNack(i, "The Username you entered does not exist. Register instead?");
            }
            return;
        }
        if (msg.type == LOGIN) {
            if (client->status == LOGGEDOUT){
                if (strcmp(msg.data, client->password) == 0) {
                    // Password is correct
                    // Set client as logged in
                    client->status = LOGGEDIN;
                    client->sockfd = i;
                    server_loginAck(i);
                    printf("client %s has logged in\n", client->clientID);
                } else {
                    server_loginNack(i, "Login Password Incorrect");
                }

            }else{
                server_loginNack(i, "Client with same ID is already logged in");
            }
        }else if (msg.type == EXIT) {
            // close connection on the client
            server_closeConnection(&server, client);
            printf("Client %s has exited\n", client->clientID);
        }else if (msg.type == JOIN) {
            // handle join message
            if (client->status == LOGGEDIN) {
                // Search if a session is available (msg.data should contain sessionID)
                Session* session = sessionTable_getSession(sTable, msg.data);
                if (session == NULL) { // Session not found
                    server_joinSessionNack(i, msg.data, "Session not found");
                } else { // session is found
                    // Add to session's list of clientID
                    list_insert(session->list, listNode_initAll(client->clientID));
                    client_setSessionID(client, msg.data);
                    client->status = INSESSION;

                    server_joinSessionAck(i, msg.data);
                    printf("Client %s has joined session %s\n", client->clientID, client->sessionID);
                }
            } else if (client->status == INSESSION) {
                server_joinSessionNack(i, msg.data, "Already in a session");
            } else if (client->status == LOGGEDOUT) {
                server_joinSessionNack(i, msg.data, "Not logged in to server");
            } else {}
        }else if (msg.type == LEAVE_SESS) {
            if (client->status == INSESSION){
                printf("Client %s has left session %s\n", client->clientID, client->sessionID);
                server_leaveSession(&server, client, sessionTable_getSession(sTable, client->sessionID));
            } else {}
        } else if (msg.type == NEW_SESS) {
            if (client->status != LOGGEDOUT) {
                if (client->status == INSESSION) {
                    printf("Client %s will leave session %s to create a new session\n", client->clientID, client->sessionID);
                    server_leaveSession(&server, client, sessionTable_getSession(sTable, client->sessionID));

                }
                //(client->status == LOGGEDIN) {
                Session* session = sessionTable_getSession(sTable, msg.data);
                if (session == NULL) { //Session not found, client is allowed to create new session
                    Session* newSession = session_init(msg.data);
                    list_insert(newSession->list, listNode_initAll(client->clientID));
                    sessionTable_addSession(sTable, newSession);
                    client->status = INSESSION;
                    client_setSessionID(client, msg.data);
                    printf("Client %s has created new session %s\n", client->clientID, client->sessionID);
                    server_newSessionAck(i, client->sessionID);
                } else {  // Session specified by msg.data already exists (session is not null)
                    printf("Client %s unable to create new session %s because this session already exists\n", client->clientID, msg.data);
                }

            } else { // Client is logged out
                //server_loginNack(i, "Wrong command used");
            }
        } else if (msg.type == MESSAGE) {
            // send to all other connected clients in the session

            if (client->status == INSESSION) {
                //puts("Client is sending a message and is in session");
                Session* session = sessionTable_getSession(sTable, client->sessionID);
                ListNode* clientInfo = session->list->head;
                
                char newSource[MAX_NAME];
                snprintf(newSource, MAX_NAME,"(%s) %s", session->sessionID, msg.source);
                strncpy(msg.source, newSource, MAX_NAME);
                
                while (clientInfo != NULL) {
                    // send to everyone except this client
                    Client* inSessionClient = clientTable_getClient(cTable, clientInfo->str);
                    //puts("searching for session's clients\n");
                    if (inSessionClient->sockfd != i) {
                        message_send(inSessionClient->sockfd, &msg);
                        //printf("client is sending: %s\n", msg.data);
                    }
                    clientInfo = clientInfo->next;
                }
            } else { } // Do nothing
        } else if (msg.type == PRIVATE) {
            char* found = strtok(msg.data," ");
            char* arguments[1];
            for (int argumentCount = 0; found != NULL && argumentCount < 1; argumentCount++) {
                arguments[argumentCount] = found;
                //if (argumentCount == 1) break;
                //found = strtok(NULL," ");
            }
            char newSource[MAX_NAME];
            snprintf(newSource, MAX_NAME,"(%s) %s", "Private", msg.source);
            strncpy(msg.source, newSource, MAX_NAME);
            Client* destClient = clientTable_getClient(cTable, arguments[0]); 
            char newData[MAX_DATA];
            snprintf(newData, MAX_DATA,"%s", msg.data+strlen(arguments[0])+1);
            strncpy(msg.data, newData, MAX_DATA); 
            //puts(msg.data);
            if (destClient != NULL && destClient->status != LOGGEDOUT && destClient->sockfd != i) {
                message_send(destClient->sockfd, &msg);
            } else {
                server_privateNack(i);
            } // Dont send (destClient not exist)
        
        } else if (msg.type == QUERY) {
            char list[MAX_DATA];
            int pos = 0;
            pos = clientTable_getConnectedClients(cTable, list, MAX_DATA);
            pos = sessionTable_getAvailableSessions(sTable, list+pos, MAX_DATA-pos);
            server_queryAck(i, list);
        } else {}
           
    }
}
void server_init(char* port){
    int expectedClientsNum = HARDCODEDCLIENTS + 10;
    mBuf = msgBuf_init();
    cTable = clientTable_init(expectedClientsNum);
    for (int i = 0; i < HARDCODEDCLIENTS; i++) {
        clientTable_addClient(
            cTable, 
            client_initAll(
                LOGGEDOUT,      // status
                clientPassword[i][0], // Username
                clientPassword[i][1], // Password
                -1,        //sockfd set to -1 since not connected
                NULL    // sessionID set to NULL (not in a session)
            )
        ); 
    }
    Text_obj* text_obj = file_get_lines();
    if (text_obj != NULL) {
        for (int i=0; i<text_obj->numOfLines; i++){
            int userLen = 0;
            for (int j = 0; j < strlen(text_obj->text[i]); j++) {
                if (text_obj->text[i][j] == ' ') {
                    break;
                } else {
                    userLen++;
                }
            }
            char* userID = (char*)malloc(sizeof(char) * userLen + 1);
            memcpy(userID, text_obj->text[i], userLen);
            userID[userLen] = '\0';
            char* password = &(text_obj->text[i][userLen+1]);
            //puts(password);
            //printf("%s",text_obj->text[i]);
            //printf("%s %s",arguments[0], arguments[1]);
            clientTable_addClient(
                cTable, 
                client_initAll(
                    LOGGEDOUT,      // status
                    userID, // Username
                    password, // Password
                    -1,        //sockfd set to -1 since not connected
                    NULL    // sessionID set to NULL (not in a session)
                )
            ); 
            free (userID);
        }
    }
    text_obj = text_obj_destroy(text_obj);

    int avgClientsPerSession = 4;
    sTable = sessionTable_init(expectedClientsNum, avgClientsPerSession);
    
    clientTable_print(cTable);
    sessionTable_print(sTable);
    
    int backlog = 20;
    server.port = port;
    server.listener = tcp_server_listener_start(port, backlog);
}



void server_run() {
    //fd_set master;    // master file descriptor list
    // Using server.master
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number
    
    // add the listener to the master set
    FD_SET(server.listener, &server.master);

    // keep track of the biggest file descriptor
    fdmax = server.listener; // so far, it's this one    
     for(;;) {
        read_fds = server.master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == server.listener) {
                    // handle new connections
                    server.addrlen = sizeof server.remoteaddr;
                    int newfd = accept(server.listener,
                        (struct sockaddr *)&server.remoteaddr,
                        &server.addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &server.master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("server: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(server.remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&server.remoteaddr),
                                server.remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                        // Now, client is connected, but logged out. We need
                        // to check for password when we receive from it next time
                        
                    }
                } else {
                    server_handleClientMessage(i);
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)
    

    
}


int main(int argc, char *argv[]) {

    server_init(argv[1]);
    
    server_run();
    
    return 0;
}
