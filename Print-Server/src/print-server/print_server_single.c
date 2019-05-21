/**
 * @file      main.c
 * @author    Jeramie Vens
 * @date      2015-02-11: Created
 * @date      2015-02-15: Last updated
 * @date      2016-02-16: Complete re-write
 * @date      2016-02-20: convert to single threaded
 * @brief     Emulate a print server system
 * @copyright MIT License (c) 2015, 2016
 */
 
/*
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <semaphore.h>

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

// Stores the size of the buffer
#define BUFFER_SIZE 1024

#include "print_job.h"
#include "printer_driver.h"
#include "debug.h"

// -- GLOBAL VARIABLES -- //
int verbose_flag = 0;
int exit_flag = 0;
int daemon_process = 0; // Variable that stores whether the process is a daemon

// -- STATIC VARIABLES -- //
static struct printer_group * printer_group_head;

// -- FUNCTION PROTOTYPES -- //
static void parse_command_line(int argc, char * argv[]);
static void parse_rc_file(FILE* fp);


/**
 * A list of print jobs that must be kept thread safe
 */
struct print_job_list
{
	// the head of the list
	struct print_job * head;
	// the number of jobs in the list
	sem_t num_jobs;
	// a lock for the list
	pthread_mutex_t lock;
};

/**
 * A printer object with associated thread
 */
struct printer
{
	// the next printer in the group
	struct printer * next;
	// the driver for this printer
	struct printer_driver driver;
	// the list of jobs this printer can pull from
	struct print_job_list * job_queue;
	// the thread id for this printer thread
	pthread_t tid;
};

/**
 * A printer group.  A group represents a collection of printers that can pull
 * from the same job queue.
 */
struct printer_group
{
	// the next group in the system
	struct printer_group * next_group;
	// the name of this group
	char * name;
	// the list of printers in this group
	struct printer * printer_queue;
	// the list of jobs for this group
	struct print_job_list job_queue;
};

int main(int argc, char* argv[])
{
	int produce = 0;
	int n_jobs = 0;
	struct printer_group * g;
	struct printer * p;
	struct print_job * job;
	struct print_job * prev = NULL;
	char * line = NULL;
	size_t n = 0;
	long long job_number = 0;

	// parse the command line arguments
	parse_command_line(argc, argv);

	// open the runtime config file
	FILE* config = fopen("config.rc", "r");
	// parse the config file
	parse_rc_file(config);
	// close the config file
	fclose(config);

	// order of opperation:
	// 1. while the exit flag has not been set
	// 2. read from standard in
	// 3. do the necessary checks from standard in
	// 4. if PRINT is received, go through each printer group, and check
	//    to see if the group name matches what is specified by the job
	// 5. Once a match is made, it puts the job in the job queue of the
	//    correct printer group
	// 6. then loop through the list of printer groups to get the print
	//    job and send it to the printers
	for(g = printer_group_head; g; g = g->next_group)
	{
		sem_init(&g->job_queue.num_jobs, 0, 0);
	}
	
	// Set server to run as background daemon if user specified argument is received
	if (daemon_process == 1) {
		daemon(1, 0); // Run daemon in root directory so it can be seen by client
	}
	
	// Setup socket connection (or message queue) with client
	// Declare server and client addresses
	struct sockaddr_in serveraddr, clientaddr;
	
	// Declare sersock and consock variables with IP information
	int sersock, consock;
	int PORT_NUM = 51181;
	char* SERVER_IP = "127.0.0.1";
	
	socklen_t len = sizeof(clientaddr);
	
	// Create TCP socket and check for error handling
	if ( (sersock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "socket() error:%d\n", errno);
		exit(1);
	}
	
	// Specify an endpoint address for server
	serveraddr.sin_family = PF_INET;
	serveraddr.sin_port = htons(PORT_NUM);
	serveraddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	
	// Bind socket to address
	int bind_result;
	bind_result = bind(sersock, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
	
	// Bind error checking
	if (bind_result < 0) {
		fprintf(stderr, "Socket could not be bound to server address!\n");
		return -1;
	}
	
	// Listen for connections 
	int listen_result;
	listen_result = listen(sersock, 1);
	 
	// Listen error checking
	if (listen_result < 0) {
		fprintf(stderr, "Server failed to begin listening!\n");
		return -2;
	}
	
	while(!exit_flag)
	{
		
		// Read command sent from client (cmd specified by client)
		// Accept an incoming connection request
		consock = accept(sersock, (struct sockaddr *) &clientaddr, &len);
		
		// Accept error checking
		if (consock < 0) {
			fprintf(stderr, "The following error occurred: %d\n", errno);
		}
		
		// Read data sent from client
		int read_value;
		// Declare buffer for writing and reading from client
		char buffer[BUFFER_SIZE];
	
		read_value = read(consock, buffer, BUFFER_SIZE);
	
		if (read_value < 0) {
			perror("The following error occurred:");
		}
		
		// Detetrmine which command was sent from client
		if (strncmp(buffer, "PRINTER_PRINT", 13) == 0) {
			// If a create print command sent:
			// Parse sent print job details (in client-specified format)
			produce = 1;
			
			// Read in print job information
			read_value = read(consock, buffer, BUFFER_SIZE);
	
			if (read_value < 0) {
				perror("The following error occurred:");
			}
		
			// Declare variables to store user input
			int numArgs = 1;

			// Count the number of arguments
			int i;
			for (i = 0; i < strlen(buffer); i++) {
				if (buffer[i] == ' ') {
					numArgs++;
				}
			}
		
			// Separate the arguments by @
			char* prints = strtok(buffer, "@");

			// Declare Variables to store user arguments
			char* print_requests[numArgs];

			// Get the command and arguments from user
			i = 0;
			while (prints != NULL) {
				print_requests[i] = prints;
				prints = strtok(NULL, "@");

				i++;
			}
		
			// If we are able to produce
			if(produce){
				
				// For each argument sent from client
				for (i = 0; i < numArgs; i++) {
					// Stores one argument
					line = print_requests[i];
					
					if (line == NULL) {
						break;
					}
					
					// Determin what the argument is and process
					if(strncmp(line, "NEW", 3) == 0)
					{
						job = calloc(1, sizeof(struct print_job));
						job->job_number = job_number++;
					}
					else if(job && strncmp(line, "FILE", 4) == 0)
					{
						n = strlen(line)-4;
						strtok(line, " ");
						job->file_name = malloc(n);
						strncpy(job->file_name, strtok(NULL, " "), n);
					}
					else if(job && strncmp(line, "NAME", 4) == 0)
					{
						n = strlen(line)-4;
						strtok(line, " ");
						job->job_name = malloc(n);
						strncpy(job->job_name, strtok(NULL, " "), n);
					}
					else if(job && strncmp(line, "DESCRIPTION", 11) == 0)
					{
						n = strlen(line)-11;
						strtok(line, " ");
						job->description = malloc(n);
						strncpy(job->description, strtok(NULL, " "), n);
					}
					else if(job && strncmp(line, "PRINTER", 7) == 0)
					{
						n = strlen(line)-7;
						strtok(line, " ");
						job->group_name = malloc(n);	
						strncpy(job->group_name, strtok(NULL, " "), n);
					}
					else if(job && strncmp(line, "PRINT", 5) == 0)
					{
						if(!job->group_name)
						{
							eprintf("Trying to print without setting printer\n");
							continue;
						}
						if(!job->file_name)
						{
							eprintf("Trying to print without providing input file\n");
							continue;
						}
						for(g = printer_group_head; g; g=g->next_group)
						{
							if(strcmp(job->group_name, g->name) == 0)
							{
		 						job->next_job = g->job_queue.head;
		 						g->job_queue.head = job;
		 						sem_post(&g->job_queue.num_jobs);
								//
						
								job = NULL;
								produce = 0;
								break;
							}
						}
						if(job)
						{
							eprintf("Invalid printer group name given: %s\n", job->group_name);
							continue;
						}
					}
					else if(strncmp(line, "EXIT", 4) == 0)
					{
						exit_flag = 1;
					}
				}
			}
		} else if (strncmp(buffer, "PRINTER_DRIVERS", 15) == 0) {
			// If a get drivers was sent
			// Loop through and send each driver to client (in printer group)
			
			// Clear out the buffer contents
			int i = 0;
			for (i = 0; i < BUFFER_SIZE; i++) {
				buffer[i] = '\0';
			}
			
			// Find all printer drivers from groups
			for(g = printer_group_head; g; g = g->next_group) {
				// for each printer in the group
				for(p = g->printer_queue; p; p = p->next) {
					if (p != NULL && p->driver.name != NULL && p->driver.description != NULL && p->driver.location) {
						// Send driver
						strcat(buffer, p->driver.name);
						strcat(buffer, "@");
						
						strcat(buffer, p->driver.description);
						strcat(buffer, "@");
						
						strcat(buffer, p->driver.location);
						strcat(buffer, "@");
					}
				}
			}
			
			// Write drivers to client
			int write_result;
			write_result = write(consock, buffer, sizeof(buffer));
	
			// Write error checking
			if (write_result < 0) {
				fprintf(stderr, "Failed to write to server!\n");
			}
		}
		
		// If there is nothing to produce, process print jobs
		if (produce == 0) {
			for(g = printer_group_head; g; g = g->next_group){
				for(p = g->printer_queue; p; p = p->next){
					if(sem_getvalue(&p->job_queue->num_jobs, &n_jobs)){
						perror("sem_getvalue");
						abort();
					}

					if(n_jobs){
						// wait for an item to be in the list
	 					sem_wait(&p->job_queue->num_jobs);
	 					// walk the list to the end
	 					for(job = p->job_queue->head; job->next_job; prev = job, job = job->next_job);
	 					if(prev)		
	 						// fix the tail of the list
							prev->next_job = NULL;
						else
							// There is only one item in the list
							p->job_queue->head = NULL;
		
	 					printf("consumed job %s\n", job->job_name);
	
						// send the job to the printer
						printer_print(&p->driver, job);
					}
				}
			}
		}
		
		// Clear out the buffer contents
		int i = 0;
		for (i = 0; i < BUFFER_SIZE; i++) {
			buffer[i] = '\0';
		}
		
		// Set produce = 0
		produce = 0;
	}

	return 0;
}

/**
 * Parse the command line arguments and set the appropriate flags and variables
 * 
 * Recognized arguments:
 *   - `-v`: Turn on Verbose mode
 *   - `-?`: Print help information
 */
static void parse_command_line(int argc, char * argv[])
{
	int c;
	while((c = getopt(argc, argv, "dv?")) != -1)
	{
		switch(c)
		{
			case 'd': // accept daemon flag
				daemon_process = 1; // Sets to daemon process in main
				break;
			case 'v': // turn on verbose mode
				verbose_flag = 1;
				break;
			case '?': // print help information
				fprintf(stdout, "Usage: %s [options]\n", argv[0]);
				exit(0);
				break;
		}
	}
}

static void parse_rc_file(FILE* fp)
{
	char * line = NULL;
	char * ptr;
	size_t n = 0;
	struct printer_group * group = NULL;
	struct printer_group * g;
	struct printer * printer = NULL;
	struct printer * p;

	// get each line of text from the config file
	while(getline(&line, &n, fp) > 0)
	{
		// if the line is a comment
		if(line[0] == '#')
				continue;

		// If the line is defining a new printer group
		if(strncmp(line, "PRINTER_GROUP", 13) == 0)
		{
			strtok(line, " ");
			ptr = strtok(NULL, "\n");
			group = calloc(1, sizeof(struct printer_group));
			group->name = malloc(strlen(ptr)+1);
			strcpy(group->name, ptr);

			if(printer_group_head)
			{
				for(g = printer_group_head; g->next_group; g=g->next_group);
				g->next_group = group;
			}
			else
			{
				printer_group_head = group;
			}
		}
		// If the line is defining a new printer
		else if(strncmp(line, "PRINTER", 7) == 0)
		{
			strtok(line, " ");
			ptr = strtok(NULL, "\n");
			printer = calloc(1, sizeof(struct printer));
			printer_install(&printer->driver, ptr);
			printer->job_queue =  &(group->job_queue);
			if(group->printer_queue)
			{
				for(p = group->printer_queue; p->next; p = p->next);
				p->next = printer;
			}
			else
			{
					group->printer_queue = printer;

			}
		}
	}

	// print out the printer groups
	dprintf("\n--- Printers ---\n"); 
	for(g = printer_group_head; g; g = g->next_group)
	{
		dprintf("Printer Group %s\n", g->name);
		for(p = g->printer_queue; p; p = p->next)
		{
			dprintf("\tPrinter %s\n", p->driver.name);
		}
	}
	dprintf("----------------\n\n");

}


