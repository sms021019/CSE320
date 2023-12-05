// #include "debug.h"
// #include "client_registry.h"
// #include "transaction.h"
// #include "protocol.h"
// #include <pthread.h>
// #include <stdlib.h>
// #include <stdio.h>

// CLIENT_REGISTRY *client_registry;

// void *xacto_client_service(void *arg){
// 	if(client_registry == NULL){
// 		error("Client registry is empty");
// 		return NULL;
// 	}
// 	int fd = *((int*)arg);
// 	info("Retrieved file discriptor: %d", fd);
// 	free(arg);

// 	pthread_detach(pthread_self());

// 	if(creg_register(client_registry, fd) == -1){
// 		error("creg_register failed");
// 		return NULL;
// 	}

// 	TRANSACTION *new_transaction = trans_create();
// 	if(new_transaction == NULL){
// 		error("trans_create failed");
// 		return NULL;
// 	}

// 	while(1){
// 		XACTO_PACKET request;
// 		XACTO_PACKET response;
// 		void *data;

// 		if(proto_recv_packet(fd, &request, &data) == -1){
// 			error("proto_recv_packet failed");
// 			break;
// 		}


// 		if(proto_send_packet(fd, &response, &data) == -1){
// 			error("proto_send_packet failed");
// 			break;
// 		}

// 		if


// 	}

// 	return NULL;
// }