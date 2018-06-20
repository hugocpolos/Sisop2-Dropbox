#ifndef SERVER_CODE
#define SERVER_CODE
#include "sync-server.h"
#include "dropboxServer.h"
#include "watcher.h"

// Uso de variaveis globais para simplificar
ServerInfo serverInfo;
sem_t semaforo;
ClientList listaClientes;
ServerList *listaServidores;
UserFrontEnd lUserFrontEnd = NULL; 
int syncro;
int server_primario;
int eleicao_id;
int eleicao_running = 0;

char endereco_global[16];
int porta_global;


void sincronilis(){
    while(1){
       sleep(5);
       syncro = 1;
       sleep(5);
       syncro = 0;
    }
}

void get_servers(){

	FILE *fp;
	char s_host[16];
	char s_port_char[16];
	char s_primario[8];
	char *ret_val;
	int server_port;
	int primario;
	int i = 0;
	char comp_char = '0';	

	listaServidores = init_serverlist(listaServidores);
	fp = fopen ("sinfo.txt", "r");
	do{
		i = 0;
		comp_char = '0';
		ret_val = fgets(s_primario, sizeof(s_primario), fp);
		if(s_primario[0] == 'p' || s_primario[0] == 'P'){
			primario = 1;
		}else{
			primario = 0;		
		}
		ret_val = fgets(s_host, sizeof(s_host), fp);
		while(comp_char != '\n'){
			comp_char = s_host[i];
			i++;
		}
		s_host[i-1] = '\0';
		ret_val = fgets(s_port_char, sizeof(s_port_char), fp);
		server_port = atoi(s_port_char);
		
		if(ret_val != NULL){
			//printf("\ntipo: %s host %s port %d\n", s_primario, s_host, server_port);
			listaServidores = adiciona_server(primario, s_host, server_port, listaServidores);
		}
	}while(ret_val != NULL);
	fclose(fp);
	//print_listaServidor(listaServidores);
}

void atualiza_primario(char *host_primario, int porta_primario){
	ServerList *aux;

	//iteração: seta todos como primario, e quando encontrar o novo primario seta ele como primario.
	for (aux = listaServidores; aux != NULL;  aux = aux->prox) {
		aux->primario = 0;
		if(strcmp(aux->host, host_primario) == 0 && aux->port == porta_primario){
			aux->primario = 1;
		}	
	}
	return;
}

void envia_novoPrimario_frontend(){
	UserFrontEnd aux;
	int sock;
	Frame pacote;
	int valor_ret;
	struct sockaddr_in conexao;

	
	printf("\niniciando envia_novoPrimario_frontend()\n");	

	aux = lUserFrontEnd;
	if(aux == NULL){
		printf("lista nao existe\n");	
	}
	if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		printf("Erro ao abrir o socket! \n");

	bzero(pacote.user, MAXNAME-1);
	bzero(pacote.buffer, BUFFER_SIZE -1);
	strcpy(pacote.user, endereco_global);
	strcpy(pacote.buffer, "NOVO_LIDER");
	pacote.message_id = porta_global;
	pacote.ack = FALSE;	

	for(aux = lUserFrontEnd; aux != NULL; aux = aux->next){
		

		printf("\nenviando novo primario para o cliente ip: %s porta %d\n", aux->ip, aux->port);
		//prepara a estrutura de destino para cada front-end.
		bzero((char *) &conexao, sizeof(conexao));
		conexao.sin_family = AF_INET;
		conexao.sin_port = htons(aux->port);
		conexao.sin_addr.s_addr = inet_addr(aux->ip);

		valor_ret = sendto(sock, &pacote, sizeof(pacote), 0, (const struct sockaddr *) &conexao, sizeof(struct sockaddr_in));

		if(valor_ret < 0){
			printf("\nerro em send\n");		
		}
	}
	close(sock);
}	

void envia_novoPrimario_servers(){
	ServerList *aux;
	Frame pacote;
	int sock;
	struct sockaddr_in conexao;
	int valor_ret;

	if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		printf("Erro ao abrir o socket! \n");
	
	bzero(pacote.user, MAXNAME-1);
	bzero(pacote.buffer, BUFFER_SIZE -1);
	strcpy(pacote.user, endereco_global);
	strcpy(pacote.buffer, "NOVO_LIDER");
	pacote.message_id = porta_global;
	pacote.ack = FALSE;
	
	for (aux = listaServidores; aux != NULL;  aux = aux->prox) {

		//prepara a estrutura de destino para cada servidor
		bzero((char *) &conexao, sizeof(conexao));
		conexao.sin_family = AF_INET;
		conexao.sin_port = htons(aux->port);
		conexao.sin_addr.s_addr = inet_addr(aux->host);

		valor_ret = sendto(sock, &pacote, sizeof(pacote), 0, (const struct sockaddr *) &conexao, sizeof(struct sockaddr_in));

		if(valor_ret < 0){
			printf("\nerro em send\n");		
		}
	
	}
	
	atualiza_primario(endereco_global, porta_global);

	close(sock);
	return;
}

void eleicao(){
	ServerList *aux;
	Frame pacote;
	int valor_ret;
	struct timeval tv;
	struct sockaddr_in conexao, from;
	char buffer[16];
	unsigned int tamanho;
	int sock;
	
	
	tamanho = sizeof(struct sockaddr_in);

	printf("\nInicio da eleicao\n");

	aux = listaServidores;
	if (aux == NULL) {//não há lista de servidores
		return;
	}//else

	//abre socket para comunicação com os outros servidores.
	if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		printf("Erro ao abrir o socket! \n");
	
	//preenche estrutura pacote
	bzero(pacote.user, MAXNAME-1);
	bzero(pacote.buffer, BUFFER_SIZE -1);
	strcpy(pacote.buffer, "_ELEICAO_");
	pacote.message_id = eleicao_id;
	pacote.ack = FALSE;

	strcpy(buffer, "000");
	//envia o pacote para todos os servidores.
	for (aux = listaServidores; aux != NULL;  aux = aux->prox) {
		//prepara a estrutura de destino para cada servidor
		bzero((char *) &conexao, sizeof(conexao));
		conexao.sin_family = AF_INET;
		conexao.sin_port = htons(aux->port);
		conexao.sin_addr.s_addr = inet_addr(aux->host);

		printf("\nEnviando id = %d para o servidor host=%s porta=%d\n", pacote.message_id, aux->host, aux->port);		

		//envia o pacote com timeout de 2 segundos.
		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
			perror("Error");
		}

		valor_ret = sendto(sock, &pacote, sizeof(pacote), 0, (const struct sockaddr *) &conexao, sizeof(struct sockaddr_in));

		if(valor_ret < 0){
			printf("\nerro em send\n");		
		}
		recvfrom(sock, &buffer, sizeof(buffer), 0, (struct sockaddr *) &from, &tamanho);
		
		if(strcmp(buffer,"_ELEICAO_RESP") == 0){
			printf("\nNão sou o primario.\n");
			close(sock);
			return;		
		}
		if(strcmp(buffer,"000") == 0){
			printf("\ntimeout da mensagem de eleicao\n");		
		}
		if(pacote.message_id == eleicao_id){
			//enviou a ele mesmo. descartar.
			strcpy(buffer, "000");	
		}
	}
	if(strcmp(buffer,"000") == 0){ //nenhuma mensagem teve resposta, esse é o novo server primario.
		printf("\n Sou o novo primario\n");
		server_primario = 1;
		envia_novoPrimario_servers();
		envia_novoPrimario_frontend();
		eleicao_running = 0;
		close(sock);
		return;
	
	}else{//segue esperando pelo novo primario na função eleicao_receive().
		return;
	}	
	
}

void atualizaFrontEnd() {
	ServerList *aux = listaServidores;
	Frame pacote;
	int valor_ret;
	struct sockaddr_in conexao, from;
	char buffer[16];
	int sock;
	unsigned int tamanho;
	tamanho = sizeof(struct sockaddr_in);

	
	if (aux == NULL) {//não há lista de servidores
		//return;
	}
	//abre socket para comunicação com os outros servidores.
	if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		printf("Erro ao abrir o socket! \n");
	
	strcpy(pacote.buffer,"_ATUALIZA_FRONTEND");
	pacote.message_id = lUserFrontEnd->port;
	strcpy(pacote.user,lUserFrontEnd->ip);
	
	for (aux = listaServidores; aux != NULL;  aux = aux->prox) {
		//prepara a estrutura de destino para cada servidor
		bzero((char *) &conexao, sizeof(conexao));
		conexao.sin_family = AF_INET;
		conexao.sin_port = htons(aux->port);
		conexao.sin_addr.s_addr = inet_addr(aux->host);

		printf("\nEnviando id = %d para o servidor host=%s porta=%d\n", pacote.message_id, aux->host, aux->port);		

		valor_ret = sendto(sock, &pacote, sizeof(pacote), 0, (const struct sockaddr *) &conexao, sizeof(struct sockaddr_in));

		if(valor_ret < 0){
			printf("\nerro em send\n");		
		}
		//recvfrom(sock, &buffer, sizeof(buffer), 0, (struct sockaddr *) &from, &tamanho);
		
		if(strcmp(buffer,"ACK") == 0){
			printf("\natualizado.\n");
			return;		
		}
	}
}

void listen_servers(void *unused){
	char *host_primario;
	int port_primario;
	int sock = 0;
	struct sockaddr_in conexao, from;
	int valor_retorno;
	char buffer[8];
	unsigned int tamanho;
	Frame pacote;
	struct timeval tv;

	
	tamanho = sizeof(struct sockaddr_in);

	while(1){

		if(server_primario == 1){
			//se o servidor for primário, ele deve receber pings dos servidores secundários e enviar acks, identificando-o como online.	
		}
		else if(eleicao_running == 0){//servidor secundário:
			//descobre qual o server primario.
			host_primario = get_hostPrimario(listaServidores);
			port_primario = get_portPrimario(listaServidores);

			if(!sock){
				//abre socket para comunicação com o server primario.
				if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
					printf("Erro ao abrir o socket! \n");
			}
			bzero((char *) &conexao, sizeof(conexao));
			conexao.sin_family = AF_INET;
			conexao.sin_port = htons(port_primario);
			conexao.sin_addr.s_addr = inet_addr(host_primario);
			//preenche estrutura pacote. Isso acontece pois a recepção de ping acontece no mesmo local da conexão de novos clientes.
			bzero(pacote.user, MAXNAME-1);
			bzero(pacote.buffer, BUFFER_SIZE -1);
			strcpy(pacote.buffer, "PING");
			pacote.message_id = 0;
			pacote.ack = FALSE;

			while (eleicao_running == 0){//envia ping
				sleep(3);
				strcpy(buffer, "000");
				tv.tv_sec = 1;
				tv.tv_usec = 100000;
				if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
					perror("Error");
				}
				sendto(sock, &pacote, sizeof(pacote), 0, (const struct sockaddr *) &conexao, sizeof(struct sockaddr_in));
				//espera resposta.
				valor_retorno = recvfrom(sock, &buffer, sizeof(buffer), 0, (struct sockaddr *) &from, &tamanho);
				if(strcmp(buffer,"000") == 0){
					//timeout entre o secundário e o primário, começar eleição.
					printf("time out.\n");
					eleicao_running = 1;		
				}
			}
			eleicao();
		}
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

void envia_conexao_secundarios(Frame pacote){

	//envia o pacote de conexão ao primario para os demais servidores.
	ServerList *aux;
	int sock;
	struct sockaddr_in conexao;
	int valor_ret;

	if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		printf("Erro ao abrir o socket! \n");
	
	
	for (aux = listaServidores; aux != NULL;  aux = aux->prox) {

		if(aux->port == porta_global && strcmp(aux->host, endereco_global) == 0){
			printf("\n\n****nao enviando para mim mesmo\n\n");		
		}else{
		
			//prepara a estrutura de destino para cada servidor
			bzero((char *) &conexao, sizeof(conexao));
			conexao.sin_family = AF_INET;
			conexao.sin_port = htons(aux->port);
			conexao.sin_addr.s_addr = inet_addr(aux->host);

			valor_ret = sendto(sock, &pacote, sizeof(pacote), 0, (const struct sockaddr *) &conexao, sizeof(struct sockaddr_in));

			if(valor_ret < 0){
				printf("\nerro em send\n");		
			}
		}
	}

	close(sock);
	return;
}

// Conecta servidor a algum cliente
void esperaConexao(char* endereco, int sockid) {	
	char *client_ip;
	int  funcaoRetorno;
	unsigned int frontEnd_port;
	char ack_message[8];
	char resp_eleicao[16];
	socklen_t clilen;
	Frame pacote_server, pacote; 
	
	// Identificador pra thread que vai controlar maximo de acessos
	pthread_t thread_id;

	struct sockaddr_in cli_addr;
	struct sockaddr_in cli_front;
	clilen = sizeof(struct sockaddr_in);
    // Controla ack recebidos pelo servidor
	pacote_server.ack = FALSE;
	strcpy(ack_message, "ACK");
	strcpy(resp_eleicao, "_ELEICAO_RESP");
	while (TRUE) {
		funcaoRetorno = recvfrom(sockid, &pacote, sizeof(pacote), 0, (struct sockaddr *) &cli_addr, &clilen);
		printf("\n=============recebeu: %s ================\n", pacote.buffer);
		if (funcaoRetorno < 0) 
			printf("Erro em receive \n");
		

		if (strcmp(pacote.buffer, "_ATUALIZA_FRONTEND") == 0) {
			printf("atualiza lista frontend\n");
			lUserFrontEnd = insereUser(lUserFrontEnd, pacote.message_id, pacote.user);
			imprimeUser(lUserFrontEnd);
		}
		if (strcmp(pacote.buffer, "_ELEICAO_") == 0){
			//recebeu pedido de eleição.
			//se o id recebido for menor que o da thread, responde a eleição.
			printf("\nRecebeu pedido de eleição com id : %d \n", pacote.message_id);
			if(pacote.message_id < eleicao_id){
				printf("\nminha id é maior. Respondendo\n");
				sendto(sockid, &resp_eleicao, sizeof(resp_eleicao), 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));				
				//resp_eleicao(sockid, pacote, cli_addr);
			}
			else{
				//espera novo primario.			
			}
		}
		if (strcmp(pacote.buffer, "NOVO_LIDER") == 0){ //host é enviado no pacote.user e porta no pacote.message_id.
			if(strcmp(pacote.user, endereco_global) == 0 && pacote.message_id == porta_global){				
				//novo lider é ele mesmo, não faz nada
			}else{
				atualiza_primario(pacote.user, pacote.message_id);
				eleicao_running = 0;
				//atualiza estrutura dos servidores e avisa as outras threads que a eleicao acabou.	
			}
		}
		if (strcmp(pacote.buffer, "PING") == 0){
			//responde ao ping.	
			//printf("ping\n");
			funcaoRetorno = sendto(sockid, &ack_message, sizeof(ack_message), 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
		}
		if (strcmp(pacote.buffer, "PING") == 0 || strcmp(pacote.buffer, "NOVO_LIDER") == 0 || strcmp(pacote.buffer, "_ELEICAO_") == 0  || strcmp(pacote.buffer, "_ATUALIZA_FRONTEND") == 0){
			//faz nada.		
		}
		else{
			printf("     Iniciou a conexao com um cliente");
		
			
			if(server_primario)
				envia_conexao_secundarios(pacote);

			//Atualiza lista FrontEnd User
			frontEnd_port = atoi(pacote.buffer);
			printf("\nPorta recebida de front end: %d", frontEnd_port);
			client_ip = inet_ntoa(cli_addr.sin_addr);
			lUserFrontEnd = insereUser(lUserFrontEnd, frontEnd_port, client_ip);
			imprimeUser(lUserFrontEnd);
			//atualizaFrontEnd();


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
}

int main(int argc, char *argv[]) {

	int port, sockid;
	char *endereco;
	pthread_t thread_id;


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

	strcpy(endereco_global, endereco);
	porta_global = port;	

	srand(time(NULL));
	eleicao_id = rand()%20000;

	printf("\n\n -- id de eleicao : %d -- ", eleicao_id );

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
		//função para preencher a estrutura com o endereço dos outros servers.
		get_servers();
		// Cria nova thread para dar listen nos outros servidores.
		if(pthread_create(&thread_id, NULL, (void *)listen_servers, NULL))
				printf("Erro ao criar a thread! \n");                 	                	
		esperaConexao(endereco, sockid);
	}
	return 0;
}
#endif
