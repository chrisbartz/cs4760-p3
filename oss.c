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

#define DEBUG 0 						// setting to 1 greatly increases number of logging events

//int i = 0;
int totalChildProcessCount = 0; 	// number of total child processes spawned
int parentProcess = 0;
int signalIntercepted = 0; 				// flag to keep track when sigint occurs
int lastChildProcesses = -1;
int ossSeconds;							// store seconds
int ossUSeconds;						// store nanoseconds
int quantum = 10000;					// how many nanoseconds to increment each loop

char* smOssSeconds;
char* smOssUSeconds;
char* shmMsg;
char* smUserSeconds;
char* smUserUSeconds;
pid_t childpids[5000]; 				// keep track of all spawned child pids

//void trim_newline(char *string);
void signal_handler(int signalIntercepted); // handle sigint interrupt
void increment_clock(); // update oss clock in shared memory
void kill_detach_destroy_exit(int status); // kill off all child processes and shared memory

int main(int argc, char *argv[]) {
	int childProcessCount = 0;			// number of child processes spawned

	int maxChildProcessCount = 100; 	// limit of total child processes spawned
	int opt; 							// to support argument switches below
	pid_t childpid;						// store child pid
	char timeVal[30]; 					// store formatted time string for display in logging

	int maxConcSlaveProcesses = 5; 		// max concurrent child processes
	char logFileName[50]; 				// name of log file
	strncpy(logFileName, "log.out", sizeof(logFileName)); // set default log file name
	int totalRunSeconds = 20; 			// set default total run time in seconds


	//gather option flags
	while ((opt = getopt(argc, argv, "hl:q:s:t:")) != -1) {
		switch (opt) {
		case 'l': // set log file name
			strncpy(logFileName, optarg, sizeof(logFileName));
			if (DEBUG) printf("opt l detected: %s\n", logFileName);
			break;
		case 'q': // set quantum amount
			quantum = atoi(optarg);
			if (DEBUG) printf("opt q detected: %d\n", quantum);
			break;
		case 's': // set number of concurrent slave processes
			maxConcSlaveProcesses = atoi(optarg);
			if (DEBUG) printf("opt s detected: %d\n", maxConcSlaveProcesses);
			break;
		case 't': // set number of total run seconds
			totalRunSeconds = atoi(optarg);
			if (DEBUG) printf("opt t detected: %d\n", totalRunSeconds);
			break;
		case 'h': // print help message
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
	smOssSeconds = create_shared_memory(OSS_SECONDS_KEY,1);
	smOssUSeconds = create_shared_memory(OSS_USECONDS_KEY,1);
	shmMsg = create_shared_memory(SHM_MSG_KEY,1);
	smUserSeconds = create_shared_memory(USER_SECONDS_KEY,1);
	smUserUSeconds = create_shared_memory(USER_USECONDS_KEY,1);

	write_shared_memory(smOssSeconds,0);
	write_shared_memory(smOssUSeconds,0);
	write_shared_memory(shmMsg,0);

	if (argc < 1) { /* check for valid number of command-line arguments */
		fprintf(stderr, "Usage: %s command arg1 arg2 ...\n", argv[0]);
		return 1;
	}


	//register signal handler
	signal(SIGINT, signal_handler);

	// this is the main loop
	while (1) {

		//what to do when signal encountered
		if (signalIntercepted) { // signalIntercepted is set by signal handler
			printf("\nmaster: //////////// oss terminating children due to a signal! //////////// \n\n");
			printf("master: parent terminated due to a signal!\n\n");

			kill_detach_destroy_exit(130);
		}

		getTime(timeVal);
		if (DEBUG && lastChildProcesses != childProcessCount) printf("master %s: Child processes count: %d\n", timeVal, childProcessCount);

		// if total forked processes exceed the maximum number
		// then we kill em all
		if (totalChildProcessCount >= maxChildProcessCount
				|| ossSeconds > 1) {
			kill_detach_destroy_exit(0);
		}

		if (parentProcess) increment_clock();

		// if we have forked up to the max concurrent child processes
		// then we wait for one to exit before forking another
		if (childProcessCount >= maxConcSlaveProcesses) {

			// wait for child to send message
			if (atoi(shmMsg) == 0)
				continue;

			getTime(timeVal);
//			if (DEBUG)
				printf("master %s: Child process %d has sent a message: %d.%09d\n", timeVal, atoi(shmMsg), atoi(smUserSeconds), atoi(smUserUSeconds));
			fprintf(logFile,"master %s: Child %d is terminating at my time %d.%09d because it reached %d.%09d in slave\n",
					timeVal, atoi(shmMsg), ossSeconds, ossUSeconds, atoi(smUserSeconds), atoi(smUserUSeconds));

			write_shared_memory(smOssSeconds, 0);
			write_shared_memory(smOssUSeconds, 0);
			write_shared_memory(shmMsg, 0);
			childProcessCount--; //because a child process completed
			lastChildProcesses--;

		}

		char iStr[1];
		sprintf(iStr, "%d", totalChildProcessCount);

		childpid = fork();

		// if error creating fork
		if (childpid == -1) {
			perror("master: Failed to fork");
			return 1;
		}

		// child will execute
		if (childpid == 0) {
			getTime(timeVal);
			if (DEBUG) printf("master %s: Child (fork #%d from parent) will attempt to execl user\n", timeVal, totalChildProcessCount);
			execl("./user", iStr, NULL);
			perror("master: Child failed to execl() the command");
			return 1;
		}

		// parent will execute
		if (childpid != 0) {
			parentProcess = 1;
			childpids[totalChildProcessCount] = childpid; // save child pids in an array
			childProcessCount++; // because we forked above
			totalChildProcessCount++;

			getTime(timeVal);
			if (DEBUG) printf("master %s: parent forked child %d = childPid: %d\n", timeVal, totalChildProcessCount, (int) childpid);

		}



	} //end while loop
	fclose(logFile);

	kill_detach_destroy_exit(0);

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

void increment_clock() {
	const int oneMillion = 1000000000;

	ossUSeconds += quantum;

	if (ossUSeconds >= oneMillion) {
		ossSeconds++;
		ossUSeconds -= oneMillion;
	}

	if (DEBUG)
		printf("master: updating oss clock to %d.%09d\n", ossSeconds, ossUSeconds );
	write_shared_memory(smOssSeconds, ossSeconds);
	write_shared_memory(smOssUSeconds, ossUSeconds);
}

void kill_detach_destroy_exit(int status) {
	// kill all running child processes
	for (int p = 0; p < totalChildProcessCount; p++) {
		if (DEBUG) printf("master: //////////// oss terminating child process %d //////////// \n", (int) childpids[p]);
		kill(childpids[p], SIGTERM);
	}

	// clean up
	detach_shared_memory(smOssSeconds);
	detach_shared_memory(smOssUSeconds);
	detach_shared_memory(shmMsg);
	detach_shared_memory(smUserSeconds);
	detach_shared_memory(smUserUSeconds);
	destroy_shared_memory();
	if (status == 0) printf("master: parent terminated normally %d\n\n", totalChildProcessCount);

	exit(status);
}
