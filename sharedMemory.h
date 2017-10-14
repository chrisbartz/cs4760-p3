//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 2

#ifndef SHAREDMEMORY_H_
#define SHAREDMEMORY_H_

// set up shared memory keys for communication
#define SHM_MSG_KEY 98753
#define OSS_SECONDS_KEY 13579
#define OSS_USECONDS_KEY 24680
#define USER_SECONDS_KEY 05050
#define USER_USECONDS_KEY 03030
#define SEM_NAME "cyb01b_p3"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

char* create_shared_memory(int shmKey, int isParent);

char* detach_shared_memory(char* shmpnt);

int write_shared_memory(char* memory, int newdata);

void destroy_shared_memory();

sem_t* open_semaphore(int createSemaphore);

void close_semaphore();

#endif /* SHAREDMEMORY_H_ */
