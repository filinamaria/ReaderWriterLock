#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
	pthread_mutex_t *mut;
	int writers;
	int readers;
	int waiting;
	pthread_cond_t *writeOK, *readOK;
} rwl;

rwl *lock;

rwl *initlock (void) {
	rwl *lock;
	
	lock = (rwl *)malloc (sizeof (rwl));
	
	if (lock == NULL) return (NULL);
		lock->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
		
	if (lock->mut == NULL) { 
		free (lock); return (NULL); 
	}
	
	lock->writeOK = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
	
	if (lock->writeOK == NULL) { 
		free (lock->mut); 
		free (lock);
		return (NULL); 
	}
	
	lock->readOK = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));

	if (lock->writeOK == NULL) { 
		free (lock->mut); 
		free (lock->writeOK);
		free (lock); 
		return (NULL);
	}
	
	pthread_mutex_init (lock->mut, NULL);
	pthread_cond_init (lock->writeOK, NULL);
	pthread_cond_init (lock->readOK, NULL);
	lock->readers = 0;
	lock->writers = 0;
	lock->waiting = 0;
	return (lock);
}

void readlock (rwl *lock, int d) {
	pthread_mutex_lock (lock->mut);
	
	if (lock->writers || lock->waiting) {
		do {
			printf ("reader %d blocked.\n", d);
			pthread_cond_wait (lock->readOK, lock->mut);
			printf ("reader %d unblocked.\n", d);
		} while (lock->writers);
	}
	
	lock->readers++;
	pthread_mutex_unlock (lock->mut);
	return;
}

void writelock (rwl *lock, int d) {
	pthread_mutex_lock (lock->mut);
	lock->waiting++;
		
	while (lock->readers || lock->writers) {
		printf ("writer %d blocked.\n", d);
		pthread_cond_wait (lock->writeOK, lock->mut);
		printf ("writer %d unblocked.\n", d);
	}
	
	lock->waiting--;
	lock->writers++;
	pthread_mutex_unlock (lock->mut);
	return;
}

void readunlock (rwl *lock) {
	pthread_mutex_lock (lock->mut);
	lock->readers--;
	pthread_cond_signal (lock->writeOK);
	pthread_mutex_unlock (lock->mut);
}

void writeunlock (rwl *lock) {
	pthread_mutex_lock (lock->mut);
	lock->writers--;
	if(lock->waiting){
		pthread_cond_broadcast (lock->writeOK);
	}else{
		pthread_cond_broadcast (lock->readOK);
	}
	pthread_mutex_unlock (lock->mut);
}

void deletelock (rwl *lock) {
	pthread_mutex_destroy (lock->mut);
	pthread_cond_destroy (lock->readOK);
	pthread_cond_destroy (lock->writeOK);
	free (lock);
	return;
}

void* reader(void *d){
	int id = *((int *) d);
	readlock(lock, id);
	printf("reading %d\n", id);
	usleep(1000);
	printf("read %d done\n", id);
	readunlock(lock);
}

void* writer(void *d){
	int id = *((int *) d);
	writelock(lock, id);
	printf("writing %d\n", id);
	usleep(1000);
	printf("write %d done\n", id);
	writeunlock(lock);
}

int main(){
	int numberOfThread = 5;
	int id[numberOfThread];
	pthread_t thread[numberOfThread];
	
	lock = initlock();
	
	int i;
	for(i = 0; i < numberOfThread; i++){
		id[i] = i;
	}
	
	lock = initlock();
	
	pthread_create(&thread[0], NULL, &writer, &id[0]);
	pthread_create(&thread[1], NULL, &reader, &id[1]);
	pthread_create(&thread[2], NULL, &writer, &id[2]);
	pthread_create(&thread[3], NULL, &writer, &id[3]);
	pthread_create(&thread[4], NULL, &reader, &id[4]);
	
	int j;
	for(j = 0; j < numberOfThread; j++){
		pthread_join(thread[j], NULL);
	}
		
	return 0;
}
