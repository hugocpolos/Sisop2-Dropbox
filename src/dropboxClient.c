#include <stdio.h>
#include "../include/dropboxClient.h"



/*
    Estabelece uma sessão entre o cliente com o servidor.
    Parametros:
        host: endereço do servidor.
        port: porta do servidor.    
*/
int login_server (char *host, int port){
    return 0;
}

/*
    Sincroniza o diretório "sync_dir_nomeusuario" com o servidor.
*/
void sync_client (){
    return;
}

/*
    Envia um arquivo file para o servidor. Deverá ser executada
    quando for realizar upload de um arquivo.
    Parâmetros:
        file: filename.ext do arquivo a ser enviado.
*/
void send_file (char *file){
    return;
}

/*
    Obtém um arquivo file do servidor.
    Deverá ser executada quando for realizar download de um arquivo.
    Parâmetros:
        file: filename.ext do arquivo a ser recebido.
*/
void get_file (char *file){
    return;
}

/*
    Exclui um arquivo file de "sync_dir_nomeusuario".
    Parâmetros:
        file: filename.ext do arquivo a ser deletado.
*/
void delete_file (char *file){
    return;
}

/*
    Fecha sessão com o servidor.
*/
void close_session (){
    return;
}

void main(){
    return;
}
