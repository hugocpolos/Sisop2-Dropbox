#include "../include/dropboxUtil.h"

/*
	Estrutura message_data:
	Estrutura de cada bloco de dados da mensagem.
	int order_number:	Inteiro que armazena a ordem da mensagem, para realizar o ordenamento no servidor e o comando de reenvio.
	char blockData[MSGSIZE]:	Bloco de tamanho MSGSIZE com os dados em si.
	struct message_data *Next_Block:	ponteiro para o próximo bloco dessa mensagem.

	obs: quem garante o ordenamento dos blocos no cliente é a função create_message.
*/
typedef struct message_data{
	int order_number;
	char blockData[MSGSIZE];
	struct message_data *Next_Block;
}MESSAGE_DATA_t;

/*
	Estrutura message:
	contém o primeiro bloco de dados da mensagem.
	Por apresentar apenas um campo se torna um pouco inútil, mas talvez seja utilizado com mais campos no futuro.
*/
typedef struct message{
	MESSAGE_DATA_t *data;
}MESSAGE_t;

/*
	função print_msg:
	função para debug, exibe todas as informações dentro de uma mensagem no formato de texto.
	Saída esperada:
	>print_msg(mensagem_de_teste.data)
	0: conteudo do primeiro bloco....
	.
	.
	.
	N: conteudo do ultimo bloco da mensagem
	^
	|
	valor do inteiro de ordenamento

	obs:É passado o campo .data da estrutura message para facilitar a implementação. 
*/

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


/*
	Função create_message:
	função para ser utilizada no cliente/servidor para criar uma série de blocos de mensagens a serem enviados por UDP a partir de um arquivo.
	cria uma mensagem no formato de blocos de tamanho MSGSIZE a partir de um handler de arquivo.

	testado para arquivos de 0 até 5Mb e não houve nenhum problema.			
*/

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
			fread(new->blockData,sizeof(char),MSGSIZE,file);
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
		printf("not_EOF %d\n ERROR = %d\n",not_EOF,ERROR);	
	}

	return newMsg;
}



/*
	função send_message:
	envia uma mensagem do formato acima para um servidor por protocolo udp. Mensagens são enviadas em múltiplos blocos de tamanho MSGSIZE.
	parametros:
		server: endereço do servidor.
		message: estrutura contendo a mensagem completa.
	
	retorna 0 em caso de envio bem sucedido e -1 em caso de erro.
	
	@TODO: é necessário acrescentar o parâmetro porta a função. (Função foi testada no programa Exemplo/udp_files.c que tem porta padrão definida em 4000.

	@TODO: único erro mapeado foi quando a mensagem não tem dados para serem enviados. localizar mais.

	@TODO: acrescentar à mensagem o número de ordem.
*/

int send_message(struct hostent *server, MESSAGE_t message){

	int sockfd, n;
	unsigned int length;
	struct sockaddr_in serv_addr, from;
	MESSAGE_DATA_t *aux;
	char buffer[MSGSIZE];
	
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return -1;
	}	
	
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		printf("ERROR opening socket");
		return -1;
	}
	
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(PORT);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);  

	if (message.data != NULL){
		aux = message.data;
		do{
			bzero(buffer, MSGSIZE);
			strcpy(buffer,aux->blockData);
			n = sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
			if (n < 0) 
				printf("ERROR sendto");
			
			length = sizeof(struct sockaddr_in);
			n = recvfrom(sockfd, buffer, MSGSIZE, 0, (struct sockaddr *) &from, &length);
			if (n < 0)
				printf("ERROR recvfrom");

			printf("Got an ack: %s\n", buffer);
			aux = aux->Next_Block;

		}while(aux != NULL);	
		close(sockfd);
		return 0;
	}else return -1; //no data to send.

	
}



