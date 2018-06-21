#ifndef UTIL_CODE
#define UTIL_CODE

#include "dropboxUtil.h"
#include "sync-server.h"
#include "sync-client.h"
#include "watcher.h"

void *dirConteudoThread(void *ptr) {
   struct dir_content *args = (struct dir_content *) ptr;

   getDirConteudo(args->path, args->files, args->counter);

   pthread_exit(NULL);
   return NULL;
}

int getInfoDirArq(char * path, FileInfo files[]) {
	struct d_file dfiles[MAXFILES];
  	char path_file[MAXNAME*2 + 1];
  	int counter = 0;

  	getDirConteudo(path, dfiles, &counter);

  	for(int i = 0; i < counter; i++) {
  		strcpy(files[i].name, dfiles[i].name);
    		sprintf(path_file, "%s/%s", dfiles[i].path, dfiles[i].name);
  		getModificado((char*) &path_file, (char*) &files[i].last_modified);
    		getArqExtensao(dfiles[i].name, (char*) &files[i].extension);
  		files[i].size = getFileSize(dfiles[i].path);
  	}
  	return counter;
}

void getArqExtensao(const char *filename, char* extension) {
	const char *dot = strrchr(filename, '.');
  	if(!dot || !strcmp(dot, filename)) {
    		strcpy(extension, "");
  	} else {
  		strcpy(extension, dot+1);
  	}
}

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int getDirConteudo(char *path, struct d_file files[], int* counter) {
	DIR * d = opendir(path);
  	if(d == NULL) {
    		return ERROR;
  	}

  	struct dirent * entry;
  	while (((entry = readdir(d)) != NULL) && (*counter) < MAXFILES) {
    		if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
      			struct d_file newFile;
      			strcpy(newFile.path, path);
      			strcpy(newFile.name, entry->d_name);

      			pthread_mutex_lock(&lock);
      			memcpy(&files[(*counter)], &newFile, sizeof(newFile));
      			(*counter)++;
      			pthread_mutex_unlock(&lock);

      			int rc;
      			pthread_t thread;
      			if(entry->d_type == DT_DIR) { // Arquivo é um diretório
        			struct dir_content args;
        			args.path = malloc(sizeof(char) * MAXNAME * 2 + 1); // MAXNAME + / + MAXNAME

        			sprintf(args.path, "%s/%s", newFile.path, newFile.name);
        			args.files = files;
        			args.counter = counter;

        			if((rc = pthread_create(&thread, NULL, &dirConteudoThread, (void *) &args))) {
          				printf("Thread creation failed: %d\n", rc);
        			}

        			pthread_join(thread, NULL);
      			}
    		}
  	}

  	closedir(d);
	return 0;
}

//Pega hora de modificacao
void getModificado(char *path, char *last_modified) {
	struct stat attr;
	stat(path, &attr);

	strftime(last_modified, 20, "%d.%m.%Y %H:%M:%S", localtime(&(attr.st_mtime)));
}

//Pega hora atual
time_t getTime(char *last_modified){
	time_t result = 0;

	int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;

	if (sscanf(last_modified, "%2d.%2d.%4d %2d:%2d:%2d", &day, &month, &year, &hour, &min, &sec) == 6) {
		struct tm breakdown = { 0 };
		breakdown.tm_year = year - 1900; /* years since 1900 */
		breakdown.tm_mon = month - 1;
		breakdown.tm_mday = day;
		breakdown.tm_hour = hour;
		breakdown.tm_min = min;
		breakdown.tm_sec = sec;

		if ((result = mktime(&breakdown)) == (time_t)-1) {
			fprintf(stderr, "Could not convert time input to time_t\n");
			return EXIT_FAILURE;
		}

		return result;
	} else {
		printf("The input was not a valid time format: %s\n", last_modified);
		return ERROR;
	}
}

//pega arquivo mais antigo
int arqAntigo(char *last_modified, char *aux) {
	time_t time_f1 = getTime(last_modified);
	time_t time_f2 = getTime(aux);

	if(difftime(time_f1, time_f2) > 0)
		return SUCCESS;
	return 0;
}

char* getUsuario() {
 	struct passwd *pw = getpwuid(geteuid());	 

	if (pw) {
    		return pw->pw_dir;
  	}
	
	printf("No root directory found! Returning empty...\n");
 	return "";
}

int getFilesize(FILE* file) {
	int size;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	rewind(file);

	return size;
}

int getFileSize(char *path) {
	struct stat attr;
	stat(path, &attr);
	
	return attr.st_size;
}

//verifica se o dir existe
bool verificaDir(char *pathname) {
	struct stat st = {0};
	
	if (stat(pathname, &st) == ERROR)
		return FALSE;
	
	return TRUE;
}

//procura cliente pela userId na lista de clientes
Client* procuraCliente(char* userId, ClientList user_list) {

	ClientList current = user_list;

	while(current != NULL) {
		if(strcmp(userId, current->client->userid) == 0 && current->client->logged_in == 1) {
			return current->client;
		}
		current = current->next;
	}
	return NULL;
}


Client* procuraCliente_index(int index, ClientList user_list){
	
	int i = 0;	
	ClientList current = user_list;
	
	while(current != NULL && i <= index) {
		if (i == index){
			return current->client;		
		}
		else{
			i++;
			current = current->next;
		}
	}
	return NULL;
}


//adiciona um novo cliente na lista de clientes
ClientList adicionaCliente(char* userID, int socket, ClientList user_list) {

	Client* new_client = (Client*) malloc(sizeof(Client));

	strcpy(new_client->userid, userID);
	new_client->devices[0] = socket;
	new_client->devices[1] = -1;
	new_client->logged_in = 1;
	new_client->n_files = 0;

	ClientNode* new_node = (ClientNode*) malloc(sizeof(ClientNode));
	new_node->client = new_client;

	if(user_list == NULL) {
		user_list = new_node;
		new_node->next = NULL;
		new_node->prev = NULL;
	} else {
	ClientNode* current_node = user_list;

	while(current_node->next != NULL) {
	    current_node = current_node->next;
	}

	current_node->next = new_node;
	new_node->prev = current_node;

	}

	return user_list;
}

UserFrontEnd iniciaLista() {
	return NULL;
}

int existe_frontend(UserFrontEnd l, unsigned short port, char ip[]){
	UserFrontEnd aux;
	aux = l;
	for(aux = l; aux != NULL; aux = aux->next){
		if(port == aux->port && strcmp(ip, aux->ip) == 0){
			return 1;		
		}
	}
	return 0;
}

UserFrontEnd insereUser(UserFrontEnd l, unsigned short port, char ip[]) {

	if(existe_frontend (l, port, ip) == 1 ){
		return l;	
	}	
	
	UserFrontEndInfo *novo = malloc(sizeof(UserFrontEndInfo));
	novo->port = port;
	strcpy(novo->ip,ip);
	novo->next = l;
	return novo;
}

void imprimeUser(UserFrontEnd l) {
	printf("\nimprimindo lista User front End\n\n");
	while (l != NULL) {
		printf("ip:%s\tporta:%d\n",l->ip,l->port);
		l = l->next;
	}
}


//tenta adicionar um novo device para o cliente
int newDevice(Client* client, int socket) {
	if(client->devices[0] == -1) {
		client->devices[0] = socket;
		return 0;
	}
	if(client->devices[1] == -1) {
		client->devices[1] = socket;
		return 0;
	}

	return -1;
}

int arqExiste(char* filename) {
	struct stat buffer;
	return (stat(filename, &buffer) == 0);
}

void imprimeListaUsuarios(ClientList user_list){

    ClientNode* current_node = user_list;
  
    while(current_node != NULL){
        printf("\n     Usario: %s \n     Device:", current_node->client->userid);
        if(current_node->client->devices[0] > 0)
            printf(" socket-%i ", current_node->client->devices[0]);
        if(current_node->client->devices[1] > 0)
            printf(" socket-%i ", current_node->client->devices[1]);
        printf("\n");
        current_node = current_node->next;
    }
}

void imprimeArqsCliente(Client* client){
	printf("\nArquivos na pasta do cliente:");

	for(int i = 0; i < client->n_files; i++){
		printf("\n\n\tNome: %s", client->files[i].name);
		printf("\n\tExtensao: %s", client->files[i].extension);
                printf("\n\tTamanho: %d", client->files[i].size);
		printf("\n\tUltima modificacao: %s", client->files[i].last_modified);		
	}
}

//funções referentes a fila de servidores.
ServerList *init_serverlist(ServerList *lista){
	lista = NULL;
	return lista;
}

ServerList *adiciona_server(int tipo, char *host, int port, ServerList *lista){
	ServerList *novo;		

	//printf("\n adicionando server:\ntipo: %d\nhost: %s\nporta: %d \n", tipo, host,port);
	novo = malloc(sizeof(ServerList));
	//printf("malloc ok\n");
	novo->primario = tipo;
	//printf("novo->primario: %d\n", novo->primario);
	strcpy(novo->host, host);
	//printf("novo->host: %s\n", novo->host);
	novo->port = port;
	//printf("novo->port: %d\n", novo->port);
	//printf("\nalocação bem sucedida\n");

	if(lista == NULL){ //lista vazia, novo elemento é a própria lista.
		novo->prox = NULL;
		return novo;
	}else{
		novo->prox = lista;
		return novo;
	}
}


char *get_hostPrimario(ServerList *listaServidores){
	ServerList *aux;
	aux = listaServidores;
	while (aux != NULL){
		if(aux->primario == 1){
			//printf("host: %s", aux->host);
			return aux->host;
		}
		aux = aux->prox;	
	}
	return NULL;
	
}

int get_portPrimario(ServerList *listaServidores){
	ServerList *aux;
	aux = listaServidores;
	while (aux != NULL){
		if(aux->primario == 1){
			return aux->port;
		}
		aux = aux->prox;	
	}
	return NULL;
}

void print_listaServidor(ServerList *listaServidores){
	ServerList *aux;
	aux = listaServidores;
	while (aux != NULL){
		printf("\nServer:\nTipo: %d\nHost: %s\nPorta: %d\n \n", aux->primario, aux->host, aux->port);
		aux = aux->prox;	
	}
	return NULL;
}

#endif
