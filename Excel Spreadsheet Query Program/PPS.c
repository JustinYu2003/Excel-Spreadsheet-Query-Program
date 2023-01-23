#include "PPS.h"

int main() {
	//Declare variables
	FILE *pokeFile;
	char *input = NULL;
	char buffer[90];
	int num_lines = 0;
	char* line = NULL;
	char* lptr = NULL;
	char* fullLine = NULL;
	char hold[5];
	
	int serverSocket, clientSocket;
	struct sockaddr_in serverAddress, clientAddress;
	int status, addrSize, bytesRcv;
	
	//Prompt user to enter the name of the file that contains the pokemon descriptions
	printf("Enter the name of the file that contains the Pokemon descriptions\n");
	scanf("%ms", &input);
	getchar();
	
	//Try to open the file
	pokeFile = fopen(input, "r");
	
	//If the file isn't found, print a message and prompt the user to enter the name again or quit
	while(!pokeFile) {
		printf("Couldn't open the file. Please enter the filename again or enter '-1' to quit\n");
		free(input);
		scanf("%ms", &input);
		getchar();
		if(strcmp(input, "-1") == 0) {
			printf("Exiting program\n");
			free(input);
			exit(1);
		}
		pokeFile = fopen(input, "r");
	}
	free(input);
	
	//Create socket
	serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket < 0) {
		printf("*** SERVER ERROR: Could not open socket.\n");
		exit(-1);
	}
	
	//Setup server address
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons((unsigned short) SERVER_PORT);
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//BIND SOCKET
	status = bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
	if (status < 0) {
		printf("*** SERVER ERROR: Could not bind socket.\n");
		perror("socket Error");
		exit(-1);
	}
	
	//Set up socket for listening
	status = listen(serverSocket, 5);
	if (status < 0) {
		printf("*** SERVER ERROR: Could not listen on socket.\n");
		exit(-1);
	}
	
	while(1) {
		//Accept client
		addrSize = sizeof(clientAddress);
		clientSocket = accept(serverSocket,(struct sockaddr *)&clientAddress,&addrSize);
		if (clientSocket < 0) {
			printf("*** SERVER ERROR: Couldn't accept incoming client connection.\n");
			exit(-1);
		}
		printf("SERVER: Received client connection.\n");
		
		//Go into infinite loop to talk to client
		while(1) {		
			//Get the message from the client
			bytesRcv = recv(clientSocket, buffer, sizeof(buffer), 0);
			buffer[bytesRcv] = 0; // put a 0 at the end so we can display the string
			printf("SERVER: Received client request: %s\n", buffer);
			
			//Check end
			if (strcmp(buffer,"stop") == 0) {
 				break;
			}
			
			//Go through every line in the csv file
			while(fscanf(pokeFile, "%m[^\n]\n", &line) != EOF) {
				lptr = line;
				sscanf(line, "%m[^\n]\n", &fullLine);
				if (num_lines > 0) { //To skip the header
					strsep(&line, SEPARATOR);
					strsep(&line, SEPARATOR);
					//Check if type matches the type wanted
					if(strcasecmp(strsep(&line, SEPARATOR), buffer) == 0) {
						//Send the line then wait for confirmation that the client is done with it
						send(clientSocket, fullLine, strlen(fullLine), 0);
						bytesRcv = recv(clientSocket, hold, sizeof(hold), 0);
						hold[bytesRcv] = 0; // put a 0 at the end so we can display the string
					}
				}
				num_lines++;
				free(lptr);
				free(fullLine);
			}
			rewind(pokeFile);
			
			printf("SERVER: Sending END to client\n");
			send(clientSocket, "END", strlen("END"), 0);
		}
		
		//Close client socket and stop while loop
		printf("SERVER: Closing client connection.\n");
		close(clientSocket);
		break;
	}
	
	//Close server socket
	close(serverSocket);
	printf("SERVER: Shutting down.\n");
	
	//Close the opened file and check if the files were closed without error
	if (fclose(pokeFile) == EOF) {
 		printf("Could not close file\n");
 		exit(1);
	}
}
