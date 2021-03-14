#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

//Functie care citeste de la stdin date
//Parseaza inputul scotand newlineul de la final
//Intoarce stringul
char *citire_input(){
	char buffer[BUFLEN];
    char *ptr;
    memset(buffer,0,BUFLEN);
    fgets(buffer,BUFLEN-1,stdin);
    ptr=strtok(buffer,"\n");
    return ptr;
}
//Functie care primeste raspunsul de la un server ca string
//Intoarce cookie-ul de autentificare din raspuns
//Cu acest cookie asiguram serverul ca suntem logati in anumite conditii
char *cookie_auth(char *string){
	char *buffer=(char*)malloc(1024*sizeof(char));
	char *ptr=strtok(string,"\r\n");
	//Cauta in raspuns linia care contine Set-Cookie
	while(ptr){
		if(strstr(ptr,"Set-Cookie")){
			//Folosesc ptr+12 pentru avea raspunsul campului Set-Cookie
			memcpy(buffer,ptr+12,1024);
		}
		ptr=strtok(NULL,"\r\n");
	}
	return buffer;
}
//Functie care primeste raspunsul de la un server ca string
//Intoarce tokenul JWT care ne permite accesul
char *get_token(char *string){
	JSON_Value *value = json_value_init_object();
	JSON_Object *object;
	char *buffer=(char*)malloc(1024*sizeof(char));
	char *ptr=strtok(string,"\r\n");
	//Parcurge raspunsul pana ajunge la sectiunea de DATA
	while(ptr){
		if(strstr(ptr,"token")){
			//Parseaza raspunsul cu parson
			value=json_parse_string(ptr);
			//Se intoarce obiectul corespunzator lui value
			//Asociaza obiectul cu value
			object = json_value_get_object(value);
			//Se copiaza in buffer valoarea corespunzatoare cheii token
			sprintf(buffer,"%s",json_object_get_string(object,"token"));
			//Se elibereaza obiectul
			json_object_clear(object);
		}
		ptr=strtok(NULL,"\r\n");
	}
	return buffer;
}
//Functie care citeste datele despre o carte de la tastatura
//Intoarce un string obtinut prin serializarea unui obiect JSON
char *add_book(){
	JSON_Value *value = json_value_init_object();
	JSON_Object *object = json_value_get_object(value);
	char *string=NULL;
	//Citeste titlul si il introduce in obiect cu cheia title
	printf("title=");
	json_object_set_string(object,"title",citire_input());
	//Citeste autorul si il introduce in obiect cu cheia author
	printf("author=");
	json_object_set_string(object,"author",citire_input());
	//Citeste genul cartii si il introduce in obiect cu cheia genre
	printf("genre=");
	json_object_set_string(object,"genre",citire_input());
	//Citeste publisherul si il salveaza intr-un string pentru a fi introdus ulterior
	printf("publisher=");
	char *publisher=(char*)malloc(50*sizeof(char));
	memcpy(publisher,citire_input(),50);
	//Citeste numarul de pagini
	printf("page_count=");
	char *nrpage=(char*)malloc(10*sizeof(char));
	int nr;
	nrpage=citire_input();
	//Daca numarul de pagini exista(nu este NULL), incearca transformarea intr-un integer
	if(nrpage)
		nr=atoi(nrpage);
	//Daca nu s-a citit numarul de pagini se retine -1
	else nr=-1;
	//Se introduce numarul de pagini in obiect cu cheia page_count
	json_object_set_number(object,"page_count",nr);
	//Se introduce publiesherul in obiect cu cheia publisher
	//In enunt este prezentat ca se citeste publisherul inainte de page count
	//Dar page count trebuie sa fie inaintea lui publisher in JSON
	json_object_set_string(object,"publisher",publisher);
	//Daca am citit un numar valid de pagini, nr o sa fie mai mare ca 0
	if(nr>0){
		string = json_serialize_to_string_pretty(value);
	}
	json_object_clear(object);
	return string;
}
//Verifica codul primit de la server
//2XX->OK
//4XX->eroare client
//5XX->eroare server
char *check_succes(char *string){
	char *buffer=(char*)malloc(1024*sizeof(char));
	char *aux=strtok(string," ");
	aux=strtok(NULL," ");
	if(aux){
		//Daca raspunusl este de forma 2XX intoarce un mesaj de forma "Succes!"
		if(aux[0]=='2'){
			buffer="Succes!";
		}
	}
	return buffer;
}
//Asemanator cu get_token
//Primeste un raspuns de la server
//Intoarce un mesaj de eroare sau de succes sau NULL daca nu s-a gasit mesaj de eroare
char *get_error(char *string){
	JSON_Value *value = json_value_init_object();
	JSON_Object *object;
	char *buffer;
	//Verifica daca comanda a avut succes cu functia check_succes
	char *copy=(char*)malloc(BUFLEN*sizeof(char));
	memcpy(copy,string,BUFLEN);
	buffer=check_succes(copy);
	char *ptr=strtok(string,"\r\n");
	if(strcmp(buffer,"Succes!")!=0){
		//Parcurge raspunsul pana se ajunge la sectiunea de DATA
		while(ptr){
			//Verifica daca avem mesaj de eroare
			if(strstr(ptr,"error")){
				value=json_parse_string(ptr);
				object = json_value_get_object(value);
				//Pastreaza in buffer valoarea cheii eroare.
				memcpy(buffer,json_object_get_string(object,"error"),1024);
				json_object_clear(object);
				break;
			}
		ptr=strtok(NULL,"\r\n");
		}
	}
	//Daca ptr mai contine informatii inseamna ca s-a gasit un mesaj de eroare
	//Sau ca s-a gasit codul 2XX de succes
	if(ptr){
		return buffer;
	}
	//Daca pointerul s-a ajuns la final fara sa gaseasca succes sau eroare intoarce NULL
	free(buffer);
	return NULL;
}
//Asemanator cu get token
//Primeste un raspuns de la server
//Intoarce un string serializat reprezentand un obiect JSON
char *get_response(char *string){
	JSON_Value *value = json_value_init_object();
	char *buffer=(char*)malloc(1024*sizeof(char));
	char *ptr=strtok(string,"\r\n");
	char *ant=(char*)malloc(1024*sizeof(char));
	//Cauta finalul raspunsului si pastreaza in anterior(ant) ultima linie(sectiunea DATA)
	while(ptr){
		memcpy(ant,ptr,1024);
		ptr=strtok(NULL,"\r\n");
	}
	//Daca ant exista, il parseaza la un obiect json 
	//Se serializeaza obiectul si intoarce stringul
	if(ant){
			value=json_parse_string(ant);
			buffer = json_serialize_to_string_pretty(value);
	}
	return buffer;
}
int main(int argc, char *argv[])
{	
	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);
	char *string=NULL;
    char *message;
    char *response;
    int sockfd;
    //Hostul serverului
    char host[]="ec2-3-8-116-10.eu-west-2.compute.amazonaws.com";
    //Portul serverului
    int port=8080;
    //Ip-ul serverului
    char ip[]="3.8.116.10";
    //Tipul de request
    char type[]="application/json";
    //cookie de autentificare
    char *auth=NULL;
    //Token JWT
    char *token=NULL;
    char *error;
    //Se citeste o comanda de la tastatura intr-un while infinit
    while(1){
    	char *buffer=citire_input();
    	//Daca s-a primit comanda "exit" se incheie programul
    	if(strcmp(buffer,"exit")==0)
    		break;
    	//Daca s-a primit comanda "register" se face o inregistrare de utilizator
    	if(strcmp(buffer,"register")==0){
    		//Se citeste usernameul
    		printf("username=");
    		json_object_set_string(root_object,"username",citire_input());
    		//Se citeste parola
    		printf("password=");
    		json_object_set_string(root_object,"password",citire_input());
    		//Se serializeaza obiectul json la un string
			string = json_serialize_to_string_pretty(root_value);
			//Se deschide conexiunea la server    		
    		sockfd=open_connection(ip,port,AF_INET,SOCK_STREAM,0);
    		//Se creaza un mesaj de request catre server pe linkul de register
    		message=compute_post_request(host,"/api/v1/tema/auth/register",type,string,1,NULL,NULL);
    		//Se trimite mesajul la server
    		send_to_server(sockfd,message);
    		//Se primeste raspunsul de la server
    		response=receive_from_server(sockfd);
    		//Se verifica raspunsul cu get_error
    		error=get_error(response);
    		if(error){
    			printf("\n%s\n\n",error );
    		}else{
    			printf("%s\n",response);
    		}
    		json_object_clear(root_object);
    		//Se inchide conexiunea la server
    		close_connection(sockfd);
    	}
    	//Daca s-a primit comanda "login" se face o logare de utilizator
    	//Asemanator cu register
    	if(strcmp(buffer,"login")==0){
    		printf("username=");
    		json_object_set_string(root_object,"username",citire_input());
    		printf("password=");
    		json_object_set_string(root_object,"password",citire_input());
			string = json_serialize_to_string_pretty(root_value);		
    		sockfd=open_connection(ip,port,AF_INET,SOCK_STREAM,0);
    		message=compute_post_request(host,"/api/v1/tema/auth/login",type,string,1,NULL,NULL);
    		send_to_server(sockfd,message);
    		response=receive_from_server(sockfd);
    		char *copy=(char*) malloc(BUFLEN*sizeof(char));
    		memcpy(copy,response,BUFLEN);
    		error=get_error(copy);
    		if(error){
    			printf("\n%s\n\n",error );
    		}else{
    			printf("%s\n",response);
    		}
    		json_object_clear(root_object);
    		close_connection(sockfd);
    		//Se pastreaza in stringul auth cookieul de logare
    		//Se foloseste pentru a arata serverului ca utilizatorul este logat
    		auth=cookie_auth(response);
    	}
    	//Daca s-a primit comanda "enter_library" se aceseaza libraria
    	if(strcmp(buffer,"enter_library")==0){
    		//Se deschide conexiunea la server
    		sockfd=open_connection(ip,port,AF_INET,SOCK_STREAM,0);
    		//Se creaza un mesaj de request pentru un get pe linkul de acces
    		//Se face request cu cookieul de autentificare primit la login
    		message=compute_get_request(host,"/api/v1/tema/library/access",NULL,auth,NULL);
    		//Se trimite mesajul la server
    		send_to_server(sockfd,message);
    		//Se primeste raspuns
    		response=receive_from_server(sockfd);
    		//Se verifica raspunsul cu get_error
    		char *copy=(char*) malloc(BUFLEN*sizeof(char));
    		memcpy(copy,response,BUFLEN);
    		error=get_error(copy);
    		if(error){
    			printf("\n%s\n\n",error );
    		}else{
    			printf("%s\n",response);
    		}
    		//Daca am primit mesaj de ok de la server, pastram tokenul JWT de la server
    		if(strcmp(error,"Succes!")==0)
    			token=get_token(response);
    		close_connection(sockfd);
    	}
    	//Daca s-a primit comanda "get_books" se face un request pentru afisarea tuturor cartilor
    	//Asemanator cu enter_library
    	if(strcmp(buffer,"get_books")==0){
    		sockfd=open_connection(ip,port,AF_INET,SOCK_STREAM,0);
    		//In request se adauga tokenul primit la comanda "enter_library"
    		message=compute_get_request(host,"/api/v1/tema/library/books",NULL,NULL,token);
    		send_to_server(sockfd,message);
    		response=receive_from_server(sockfd);
    		char *copy=(char*) malloc(BUFLEN*sizeof(char));
    		memcpy(copy,response,BUFLEN);
    		error=get_error(copy);
    		if(error){
    			printf("\n%s\n\n",error );
    		}else{
    			printf("%s\n",response);
    		}
    		//Daca am primit mesaj de ok de la server, afisam raspunsul din sectiunea de DATA
    		if(strcmp(error,"Succes!")==0){
    			printf("%s\n",get_response(response));
    		}
    		close_connection(sockfd);
    	}
    	//Daca s-a primti comanda "add_books" se citeste o carte si se adauga la server
    	if(strcmp(buffer,"add_book")==0){
    		//Foloseste functia de add_book
    		string = add_book();
    		//Stringul este NULL daca numarul de pagini nu este un numar natural
    		if(string==NULL){
    			//Se afiseaza un mesaj de eroare
    			printf("Invalid page number\n");
    			continue;
    		} 
    		//Se deschide conexiunea la server
    		sockfd=open_connection(ip,port,AF_INET,SOCK_STREAM,0);
    		//Se creaza un mesaj de request catre linkul de books
    		//Se adauga tokenul de autentificare primit la comanda "enter_library"
    		message=compute_post_request(host,"/api/v1/tema/library/books",type,string,1,NULL,token);
    		//Se trimite mesajul si se primeste un raspuns
    		send_to_server(sockfd,message);
    		response=receive_from_server(sockfd);
    		//Se verifica raspunsul cu get_error
    		error=get_error(response);
    		if(error){
    			printf("\n%s\n\n",error );
    		}else{
    			printf("%s\n",response);
    		}
    		close_connection(sockfd);
    	}
    	//Daca s-a primit comanda "get_book" acceseaza informatiile despre o carte
    	if(strcmp(buffer,"get_book")==0){
    		//Se citeste id-ul si se parseaza linkul de la request
    		printf("id=");
    		char *link=(char*)malloc(200*sizeof(char));
    		sprintf(link,"/api/v1/tema/library/books/%s",citire_input());
    		//Se deschide conexiunea la server
    		sockfd=open_connection(ip,port,AF_INET,SOCK_STREAM,0);
    		//Se creaza mesajul de request pe linkul parsat
    		message=compute_get_request(host,link,NULL,NULL,token);
    		//Se trimite mesajul la server si se primeste raspuns
    		send_to_server(sockfd,message);
    		response=receive_from_server(sockfd);
    		//Se verifica raspunsul cu get_error
    		char *copy=(char*) malloc(BUFLEN*sizeof(char));
    		memcpy(copy,response,BUFLEN);
    		error=get_error(copy);
    		if(error){
    			printf("\n%s\n\n",error );
    		}else{
    			printf("%s\n",response);
    		}
    		//Daca raspunsul este ok se afiseaza raspunsul
    		if(strcmp(error,"Succes!")==0){
    			printf("%s\n",get_response(response));
    		}
    		close_connection(sockfd);
    	}
    	//comanda "logout" transmite serverului delogarea
    	//asemanator cu toate de pana acum
    	if(strcmp(buffer,"logout")==0){
    		sockfd=open_connection(ip,port,AF_INET,SOCK_STREAM,0);
    		message=compute_get_request(host,"/api/v1/tema/auth/logout",NULL,auth,NULL);
    		send_to_server(sockfd,message);
    		response=receive_from_server(sockfd);
    		error=get_error(response);
    		if(error){
    			printf("\n%s\n\n",error );
    		}else{
    			printf("%s\n",response);
    		}
    		//Tokenul JWT si cookieul de logare devin nule cand se da logout
    		token=NULL;
    		auth=NULL;
    		close_connection(sockfd);
    	}
    	//Comanda "delete_book" primeste un id si trimite un request de stergere a unei carti
    	if(strcmp(buffer,"delete_book")==0){
    		//Se citeste id-ul si se parseaza linkul
    		printf("id=");
    		char *link=(char*)malloc(200*sizeof(char));
    		sprintf(link,"/api/v1/tema/library/books/%s",citire_input());
    		//Se deschide conexiunea la server
    		sockfd=open_connection(ip,port,AF_INET,SOCK_STREAM,0);
    		//Se face request de delete pe linkul parsat
    		message=compute_delete_request(host,link,token);
    		//Se trimite mesajul si se primeste raspuns
    		send_to_server(sockfd,message);
    		response=receive_from_server(sockfd);
    		//Se verifica raspunsul cu get_error;
    		error=get_error(response);
    		if(error){
    			printf("\n%s\n\n",error );
    		}else{
    			printf("%s\n",response);
    		}
    		close_connection(sockfd);
    	}
    }
    return 0;
}
