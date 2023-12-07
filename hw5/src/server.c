// #include "debug.h"
// #include "client_registry.h"
// #include "transaction.h"
// #include "protocol.h"
// #include "data.h"
// #include <inttypes.h>
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
// 	info("[%d] Starting client service", fd);
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
// 	// trans_show(new_transaction);

	

// 	while (1) {
// 		int get_flag = 0;
// 		int put_flag = 0;
// 		int key_flag = 0;
// 		int value_flag = 0;
// 		int commit_flag = 0;

//         XACTO_PACKET request_pkt;
//         void* request_data = NULL;

//         if (proto_recv_packet(fd, &request_pkt, &request_data) == -1) {
//             break; // Handle errors or client disconnection
//         }

//         // typedef enum {
// 		//     XACTO_NO_PKT,  // Not used
// 		//     XACTO_PUT_PKT, XACTO_GET_PKT, XACTO_KEY_PKT, XACTO_VALUE_PKT, XACTO_COMMIT_PKT,
// 		//     XACTO_REPLY_PKT
// 		// } XACTO_PACKET_TYPE;

//         switch(request_pkt.type){
//         	case XACTO_NO_PKT:
//         		error("XACTO_NO_PKT");
//         		break;
//         	case XACTO_PUT_PKT:
//         		info("[%d:" PRIu32 "] PUT packet received", fd, request_pkt.timestamp_sec);
//         		put_flag = 1;
//         		break;

//         	case XACTO_GET_PKT:
//         		info("[%d:" PRIu32 "] GET packet received", fd, request_pkt.timestamp_sec);
//         		get_flag = 1;
//         		break;

//         	case XACTO_KEY_PKT:
//         		info("[%d:" PRIu32 "] Received key, size %d", fd, request_pkt.timestamp_sec, request_pkt.size);
//         		key_flag = 1;
//         		break;

//         	case XACTO_VALUE_PKT:
//         		info("[%d:" PRIu32 "] Received value, size %d", fd, request_pkt.timestamp_sec, request_pkt.size);
//         		value_flag = 1;
//         		break;

//         	case XACTO_COMMIT_PKT:
//         		info("[%d:" PRIu32 "] COMMIT packet received", fd, request_pkt.timestamp_sec);
//         		commit_flag = 1;
//         		break;


//         	case XACTO_REPLY_PKT:

//         		break;

//         	default:
//         		error("Invalid request");
//         		break;
//         }

//         if(put_flag == 1){
//         	if(key_flag && value_flag){
//         		BLOB *bp = blob_create((char *)request_data, request_pkt.size);
//         		KEY *kp = key_create(bp);

//         	}
//         }

//         // Free any dynamically allocated memory for request_data or response_data
//         if (request_data) {
//             free(request_data);
//         }

//         // [transaction commit/abort checks and other logic...]
//     }

// 	return NULL;
// }