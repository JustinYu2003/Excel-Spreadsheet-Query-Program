#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define SEPARATOR ','

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

//Pokemon structure
typedef struct { 
	//Number
	int number;
	//Name
	char pName[32];
	//Types
	char type1[9];
	char type2[9];
	//Stats Array: Total, HP, Attack, Defense, Sp.Atk, Sp.Def, Speed,
	int stats[7];
	//Generation
	int gen;
	//Legendary
	char legend;
} pokemonType;

//Linked List Node Structure for Pokemon
typedef struct Node {
  pokemonType *data;
  struct Node *next;
} NodeType;

//Linked List Node Structure for strings
typedef struct createdFile {
	char *data;
	struct createdFile *next;
} createdFileType;

//Structure that stores all the relevant information
typedef struct {
	FILE *pokeFile;
	NodeType *pokemonList;
	createdFileType *createdFiles;
	char separator;
	char* input;
	volatile int totalQueries;
	volatile char activeQuery;
	volatile char activeSave;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int clientSocket;
} paramsType;

//Forward references
void *readPoke(void*);
void *writePoke(void*);
void addPokemon(NodeType **list, pokemonType *poke);
void addFile(createdFileType **list, char *fileName);
void cleanup(NodeType *head);
