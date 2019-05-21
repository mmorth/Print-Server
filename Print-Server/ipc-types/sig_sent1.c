/**
 * @file      mq_test1.c
 * @author    Jeramie Vens
 * @date      2015-02-24: Last updated
 * @brief     Memory queue example program
 * @copyright MIT License (c) 2015
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
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <string.h>

int signal_sent = 0;

void send_signal() {

	int signal = 42;
	char number[2];
	
	sprintf(number, "%d", signal);

	mqd_t msg_queue = mq_open("/CprE308-Queue", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP, NULL);
	if(msg_queue == -1)
	{
		perror("mq_open\n");
		return;
	}

	if( mq_send(msg_queue, number, strlen(number), 42))
	{
		perror("mq_send");
		return;
	}
	
	mq_unlink("/CprE308-Queue");
}

int main(int argc, char** argv)
{
	signal(42, send_signal);
	
	while (signal_sent == 0) {
		
	}
	
	return 0;
}


