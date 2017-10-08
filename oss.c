//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 3

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "sharedMemory.h"
#include "timestamp.h"

#define DEBUG 1 						// setting to 1 greatly increases number of logging events

int signalIntercepted = 0; 				// flag to keep track when sigint occurs

//void trim_newline(char *string);
void signal_handler(int signalIntercepted); // handle sigint interrupt

int main(int argc, char *argv[]) {
	int childProcessCount = 0;			// number of child processes spawned
	int opt; 							// to support argument switches below
	pid_t childpid;						// store child pid
	char timeVal[30]; 					// store formatted time string for display in logging
	pid_t childpids[5000]; 				// keep track of all spawned child pids
	int maxConcSlaveProcesses = 5; 		// max concurrent child processes
	char logFileName[50]; 				// name of log file
	logFileName = "log.out"; 			// set default log file name
	int totalRunSeconds = 20; 			// set default total run time in seconds

	//gather option flags
	while ((opt = getopt(argc, argv, "hl:s:t:")) != -1) {
		switch (opt) {
		case 'l':
			logFileName = optarg;
			if (DEBUG) printf("opt f detected: %s\n", logFileName);
			break;
		case 's':
			maxConcSlaveProcesses = atoi(optarg);
			if (DEBUG) printf("opt f detected: %d\n", maxConcSlaveProcesses);
			break;
		case 't':
			totalRunSeconds = atoi(optarg);
			if (DEBUG) printf("opt f detected: %d\n", totalRunSeconds);
			break;
		case 'h':
			if (DEBUG) printf("opt h detected\n");
			fprintf(stderr,"Usage: ./%s <arguments>\n", argv[0]);
			break;
		default:
			break;
		}
	}

	// open log file for writing
	FILE *logFile;
	logFile = fopen(logFileName,"w+");

	if (logFile == NULL) {
		perror("Cannot open log file");
		exit(1);
	}

	// instantiate shared memory from oss
	getTime(timeVal);
	if (DEBUG) printf("\n\nmaster %s: create shared memory\n", timeVal);
	char* smOssSeconds = create_shared_memory(OSS_SECONDS_KEY,1);
	char* smOssUSeconds = create_shared_memory(OSS_USECONDS_KEY,1);
	char* smUserSeconds = create_shared_memory(USER_SECONDS_KEY,1);
	char* smUserUSeconds = create_shared_memory(USER_USECONDS_KEY,1);

//	// attempt to insert all palin values into shared memory - not working properly
//	if (DEBUG) for (int l = 0; l < palinValuesLength; l++) {
//		if (DEBUG) printf("%s\n", palinValues[l]);
//		// simple message example
//		if (write_shared_memory(&sharedMemory[l], palinValues[l])) {
//			getTime(timeVal);
//			if (DEBUG) printf("master %s: write successful\n", timeVal);
//		}
//	}


	if (argc < 1) { /* check for valid number of command-line arguments */
		fprintf(stderr, "Usage: %s command arg1 arg2 ...\n", argv[0]);
		return 1;
	}
	int i = 0;

	//register signal handler
	signal(SIGINT, signal_handler);

	// this is the main loop
	while (1) {

		//what to do when signal encountered
		if (signalIntercepted) { // signalIntercepted is set by signal handler
			printf("\nmaster: //////////// oss terminating children due to a signal! //////////// \n\n");
			// kill all running child processes
			for (int p = 0; p < i; p++) {
				printf("master: //////////// oss terminating child process %d //////////// \n", (int) childpids[p]);
				kill(childpids[p], SIGTERM);
			}

			// close all open shared memory
			detatch_shared_memory(sharedMemory);
			detatch_shared_memory(sharedMemory);
			detatch_shared_memory(sharedMemory);
			detatch_shared_memory(sharedMemory);

			destroy_shared_memory();
			printf("master: parent terminated due to a signal!\n\n");
			exit(130);
		}

		getTime(timeVal);
		if (DEBUG) printf("master %s: Child processes count: %d\n", timeVal, childProcessCount);

		// if total forked processes exceed the number of palindromes
		// then we wait for them to exit and then break out of the while loop
		if (i >= palinValuesLength) {
			while (childProcessCount > 0) {

				int status;
				if (wait(&status) >= 0) {
					getTime(timeVal);
					printf("master %s: Child process exited with %d status\n", timeVal, WEXITSTATUS(status));
					childProcessCount--; //because a child process completed
				}

				if (DEBUG) printf("master %s: Child processes count: %d\n", timeVal, childProcessCount);
			}
			break;
		}

		// if we have forked up to the max concurrent child processes
		// then we wait for one to exit before forking another
		if (childProcessCount >= MAX_CONCURRENT_CHILD_PROCESSES) {
			getTime(timeVal);
			printf("master %s: Maximum child processes (%d) reached.  Currently on %d of %d.  Waiting for a child to terminate\n", timeVal, childProcessCount, i + 1, palinValuesLength);

			int status;
			if (wait(&status) >= 0) {
				getTime(timeVal);
				printf("master %s: Child process exited with %d status\n", timeVal, WEXITSTATUS(status));
				childProcessCount--; //because a child process completed
			}

		}

		char iStr[1];
		sprintf(iStr, "%d", i);

		childpid = fork();

		// if error creating fork
		if (childpid == -1) {
			perror("master: Failed to fork");
			return 1;
		}

		// child will execute
		if (childpid == 0) {
			getTime(timeVal);
			if (DEBUG) printf("master %s: Child (fork #%d from parent) will attempt to execl palin\n", timeVal, i);
			execl("./palin", iStr, palinValues[i], NULL);
			perror("master: Child failed to execl() the command");
			return 1;
		}

		// parent will execute
		if (childpid != 0) {
			childpids[i] = childpid; // save child pids in an array
			childProcessCount++; // because we forked above

			getTime(timeVal);
			if (DEBUG) printf("master %s: parent forked child %d = childPid: %d\n", timeVal, i, (int) childpid);
		}

		i++;

	} //end while loop

	// clean up
	detatch_shared_memory(sharedMemory);
	destroy_shared_memory();
	printf("master: parent terminated normally\n\n");
	fclose(file);
	return 0;
}

// remove newline characters from palinValues
void trim_newline(char *string) {
	string[strcspn(string, "\r\n")] = 0;
}

// handle the ^C interrupt
void signal_handler(int signal) {
	if (DEBUG) printf("\nmaster: //////////// Encountered signal! //////////// \n\n");
	signalIntercepted = 1;
}
