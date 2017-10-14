//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 2

//This was based from an example on stack overflow
//https://stackoverflow.com/questions/5656530/how-to-use-shared-memory-with-linux-in-c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define DEBUG 0
#define SHMSIZE 16

int shmids[100]; // store all shmids generated
int numShmids; // keep count of shmids generated

char* create_shared_memory(int shmKey, int isParent) {
	int shmid;
	int isNewShmid = 1;
	if (isParent) {
		if (DEBUG) printf("sharedMemory: Creating shared memory segment\n");
		if ((shmid = shmget(shmKey, SHMSIZE, IPC_CREAT | 0660)) == -1) {
			fprintf(stderr, "sharedMemory: shmget error code: %d", errno);
			perror("sharedMemory: Creating shared memory segment failed\n");
			exit(1);
		}
	} else {
		if (DEBUG) printf("sharedMemory: Opening shared memory segment\n");
		if ((shmid = shmget(shmKey, SHMSIZE, 0660)) == -1) {
			fprintf(stderr, "sharedMemory: shmget error code: %d", errno);
			perror("sharedMemory: Opening shared memory segment failed\n");
			exit(1);
		}
	}
//	if (DEBUG) printf("sharedMemory: Opened shared memory id: %d\n",shmid);

	// check to see if shmid is already stored; if not then store shmid
	for (int i = 0; i < numShmids; i++) {
		if (shmids[i] == shmid) {
			isNewShmid = 0;
			break;
		}
	}

	if (isNewShmid) {
		shmids[++numShmids] = shmid;
	}

	char* sharedMemory = shmat(shmid,NULL,0);
	if (DEBUG)printf("sharedMemory: Shared memory attached at address %p\n", sharedMemory);
	return sharedMemory;

}

char* detach_shared_memory(char* shmpnt) {

	if (DEBUG) printf("sharedMemory: Detaching shared memory segment\n");
	shmdt(&shmpnt);

}

int write_shared_memory(char* sharedMemory, int newdata) {
	if (DEBUG) printf("sharedMemory: Writing to shared memory segment: %d\n", newdata);
	char newdataString[SHMSIZE];
	sprintf(newdataString,"%016d",newdata);
	strncpy(sharedMemory, newdataString, SHMSIZE);
	return 1;

}

void destroy_shared_memory() {
	if (DEBUG) printf("sharedMemory: Destroying shared memory segments\n");
	for (int i = 0; i < numShmids; i++) {
		int status = shmctl(shmids[i], IPC_RMID, NULL) != 0;
		if(status) printf("sharedMemory: Destroying shared memory segments error: %d\n", errno);
	}

}
