#ifndef SYNC_CLIENT_CODE
#define SYNC_CLIENT_CODE


/* Defining constants to the watcher thread */

#include "sync-client.h"


void synchronize_local(UserInfo user) {

	char path[MAXPATH];
	char file_name[MAXNAME];
	char last_modified[MAXNAME];
	char last_modified_file_2[MAXNAME];

	int number_files_server = 0;
	int status;
	unsigned int length;

	int sockid = user.socket_id;

	struct sockaddr_in* serv_addr = user.serv_conn;
	struct sockaddr_in from, to;
	Frame packet;
	length = sizeof(struct sockaddr_in);

	//printf(" Inicializando sincronizacao cliente\n");		//debug
	strcpy(packet.buffer, S_NSYNC);
	packet.ack == FALSE;
	
	/* Getting an ACK */
	/* SYNC	*/
	while (strcmp(packet.buffer, S_SYNC) != 0) {

		status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));

		status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &to, &length);
	}
	if (status < 0) {
			printf("ERROR reading from socket in sync-client local\n");
	}

	do {
		/* Receive the number of files at server */
		status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		if(strcmp(packet.user, SERVER_USER)==0){
			packet.ack = TRUE;
			status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
		}
	}while(packet.ack != TRUE);
	
	if (status < 0) {
		printf("ERROR reading from socket in sync-client local\n");
	}

	number_files_server = atoi(packet.buffer);
	//printf("%d arquivos no servidor\n", number_files_server); //debug

	for(int i = 0; i < number_files_server; i++) {
		do { 
		    /* Receives file name from server */
		    status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		    if(strcmp(packet.user, SERVER_USER)==0){
			 packet.ack = TRUE;
			 status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
		    }

		}while(packet.ack != TRUE);

		if (status < 0) {
			printf("ERROR reading from socket in sync-client\n");
		}

		strcpy(file_name, packet.buffer);
		printf(" Nome recebido: %s\n", file_name);			//debug

		do {
		    /* Receives the date of last file modification from server */
		    status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		    if(strcmp(packet.user, SERVER_USER)==0){
			 packet.ack = TRUE;
			 status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
		    }

		}while(packet.ack != TRUE); 

		if (status < 0) {
			printf("ERROR reading from socket in sync-client\n");	//debug
		}

		strcpy(last_modified, packet.buffer);
		printf(" Ultima modificacao: %s\n", last_modified);			//debug

		sprintf(path, "%s/%s", user.folder, file_name);

		/* Function to acquire modification time of sync file */
		getModificado(path, last_modified_file_2);

		if(verificaDir(path) == FALSE) {;					
			printf("Arquivo %s nao existe... Baixando\n", file_name);	//debug
			getArquivoCliente(file_name);

		} else if (arqAntigo(last_modified, last_modified_file_2) == SUCCESS) {
			printf("Arquivo %s antigo... Baixando\n", file_name);	//debug
			getArquivoCliente(file_name);

		} else {
			strcpy(packet.buffer, S_OK);
			do { /* ACK -> confirming OK! */
				status=sendto(sockid,&packet,sizeof(packet),0,(const struct sockaddr *) serv_addr,sizeof(struct sockaddr_in));
				status=recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);

			}while(strcmp(packet.buffer, S_OK) != 0 || (packet.ack == FALSE));

			if (status < 0) {
				printf("ERROR writing to socket in sync-client\n");
			}
		}	
	}

	//printf(" Encerrando sincronizacao do cliente\n");		//debug
}


void synchronize_remote(UserInfo user) {

	FileInfo localFiles[MAXFILES];
	char path[MAXPATH];
	int number_files_client = 0;
	int status;

	Frame packet;
	struct sockaddr_in from;

	int sockid = user.socket_id;

	struct sockaddr_in* serv_addr = user.serv_conn;
	unsigned int length = sizeof(struct sockaddr_in);

	//printf(" Iniciando sincronizacao do servidor\n");	//debug

	number_files_client = getInfoDirArq(user.folder, localFiles);
	sprintf(packet.buffer, "%d", number_files_client);
	packet.ack = FALSE;

	/* Getting an ACK */
	while (packet.ack == FALSE) {

		/* Sends the number of files to server */
		status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
		status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length); 
		if (status < 0) {
			printf("ERROR reading from socket in sync-client remote\n");
		}
	}
	packet.ack = FALSE;
	for(int i = 0; i < number_files_client; i++) {

		strcpy(packet.buffer, localFiles[i].name);
		printf(" Nome enviado: %s\n", localFiles[i].name);	//debug

		do { /* ACK */
			/* Sends file's name to server */
			status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
			status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		}while(packet.ack != TRUE || (strcmp(packet.user, SERVER_USER) != 0));

		if (status < 0) {
			printf("ERROR writing to socket\n");
		}

		strcpy(packet.buffer, localFiles[i].last_modified);
		printf(" Ultima modificacao: %s\n", localFiles[i].last_modified);	//debug
		packet.ack = FALSE;

		do { /* ACK */
			/* Sends last modified to server */
			status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
			status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		}while(packet.ack != TRUE || (strcmp(packet.user, SERVER_USER) != 0));

		if (status < 0) {
			printf("ERROR writing to socket on sync client\n");	//debug
		}
		
		packet.ack = FALSE;
		do { /* ACK */
			/* Reads from server */
			status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
			
			packet.ack = TRUE;
			status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
		
		}while(packet.ack != TRUE);

		if (status < 0) {
			printf("ERROR reading from socket\n");
		}
		printf(" Recebido: %s\n", packet.buffer);	//debug

		if(strcmp(packet.buffer, S_GET) == 0) {
			sprintf(path, "%s/%s", user.folder, localFiles[i].name);
			//send_file(path, FALSE);					//interface implementation
			printf(" Enviando arquivo %s\n", path);	//debug
		}
	}

	//printf(" Encerrando sincronizacao servidor\n");		//debug
}

#endif
