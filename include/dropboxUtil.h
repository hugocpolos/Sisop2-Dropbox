#include <stdio.h>
#include <stdlib.h>

#define MAXNAME 64
#define MAXFILES 10
#define MSGSIZE 1024

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
