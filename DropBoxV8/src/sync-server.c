#ifndef SYNC_SERVER_CODE
#define SYNC_SERVER_CODE

#include "sync-client.h"

void synchronize_client(int sockid, Client* client_sync) { 

	char buffer[BUFFER_SIZE];	
	int status = 0;
	bool flag = FALSE;
	Frame packet;
	socklen_t clilen;

	struct sockaddr_in cli_addr;
	clilen = sizeof(struct sockaddr_in);

	printf("\n     Iniciando sincronização do cliente.\n");	//debug

	/* Getting an ACK */
	/* SYNC	*/
	do {
		status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen);
		if (strcmp(packet.buffer, S_NSYNC) == 0)
			strcpy(packet.buffer, S_SYNC);

		status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &cli_addr, sizeof(struct sockaddr_in));

	}while (strcmp(packet.buffer, S_SYNC) != 0);

	if (status < 0) {
		printf("ERROR reading from socket in sync-server client\n");
	}

	sprintf(packet.buffer, "%d", client_sync->n_files);
	packet.ack = FALSE; strcpy(packet.user, SERVER_USER);
	printf("     Numero de arquivos do cliente: %d.\n", client_sync->n_files);	//debug

	/* Writes the number of files in server */
	while(packet.ack != TRUE){
		status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &cli_addr, sizeof(struct sockaddr_in));
		status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen); 
	}
	if (status < 0) {
	    	printf("ERROR writing to socket in sync-server client\n");
	}
		

	for(int i = 0; i < client_sync->n_files; i++) {
			strcpy(packet.user, SERVER_USER);
			packet.ack = FALSE;

		    strcpy(packet.buffer, client_sync->files[i].name);
		    //printf("     Nome do arquivo a enviar: %s\n", client_sync->files[i].name);	//debug
		    
		    /* Sends the file name to client */
		    while(packet.ack != TRUE) {
		    	status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &cli_addr, sizeof(struct sockaddr_in));
		    	status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen);
			}

		    if (status < 0) {
		      	printf("ERROR writing to socket in sync-server client\n");
		    }
		    packet.ack = FALSE;

		    strcpy(packet.buffer, client_sync->files[i].last_modified);
		    //printf("     Ultima modificacao: %s\n", client_sync->files[i].last_modified);	//debug
		
		    /* Sends the file's last modification */
		    while(packet.ack != TRUE) {
		    	status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &cli_addr, sizeof(struct sockaddr_in));
		    	status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen);
			}

		    if (status < 0) {
		      	printf("ERROR writing to socket in sync-server client\n");
		    }
		    packet.ack = FALSE;
		    
		    do{
		      status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen);
		      if (strcmp(packet.buffer, S_OK) == 0) {
			packet.ack = TRUE; flag = TRUE;
		      }
		      else {
			flag = TRUE; packet.ack = TRUE;
			strcpy(buffer, packet.buffer);
			strcpy(packet.buffer, S_OK);
		       }
		      status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &cli_addr, sizeof(struct sockaddr_in));

		    }while(flag != TRUE);		

		    if (status < 0) {
		      	printf("ERROR reading from socket\n");
		    }
		    //printf("     Recebido: %s\n", buffer);
		    if(strcmp(buffer, DOWN_REQ) == 0){ 
		      	sendArquivo(client_sync->files[i].name, sockid, atoi(client_sync->userid), &cli_addr);
		    }
	  }

	  printf("     Encerrando sincronização do cliente.\n");
}

void synchronize_server(int sockid_sync, Client* client_sync, ServerInfo serverInfo) {

	char buffer[BUFFER_SIZE]; // 1 KB buffer
	char path[MAXNAME * 3 + 1];

	char last_modified[MAXNAME];
	char last_modified_file_2[MAXNAME];
	char file_name[MAXNAME];
	
	int  number_files_client = 0;
	
	int status = 0;
	bool flag = FALSE;
	Frame packet;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
	
	int sockid = sockid_sync;


	printf("     Iniciando sincronização do servidor.\n");	//debug

	/* Reads number of client's files */
	do {
		status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen);
		packet.ack = TRUE;
		status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &cli_addr, sizeof(struct sockaddr_in));

	}while (packet.ack != TRUE);


	number_files_client = atoi(packet.buffer);
	printf("     Numero de arquivos do cliente no servidor: %d\n", number_files_client);

	for(int i = 0; i < number_files_client; i++) {

		/* Reads the file name from client */
	    do {
			status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen);
			if (status >= 0){
				packet.ack = TRUE;
				strcpy(packet.user, SERVER_USER);
				status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &cli_addr, sizeof(struct sockaddr_in));
			}	    	
		}while(packet.ack != TRUE);

		if (status < 0) {
			printf("ERROR reading from socket\n");		//debug
		}

	    strcpy(file_name, packet.buffer);
	    //printf("     Nome recebido: %s\n", file_name);		//debug
		
		/* Reads last modified from client */
		do {
			status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen);
			if (status >= 0){
				packet.ack = TRUE;
				strcpy(packet.user, SERVER_USER);
				status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &cli_addr, sizeof(struct sockaddr_in));
			}
		}while(packet.ack != TRUE);

	     
	    if (status < 0) {
	    	printf("ERROR reading from socket\n");
	    }
	    strcpy(last_modified, packet.buffer);
	    //printf("     Ultima modificacao recebido: %s\n", last_modified);	

    	sprintf(path, "%s/%s/%s", serverInfo.folder, client_sync->userid, file_name);
    	getModificado(path, last_modified_file_2);
		packet.ack = FALSE;

    	if((verificaDir(path) == FALSE) || arqAntigo(last_modified, last_modified_file_2) == 1) {

      		strcpy(packet.buffer, S_GET);
			do{
				status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &cli_addr, sizeof(struct sockaddr_in));
				status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen);
			} while (packet.ack == FALSE);
	      		
      		if (status < 0) {
				printf("ERROR writing to socket\n");
			}

	      	//printf("     Recebido: %s\n", packet.buffer);	//buffer

	      	if(strcmp(packet.buffer, UP_REQ) == 0) {
				//upload(sockid_sync, client_sync);	//interface
	      	}
	    } else {
	  		strcpy(packet.buffer, S_OK); packet.ack = FALSE;
			do{
				status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &cli_addr, sizeof(struct sockaddr_in));
				status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen);
			} while (packet.ack == FALSE);

	  		
	      	if (status < 0) {
				printf("ERROR writing to socket\n");	//debug
	      	}
	    }
	}

	printf("     Encerrando sincronização do servidor.\n");		//debug
}

#endif
