#include "systemcalls.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
	bool ret = false;
	int sysRes = 0;
	sysRes = system(cmd);
	//printf("[DAVID DEBUG] system(%s) returns %d", cmd, sysRes);
	if(sysRes>=0)
	{
		ret = true;
	}

    return ret;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    bool ret = false;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
	int status;
	pid_t pid;
	
	pid = fork();
	if (pid == -1) //In case of error
	{
		ret = false;
		printf("[DAVID DEBUG] pid == -1\n");
	}
	else if (pid == 0) //If is child
	{
	
		execv(command[0], command);
		
		exit(-1);
	}
	int ret_stat=waitpid(pid, &status, 0); //Wait for the launched process to end
	if(ret_stat == -1)
	{
		ret = false;
	}
	else if(WEXITSTATUS (status) != 0)
	{
		ret = false;
	}
	else
	{
		ret = true;
	}	
    va_end(args);
    return ret;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    bool ret = false;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
	int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644); //Open the file to use as output
	int status;
	if (fd<0)
	{
		perror("open"); 
		return false;
	}
	pid_t kidpid = fork();
	switch (kidpid) 
	{
	  case -1: 
	  {
	  	ret = false;
	  }
	  case 0:
	  {
	  	//Redirect stdout to the file fd
		if (dup2(fd, 1) < 0) 
		{ 
			ret = false;
		}
		close(fd);
	
		execv(command[0],command);
		
		exit(-1);	
	  }
	  default:
		close(fd);
	}
	int ret_stat=waitpid(kidpid, &status, 0); //Wait for the launched process to end
	if(ret_stat == -1)
	{
		ret = false;
	}
	else if(WEXITSTATUS (status) != 0)
	{
		ret = false;
	}
	else
	{
		ret = true;
	}	

    va_end(args);

    return ret;
}
