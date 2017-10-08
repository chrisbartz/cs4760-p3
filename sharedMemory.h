//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 2

#ifndef SHAREDMEMORY_H_
#define SHAREDMEMORY_H_

// set up shared memory keys for communication
#define OSS_SECONDS_KEY 13579
#define OSS_USECONDS_KEY 24680
#define USER_SECONDS_KEY 05050
#define USER_USECONDS_KEY 03030

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

char* create_shared_memory(int shmKey, int isParent);

char* detatch_shared_memory(char* shmpnt);

int write_shared_memory(char* memory, char* newdata);

void destroy_shared_memory();

#endif /* SHAREDMEMORY_H_ */
