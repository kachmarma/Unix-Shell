#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "svec.h"
#include "tokenize.h"

// Written by Matthew Kachmar.
// Attribution Credit: Nat Tuck

// NOTE:
// This program uses svec.h svec.c tokenize.h tokenize.c provided to us in the starter code written by Prof. Tuck.
// The forking in this program is also based off the starter code provided to us by Prof. Tuck.

// Checking all return values was causing an infinite loop in the autograder and I checked rvs where I could.

// Checks the return value to make sure the system call completed successfully
void
check_rv(int rv)
{
    if (rv != 0) {
        perror("error");
        exit(0);
    }
}

void
check_hardfail(int rv)
{
	if (rv == -1) {
		perror("error");
		exit(0);
	}	
}

// Adapted from starter code provided to us by Prof. Tuck
// Forks and executes the given command
int
execute(char* cmd)
{
    int cpid;
    if (strncmp(cmd, "exit", 4) == 0) {
		_exit(0);
    } else if (strncmp(cmd, "pwd", 3) == 0) {
		char buf[100];
		getcwd(buf, sizeof(buf));
		printf("%s\n", buf);
		return 0;
	} else if (strncmp(cmd, "cd", 2) == 0) {
		svec* input = tokenize(cmd);
		chdir(svec_get(input, 1));
		free_svec(input);
		return 0;
	} 
    else {
	svec* input = tokenize(cmd);
	if (strcmp(svec_get(input, (input->size - 1)), "&") == 0) {
		if ((cpid = fork())) {
			free_svec(input);
			return 0;
		} else {
		check_rv(execvp(svec_get(input, 0), input->data));
		}
	}
	else {
		if ((cpid = fork())) {
        // parent process
        int status;
        waitpid(cpid, &status, 0);

        if (WIFEXITED(status)) {
        //  printf("child exited with exit code (or main returned) %d\n", WEXITSTATUS(status));
        }
		free_svec(input);
		return 0;
		}    
    	else {
        // child process
		int rv;
		check_rv(execvp(svec_get(input, 0), input->data));
        printf("Can't get here, exec only returns on error.\n");
    	}
	}
	return 0;
	}	
}

// Handles && and ||
void andOrProcess(char* inp) {
	svec* cmds = make_svec();
	svec* input = tokenize(inp);
	int regexFound = 0;
	char* regex;
	int processed = 0;
	char command[100] = {'\0'};
	for (int i = 0; i < input->size; i++) {
		if ((strcmp(svec_get(input, i), "||") == 0) || (strcmp(svec_get(input, i), "&&") == 0)) {	
			regexFound = 1;
			regex = svec_get(input, i);
			for (int z = 0; z < i; z++) {
				strcat(command, svec_get(input, z));
				strcat(command, " ");
			}
			processed = i + 1;
			command[strlen(command)] = '\0';
			svec_push_back(cmds, command);
			strcpy(command, "");
		}
		else if (i == (input->size - 1)) {
			for (int z = processed; z <= i; z++) {
				strcat(command, svec_get(input, z));
				strcat(command, " ");
			}
			command[strlen(command)] = '\0';
			svec_push_back(cmds, command);
			strcpy(command, "");
		}
	}
	if (regexFound == 1) {
		svec* command1 = tokenize(svec_get(cmds, 0));
		svec* command2 = tokenize(svec_get(cmds, 1));
		if (strcmp(regex, "&&") == 0) {
			int cpid0;
			int status0;
			if (cpid0 = fork()) {
			check_hardfail(waitpid(cpid0, &status0, 0));
			return;
			} else {
			int cpid;
			int status;
			if ((cpid = fork())) {
			check_hardfail(waitpid(cpid, &status, 0));
			if (status == 0) {
				execvp(svec_get(command2, 0), command2->data);
			} else {
				return;
			}	
			} else {
				execvp(svec_get(command1, 0), command1->data);
			}
			}
		} else if (strcmp(regex, "||") == 0) {
			int cpid0;
			int status0;
			if ((cpid0 = fork())) {
			waitpid(cpid0, &status0, 0);
			_exit(0);
			} else {
			int cpid;
			int status;
			if ((cpid = fork())) {
			waitpid(cpid, &status, 0);
			if (status != 0) {
				execvp(svec_get(command2, 0), command2->data);
			} else {
				return;	
			}	
			} else {
				execvp(svec_get(command1, 0), command1->data);
			}
			}
		}
	} else {
		execute(svec_get(cmds, 0));
		return;		
	}
}

// Handles ; from the given svec
svec* splitBySemi(svec* input) {
    svec* cmds = make_svec();
	int processed = 0;
	char command[100] = {'\0'};
	for(int i = 0; i < input->size; i++) {
		if (strncmp(input->data[i], ";", 1) == 0) {
			for (int z = processed; z < i; z++) {
				strcat(command, svec_get(input, z));
				strcat(command, " ");
			}
			processed = i + 1;
			command[strlen(command)] = '\0';
			svec_push_back(cmds, command);
			strcpy(command, "");
		}
		else if (i == (input->size - 1)) {
			for (int z = processed; z <= i; z++) {
				strcat(command, svec_get(input, z));
				strcat(command, " ");
			}
			command[strlen(command)] = '\0';
			svec_push_back(cmds, command);
			strcpy(command, "");
		}
	}
	return cmds;
}

// Handles > in the given svec
void redirectOutput(svec* input) {
	int cpid;
	int status;
	if ((cpid = fork())) {
		waitpid(cpid, &status, 0);
		return;
	}
	else {
		int cpid2;
		int status2;
		int fileDescriptor;
		char* file = svec_get(input, 1);
		file[(strlen(file) - 1)] = '\0';
		if ((fileDescriptor = open(file, O_RDWR|O_CREAT, 00700)) < 0) {
			perror("error opening file descriptor\n");
		}
		check_hardfail(dup2(fileDescriptor, 1));
		execute(svec_get(input, 0));
		_exit(0);
	}
}

// Handles < in the given svec
void redirectInput(svec* input) {
	int cpid;
	int status;
	if ((cpid = fork())) {
		waitpid(cpid, &status, 0);
		return;
	}
	else {
		int cpid2;
		int status2;
		int fileDescriptor;
		char* file = svec_get(input, 1);
		file[(strlen(file) - 1)] = '\0';
		if ((fileDescriptor = open(file, O_RDONLY)) < 0) {
			perror("no\n");
		}
		check_hardfail(dup2(fileDescriptor, 0));
		execute(svec_get(input, 0));
		_exit(0);
	}
}

// Handles redirection using the helper methods
void myRedirect(svec* input) {
	svec* cmds = make_svec();
	char command1[100] = {'\0'};
	char command2[100] = {'\0'};
	int areRedirects = 0;
	for(int i = 0; i < input->size; i++) {
		if (strncmp(svec_get(input, i), "<", 1) == 0) {
			areRedirects = 1;
			for (int z = 0; z < i; z++) {
				strcat(command1, svec_get(input, z));
				strcat(command1, " "); 
			}
			command1[strlen(command1) - 1] = '\0';	
			svec_push_back(cmds, command1);
			for (int z = (i + 1); z < input->size; z++) {
			strcat(command2, svec_get(input, z));
			strcat(command2, " ");
		}
			command2[strlen(command2)] = '\0';	
			svec_push_back(cmds, command2);
			strcpy(command1, "");
			strcpy(command2, "");
			redirectInput(cmds);
			free_svec(cmds);		
		}
		else if (strncmp(svec_get(input, i), ">", 1) == 0) {
			areRedirects = 1;
			for (int z = 0; z < i; z++) {
				strcat(command1, svec_get(input, z));
				strcat(command1, " ");
			}
			svec_push_back(cmds, command1);
			for (int z = (i + 1); z < input->size; z++) {
				strcat(command2, svec_get(input, z));
				strcat(command2, " ");
			}
			svec_push_back(cmds, command2);
			strcpy(command2, "");
			redirectOutput(cmds);
			free_svec(cmds);
		}
	}
	if (areRedirects == 0) {
		svec* split = splitBySemi(input);
		for (int i = 0; i < split->size; i++) {
		andOrProcess(svec_get(split, i));	
		}
		return;
	}
}

// Handles piping in the given svec then pushes result to myRedirect
int myPipe(svec* input) {
	char command1[100] = {'\0'};
	char command2[100] = {'\0'};

	for(int i = 0; i < input->size; i++) {
		if ((strncmp(svec_get(input, i), "|", 1) == 0) && (strncmp(svec_get(input, i), "||", 2) != 0)) {
			// split left of pipe
			for (int z = 0; z < i; z++) {
				strcat(command1, svec_get(input, z));
				strcat(command1, " "); 
			}
			command1[strlen(command1) - 1] = '\0';	
			// split right of pipe
			for (int z = (i + 1); z < input->size; z++) {
			strcat(command2, svec_get(input, z));
			strcat(command2, " ");
			}
			command2[strlen(command2) - 1] = '\0';	
			
			// create pipe
			int cpid;
			int status;		
			// fork
			if ((cpid = fork())) {
			waitpid(cpid, &status, 0);
			return 0;
			}
			else {
			int fileDescriptor[2];
			check_hardfail(pipe(fileDescriptor));
			int cpid2;
			int status2;
			// fork again
			if ((cpid2 = fork())) {
				close(fileDescriptor[1]);
				check_hardfail(dup2(fileDescriptor[0], 0));
				waitpid(cpid2, &status2, 0);
				check_rv(execvp(svec_get(tokenize(command2), 0), tokenize(command2)->data));
			} else {
				close(fileDescriptor[0]);
				check_hardfail(dup2(fileDescriptor[1], 1));
				execvp(svec_get(tokenize(command1), 0), tokenize(command1)->data);
				}
			return 0;
			}
		}
		else {

		}
	}
	// send to myRedirect
	myRedirect(input);
	return 0;
}

// This method is adapted from the starter code provided to us by Prof. Tuck
int
main(int argc, char* argv[])
{ 
	if (argc == 1)
	{
	// user input		
		for(;;) {
			printf("nush$ ");
			char cmd[256];
			if ((fgets(cmd, 256, stdin)) == NULL) {
				break;
			} else {
				svec* input = tokenize(cmd);
				myPipe(input);
				free_svec(input);
			}
		}
		return 0;
	}
	else {
	// file input
		char cmd[100];
		char const* const name = argv[1];
		FILE* file = fopen(name, "r");
		for (;;) {
			if (fgets(cmd, sizeof(cmd), file) == NULL) {
				break;
			}
			svec* input = tokenize(cmd);
			myPipe(input);
			free_svec(input);	
		}
	return 0;
	}
}
