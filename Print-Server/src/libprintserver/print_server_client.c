#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "print_server_client.h"

// Stores the size of the buffer
#define BUFFER_SIZE 1024

int printer_print(int* handle, char* driver, char* job_name, char* description, char* data) {
	// Setup socket connection with server
	// Declare clisock with IP address
	int clisock;
	int PORT_NUM = 51181;
	char* CLIENT_IP = "127.0.0.1";
	
	// Declare storage for remote address
	struct sockaddr_in remoteaddr;
	
	// Create TCP Socket and error checking
	if ( (clisock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket() error:\n");
		close(clisock);
		exit(1);
		return -1;
	}
	
	// Specify an endpoint address for server
	remoteaddr.sin_family = PF_INET;
	remoteaddr.sin_port = htons(PORT_NUM);
	remoteaddr.sin_addr.s_addr = inet_addr(CLIENT_IP);
	
	// Connect to server
	int connect_result;
	connect_result = connect(clisock, (struct sockaddr *) &remoteaddr, sizeof(remoteaddr));
	
	// Connect error checking
	if (connect_result < 0) {
		perror("The following error occurred (Client Print Connection):");
		close(clisock);
		return -1;
	}
	
	// Send the print command to the server
	// Declare buffer for writing and reading from server
	char buffer[BUFFER_SIZE];
	
	// Send printer print command
	memset(buffer, '\0', sizeof(buffer));
	strcpy(buffer, "PRINTER_PRINT");
	
	// Write printer print result to server
	int write_result;
	write_result = write(clisock, buffer, BUFFER_SIZE);
	
	// Write error checking
	if (write_result < 0) {
		fprintf(stderr, "Failed to write to server!\n");
		close(clisock);
		return -1;
	}
	
	// Clear out the buffer contents
	int i = 0;
	for (i = 0; i < BUFFER_SIZE; i++) {
		buffer[i] = '\0';
	}

	// Send the print job information to the server
	strcat(buffer, "NEW ");
	buffer[strlen(buffer)] = '@';
	
	strcat(buffer, "PRINTER ");
	strcat(buffer, driver);
	buffer[strlen(buffer)] = '@';
	
	strcat(buffer, "NAME ");
	strcat(buffer, job_name);
	buffer[strlen(buffer)] = '@';
	
	strcat(buffer, "DESCRIPTION ");
	strcat(buffer, description);
	buffer[strlen(buffer)] = '@';
	
	strcat(buffer, "FILE ");
	strcat(buffer, data);
	buffer[strlen(buffer)] = '@';
	
	strcat(buffer, "PRINT");
	
	// Check if buffer size has been exceeded and let user know
	if (strlen(buffer) == BUFFER_SIZE) {
		printf("ERROR: buffer size exceeded. Some data may be lost.");
		close(clisock);
		return -1;
	}
	
	// Send everything to server (create certain structure recognized by server
	// Write printer print result to server
	write_result = write(clisock, buffer, BUFFER_SIZE);
	
	// Write error checking
	if (write_result < 0) {
		fprintf(stderr, "Failed to write to server!\n");
		close(clisock);
		return -1;
	}

	// Close client connection
	int close_result;
	close_result = close(clisock);
	
	// Close error checking
	if (close_result < 0) {
		perror("The following error occurred:");
		return -1;
	}
	
	// Return 0 when successful
	return 0;
}




printer_driver_t** printer_list_drivers(int *number) {
	// Setup socket connection with server
	// Declare clisock with IP address
	int clisock;
	int PORT_NUM = 51181;
	char* CLIENT_IP = "127.0.0.1";
	
	// Declare storage for remote address
	struct sockaddr_in remoteaddr;
	
	// Create TCP Socket and error checking
	if ( (clisock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket() error:\n");
		close(clisock);
		exit(1);
	}
	
	// Specify an endpoint address for server
	remoteaddr.sin_family = PF_INET;
	remoteaddr.sin_port = htons(PORT_NUM);
	remoteaddr.sin_addr.s_addr = inet_addr(CLIENT_IP);
	
	// Connect to server
	int connect_result;
	connect_result = connect(clisock, (struct sockaddr *) &remoteaddr, sizeof(remoteaddr));
	
	// Connect error checking
	if (connect_result < 0) {
		perror("The following error occurred (Client Print Connection):");
		close(clisock);
	}
	
	// Send get drivers command
	// Declare buffer for writing and reading from server
	char buffer[BUFFER_SIZE];
	
	// Send printer print command
	memset(buffer, '\0', sizeof(buffer));
	strcpy(buffer, "PRINTER_DRIVERS");
	
	// Write printer driver command to server
	int write_result;
	write_result = write(clisock, buffer, sizeof(buffer));
	
	// Write error checking
	if (write_result < 0) {
		fprintf(stderr, "Failed to write to server!\n");
		close(clisock);
	}
	
	// Receive drivers data from server (in certain format specified by server)
	// Read data sent from server (ruptime output)
	int read_value;
	
	read_value = read(clisock, buffer, BUFFER_SIZE);
	
	if (read_value < 0) {
		perror("The following error occurred:");
		close(clisock);
	}
	
	// Set last character in buffer to null terminator
	buffer[strlen(buffer)] = '\0';
	
	// Declare variables to store user input
	int numArgs = 1;

	// Count the number of arguments
	int i;
	for (i = 0; i < strlen(buffer); i++) {
		if (buffer[i] == '@') {
			numArgs++;
		}
	}
	
	// Create null terminated list of printer drivers from server
	// Separate the arguments by spaces
	char* buffer_drivers = strtok(buffer, "@");
	
	// Set number to number of arguments divide by 3 plus 1 for null terminated end
	*number = (numArgs/3) + 1;
	
	// Declare Variables to store user arguments	
	printer_driver_t** printer_drivers = calloc(*number, sizeof(printer_driver_t*));
	int j = 0;
	for (j = 0; j < numArgs; j++) {
		printer_drivers[j] = calloc(1, sizeof(printer_driver_t));
	}

	// Get the command and arguments from user
	int jobs = 0; // Stores the number of print drivers
	i = 0; // Stores the attribute number
	while (buffer_drivers != NULL) {
	
		// Based on the value of i, store the information about the print driver
		// i=0 is printer_name, i=1 is driver_name, i=2 is driver_version
		if (i == 0) {
			printer_drivers[jobs]->printer_name = calloc(strlen(buffer_drivers), sizeof(char));
			strcpy(printer_drivers[jobs]->printer_name, buffer_drivers);
		} else if (i == 1) {
			printer_drivers[jobs]->driver_name = calloc(strlen(buffer_drivers), sizeof(char));
			strcpy(printer_drivers[jobs]->driver_name, buffer_drivers);
		} else {
			printer_drivers[jobs]->driver_version = calloc(strlen(buffer_drivers), sizeof(char));
			strcpy(printer_drivers[jobs]->driver_version, buffer_drivers);
		}
	
		// Move to next driver
		buffer_drivers = strtok(NULL, "@");
	
		// Increment argument
		i++;
		
		// Increase number of drivers every 3 attributes
		if (i == 2) {
			jobs++;
		}
		
		// Ensure i is between 0 and 2
		i = i%3;
	}

	// Terminate list of arguments by NULL pointer
	printer_drivers[*number-1] = (printer_driver_t*) NULL; // foreground process

	// Close client connection
	int close_result;
	close_result = close(clisock);
	
	// Close error checking
	if (close_result < 0) {
		perror("The following error occurred:");
		close(clisock);
	}
	
	// Set number to not include null terminator
	*number = *number - 1;
	
	// Return the null-terminated list of printer drivers from server
	return printer_drivers;
}


