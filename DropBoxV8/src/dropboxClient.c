#include "dropboxClient.h"
#include "sys/socket.h" 
#include "arpa/inet.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

UserInfo user;
int ID_MSG_CLIENT = 0;
pthread_t sync_thread;


void deleteArquivoCliente(char *arquivo) {
	// Preenche estrutura pacote
	Frame pacote;
	bzero(pacote.user, MAXNAME-1);
	strcpy(pacote.user, user.nome);
	bzero(pacote.buffer, BUFFER_SIZE -1);
	pacote.ack = FALSE;
	int socketID, funcaoRetorno;

	char nomeArq[MAXNAME];

	struct sockaddr_in serv_conn;

	// Envia requisicao de delecao para servidor
	strcpy(pacote.buffer, DEL_REQ);
	funcaoRetorno = sendto(socketID, &pacote, sizeof(pacote), 0, (const struct sockaddr *) &serv_conn, sizeof(struct sockaddr_in));
	if (funcaoRetorno < 0) {
		printf("Erro em send requisicao delecao \n");
	}

	// Recebimento de ack do servidor
	struct sockaddr_in from;
	unsigned int tamanho = sizeof(struct sockaddr_in);
	funcaoRetorno = recvfrom(socketID, &pacote, sizeof(pacote), 0, (struct sockaddr *) &from, &tamanho);
	if (funcaoRetorno < 0) {
		printf("Erro em receive ack delete \n");
	}

	if(pacote.ack == FALSE) {
		printf("\n Requisicao de delecao negava pelo servidor. \n");
	}

	// Envio de info do nome do arq para servidor
	if(strcmp(pacote.buffer, F_NAME_REQ) == 0) {
		strcpy(pacote.buffer, nomeArq);
		printf("Delecao de arquivo = %s\n", pacote.buffer); //DEBUG
		funcaoRetorno = sendto(socketID, &pacote, sizeof(pacote), 0, (const struct sockaddr *) &serv_conn, sizeof(struct sockaddr_in));
		if (funcaoRetorno < 0) {
			printf("Erro em send requisicao nome delecao! \n");
		}
	}	

	// Recebimento de ack do servidor
	funcaoRetorno = recvfrom(socketID, &pacote, sizeof(pacote), 0, (struct sockaddr *) &from, &tamanho);
	if (funcaoRetorno < 0) {
		printf("Erro em receive delete ack! \n");
	}
	if(strcmp(pacote.buffer, DEL_COMPLETE) == 0){
		printf("Arquivo removido!\n");
	}

}

void sendArquivoCliente(char *nomeArq) {
	int funcaoRetorno;
	char caminho[MAXNAME*4];
	Frame pacote;
	int bytesEnviados;
        int bytesEnvAux;
	int tamanhoArq;
	int socketID = user.socket_id;
	struct sockaddr_in* serv_conn = user.serv_conn;
    
	// Preenche a estrutura pacote 
	sprintf(caminho, "%s/%s", user.folder, nomeArq);
	bzero(pacote.user, MAXNAME-1);
	strcpy(pacote.user, user.nome);
	bzero(pacote.buffer, BUFFER_SIZE -1);
	pacote.ack = FALSE;

	printf("\n Pedindo requisicao de upload na PORTA = %d , SOCKET = %d \n", ntohs(serv_conn->sin_port), socketID);
	
	// Envia pedido de upload para o servidor
	strcpy(pacote.buffer, UP_REQ);
	funcaoRetorno = sendto(socketID, &pacote, sizeof(pacote), 0, (const struct sockaddr *) serv_conn, sizeof(struct sockaddr_in));
	if (funcaoRetorno < 0) {
		printf("Erro em send requisicao upload! \n");
		return;
	}

	// Recebimento de ACK de requisicao de upload do servidor
	struct sockaddr_in from;
	unsigned int tamanho = sizeof(struct sockaddr_in);
	funcaoRetorno = recvfrom(socketID, &pacote, sizeof(pacote), 0, (struct sockaddr *) &from, &tamanho);
	if (funcaoRetorno < 0) {
		printf("ERROR receive ack upload servidor! \n");
		return;
	}

	// Se ACK falso, entao foi negado pelo servidor
	if(pacote.ack == FALSE) {
		printf("\n Requisicao de upload foi negada pelo servidor. \n");
		return;
	}

	// Processo de envio de arquivo
	if(strcmp(pacote.buffer, F_NAME_REQ) == 0) {
		strcpy(pacote.buffer, nomeArq);
		printf("\n Envio de arquivo = %s \n", pacote.buffer);
		funcaoRetorno = sendto(socketID, &pacote, sizeof(pacote), 0, (const struct sockaddr *) serv_conn, sizeof(struct sockaddr_in));
		if (funcaoRetorno < 0) {
			printf("Erro em send requisicao nome upload! \n");
			return;
		}
	}
	FILE* arquivo;
	arquivo = fopen(caminho, "rb");

	if(arquivo) {
		tamanhoArq = getFilesize(arquivo);
		if(tamanhoArq == 0) {
			fclose(arquivo);
			printf("Arquivo vazio! \n");
			return;
		}		
		sprintf(pacote.buffer, "%d", tamanhoArq);

		// Envia info de tamanho do arquivo para o servidor
		funcaoRetorno = sendto(socketID, &pacote, sizeof(pacote), 0,(struct sockaddr *) serv_conn, sizeof(struct sockaddr));
		if (funcaoRetorno < 0) 
			printf("Erro em send info tam arq up! \n");
	
		// Envia o arquivo em partes do tamanho maximo do buffer
		bytesEnviados = 0;
		while(!feof(arquivo)) {
			fread(pacote.buffer, sizeof(char), BUFFER_SIZE, arquivo);
			bytesEnviados += sizeof(char) * BUFFER_SIZE;

			funcaoRetorno = sendto(socketID, &pacote, sizeof(pacote), 0,(struct sockaddr *) serv_conn, sizeof(struct sockaddr));
			if (funcaoRetorno < 0) 
				printf("Erro em send buffer arq! \n");
            //imprime status de envio
			bytesEnvAux = bytesEnviados*100/tamanhoArq;
                        if (bytesEnvAux > 100) bytesEnvAux = 100;
			printf("\n Enviando arquivo < %s > ==> Concluido: %d %% (%d / %d) ", caminho, bytesEnvAux,bytesEnviados, tamanhoArq);
		}
		printf("\n Concluido envio de arquivo = %s\n", caminho);
		fclose(arquivo);
	}
	else
		printf("\nErro ao abrir o arquivo = %s\n", caminho);
}

void getArquivoCliente(char *nomeArq) {
	int funcaoRetorno;
	int bytesRecebidos;
	Frame pacote;
    int tamanhoArq;
	int socketID = user.socket_id;
	struct sockaddr_in* serv_conn = user.serv_conn;

	// Preenche a estrutura pacote 
	bzero(pacote.user, MAXNAME-1);
	strcpy(pacote.user, user.nome);
	bzero(pacote.buffer, BUFFER_SIZE -1);
	pacote.ack = FALSE;

	// Envia requisicao de download para o servidor
	strcpy(pacote.buffer, DOWN_REQ);
	funcaoRetorno = sendto(socketID, &pacote, sizeof(pacote), 0, (const struct sockaddr *) serv_conn, sizeof(struct sockaddr_in));
	if (funcaoRetorno < 0) {
		printf("Erro em send requisicao download!\n");
		return;
	}

	// Recebimento de ack de requisicao de download do servidor
	struct sockaddr_in from;
	unsigned int tamanho = sizeof(struct sockaddr_in);
	funcaoRetorno = recvfrom(socketID, &pacote, sizeof(pacote), 0, (struct sockaddr *) &from, &tamanho);
	if (funcaoRetorno < 0) {
		printf("Erro em receive ack downlaod servidor! \n");
		return;
	}
    // Se ACK falso, requisicao de download foi negada
	if(pacote.ack == FALSE) {
		printf("\n Requisicao de download foi negada pelo servidor. \n");
		return;
	}
    // Processo de recebimento do arquivo
	if(strcmp(pacote.buffer, F_NAME_REQ) == 0) {
		strcpy(pacote.buffer, nomeArq);
		printf("\n Requerindo arquivo = %s\n", pacote.buffer);
		funcaoRetorno = sendto(socketID, &pacote, sizeof(pacote), 0, (const struct sockaddr *) serv_conn, sizeof(struct sockaddr_in));
		if (funcaoRetorno < 0) {
			printf("Erro em send requisicao nome download\n");
			return;
		}
	}

	char caminho[MAXNAME*4];
	sprintf(caminho, "%s/%s", user.folder, nomeArq);
	printf(" Recebendo arquivo = %s", caminho);
	FILE* arquivo;
	arquivo = fopen(caminho, "wb");

	if(arquivo) {
		// Recebe info de tamanho do arquivo
		funcaoRetorno = recvfrom(socketID, &pacote, sizeof(pacote), 0, (struct sockaddr *) &from, &tamanho);
		if (funcaoRetorno < 0) 
			printf("Erro receive info tam arq down\n");

		tamanhoArq = atoi(pacote.buffer);
		bzero(pacote.buffer, BUFFER_SIZE -1);		
		pacote.ack = TRUE;
		bytesRecebidos = 0;
		
		// Recebe arq em partes ate a conclusao
		while(tamanhoArq > bytesRecebidos) {
			funcaoRetorno = recvfrom(socketID, &pacote, sizeof(pacote), 0, (struct sockaddr *) &from, &tamanho);
			if (funcaoRetorno < 0) 
				printf("Erro receive\n");

			if((tamanhoArq - bytesRecebidos) > BUFFER_SIZE) {
				fwrite(pacote.buffer, sizeof(char), BUFFER_SIZE, arquivo);
				bytesRecebidos += sizeof(char) * BUFFER_SIZE; 
			}
			else {
				fwrite(pacote.buffer, sizeof(char), (tamanhoArq - bytesRecebidos), arquivo);
				bytesRecebidos += sizeof(char) * (tamanhoArq - bytesRecebidos);
			}
			printf("\n Recebendo arquivo < %s > ==> Concluido: %d %% (%d / %d) ", nomeArq, bytesRecebidos*100/tamanhoArq, bytesRecebidos, tamanhoArq); 
		}
		printf("\n Concluido recebimento de arquivo = %s \n", nomeArq);
		fclose(arquivo);
	}
	else
		printf("\nErro ao abrir o arquivo = %s", caminho);
}

int openFrontEnd(int *frontend_port) {

	struct sockaddr_in frontend;
	int sockid;
	int open = 0;

	while(open == 0){
		// Inicia estrutura de socket e servidor
	  	bzero((char *) &frontend, sizeof(frontend));
		frontend.sin_family = AF_INET; 
		frontend.sin_port = htons(*frontend_port); 
		frontend.sin_addr.s_addr = INADDR_ANY;

	  	// Cria socket
		sockid = socket(AF_INET, SOCK_DGRAM, 0);
	 	if (sockid == ERROR) {
			printf("Error opening socket\n");
			(*frontend_port) += 1;
		}
		else{
			// Faz o bind
		  	if(bind(sockid, (struct sockaddr *) &frontend, sizeof(frontend)) == ERROR) { 
					perror("Falha na nomeação do socket\n");
					(*frontend_port) += 1;
		  	}
			else{
				//socket aberto com sucesso. open = 1 encerra o while.
				open = 1;
			}	
		}
	}
	
	

	printf("\n Porta frontEnd: %d", *frontend_port);
	return sockid;	
}


int loginServidor(char *host, int port) {
	struct sockaddr_in serv_conn, from;
	Frame pacote;
	int funcaoRetorno, sock, sock2;
	pthread_t frontendthread_id;
	int frontend_port = 32000;
	char frontend_itoa[8];
	
	unsigned int tamanho;
	
	//Configuração de abertura do socket para transmissão de arquivos
	if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		printf("Erro ao abrir o socket! \n");
	else
		printf("Primeiro cliente socket = %i\n", sock);
	
	user.socket_id = sock;
	
	
	//configuração de abertura do socket do front end
	// frontend_port pode ter sido alterado durante a função openFrontEnd.
	sock2 = openFrontEnd(&frontend_port);
	sprintf(frontend_itoa, "%d", frontend_port);

	//Configuração de conexao IP/Porta
	bzero((char *) &serv_conn, sizeof(serv_conn));
	serv_conn.sin_family = AF_INET;
	serv_conn.sin_port = htons(port);
	serv_conn.sin_addr.s_addr = inet_addr(host);
	user.serv_conn = &serv_conn;

	// Preenche a estrutura pacote 
	bzero(pacote.user, MAXNAME-1);
	strcpy(pacote.user, user.nome);
	bzero(pacote.buffer, BUFFER_SIZE -1);
	strcpy(pacote.buffer, frontend_itoa);
	pacote.message_id = ID_MSG_CLIENT;
	pacote.ack = FALSE;
 
    //Confirmação de conexao, entrega e recebimento
	while((strcmp(user.nome, pacote.user) != 0) || (pacote.ack != TRUE) || (pacote.message_id != ID_MSG_CLIENT)) {	
		//envia 
		funcaoRetorno = sendto(sock, &pacote, sizeof(pacote), 0, (const struct sockaddr *) &serv_conn, sizeof(struct sockaddr_in));
		if (funcaoRetorno < 0) {
			printf("Erro em send confirmacao! \n");
			return ERROR;
		}
		//recebe
		tamanho = sizeof(struct sockaddr_in);
		funcaoRetorno = recvfrom(sock, &pacote, sizeof(pacote), 0, (struct sockaddr *) &from, &tamanho);
		if (funcaoRetorno < 0) {
			printf("Erro em receive confirmacao! \n");
			return ERROR;
		}
	} 
	ID_MSG_CLIENT += 1;
	
	//Atualiza a porta atual com a porta recebida pelo servidor
	serv_conn.sin_port = htons(atoi(pacote.buffer)); 

	// Sincroniza arquivos do usuario no servidor
	sync_client(); 

	//cria a thread de front end
	if(pthread_create(&frontendthread_id, NULL, frontend_thread, NULL) < 0){
		printf("Erro ao criar a thread! \n");
	}
	return SUCCESS;
}

void sync_client() {
	if(verificaDir(user.folder) == FALSE) 
		mkdir(user.folder, 0777);
			
	synchronize_local(user);
	synchronize_remote(user);
}

void close_session() { 
    int funcaoRetorno;
	int socketID;
	struct sockaddr_in serv_conn;
	
	// Preenche estrutura pacote
	Frame pacote;
	bzero(pacote.user, MAXNAME-1);
	strcpy(pacote.user, user.nome);
	bzero(pacote.buffer, BUFFER_SIZE -1);
	pacote.ack = FALSE;

	// Fecha a thread de sinc
	pthread_cancel(sync_thread);

	// Termina thread no servidor
	strcpy(pacote.buffer, END_REQ);
	funcaoRetorno = sendto(socketID, &pacote, sizeof(pacote), 0, (const struct sockaddr *) &serv_conn, sizeof(struct sockaddr_in));
	if (funcaoRetorno < 0) {
		printf("Erro em send reuqisicao end\n");
	}

	//Fecha o socket
	close(user.socket_id);
}

void menuCliente() {
	char linhaComando[MAXPATH];
	char *comando;
	char *prop;
	
	int saiu = FALSE;
	while(!saiu){
		printf("\n Insira um comando: ");

		if(fgets(linhaComando, sizeof(linhaComando)-1, stdin) != NULL) {
			linhaComando[strcspn(linhaComando, "\r\n")] = 0;

			if (strcmp(linhaComando, "exit") == 0) 
				saiu = TRUE;
			else {
				comando = strtok(linhaComando, " ");
				prop = strtok(NULL, " ");
			}
            // download
			if(strcmp(comando, "download") == 0) {
				getArquivoCliente(prop);
			}
			// upload
			else if(strcmp(comando, "upload") == 0) {
				sendArquivoCliente(prop);
			}			
		}
		else
			printf("\n Comando nao e valido! ");
	}

	close_session();
}


void frontend_thread(){
	// estruturas e variaveis para a utilização de select()
	fd_set rfds;
	struct timeval tv;
	int retval;
	// estruturas e variaveis de comunicação em socket udp
	int socketID = user.socket_id;
	struct sockaddr_in* serv_conn = user.serv_conn;
	struct sockaddr_in from;
	char *new_host;
	int new_port;
	char buffer[BUFFER_SIZE];
	unsigned int size = sizeof(struct sockaddr_in);


	

	while(1){

		/* Watch socketID to see when it has input. */
		FD_ZERO(&rfds);
		FD_SET(socketID, &rfds);

		/* Wait up to one second. */
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	
		retval = select(1, &rfds, NULL, NULL, &tv);
	
		if (retval == -1)
			perror("select()");
		else if (retval){
			printf("receiving");
			recvfrom(socketID, &buffer, sizeof(buffer), 0, (struct sockaddr *) &from, &size);
		printf("frontEnd> %s \n", buffer);
		}
		else{
			//printf("No data within one second.\n");
		}
	}

	loginServidor(new_host, new_port);
}

int main(int argc, char *argv[]) {
	int port, socketID;
	char *endereco;

	if (argc != 4) {
		puts("Número de argumentos inválido <usuario> <ip> <porta>");
		return ERROR;
	}

	strcpy(user.nome, argv[1]);
	sprintf(user.folder, "%s/sync_dir_%s", getUsuario(), user.nome);
		
	endereco = malloc(strlen(argv[2]));
	strcpy(endereco, argv[2]);

	port = atoi(argv[3]);
	socketID = loginServidor(endereco, port); 
	menuCliente();
}

