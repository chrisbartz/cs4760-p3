//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 3

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include "sharedMemory.h"
#include "timestamp.h"

#define DEBUG 1 // setting to 1 greatly increases number of logging events
#define WAIT_INTERVAL 1000000 // max time to wait

char* smOssSeconds;
char* smOssUSeconds;
char* shmMsg;
char* smUserSeconds;
char* smUserUSeconds;
int childId; 				// store child id number assigned from parent
int startSeconds;			// store oss seconds when initializing shared memory
int startUSeconds;			// store oss nanoseconds when initializing shared memory
int endSeconds;				// store oss seconds to exit
int endUSeconds;			// store oss nanoseconds to exit
int exitSeconds;			// store oss seconds when exiting
int exitUSeconds;			// store oss nanoseconds when exiting


void critical_section();
void signal_handler(int signalIntercepted); // handle sigterm interrupt

int main(int argc, char *argv[]) {
childId = atoi(argv[0]); // saves the child id passed from the parent process
char timeVal[30]; // formatted time values for logging
time_t t;
srand((unsigned)time(&t)); // random generator
int interval = (rand() % WAIT_INTERVAL) + 1;
const int oneMillion = 1000000000;

// handle SIGTERM from parent
signal(SIGTERM, signal_handler);

// a quick check to make sure user received a child id
getTime(timeVal);
if (childId < 0) {
	if (DEBUG) fprintf(stderr, "user  %s: Something wrong with child id: %d\n", timeVal, getpid());
	exit(1);
} else {
	if (DEBUG) fprintf(stdout, "user  %s: Child %d started normally after execl\n", timeVal, (int) getpid());

	// attach to shared memory
	smOssSeconds = create_shared_memory(OSS_SECONDS_KEY,0);
	smOssUSeconds = create_shared_memory(OSS_USECONDS_KEY,0);
	shmMsg = create_shared_memory(SHM_MSG_KEY,0);
	smUserSeconds = create_shared_memory(USER_SECONDS_KEY,0);
	smUserUSeconds = create_shared_memory(USER_USECONDS_KEY,0);

	startSeconds = atoi(smOssSeconds);
	startUSeconds = atoi(smOssUSeconds);

	endSeconds = startSeconds;
	endUSeconds = startUSeconds + interval;

	if (endUSeconds > oneMillion) {
		endSeconds++;
		endUSeconds -= oneMillion;
	}

	getTime(timeVal);
	if (DEBUG) fprintf(stdout, "user  %s: Child %d read start time in shared memory: %d.%d\n"
			"                               Child %d calculates end time: %d.%d\n",
			timeVal, (int) getpid(), startSeconds, startUSeconds, (int) getpid(), endSeconds, endUSeconds);

	while (atoi(smOssSeconds) < endSeconds && atoi(smOssUSeconds) < endUSeconds); // wait for the end

	// critical section
	// this is where the multiple processor solution is supposed to be implemented if shared memory was working
	int flag[500];
	int idle = 0;
	int want_in = 1;
	int in_cs = 3;
	int i = childId;
	int j, n, turn;

	//n = number_of_processes;

//	do {
//			do {
//				flag[i] = want_in; // Raise my flag
//				j = turn; // Set local variable
//				// wait until its my turn
//				while (j != i)
//					j = (flag[j] != idle) ? turn : (j + 1) % n;
//				// Declare intention to enter critical section
//				flag[i] = in_cs;
//				// Check that no one else is in critical section
//				for (j = 0; j < n; j++)
//					if ((j != i) && (flag[j] == in_cs))
//						break;
//			} while ((j < n) || (turn != i && flag[turn] != idle));
//			// Assign turn to self and enter critical section
//			turn = i;
			critical_section();
//			// Exit section
//			j = (turn + 1) % n;
//			while (flag[j] == idle)
//				j = (j + 1) % n;
//			// Assign turn to next waiting process; change own flag to idle
//			turn = j;
//			flag[i] = idle;
//			remainder_section();
//		} while (1);



	// end critical section


	// clean up shared memory
	detach_shared_memory(smOssSeconds);
	detach_shared_memory(smOssUSeconds);
	detach_shared_memory(smUserSeconds);
	detach_shared_memory(smUserUSeconds);

	getTime(timeVal);
	if (DEBUG) fprintf(stdout, "user  %s: Child %d exiting normally\n", timeVal, (int) getpid());
}
exit(0);
}


// this part should occur within the critical section if
// implemented correctly since it accesses shared file resources
void critical_section() {
	char timeVal[30];
	getTime(timeVal);
	fprintf(stdout, "user  %s: Child %d entering CRITICAL SECTION\n", timeVal, (int) getpid());


	while (atoi(shmMsg) != 0); // wait until shmMsg is clear
	if (DEBUG) fprintf(stdout, "user  %s: Child %d updating shared memory\n", timeVal, (int) getpid());

	//capture this moment in time
	exitSeconds = atoi(smOssSeconds);
	exitUSeconds = atoi(smOssUSeconds);

	write_shared_memory(shmMsg, (int) getpid());
	write_shared_memory(smUserSeconds, exitSeconds);
	write_shared_memory(smUserUSeconds, exitUSeconds);

//	fprintf(file, "%s\n", user);
	//sleep(forAWhile);
//	fclose(file);
	getTime(timeVal);
	fprintf(stdout, "user  %s: Child %d exiting CRITICAL SECTION\n", timeVal, (int) getpid());
}

// handle the interrupt
void signal_handler(int signal) {
	if (DEBUG) printf("child: //////////// Encountered signal! //////////// \n\n");
	detach_shared_memory(smOssSeconds);
	detach_shared_memory(smOssUSeconds);
	detach_shared_memory(smUserSeconds);
	detach_shared_memory(smUserUSeconds);
	exit(0);
}
