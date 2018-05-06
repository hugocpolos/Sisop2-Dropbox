#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PORT 4000

typedef struct message_data{
	int order_number;
	char blockData[1024];
	struct message_data *Next_Block;
}MESSAGE_DATA_t;

typedef struct message{
	MESSAGE_DATA_t *data;
}MESSAGE_t;

int print_msg(MESSAGE_DATA_t *toprint){

	MESSAGE_DATA_t *aux;
	if(toprint != NULL){
		aux = toprint;
		do{
			printf("%d :", aux->order_number);
			puts(aux->blockData);
			aux = aux->Next_Block;
		}while (aux != NULL);
	}
	else{
	puts("Nothing to print.");
	return -1;
	}

}

MESSAGE_t create_message(FILE *file){
	MESSAGE_t newMsg;
	MESSAGE_DATA_t *aux, *new;
	int i = 0;
	int not_EOF = 0;
	int ERROR = 0;
	aux = NULL;

	while(not_EOF == 0 && ERROR == 0){
		new = malloc(sizeof(MESSAGE_DATA_t));		
		new->order_number = i;
		new->Next_Block = NULL;
		not_EOF = feof(file);
		ERROR = ferror(file);
		if(not_EOF == 0 && ERROR == 0){
			fread(new->blockData,sizeof(char),1024,file);
		}else{
			free(new);
			return newMsg;
		}

		if(i==0){
			newMsg.data = new;
		}
		i++;
		if(aux != NULL){
			aux->Next_Block = new;
		}
		aux = new;
		//printf("not_EOF %d\n ERROR = %d\n",not_EOF,ERROR);	
	}

	return newMsg;
}


int send_message(struct hostent *server, MESSAGE_t message){

	int sockfd, n;
	unsigned int length;
	struct sockaddr_in serv_addr, from;
	MESSAGE_DATA_t *aux;
	char buffer[1024];
	
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return -1;
	}	
	
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("ERROR opening socket");
	
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(PORT);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);  

	if (message.data != NULL){
		aux = message.data;
		do{
			bzero(buffer, 1024);
			strcpy(buffer,aux->blockData);
			n = sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
			if (n < 0) 
				printf("ERROR sendto");
			
			length = sizeof(struct sockaddr_in);
			n = recvfrom(sockfd, buffer, 256, 0, (struct sockaddr *) &from, &length);
			if (n < 0)
				printf("ERROR recvfrom");

			printf("Got an ack: %s\n", buffer);
			aux = aux->Next_Block;

		}while(aux != NULL);	
		close(sockfd);
		return 0;
	}else return -1; //no data to send.

	
}

int main(int argc, char *argv[])
{
	FILE *fp = NULL;
	int i = 0;
	struct hostent *server;
	MESSAGE_t message; //mensagem a ser enviada.
	char buffer[256];
	
	if (argc < 2) {
		fprintf(stderr, "usage %s hostname\n", argv[0]);
		exit(0);

	}
	
	server = gethostbyname(argv[1]);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
    	}	
	
	printf("Enter the name of the file to send: ");
	//bzero(buffer, 256);
	fgets(buffer, 32, stdin);
	//puts(buffer);
	for(i = 0; i < 32; i++){
		if(buffer[i] == '\0'){
			if(i>0){
				buffer[i-1]='\0';
			}	
		}	
	}
	fp = fopen(buffer,"r");
	if(fp == NULL){
		//printf("Erro ao abrir o arquivo\n");
		return 0;		
	}else {/*printf("Arquivo aberto com sucesso\n");*/}

	message = create_message(fp);

	//print_msg(message.data);
	
	send_message(server,message);
}
