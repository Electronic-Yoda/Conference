#include <string.h>
#include <stdbool.h>
#include "connection.h"
#include <pthread.h>
#include "message.h"

/* ====== Globals Start ==========*/
int sockfd;
char s[INET6_ADDRSTRLEN];

bool connected = false; //if not connected, only /login command and /quit is allowed
bool inSession = false; //if inSession is true, allow conference messages to be sent (string input without "/")

char clientID [MAX_DATA];

pthread_mutex_t lock;
MsgBuf* mBuf;
/*======== Globals End =============*/

void UI_HandleMessage(Message msg){
    pthread_mutex_lock(&lock);
    switch (msg.type){
        case LO_ACK:
            printf("Received successful login ack\n");
            connected = true;
            break;
        case LO_NAK:
            printf("Received failed login ack\nReason: %s\n", msg.data);
            //connected = false;
            break;
        case RE_ACK:
            printf("Received success registration ack\n");
            connected = true;
            break;    
        case RE_NCK:
            printf("Received failed registration ack. Server with username already exists.\nRegister with a different username or login with this username.\n");
            break;
        case JN_ACK:
            printf("Received successful join session ack:\n%s\n", msg.data);
            inSession = true;
            break;
        case JN_NAK:
            printf("Received failed joining a session ack:\n%s\n", msg.data);
            break;
        case NS_ACK:
            printf("Received ack that new session is created and joined that session\n");
            inSession = true;
            break;
        case QU_ACK:
            printf("Received server information:\n");
            printf("%s\n", msg.data);
            break; 
        case MESSAGE:
            printf("%s: %s", msg.source, msg.data);
            break;
        case PRIVATE:
            printf("%s: %s", msg.source, msg.data);
            break;
        case PRI_NAK:
            puts("Private message not sent! The user you are talking to is not online. Type /list to see who is online");
            break;
    }
    pthread_mutex_unlock(&lock);
}



//waits for msgs from the server + processes them
void client_receiveThread(){
    //while (connected){
    while (1) {
        Message msg;
        
        int return_val = message_receive (sockfd, &msg, mBuf);
        
        if (return_val == 0){
            //server has closed the connection
            pthread_mutex_lock(&lock);
            connected = false;
            pthread_mutex_unlock(&lock);
            break;
        } else {
            UI_HandleMessage(msg);
            if (connected == false) { // This means client has failed to login or register. We will terminate this thread
                break;
            }
        }
        
    }
    close(sockfd);       
    mBuf = msgBuf_destroy(mBuf);
    
}

//establish tcp connection with server
//store the necessary data (like socketID) to some global variable/data structure
//pthread_create() which creates a receiveThread to receive messages from server   
int client_connect(char* clientID, char* clientPassword, char* host, char* port){
    if (mBuf != NULL) {
        mBuf = msgBuf_destroy(mBuf); 
    }
    mBuf = msgBuf_init();
    sockfd = tcp_client_connect(host, port);
    //connected = true;
    
    //creates a new Thread to receives messages from server
    pthread_t threadId; //thread id of the thread that is about to be created
    int err = pthread_create(&threadId, NULL, (void*)(&client_receiveThread), NULL); 
    if (err){
        printf("Error creating thread\n");
        return err;
    } else {
        //printf("Thread created!\n");
    }
    
    Message msg;
    msg.type = LOGIN;
    msg.size = strlen(clientPassword) + 1;
    strncpy(msg.source, clientID, MAX_NAME);
    strncpy(msg.data, clientPassword, MAX_DATA);
    
    message_send(sockfd, &msg);
    
}

void client_register(char username[], char password [], char* host, char* port){
    if (mBuf != NULL) {
        mBuf = msgBuf_destroy(mBuf);
    }
    mBuf = msgBuf_init();
    sockfd = tcp_client_connect(host, port);
    //connected = true;
    
    //creates a new Thread to receives messages from server
    pthread_t threadId; //thread id of the thread that is about to be created
    int err = pthread_create(&threadId, NULL, (void*)(&client_receiveThread), NULL); 
    if (err){
        printf("Error creating thread\n");
        return;
    } else {
        //printf("Thread created!\n");
    }

    /* char *usernameAndPassword = malloc(strlen(username) + strlen(password) + 2); // +2 for whitespace and null-terminator
    strcpy(usernameAndPassword, username);
    usernameAndPassword[strlen(username)] = ' ';
    strcpy(usernameAndPassword+strlen(username)+1, password); */
    
    //puts(usernameAndPassword);
    
    Message msg;
    msg.type = REGISTER;
    msg.size = strlen(password) + 1;
    strncpy(msg.source, clientID, MAX_NAME);
    strncpy(msg.data, password, MAX_DATA);
    
    message_send(sockfd, &msg);
    //free (usernameAndPassword);
}


void client_message (char text[]){
    Message msg;
    msg.type = MESSAGE;
    msg.size = strlen(text) + 1;
    strncpy(msg.source, clientID, MAX_NAME);
    strncpy(msg.data, text, MAX_DATA);
    
    message_send(sockfd, &msg);
}

void client_query(){
    Message msg;
    msg.type = QUERY;
    msg.size = 0;
    strncpy(msg.source, clientID, MAX_NAME);
    //msg.data = NULL;
    
    message_send(sockfd, &msg);
}

void client_newSession(char sessionID []){
    Message msg;
    msg.type = NEW_SESS;
    msg.size = strlen(sessionID) + 1;
    strncpy(msg.source, clientID, MAX_NAME);
    strncpy(msg.data, sessionID, MAX_DATA);
    
    message_send(sockfd, &msg);
}

void client_join(char sessionID []){
    Message msg;
    msg.type = JOIN;
    msg.size = strlen(sessionID) + 1;
    strncpy(msg.source, clientID, MAX_NAME);
    strncpy(msg.data, sessionID, MAX_DATA);
    
    message_send(sockfd, &msg);
}



void client_leaveSession(){
    Message msg;
    msg.type = LEAVE_SESS;
    msg.size = 0;
    strncpy(msg.source, clientID, MAX_NAME);
    //msg.data = NULL;
    
    message_send(sockfd, &msg);
    
    pthread_mutex_lock(&lock);
    inSession = false;
    pthread_mutex_unlock(&lock);
}


void client_exit(){
    Message msg;
    msg.type = EXIT;
    msg.size = 0;
    strncpy(msg.source, clientID, MAX_NAME);
    //msg.data = NULL;
    
    message_send(sockfd, &msg);

    pthread_mutex_lock(&lock);
    connected = false;
    pthread_mutex_unlock(&lock);
}

void client_privateMessage(char destClientID[], char text[]){
    Message msg;
    msg.type = PRIVATE;
    msg.size = strlen(destClientID) + strlen(text) + 2;
    strncpy(msg.source, clientID, MAX_NAME);
    strcpy(msg.data, destClientID);
    msg.data[strlen(destClientID)] = ' ';
    strcpy(msg.data+strlen(destClientID)+1, text);
    
    //puts (msg.data);
    
    message_send(sockfd, &msg);
}

void ui_printCommands(){
    printf("\n-------- The commands you can enter are as follows --------\n\n");
    
    printf("Register a new account: \n/register <client ID> <password> <server-IP> <server-port> \n");
    printf("Log into the server: \n/login <client ID> <password> <server-IP> <server-port> \n");
    printf("Logout of the server: \n/logout\n");
    printf("Join an existing session: \n/joinsession <session ID>\n");
    printf("Leave your current session: \n/leavesession \n");
    printf("Create a new session and join it:\n/createsession <session ID> \n");
    printf("Get the list of the connected clients and available sessions:\n/list \n");
    printf("Quit the program: \n/quit \n");
    printf("Send a message to your currently joined session:\n<text> \n");
    printf("Send a private message to your currently joined session:\n/to <destination client ID> <text> \n");
    
    printf("\n-------- Enter /help to view these commands again --------\n\n ");
}

void ui_run(){
    const int MAX_IN = 256;
    char input[MAX_IN];
    const int maxArg = 5;
    char* arguments[maxArg]; //arguments[0] is /login, arguments[1] is <client ID> - <password - <server-IP> - <server-port>
    int argumentCount = 0;
    
    ui_printCommands();
    
    
    while (1){
        argumentCount = 0;
        
        // Flush arguments ary
        for (int i = 0; i < maxArg; i++) {
            arguments[i] = NULL;
        }
        //printf("Please input a command: \n");
           
        fgets(input, MAX_IN, stdin);
        if (input[0] == '\n') {
            continue;
        }
        char* raw = (char*)malloc(strlen(input)+1);
        strncpy(raw, input, strlen(input)+1);
        
        // Removes trailing newline
        if ((strlen(input) > 0) && (input[strlen (input) - 1] == '\n')){
            input[strlen (input) - 1] = '\0';
        }

        char* found = strtok(input," ");
        argumentCount = 0;
        for (; found != NULL && argumentCount < maxArg; argumentCount++) {
            arguments[argumentCount] = found;
            found = strtok(NULL," ");
        }

        if (strncmp(arguments[0], "/login", strlen("/login")) == 0){
            if (!connected){
                strncpy(clientID, arguments[1], MAX_NAME); 
                printf("Trying to connect w server and login...\n");
                client_connect(clientID, arguments[2], arguments[3], arguments[4]);
            } else {
                puts("You are already logged in!");
            }
            

        } else if (strncmp(arguments[0], "/help", strlen("/help")) == 0) {
            
            ui_printCommands();

        } else if (strncmp(arguments[0], "/logout", strlen("/logout")) == 0) {
            if (connected) {
                puts("You have logged out");
                client_exit();
            }
            else
                printf("You need to log in first!\n");

        } else if (strncmp(arguments[0], "/createsession", strlen("/createsession")) == 0) {
            if (connected)
                client_newSession(arguments[1] );
            else
                printf("You need to log in first!\n");

        } else if (strncmp(arguments[0], "/joinsession", strlen("/joinsession")) == 0) {
            client_join(arguments[1]);

        } else if (strncmp(arguments[0], "/leavesession", strlen("/leavesession")) == 0) {
            if (inSession)
                client_leaveSession();
            else
                printf("In order to leave a session, you need to be in a session first!\n");
            

        } else if (strncmp(arguments[0], "/list", strlen("/list")) == 0) {
            if (connected)
                client_query();
            else
                printf("You need to log in first!\n");

        } else if (strncmp(arguments[0], "/quit", strlen("/quit")) == 0) {
            printf("Quitting client program!\n");
            break;

        } else if (strncmp(arguments[0], "/register", strlen("/register")) == 0) {
            printf("Register a new user\n");
            if (!connected){
                strncpy(clientID, arguments[1], MAX_NAME); 
                printf("Trying to connect w server and register...\n");
                client_register(clientID, arguments[2], arguments[3], arguments[4]);
            } else {
                puts("You are already logged in as a user! Please logout and register.");
            }

        } else if (strncmp(arguments[0], "/to", strlen("/to")) == 0) {
            printf("Sending Private Message\n");
            client_privateMessage(arguments[1], raw+strlen("/to")+1+strlen(arguments[1])+1);

        }  else if (inSession && strncmp(arguments[0], "/", 1) != 0){
            if (connected && inSession){
                client_message(raw);
                printf("Sent your msg to everyone else in session\n"
                        );
            } else
                printf("You need to be logged in and in a session to send a msg!\n");
            
        } 
        //printf("afterwards raw input is: %s", raw);
        free(raw);
    }
    
    
    
}

    
  
int main(int argc, char *argv[]) { 
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    ui_run();
    return 0;
}
