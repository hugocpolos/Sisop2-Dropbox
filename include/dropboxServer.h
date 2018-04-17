#include <stdio.h>
#include <pthread.h>
#include "dropboxUtil.h"

struct file_info{
    char name[MAXNAME];             // Refere-se ao nome do arquivo
    char extension[MAXNAME];        // Refere-se ao tipo de extensão do arquivo
    char last_modified[MAXNAME];    // Refere-se a data da última modificação do arquivo
    int size;                       // Indica o tamanho do arquivo, em bytes
};

struct Client{
    int devices[2];                 // Associado aos dispositivos do usuário
    char userid[MAXNAME];           // Id do usuário no servidor, que deverá ser único. Informado pela linha de comando
    struct file_info files[MAXFILES]; // Metadados de cada arquivo que o cliente possui no servidor
    int logged_in;                  // Cliente está logado ou não
};

/*
    Sincroniza o servidor com o diretório "sync_dir_nomeusuario" do cliente.
*/
void sync_server();

/*
    Recebe um arquivo file do cliente.
    Deverá ser executada quando for realizar o upload de um arquivo.
    Parâmetros:
        file: path/filename.ext do arquivo a ser recebido.
*/
void receive_file(char *file);

/*
    Envia um arquivo fila para o usuário.
    Deverá ser executada quando for realizar o download de um arquivo.
    Parâmetros:
        file: filename.ext do arquivo.
*/
void send_file(char *file);

