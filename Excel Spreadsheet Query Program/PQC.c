#include "PQC.h"

int main() {
	//Declare variables, structure, thread
	paramsType *params = (paramsType*) malloc(sizeof(paramsType));
	params->pokeFile = NULL;
	params->pokemonList = NULL;
	params->createdFiles = NULL;
	params->separator = SEPARATOR;
	params->input = NULL;
	params->activeQuery = 0;
	params->activeSave = 0;
	params->totalQueries = 0;	
	pthread_t t1 = NULL; 
	pthread_t t2 = NULL; 

	//Initialize mutex
	pthread_mutex_init(&params->mutex, NULL);
	pthread_cond_init(&params->cond, NULL);
	
	struct sockaddr_in serverAddress;
	int status;
	
	//Create client socket and error check it
	params->clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (params->clientSocket < 0) {
		printf("*** CLIENT ERROR: Could not open socket.\n");
		exit(-1);
	}

	//Setup server address
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
	serverAddress.sin_port = htons((unsigned short) SERVER_PORT);
	
	//Connect to server and error check it
	status = connect(params->clientSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
	if (status < 0) {
		printf("*** CLIENT ERROR: Could not connect.\n");
		free(params);
		exit(-1);
	}
	
	//Go into loop to communicate with server
	while(1) {
		//Display menu with options
		printf("\nOptions Menu:\n1. Type Search\n2. Save Results\n3. Exit the Program\n");
		//Take user's input
		scanf("%ms", &params->input);
		getchar();
		//Check which menu option the user chose
		if(*params->input == '1') { //If Type Search is chosen
			//Wait until the last query is done
			while(params->activeQuery){};
			//Free input
			free(params->input);
			//Ask what type the user wants to search
			printf("Please enter a Pokemon typing:\n");
			scanf("%ms", &params->input);
			getchar();
			//Send the input to the server
 			send(params->clientSocket, params->input, strlen(params->input), 0);	
			//Lock the mutex, then call the function for reading, then unlock the mutex
			pthread_mutex_lock(&params->mutex);
			pthread_create(&t1, NULL, readPoke, (void*)params);
			pthread_detach(t1);
			pthread_mutex_unlock(&params->mutex);	
			free(params->input);		
		}else if(*params->input == '2') { //If Save Results is chosen
			//Free input
			free(params->input);
			//If Save Results is chosen
			//Prompt the user to enter the name of the file which will contain the query results
			printf("Please enter the name of the file which will contain the accumulated query results:\n");
			scanf("%ms", &params->input);
			getchar();		
			//Wait for the last save and query to finish
			while(params->activeSave || params->activeQuery) {}; 		
			//Lock the mutex, then call the function for writing, then unlock the mutex
			pthread_mutex_lock(&params->mutex);
			pthread_create(&t2, NULL, writePoke, (void*)params);
			pthread_detach(t2);
			pthread_mutex_unlock(&params->mutex);				
		}else if (*params->input == '3') { //If exit is chosen 	
			//Free input
			free(params->input);
			//Send the end indicator to the server
 			send(params->clientSocket, "stop", strlen("stop"), 0);
 			//Print total number of queries
 			printf("Total Successful Queries: %d\n", params->totalQueries);
 			//Print names of new files created during the session
 			printf("Created Files: ");
 			createdFileType *currNode, *nextNode;
			currNode = params->createdFiles;
			while(currNode != NULL) {
				printf("%s, ", currNode->data);
				free(currNode->data);
				nextNode = currNode->next;
				free(currNode);
				currNode = nextNode;
			}
			//Exit loop
			break;
		}else {
			free(params->input);
		}
	}
	
	//Close the socket
	close(params->clientSocket);

	//Free variables
	cleanup(params->pokemonList);
	free(params);
	
	return(0);
}

void *readPoke(void* paramStruct) {
	//Declare variables
	paramsType *params = (paramsType*)paramStruct;
	pokemonType *poke = NULL;
	int bytesRcv;
	params->activeQuery = 1;
	char buffer[90];
	char *line = NULL;
	
	//Loop until server is done query
	while(1) {
		//Receive line from server
		bytesRcv = recv(params->clientSocket, buffer, sizeof(buffer), 0);
		buffer[bytesRcv] = 0; // put a 0 at the end so we can display the string
		//If the server tells the client that its at the end of the csv file
		if(strcmp(buffer, "END") == 0) {
			break;
		}else {
			line = buffer;
			//Allocate memory for pokemon
			poke = (pokemonType*) malloc(sizeof(pokemonType));
			//Initialize the pokemon
			poke->number = atoi(strsep(&line, &params->separator));
			strcpy(poke->pName, strsep(&line, &params->separator));
			strcpy(poke->type1, strsep(&line, &params->separator));
			strcpy(poke->type2, strsep(&line, &params->separator));
			for(int i = 0; i < 7; i++) {
				poke->stats[i] = atoi(strsep(&line, &params->separator));
			}
			poke->gen = atoi(strsep(&line, &params->separator));
			if(!(strcmp(strsep(&line, &params->separator), "True"))) {
				poke->legend = '1';
			} else {
				poke->legend = '0';
			}
			//Add the pokemon to the linked list
			addPokemon(&params->pokemonList, poke);
			free(line);
	 		send(params->clientSocket, "DONE", strlen("DONE"), 0);
	 	}
	}
	params->totalQueries += 1;
	params->activeQuery = 0;
	pthread_exit(NULL);
}

//Function to add a pokemon to the linked list
void addPokemon(NodeType **list, pokemonType *poke) {
	//Create nodes
	NodeType *newNode;
  	NodeType *currNode, *prevNode;
  	
  	//Allocate memory for the new node and set data 
  	newNode = (NodeType *) malloc(sizeof(NodeType));
  	newNode->data = poke;
  	newNode->next = NULL;
  	
  	//Set the currentNode as the head of the list
  	prevNode = NULL;
  	currNode = *list;
  	
  	//Iterate through the list
  	while (currNode != NULL) {
    		prevNode = currNode;
    		currNode = currNode->next;
  	}
  	
  	//If list empty make the new node the head
  	if(prevNode == NULL) {
  		*list = newNode;
  	}else { //If List not empty then make the new node the next node
  		prevNode->next = newNode;
  	}
  	
  	//Make the new node's next node be NULL
  	newNode->next = currNode;
}

//Function to write pokemon into a file
void *writePoke(void* paramStruct) {
	//Declare variables
	paramsType *p = (paramsType*)paramStruct;
	p->activeSave = 1;
	FILE *accuQuery = NULL;
	
	//Attempt to open the file in write mode but
	//if file can't be created or written to, tell the user and prompt user to enter filename again
	while((accuQuery = fopen(p->input, "w")) == NULL) { 
		free(p->input);
		printf("Couldn't open the file for writing. Please reenter the filename:\n");
		scanf("%ms", &p->input);
		getchar();
	}
	addFile(&p->createdFiles, p->input);
	free(p->input);	
	
	//Write the linked list into the file
	NodeType *currNode = p->pokemonList;
	while(currNode != NULL) {
		fprintf(accuQuery, "%d %s %s %s %d %d %d %d %d %d %d %d %d\n", currNode->data->number, currNode->data->pName,
		currNode->data->type1, currNode->data->type2, currNode->data->stats[0], currNode->data->stats[1],
		currNode->data->stats[2], currNode->data->stats[3], currNode->data->stats[4], currNode->data->stats[5],
		currNode->data->stats[6], currNode->data->gen, currNode->data->legend);
		currNode = currNode->next;
	}
	
	//Close the file
	if (fclose(accuQuery) == EOF) {
 		printf("Could not close file\n");
 		exit(1);
	}
	
	p->activeSave = 0;
	pthread_exit(NULL);
}

//Function to add the name of a created file into a list
void addFile(createdFileType **list, char *fileName) {
	//Create nodes
	createdFileType *newNode;
  	createdFileType *currNode, *prevNode;
  	
  	//Allocate memory for the new node and set data 
  	newNode = (createdFileType *) malloc(sizeof(createdFileType));
  	newNode-> data = (char *) malloc(sizeof(fileName));
  	strcpy(newNode->data, fileName);
  	newNode->next = NULL;
  	
  	//Set the currentNode as the head of the list
  	prevNode = NULL;
  	currNode = *list;
  	
  	//Iterate through the list
  	while (currNode != NULL) {
    		prevNode = currNode;
    		currNode = currNode->next;
  	}
  	
  	//If list empty make the new node the head
  	if(prevNode == NULL) {
  		*list = newNode;
  	}else { //If List not empty then make the new node the next node
  		prevNode->next = newNode;
  	}
  	
  	//Make the new node's next node be NULL
  	newNode->next = currNode;
}

//Function to free a linked list
void cleanup(NodeType *head) {
  NodeType *currNode;
  NodeType *nextNode;

  currNode = head;

  while (currNode != NULL) {
    free(currNode->data);
    nextNode = currNode->next;
    free(currNode);
    currNode = nextNode;
  }
}
