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
	
	// Loop to read fifo
	while (1) {
		// Open for reading
		int fd = open(fifo, O_RDONLY);
		
		// Read input
		char input[100];
		read(fd, input, sizeof(input));
		
		// Print results
		printf("%s\n", input);
		
		// Close file
		close(fd);
	}

	return 0;
}
