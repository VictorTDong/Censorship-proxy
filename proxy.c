/*
Purpose: The purpose of this assignment is to create a proxy which can censor user inputs
Details: The program will create a server socket in which it listens for a client and accepts. After the server socket
         accepts the client, it will then look for either BLOCK, UNBLOCK or browser request in which it will handle. The
         server will then handle the request and then pass it on to the client portion of the program in which it will
         send back to the browser to display the HTTP website.
Limitations and assumption: The program does not support concurrency. Does not support HTTPS websites
Known bugs: Program can not block more than one word
*/
#include <stdio.h>	//printf
#include <stdlib.h>
#include <string.h>	//strlen
#include <sys/socket.h>	//socket
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>
#include <netdb.h>
#include <stdbool.h> 
#include <sys/mman.h>

struct node
{
    char *keyword;
    struct node* next;
};
typedef struct node node_t;

void handle(int socket,int timeout);
int sendHandler(int socket, char *serverResponse, int totalSize);
char * recieveHandler(char * serverResponse, int totalSize, int recvSize);
int isRequest(char * clientMessage);
bool isMethod(char * clientMessage);
bool deleteKeyword(node_t **head, char *word);
bool findInPath(node_t *head, char *path);
node_t *newKeywordNode(char *word);
void *insertKeyword(node_t **head, node_t *nodeToInsert);

#define MESSAGE_SIZE 1024 //reads 1kb
#define STRING_SIZE 50
#define METHOD_SIZE 5
#define BLOCK_SIZE 4
#define UNBLOCK_SIZE 6
#define CRLF "\r\n"

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        printf("usage: ./proxy <Port Number>\n");
        return 0;
    }

    int portNumber = atoi(argv[1]);

    //SERVER SIDE
    struct sockaddr_in server;
    {
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(portNumber);
    server.sin_addr.s_addr = INADDR_ANY;
    };

    int serverListenSocket, clientAcceptSocket, bindStatus, recieveStatus, serverResponseStatus = -1,
        clientSocket, connectStatus;
    char temp[MESSAGE_SIZE], method[METHOD_SIZE], URL[STRING_SIZE], version[STRING_SIZE], hostname[STRING_SIZE], 
         path[STRING_SIZE], clientMessage[MESSAGE_SIZE], updatedClientMessage[MESSAGE_SIZE], serverResponse[MESSAGE_SIZE];

    char err01[] = "/~carey/CPSC441/ass1/error.html";
    char err02[] = "/~carey/CPSC441/ass1/error2.html";
    char updatedURL[100] = "http://";
    char hostString[] = "Host: ";

    serverListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverListenSocket == -1)
    {
        printf("Could not create socket\n");
    }

    bindStatus = bind(serverListenSocket, (struct sockaddr *)&server, sizeof(server));
    if (bindStatus == -1)
    {
        printf("Could not bind\n");
    }

    printf("Binding done\n");
    
    node_t *head = NULL;
    node_t *tmp;

    while(1)
    { 
        listen(serverListenSocket,20);
        clientAcceptSocket = accept(serverListenSocket,NULL, NULL);
        if (clientAcceptSocket == -1)
        {
            printf("Could not accept client socket\n");
        }
        printf("Client accepted\n");
        while(1)
        {
            memset(clientMessage, 0, MESSAGE_SIZE);
            recieveStatus = recv(clientAcceptSocket, clientMessage, MESSAGE_SIZE, 0);
            char *word;

            if (recieveStatus == -1)
            {
                printf("Could not recieve message\n");
            }

            int block = isRequest(clientMessage);
            char * message;

            if(block == 1) //add word to list
            {   
                memset(word,0,sizeof(char));
                strncpy(word,clientMessage+6,strlen(clientMessage+6)-2);
                tmp = newKeywordNode(word);
                insertKeyword(&head,tmp);
                printf("Added %s to the list\n",word);
            }
            else if(block == 2) //remove word from list
            {
                memset(word,0,sizeof(char));
                memcpy(word,clientMessage+8,strlen(clientMessage+8)-2);
                bool deleted = deleteKeyword(&head, word);
                printf("Deleted %s from the list\n",word);
            }
            else if(block == 3) //if the message recieved is not a block, unblock or exit command then handle the message normally
            { 
                sscanf(clientMessage,"%s %s %s %s %s",method, URL, version, temp, hostname);
                strcpy(path,strstr(URL,hostname));
                strcpy(path,strstr(path,"/"));

                printf("\nMethod: %s\nURL: %s\nversion: %s\nhostname: %s\npath: %s",method,URL,version,hostname,path);

                //CLIENT SIDE
                struct sockaddr_in client;
                {
                    client.sin_family = AF_INET; // IPv4
                    client.sin_port = htons(80);
                };
                
                struct hostent *hostAddress;

                if((hostAddress = gethostbyname(hostname)) == NULL)
                {
                    printf("\nError getting host");
                }
                bcopy((char *) hostAddress->h_addr, (char *) &client.sin_addr.s_addr, hostAddress->h_length);
                
                clientSocket = socket(AF_INET, SOCK_STREAM, 0);
                if (clientSocket == -1)
                {
                printf("\nCould not create socket\n");
                }

                connectStatus = connect(clientSocket, (struct sockaddr *)&client, sizeof(client));
                if (clientSocket == -1)
                {
                printf("\nCould not connect to server\n");
                }
                printf("\nConnected to server\n");

                int sendStatus;

                
                if(findInPath(head, path)) //finds the word and creates a new request header
                {
                    printf("\nFound word to block\n");
                    sprintf(updatedURL,"%s%s%s","http://",hostname,err01);
                    sprintf(updatedClientMessage,"%s %s %s%s",method,updatedURL,version, CRLF);
                    sprintf(updatedClientMessage,"%s%s%s%s", updatedClientMessage, hostString, hostname, CRLF);
                    sprintf(updatedClientMessage,"%s%s", updatedClientMessage, strstr(clientMessage, "User-Agent:"));

                    sendStatus = send(clientSocket, updatedClientMessage, strlen(updatedClientMessage), 0);

                    printf("\nNew request header is: \n%s",updatedClientMessage);
                }
                else //if the path does not required to be block, handle normally
                {
                    sendStatus = send(clientSocket, clientMessage, strlen(clientMessage), 0);
                }
                if (sendStatus == -1)
                {
                    printf("Could not send get request to server\n");
                }
                handle(clientSocket, clientAcceptSocket); //handles the request and response by sending and recieving
                break;
            }
            else if(block == 4) //exits the program with word "EXIT"
            {
                close(clientAcceptSocket);
                close(clientSocket);
                close(serverListenSocket);
                printf("All sockets closed\n");
                return 0;

            }
            else //if the message is not a proper method or command, then tells the user to enter valid input
            {
                message = "Please Enter valid input\n";
                send(clientAcceptSocket,message,strlen(message),0);
            }
            close(clientAcceptSocket);
            break;
        }
    }
    close(clientSocket);
    close(serverListenSocket);  
    printf("All sockets closed\n");
    return 0;
}
/*
Function: handle
Purpose: To handle the server response and send back to the browser
Method: The function will accept the response header from the server and read all the contents of it then will send it back to the client.
        The recv function will loop until it reads all the content of the response header based on the content length from the header. Once 
        this is reached, it will stop and pass the header into the send function where it will send it back to the client
Input: Two sockets, one from the server and one for the client
Output: N/A
*/

void handle(int socket,int clientAcceptSocket)
{
    int totalSize = 0;
    int index = 0;
    int contentLength = 1;
    char buffer[MESSAGE_SIZE];
    char temp[MESSAGE_SIZE];
    char * serverResponse = malloc(sizeof(char));

    memset(serverResponse, 0, sizeof(char));

    while(totalSize < contentLength)
    {
        memset(buffer, 0, MESSAGE_SIZE);
        memset(temp, 0, MESSAGE_SIZE);
        
        //check content type and if it is a photo then leave it alone
        int recvSize = 0;
        if((recvSize =  recv(socket , buffer, MESSAGE_SIZE , 0) ) < 0)
		{
			usleep(100000);
		}
        else if(strstr(buffer, "304 Not Modified") != NULL)
        {
            sendHandler(clientAcceptSocket,buffer,recvSize);
            return;
        }
        else
        {
            if(contentLength == 1)
            {
                sscanf(strstr(buffer, "Content-Length:"),"%s %d",temp, &contentLength);
            }
            totalSize += recvSize;
            serverResponse = recieveHandler(serverResponse, totalSize, recvSize);
            memcpy(serverResponse + index, buffer, recvSize + 1);
            index += recvSize;
		}
    }    
    int sendSuccess = sendHandler(clientAcceptSocket,serverResponse,totalSize);
    if(sendSuccess == 0)
    {
        printf("\nUnable to send to client\n");
    }
    return;
}

/*
Function: sendHandler
Purpose: To make sure all the contents of the response is sent
Method: It will send the server response until the total size of the response is matched
Input: A socket for sending to the client, the server response header, and the total size of the header
Output: int 1 if the send is successfully sent, and 0 if the response cannot be sent
*/

int sendHandler(int socket, char *serverResponse, int totalSize)
{
    int bytesSent;
    int length = totalSize;
    
    while(length> 0)
    {
        bytesSent = send(socket,serverResponse,length,0);
        if(bytesSent <= 0)
        {
            break;
        }
        serverResponse += bytesSent;
        length -= bytesSent;
    }
    if (serverResponse == NULL)
    {
        return 0;
    }
    return 1;
}

/*
Function: recieveHandler
Purpose: To allotcate more space to the pointer for the server response
Method: This function will allocate more space the pointer and if it can not successfully do it, it will free the space and return NULL;
        This function will call realloc to do this and will allocate more space based on the total size plus the next size for the header
Input: Will take in a pointer to the server response, int for the current total size, and another int for the size of the next read message
Output: A pointer the newly reallocated server response
*/

char * recieveHandler(char * serverResponse, int totalSize, int recvSize)
{
    char * tmp = realloc(serverResponse, totalSize+recvSize);
    if(tmp)
    {
        serverResponse = tmp;
    }
    else
    {
        free(serverResponse);
        serverResponse = 0;
        return NULL;
    }
    return serverResponse;    
}

/*
Function: isRequest
Purpose: To check what kind of message the client sends to the server
Method: This function will compare the client message to see if its either a valid method, block request or unblock request
Input: Pointer to the client message
Output: Int representing what type of message it is
*/

int isRequest(char * clientMessage)
{
    if(clientMessage[0] == 'B' && clientMessage[1] == 'L' && clientMessage[2] == 'O' && clientMessage[3] == 'C' && clientMessage[4] == 'K')
    {
        return 1;
    }
    if(clientMessage[0] == 'U' && clientMessage[1] == 'N' && clientMessage[2] == 'B' && clientMessage[3] == 'L' && clientMessage[4] == 'O' && clientMessage[5] == 'C' && clientMessage[6] == 'K')
    {
        return 2;
    }
    if(isMethod(clientMessage))
    {
        return 3;
    }
    if(clientMessage[0] == 'E' && clientMessage[1] == 'X' && clientMessage[2] == 'I' && clientMessage[3] == 'T')
    {
        return 4;
    }
    else
        return 0;
}

/*
Function: isMethod
Purpose: To check for if the message is a valid method type
Method: Searches the client message for one of the methods type
Input: Pointer to client message
Output: Boolean to represent if the method is valid or not
*/

bool isMethod(char * clientMessage)
{
    if((strstr(clientMessage,"GET") || strstr(clientMessage,"POST") || strstr(clientMessage,"HEAD") || strstr(clientMessage,"PUT") || strstr(clientMessage,"DELETE")) != 0)
        return true;
    else
        return false;
}

/*
Function: insertKeyword
Purpose: This function inserts a new node into the linked list
Method: It will take the newly created node and connect it to heads. This function will insert at the front of the list
Input: Two nodes, one for the head of the current list and one for the next node to insert
Output: N/A
*/

void *insertKeyword(node_t **head, node_t *nodeToInsert)
{
    nodeToInsert->next = *head;
    *head = nodeToInsert;
}

/*
Function: newKeywordNode
Purpose: This will create a new node containing a word representing the word to block
Method: This function will allocate space for a new node insert the word into it
Input: A pointer to the new word
Output: New node for the word
*/

node_t *newKeywordNode(char *word)
{
    node_t *newKeyword = malloc(sizeof(node_t));
    newKeyword->keyword = word;
    newKeyword->next = NULL;
    return newKeyword;
}

/*
Function: findInPath
Purpose: This function will traverse through the linked list to look for an element of the linked list in the path
Method: Will traverse the list till the word is found in the path or if it is not there, it will return false
Input: Node for the list and pointer to the path of the request
Output: Boolean representing if the word is found or not
*/

bool findInPath(node_t *head, char *path)
{
    node_t *temp = head;
    while(temp != NULL)
    {
        if(strstr(path,temp->keyword) != NULL)
        {
            return true;
        }
        temp = temp->next;
    }
    return false;
}

/*
Function: deleteKeyword
Purpose: To delete a specified element in the linked list
Method: Will check to see if the head of the list is the word, if it is it will point the head to the next element. If the word not the head,
        the function will traverse the list until it finds the word or encounters the last element(NULL). If the head reaches NULL, the word 
        is not found and thus the current element will be deleted
Input: Node for the head of the current list and a pointer to the word it is looking for
Output: Boolean to represent if the word is deleted or not
*/

bool deleteKeyword(node_t **head, char *word)
{
    node_t *temp = *head;
    node_t *prev = NULL;
    if(temp != NULL && temp->keyword == word)
    {
        *head = temp->next;
        free(temp);
        return true;
    }

    while (temp != NULL)
    {
        if(strcmp(temp->keyword, word))
        {
            prev->next = temp->next;
            free(temp);
            return true; 
        }
        prev = temp;
        temp = temp->next;
    }
    if(temp == NULL)
    {
        return false;
    }  
}
