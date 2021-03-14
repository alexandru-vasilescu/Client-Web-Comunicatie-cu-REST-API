#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

//creaza un mesaj de GET request
char *compute_get_request(char *host, char *url, char *query_params,
                            char *cookies,char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    //Se introduc url-ul, numele metodei si protocolul
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }
    compute_message(message, line);
    // Se adauga host-ul
    if(host!=NULL){
        sprintf(line, "Host: %s", host);
        compute_message(message, line);
    }  
    // Se adauga tokenul de acces in library
    if(token!=NULL){
        sprintf(line, "Authorization: Bearer %s",token);
        compute_message(message,line);
    }
    // Se adauga cookiuri
    if(cookies!=NULL){
        sprintf(line, "Cookie: ");
        strcat(line, cookies );
        compute_message(message,line);
    }
    // Se adauga o linie goala si se intoarce mesajul
    compute_message(message, "");
    return message;
}
//creaza un mesaj de POST request
char *compute_post_request(char *host, char *url, char* content_type, char *body_data,
                            int body_data_fields_count, char *cookies,char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));

    //Se introduc numele metodei, protocolul si url-ul
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    //Se adauga host-ul
     if(host!=NULL){
        sprintf(line, "Host: %s", host);
        compute_message(message, line);
    }
    //Se adauga tokenul de acces in library
    if(token!=NULL){
        sprintf(line, "Authorization: Bearer %s",token);
        compute_message(message,line);
    }
    //Se adauga tipul de mesaj trimis din sectiunea de DATA
    sprintf(line, "Content-Type: %s",content_type);
    compute_message(message,line);
    int len=strlen(body_data);
    //Se adauga lungimea mesajului 
    sprintf(line,"Content-Length: %d", len);
    compute_message(message,line);
    //Se adauga cookieuri
    if(cookies!=NULL){
        sprintf(line, "Cookie: ");
        strcat(line, cookies );
        compute_message(message,line);
    }
    //Se adauga o linie goala pentru a separa HEADER de DATA
    compute_message(message,"");
    //Se adauga payloadul
    memset(line,0,LINELEN);
    if(body_data!=NULL){
        memset(body_data_buffer,0,LINELEN);
            sprintf(body_data_buffer,"%s",body_data);
    }
    memset(line, 0, LINELEN);
    compute_message(message, body_data_buffer);
    free(line);
    return message;
}
//creeaza un mesaj de delete request
char *compute_delete_request(char* host, char* url, char*token){
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    //Se introduc numele metodei, protocolul si url-ul
    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);
    //Se adauga hostul
     if(host!=NULL){
        sprintf(line, "Host: %s", host);
        compute_message(message, line);
    }
    //Se adauga tokenul de acces in library
    if(token!=NULL){
        sprintf(line, "Authorization: Bearer %s",token);
        compute_message(message,line);
    }
    //Se adauga o linie goala la finalul mesajului
    compute_message(message,"");
    free(line);
    return message;
}