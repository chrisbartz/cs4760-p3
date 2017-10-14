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
#define TUNING 1
#define WAIT_INTERVAL 50000 // max time to wait

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
	if (DEBUG)
		fprintf(stdout, "user  %s: child %d (#%d) started normally after execl\n", timeVal, (int) getpid(), childId);

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
	if (TUNING)
		fprintf(stdout, "user  %s: child %d (#%d) read start time in shared memory:       %d.%09d\n"
			"                               child %d (#%d) interval %09d calculates end time: %d.%09d\n",
			timeVal, (int) getpid(), childId, startSeconds, startUSeconds, (int) getpid(), childId, interval, endSeconds, endUSeconds);

	// open semaphore
	sem_t *sem = open_semaphore(0);

	while (!(atoi(smOssSeconds) > endSeconds && atoi(smOssUSeconds) > endUSeconds)); // wait for the end

	// critical section
	// implemented with semaphores

	// wait for our turn
	sem_wait(sem);

	getTime(timeVal);
	if (TUNING) fprintf(stdout, "user  %s: child %d entering CRITICAL SECTION\n", timeVal, (int) getpid());

	// when it is our turn
	critical_section();

	getTime(timeVal);
	if (TUNING) fprintf(stdout, "user  %s: child %d exiting CRITICAL SECTION\n", timeVal, (int) getpid());

	// give up the turn
	sem_post(sem);

	// clean up shared memory
	detach_shared_memory(smOssSeconds);
	detach_shared_memory(smOssUSeconds);
	detach_shared_memory(smUserSeconds);
	detach_shared_memory(smUserUSeconds);

	// close semaphore
	close_semaphore(sem);

	getTime(timeVal);
	if (DEBUG) fprintf(stdout, "user  %s: child %d exiting normally\n", timeVal, (int) getpid());
}
exit(0);
}


// this part should occur within the critical section if
// implemented correctly since it accesses shared file resources
void critical_section() {

	while (atoi(shmMsg) != 0); // wait until shmMsg is clear
//	if (DEBUG) fprintf(stdout, "user  %s: child %d updating shared memory\n", timeVal, (int) getpid());

	// lets capture this moment in time
	exitSeconds = atoi(smOssSeconds);
	exitUSeconds = atoi(smOssUSeconds);

	write_shared_memory(shmMsg, (int) getpid());
	write_shared_memory(smUserSeconds, exitSeconds);
	write_shared_memory(smUserUSeconds, exitUSeconds);

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
