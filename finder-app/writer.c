/*
Write a C application “writer” (finder-app/writer.c)  which can be used as an alternative to the “writer.sh” test script created in assignment1 and using File IO as described in LSP chapter 2.  See the Assignment 1 requirements for the writer.sh test script and these additional instructions:

One difference from the write.sh instructions in Assignment 1:  You do not need to make your "writer" utility create directories which do not exist.  You can assume the directory is created by the caller.

Setup syslog logging for your utility using the LOG_USER facility.

Use the syslog capability to write a message “Writing <string> to <file>” where <string> is the text string written to file (second argument) and <file> is the file created by the script.  This should be written with LOG_DEBUG level.

Use the syslog capability to log any unexpected errors with LOG_ERR level.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

int fd;
char *dir;
char *text;
ssize_t ret = 0;
size_t count;

int main(int argc, char *argv[]){

	openlog(NULL,0,LOG_USER);

	if(argc < 3){
		syslog(LOG_ERR, "One or more parameters missing. Please, input both arguments in the call to writer app");
		printf("One or more parameters missing. Please, input both arguments in the call to writer app\n");
		ret = 1;
	}
	else{
		dir = argv[1]; //set dir to argv input
		text = argv[2];

		syslog(LOG_DEBUG, "Writing %s to %s", text,dir);
		fd = open(dir, O_RDWR | O_APPEND | O_CREAT, 0777); //Open in read write mode
		if(fd==-1){
			syslog(LOG_ERR, "Can't open file %s", dir);
		}
		count = strlen(text);
		ret = write(fd, text, count);
		
		if(ret==-1){
			syslog(LOG_ERR, "Writing %s failed", text);
			perror("writing error");
		}
		else if(ret != count){
			//Possible error but errno not set due to partial writing
			syslog(LOG_ERR, "Possible error but errno not set due to partial writing");
			perror("Possible error but errno not set due to partial writing");
		}
		else{
			ret = 0;
		}

		if(close(fd) == -1){
			syslog(LOG_ERR, "Error closing file %s", dir);
			perror("close");
		}			
	}
	return ret;
}
