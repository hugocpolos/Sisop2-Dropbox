#ifndef SERVER_CODE
#define SERVER_CODE
#include "sync-server.h"
#include "dropboxServer.h"
#include "watcher.h"

// Uso de variaveis globais para simplificar
ServerInfo serverInfo;
sem_t semaforo;
ClientList listaClientes; 
int syncro;
int server_primario;

void enviar_novoaddress(){
	int index = 0;
	
}


void sincronilis(){
    while(1){
       sleep(5);
       syncro = 1;
       sleep(5);
       syncro = 0;
    }
}
void receiveArquivo(char* nomeArq, int sockid, int id) {
	int funcaoRetorno;
	int bytesRecebidos;
	char caminho[MAXNAME*4];
    char clienteNome[MAXNAME];
	int tamanhoArq;
	Frame pacote;
	struct sockaddr_in cli_addr;		
	socklen_t clilen = sizeof(struct sockaddr_in);

    //Copia nome de usuario
    strcpy(clienteNome, listaClientes->client->userid);

	sprintf(caminho, "%s/%s/%s", serverInfo.folder, clienteNome, nomeArq);
	printf("    Recebendo arquivo = %s", caminho);
	FILE* arquivo;
	arquivo = fopen(caminho, "wb");

	if(arquivo) {
		// Recebimento de info do tamanho do arquivo
		funcaoRetorno = recvfrom(sockid, &pacote, sizeof(pacote), 0, (struct sockaddr *) &cli_addr, &clilen);
		if (funcaoRetorno < 0) 
			printf("Erro em receive \n");
		
		tamanhoArq = atoi(pacote.buffer);
		if(tamanhoArq == 0) {
			fclose(arquivo);
			return;
		}

		bzero(pacote.buffer, BUFFER_SIZE -1);		
		pacote.ack = TRUE;

		// Recimento do arquivo em partes de tamanho maximo do buffer
		bytesRecebidos = 0;
		while(tamanhoArq > bytesRecebidos) {
			funcaoRetorno = recvfrom(sockid, &pacote, sizeof(pacote), 0, (struct sockaddr *) &cli_addr, &clilen);
			if (funcaoRetorno < 0) 
				printf("Erro em receive \n");

			if((tamanhoArq - bytesRecebidos) > BUFFER_SIZE) {
				fwrite(pacote.buffer, sizeof(char), BUFFER_SIZE, arquivo);
				bytesRecebidos += sizeof(char) * BUFFER_SIZE; 
			}
			else {
				fwrite(pacote.buffer, sizeof(char), (tamanhoArq - bytesRecebidos), arquivo);
				bytesRecebidos += sizeof(char) * (tamanhoArq - bytesRecebidos);
			}
			printf("\n     Recebimento de arquivo < %s > ==> Concluido: %d %% (%d / %d) ", nomeArq, bytesRecebidos*100/tamanhoArq, bytesRecebidos, tamanhoArq);
		} 
		printf("\n     Concluido recebimento arquivo = %s", nomeArq);
		fclose(arquivo);
	}
	else
		printf("Erro ao abrir o arquivo = %s", caminho);
}

void sendArquivo(char *nomeArq, int sockid, int id, struct sockaddr_in *cli_addr) {
	char caminho[MAXNAME*4];
	int bytesEnviados;
        int bytesEnvAux;
	int tamanhoArq;
	int funcaoRetorno;
    char clienteNome[MAXNAME];
	Frame pacote;

    //Copia nome de usuario
    strcpy(clienteNome, listaClientes->client->userid); 

	sprintf(caminho, "%s/%s/%s", serverInfo.folder, clienteNome, nomeArq);
	printf("    Envio de arquivo = %s", caminho);

	FILE* arquivo;
	arquivo = fopen(caminho, "rb");
	
	if(arquivo) {
		tamanhoArq = getFilesize(arquivo);
		if(tamanhoArq == 0) {
			fclose(arquivo);
			return;
		}
		sprintf(pacote.buffer, "%d", tamanhoArq);
		
		// Envia info de tamanho do arquivo
		funcaoRetorno = sendto(sockid, &pacote, sizeof(pacote), 0,(struct sockaddr *) cli_addr, sizeof(struct sockaddr));
		if (funcaoRetorno < 0) 
			printf("Erro em send inf tam arq\n");

		// Envia arquivo em partes de tamanho maximo do buffer
		bytesEnviados = 0;
		while(!feof(arquivo)) {
			fread(pacote.buffer, sizeof(char), BUFFER_SIZE, arquivo);
			bytesEnviados += sizeof(char) * BUFFER_SIZE;

			funcaoRetorno = sendto(sockid, &pacote, sizeof(pacote), 0,(struct sockaddr *) cli_addr, sizeof(struct sockaddr));
			if (funcaoRetorno < 0) 
				printf("Erro em send\n");

			bytesEnvAux = bytesEnviados*100/tamanhoArq;
                        if (bytesEnvAux > 100) bytesEnvAux = 100;
			printf("\n     Enviando arquivo < %s > ==> Concluido: %d %% (%d / %d) ", nomeArq, bytesEnvAux , bytesEnviados, tamanhoArq);
		}
		printf("\n     Concluido envio de arquivo = %s\n", nomeArq);
		fclose(arquivo);
	}
	else
		printf("Erro ao abrir o arquivo %s\n", caminho);
}


int openServidor(char *endereco, int port) {

	struct sockaddr_in server;
	int sockid;

	// Inicia estrutura de socket e servidor
  	bzero((char *) &server, sizeof(server));
	server.sin_family = AF_INET; 
	server.sin_port = htons(port); 
	server.sin_addr.s_addr = inet_addr(endereco);
	sprintf(serverInfo.folder, "%s/%s", getUsuario(), SERVER_FOLDER);
	strcpy(serverInfo.ip, endereco);
	serverInfo.port = port;

  	// Cria socket
	sockid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
 	if (sockid == ERROR) {
		printf("Error opening socket\n");
		return ERROR;
	}
	
	// Faz o bind
  	if(bind(sockid, (struct sockaddr *) &server, sizeof(server)) == ERROR) { 
    		perror("Falha na nomeação do socket\n");
    		return ERROR;
  	}


	return sockid;	
}

int novaPortaServidor(char *endereco, Connection* connection) {
	struct sockaddr_in server;
	int sockid;
	int novaPorta = DYN_PORT_START;
	
  	// Cria socket
	sockid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
 	if (sockid == ERROR) {
		printf("Error opening socket\n");
		return ERROR;
	}

	// Nova config servidor de acordo com a nova porta
	bzero((char *) &server, sizeof(server));
	server.sin_family = AF_INET; 
	server.sin_port = htons(novaPorta); 
	server.sin_addr.s_addr = inet_addr(endereco);
	
	// Tenta fazer o bind do socket com a porta ate achar uma disponivel
  	while (bind(sockid, (struct sockaddr *) &server, sizeof(server)) == ERROR) { 
		novaPorta++;
		server.sin_port = htons(novaPorta);
		if(novaPorta > DYN_PORT_END) {
			perror("Falha no bind do socket\n");
			return ERROR;
		}
  	}

	printf("\n     Nova porta = %d - Socket = %d \n", novaPorta, sockid); //DEGBUG
	connection->socket_id = sockid;
	connection->port = novaPorta;

	return SUCCESS;	
}

void selecionaComando(Frame* pacote, struct sockaddr_in *cli_addr, int socket) {
	int funcaoRetorno;
	socklen_t clilen = sizeof(struct sockaddr_in);
	pacote->ack = TRUE;

	// Requisicao de upload
	if(strcmp(pacote->buffer, UP_REQ) == 0) {
		strcpy(pacote->buffer, F_NAME_REQ);
		// Pede nomeArq
		funcaoRetorno = sendto(socket, pacote, sizeof(*pacote), 0,(struct sockaddr *) cli_addr, sizeof(struct sockaddr));
		if (funcaoRetorno < 0) 
			printf("Erro em send nome req upload! \n");

		funcaoRetorno = recvfrom(socket, pacote, sizeof(*pacote), 0, (struct sockaddr *) cli_addr, &clilen);
		if (funcaoRetorno < 0) 
			printf("Erro em receive nome req upload! \n");
					
		char nomeArq[MAXNAME];
		sprintf(nomeArq, "%s", pacote->buffer);
		receiveArquivo(nomeArq, socket, atoi(pacote->user));	
	} 
	// Requisicao de downlaod
	else if(strcmp(pacote->buffer, DOWN_REQ) == 0) {
		strcpy(pacote->buffer, F_NAME_REQ);
		// Pede nomeArq 
		funcaoRetorno = sendto(socket, pacote, sizeof(*pacote), 0,(struct sockaddr *) cli_addr, sizeof(struct sockaddr));
		if (funcaoRetorno < 0) 
			printf("Erro em send nome arq down! \n");

		funcaoRetorno = recvfrom(socket, pacote, sizeof(*pacote), 0, (struct sockaddr *) cli_addr, &clilen);
		if (funcaoRetorno < 0) 
			printf("Erro em receive nome arq down! \n");

		char nomeArq[MAXNAME];
		sprintf(nomeArq, "%s", pacote->buffer);
		sendArquivo(nomeArq, socket, atoi(pacote->user), cli_addr);		
	}

}

void* threadCliente(void* structConexao) {      
	//thread de controle
	int socket;
	char *clienteIP;
	char client_id[MAXNAME];
	struct sockaddr_in cli_addr;
    int funcaoRetorno;	
	socklen_t clilen = sizeof(struct sockaddr_in);
	
	// Configuracao
	Connection *connection = (Connection*) malloc(sizeof(Connection));
	Client *client = (Client*) malloc(sizeof(Client));
	connection = (Connection*) structConexao;
	socket = connection->socket_id;
	clienteIP = connection->ip;
	strncpy(client_id, connection->client_id, MAXNAME);
	client_id[MAXNAME - 1] = '\0';

	// Procura pelo cliente na lista de clientes
	client = procuraCliente(client_id, listaClientes);
	
	if (client == NULL) {
		// Cria um novo nodo para o cliente e o coloca na lista de clientes
		listaClientes = adicionaCliente(client_id, socket, listaClientes);
		client = procuraCliente(client_id, listaClientes);
		// Imprime lista de clientes ativos
		imprimeListaUsuarios(listaClientes);
	} else {
		// Senao cria novo device pro cliente
		newDevice(client, socket);
	}

	if(client->devices[0] > -1 || client->devices[1] > -1) {
		char client_folder[5*MAXNAME];

		sprintf(client_folder, "%s/%s", serverInfo.folder, client_id);
		if(verificaDir(client_folder) == FALSE) {
			if(mkdir(client_folder, 0777) != SUCCESS) {
				printf("Erro ao criar pasta do cliente = '%s'.\n", client_folder);
				return NULL;
			}
		}

		client->n_files = getInfoDirArq(client_folder, client->files);
		imprimeArqsCliente(client);

		// Inicia sincronizacao
		sync_server(socket, client);

		int connected = TRUE;
		Frame pacote;

		// Espera por algum comando
		while(connected == TRUE) {
			printf("\n Esperando por comando do cliente = %s  Porta = %d | Socket = %d\n", client_id, connection->port, socket);
			bzero(pacote.buffer, BUFFER_SIZE -1);
       
			funcaoRetorno = recvfrom(socket, &pacote, sizeof(pacote), 0, (struct sockaddr *) &cli_addr, &clilen);
			if (funcaoRetorno < 0) 
				printf("Erro em receive\n");
			
			if(strcmp(pacote.buffer, END_REQ) == 0) {
				connected = FALSE;
				sem_post(&semaforo);
			}
			else {
				selecionaComando(&pacote, &cli_addr, socket);
			}
		}
	}
	
	return 0;
}

void sync_server(int sock_s, Client *client_s) {
	synchronize_client(sock_s, client_s);
	synchronize_server(sock_s, client_s, serverInfo);
}


// Conecta servidor a algum cliente
void esperaConexao(char* endereco, int sockid) {	
	char *client_ip;
	int  funcaoRetorno;
	unsigned int frontEnd_port;
	socklen_t clilen;
	Frame pacote_server, pacote;
	
	// Identificador pra thread que vai controlar maximo de acessos
	pthread_t thread_id;

	struct sockaddr_in cli_addr;
	struct sockaddr_in cli_front;
	clilen = sizeof(struct sockaddr_in);
    // Controla ack recebidos pelo servidor
	pacote_server.ack = FALSE; 
	while (TRUE) {
		funcaoRetorno = recvfrom(sockid, &pacote, sizeof(pacote), 0, (struct sockaddr *) &cli_addr, &clilen);
		if (funcaoRetorno < 0) 
			printf("Erro em receive \n");
		printf("     Iniciou a conexao com um cliente");
		
		frontEnd_port = atoi(pacote.buffer);
		printf("\nPorta recebida de front end: %d", frontEnd_port);

		//sleep(1);

		bzero(pacote.buffer, BUFFER_SIZE -1);		
		strcpy(pacote.buffer, "Recebimento de msg\n");
		strcpy(pacote_server.user, SERVER_USER);
		pacote.ack = TRUE; pacote_server.ack = TRUE; 

		bzero((char *) &cli_front, sizeof(cli_front));
		cli_front.sin_family = cli_addr.sin_family;
		cli_front.sin_port = htons(frontEnd_port);
		cli_front.sin_addr.s_addr = cli_addr.sin_addr.s_addr;

		

		/*
			Envio de teste no socket de front End:
		funcaoRetorno = sendto(sockid, &pacote, sizeof(pacote), 0,(struct sockaddr *) &cli_front, sizeof(struct sockaddr));
			if (funcaoRetorno < 0) 
				printf("Erro em send frontend \n");
		*/


		// Atualiza semafoto quando uma nova conexao comeca
		sem_wait(&semaforo);
		
		// inet_ntoa converte o endereco de rede em string
      	client_ip = inet_ntoa(cli_addr.sin_addr); 

		// Abre nova conexao com um cliente
		Connection *connection = malloc(sizeof(*connection));
		
		if (novaPortaServidor(endereco, connection) == SUCCESS) {
			connection->ip = client_ip;
			// Pega info do cliente
			strcpy(connection->client_id, pacote.user);

			// Cria nova thread para controlar acesso do cliente no servidor
			if(pthread_create(&thread_id, NULL, threadCliente, (void*) connection) < 0)
				printf("Erro ao criar a thread! \n");                        
                                                                       
			// Enviar nova porta para o cliente
			sprintf(pacote.buffer, "%d", connection->port);
			strcpy(connection->buffer, pacote.buffer);
			funcaoRetorno = sendto(sockid, &pacote, sizeof(pacote), 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
			if (funcaoRetorno < 0) 
				printf("Erro em send \n");
		}
		else printf("\n Erro ao criar nova conexao para o cliente! \n");
	}
}

int main(int argc, char *argv[]) {

	int port, sockid;
	char *endereco;

	// Inicia semaforo para admitir um determinado numero de clientes */
	sem_init(&semaforo, 0, MAX_CLIENTS);

	if (argc > 4) {
		puts("Numero de argumentos nao e valido!");
		return ERROR;
	}

	endereco = malloc(strlen(DEFAULT_ADDRESS));	

	// Parsing
	if (argc == 4) {
		if (strlen(argv[1]) != strlen(DEFAULT_ADDRESS)) {
			free(endereco);
			endereco = malloc(strlen(argv[1]));
		}  
		strcpy(endereco, argv[1]);
		port = atoi(argv[2]);
		if ( strcmp(argv[3], "--p") == 0 ){
			server_primario = 1;		
		}
			
	} else {
		strcpy(endereco, DEFAULT_ADDRESS);
		port = DEFAULT_PORT;
		server_primario = 0;
	}

	if(server_primario){
		printf("\nServer primario\n");	
	}else{
		printf("\nServer secundario\n");	
	}
	// Criando socket e config bind
	sockid = openServidor(endereco, port);
	if (sockid != ERROR) {
		printf("\n*************************Informacoes do Servidor*************************\n");
                printf("\n             Aguardando conexao de um cliente ... \n");
                printf("     Endereco = %s   Porta Principal = %d   Socket Principal = %d\n", endereco, port, sockid);
		// Verifica se diretorio servidor existe, senao cria usando mkdir
		if(verificaDir(serverInfo.folder) == FALSE) {
			if(mkdir(serverInfo.folder, 0777) != SUCCESS) {
				printf("Erro ao criar pasta do servidor = '%s'.\n", serverInfo.folder);
				return ERROR;
			}
		}                	                	
		esperaConexao(endereco, sockid);
	}
	return 0;
}
#endif
