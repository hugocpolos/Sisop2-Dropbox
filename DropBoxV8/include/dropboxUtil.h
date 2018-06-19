#ifndef UTIL_HEADER
#define UTIL_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h> 
#include <stdbool.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/time.h>
#include <sys/select.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <dirent.h>





#define DYN_PORT_START 49200
#define DYN_PORT_END 65535
#define DEFAULT_PORT 9999
#define DEFAULT_ADDRESS "127.0.0.1"
#define SERVER_FOLDER "syncBox_users"
#define SERVER_USER "server"

#define MAXNAME 25
#define MAXFILES 50
#define MAXPATH MAXFILES*MAXNAME
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

#define ERROR -1
#define SUCCESS 0
#define TRUE 1
#define FALSE !(TRUE)

/* Communication constants */
#define END_REQ "END SESSION REQUEST"
#define UP_REQ "FILE UPLOAD REQUEST"
#define F_NAME_REQ "FILE NAME REQUEST"
#define DOWN_REQ "FILE DOWNLOAD REQUEST"
#define DEL_REQ "FILE DELETE REQUEST"
#define DEL_COMPLETE "FILE DELETED"

#define S_SYNC "sync"
#define S_NSYNC "not_sync"
#define S_DOWNLOAD "download"
#define S_GET "get"
#define S_UPLOAD "upload"
#define S_OK "ok"

typedef struct file_info{
	char name[MAXNAME];
	char extension[MAXNAME];
	char last_modified[MAXNAME];
	int size;
}FileInfo;

typedef struct server_info {
	char ip[sizeof(DEFAULT_ADDRESS) * 2];
	char folder[MAXNAME * 2];
	int port;
}ServerInfo;

typedef struct user_info {
	char nome[MAXNAME];
	char folder[MAXNAME * 2];
	int socket_id;
	struct sockaddr_in *serv_conn;
}UserInfo;

typedef struct user_front_end_info {
	unsigned short port;
	char *ip;
	struct user_front_end_info *next;
}UserFrontEndInfo;


typedef struct connection_info{
	int socket_id;
	char client_id[MAXNAME];
	char* ip;
	char buffer[BUFFER_SIZE];
	int port;
}Connection;

typedef struct client{
	int devices[2];
	char userid[MAXNAME];
	int logged_in;
	struct file_info files[MAXFILES];
	pthread_mutex_t mutex_files[MAXFILES];
	int n_files;
}Client;

typedef struct client_node{
	Client* client;
	struct client_node* next;
	struct client_node* prev;   
}ClientNode;

typedef ClientNode* ClientList;

typedef struct d_file {
  char path[MAXNAME];
  char name[MAXNAME];
} DFile;

typedef struct dir_content {
  char* path;
  struct d_file* files;
  int* counter;
} DirContent;


/* Ack Structure */
/*
	->message_id	:	contains the id of the message, must be incremented every new message
	->ack		:	if the ack is confirmed must be TRUE
	->buffer	:	contains the string content of the message
	->user		:	contains the info of who sent the message (always as client, server user is default)
*/
typedef struct frame{
    int message_id;			
    bool ack;				
    char buffer[BUFFER_SIZE];
    char user[MAXNAME];
}Frame;
/* End of Ack */


int getInfoDirArq(char * path, FileInfo files[]);
void getArqExtensao(const char *filename, char* extension);
void *dirConteudoThread(void *ptr);
int getDirConteudo(char *path, struct d_file files[], int* counter);

void getModificado(char *path, char *last_modified);
time_t getTime(char *last_modified);
int arqAntigo(char *last_modified, char *aux);
int newDevice(Client* client, int socket);
int arqExiste(char* filename);
int getFilesize(FILE* file);
int getFileSize(char *path);
char* getUsuario();
bool verificaDir(char *pathname);

Client* procuraCliente(char* userId, ClientList user_list);
Client* procuraCliente_index(int index, ClientList user_list);
ClientList adicionaCliente(char* userID, int socket, ClientList user_list);

UserFrontEndInfo *iniciaLista();
UserFrontEndInfo *insereUser(UserFrontEndInfo *l, unsigned short port, char *ip);
UserFrontEndInfo popUser(UserFrontEndInfo *l);

//DEBUG SECTION
void imprimeListaUsuarios(ClientList user_list);
void imprimeArqsCliente(Client* client);

#endif
