#include <stdlib.h>
#include <sys/select.h>//FD_SETSIZE
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <unistd.h>
#include "debug.h"
#include "client_registry.h"

#define INITIAL_CAPACITY 10

pthread_mutex_t mutex;
sem_t sem;

struct client_registry {
    int *clients;
    int capacity;
    int size;
};


CLIENT_REGISTRY *creg_init(){
	debug("Initialize client registry");
	CLIENT_REGISTRY *registry = malloc(sizeof(CLIENT_REGISTRY));
	if(registry == NULL){
		error("Memory allocating failed: CLIENT_REGISTRY");
		return NULL;
	}

	registry->clients = malloc(sizeof(int) * INITIAL_CAPACITY);
	if(registry->clients == NULL){
		error("Memomry allocating failed: CLIENTS");
		free(registry);
		return NULL;
	}
	registry->capacity = INITIAL_CAPACITY;
	registry->size = 0;

	sem_init(&sem, 0, 1);
	pthread_mutex_init(&mutex, NULL);

	return registry;
}

void creg_fini(CLIENT_REGISTRY *cr){
	free(cr->clients);
	free(cr);

	sem_destroy(&sem);
	pthread_mutex_destroy(&mutex);
}


int creg_register(CLIENT_REGISTRY *cr, int fd){
	pthread_mutex_lock(&mutex);
	if(cr->size == cr->capacity){
		int new_capacity = cr->capacity + INITIAL_CAPACITY;
		int *new_clients = realloc(cr->clients, sizeof(int) * new_capacity);
		if(new_clients == NULL){
			error("Memory reallocation failed: new_clients");
			return -1;
		}
		cr->clients = new_clients;
		cr->capacity = new_capacity;
	}

	cr->clients[cr->size] = fd;
	cr->size++;
	pthread_mutex_unlock(&mutex);
	return 0;
}

int creg_unregister(CLIENT_REGISTRY *cr, int fd){
	pthread_mutex_lock(&mutex);
	if(cr->size == 0){
		error("No registered clients");
		return -1;
	}

	for(int i = 0; i < cr->size; i++){
		if(cr->clients[i] == fd){
			for(int j = i; j < cr->size - 1; j++){
				cr->clients[j] = cr->clients[j+1];
			}
			cr->size--;
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
	if(cr->size == 0){
		debug("Releasing");
		sem_post(&sem);
	}
	return 0;
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr){
	sem_wait(&sem);
	debug("Size of registered clients reached zero.\n");
}

void creg_shutdown_all(CLIENT_REGISTRY *cr){
	for(int i = 0; i < cr->size; i++){
		if(cr->clients[i] != -1){
			shutdown(cr->clients[i], SHUT_RDWR);
			close(cr->clients[i]);
			cr->clients[i] = -1;
		}
	}
}


