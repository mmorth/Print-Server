/* A short program that uses a named FIFO to print any line
 * entered into the program on one terminal out on the other
 * terminal
*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
	// FIFO special file location
	char* fifo = "test3_fifo";
	
	// Create named fifo
	mkfifo(fifo, 0666);
	
	// Loop to write multiple results
	while (1) {
		// Open FIFO in read-only
		int fd = open(fifo, O_WRONLY);
	
		// Get input from user
		char input[100];
		fgets(input, 100, stdin);
	
		// Write input to FIFO
		write(fd, input, sizeof(input));
	
		// Close buffer
		close(fd);
	}

	return 0;
}
