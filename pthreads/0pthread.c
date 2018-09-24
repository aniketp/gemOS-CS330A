#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t myMutex;
volatile int counter = 0;

void *worker_thread(void *arg)
{
	printf("This is a worker thread #%ld\n", (long)arg);
	pthread_exit(NULL);
}

void *mutex_testing(void *param)
{
	int i;
	for (i = 0; i < 5; i++) {
		pthread_mutex_lock(&myMutex);
		counter ++;
		usleep(1);
		printf("thread %d | counter %d\n", (int)param, counter);
		pthread_mutex_unlock(&myMutex);
	}
}

int main()
{
	pthread_t my_thread[5];
	int retval;

	printf("Creating thread\n");
	long id;
	for (id = 1; id <= 5; id ++){
		retval = pthread_create(&my_thread[id], NULL, &worker_thread, (void *)id);
		if (retval != 0) {
			printf("pthread_create() failed");
			exit(EXIT_FAILURE);
		}
	}

	int status[5];
	for (id = 1; id <= 5; id++)
		pthread_join(my_thread[id], (void **)(status + id));
	
	/* Exit without exiting the child threads */
	printf("Threads exited\n");

	int apple = 1, mango = 2, banana = 3;
	pthread_t thread1, thread2, thread3;
	pthread_mutex_init(&myMutex, 0);
	pthread_create(&thread1, NULL, mutex_testing, (void *)apple);
	pthread_create(&thread2, NULL, mutex_testing, (void *)mango);
	pthread_create(&thread3, NULL, mutex_testing, (void *)banana);
	pthread_join(thread1, 0);
	pthread_join(thread2, 0);
	pthread_join(thread3, 0);
	pthread_mutex_destroy(&myMutex);
	
	pthread_exit(NULL);
	return 0;
	
}

