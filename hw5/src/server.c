#include "debug.h"
#include "client_registry.h"
#include "transaction.h"
#include "protocol.h"
#include "data.h"
#include "store.h"
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

CLIENT_REGISTRY *client_registry;
const int BUFFER_SIZE = 3;

typedef struct packet_buffer {
    XACTO_PACKET packet;
    void *data;
} PACKET_BUFFER;

void clear_packet_buffer(PACKET_BUFFER *packet_buffer, int buffer_counter);

XACTO_PACKET get_response_packet(uint32_t serial){
	XACTO_PACKET response_packet;
	response_packet.type = XACTO_REPLY_PKT;
	response_packet.status = 0;
	response_packet.null = 0;
	response_packet.serial = serial;
	response_packet.size = 0;

	return response_packet;
}


void *xacto_client_service(void *arg){
	// Registry Empty check
	if(client_registry == NULL){
		error("Client registry is empty");
		return NULL;
	}

	// Extract file descriptor
	int fd = *((int*)arg);
	info("[%d] Starting client service", fd);
	free(arg);	// The storage freed
	// Thread detach from main
	pthread_detach(pthread_self());

	// Register the client file descriptor with client registry
	if(creg_register(client_registry, fd) == -1){
		error("creg_register failed");
		return NULL;
	}

	//Create new transition
	TRANSACTION *new_transaction = trans_create();
	if(new_transaction == NULL){
		error("trans_create failed");
		return NULL;
	}

	// Allocate the packet buffer and flags
	PACKET_BUFFER *packet_buffer = malloc(sizeof(PACKET_BUFFER) * BUFFER_SIZE);
	int get_flag = 0;
	int put_flag = 0;
	int key_flag = 0;
	int value_flag = 0;
	int commit_flag = 0;
	int buffer_counter = 0;

	while (1) {		// Thread main loop start
        XACTO_PACKET request_pkt;
        void* request_data = NULL;

        // Receive packet from the user client
        if (proto_recv_packet(fd, &request_pkt, &request_data) == -1) {
        	error("proto_recv_packet failed");
            break;
        }

        // Check for buffer overflow
		if(buffer_counter >= BUFFER_SIZE){
			error("Buffer overflow");
			break;
		}

		// Flag control panel
        switch(request_pkt.type){
        	case XACTO_NO_PKT:
        		error("XACTO_NO_PKT");
        		break;
        	case XACTO_PUT_PKT:
        		info("[%d:%d] PUT packet received", fd, ntohl(request_pkt.serial));
        		packet_buffer[buffer_counter].packet = request_pkt;
        		packet_buffer[buffer_counter].data = NULL;
        		buffer_counter++;
        		put_flag = 1;
        		break;

        	case XACTO_GET_PKT:
        		info("[%d:%d] GET packet received", fd, ntohl(request_pkt.serial));
        		packet_buffer[buffer_counter].packet = request_pkt;
        		packet_buffer[buffer_counter].data = NULL;
        		buffer_counter++;
        		get_flag = 1;
        		break;

        	case XACTO_KEY_PKT:
        		info("[%d:%d] Received key, size %d", fd, ntohl(request_pkt.serial), request_pkt.size);
        		packet_buffer[buffer_counter].packet = request_pkt;
        		packet_buffer[buffer_counter].data = request_data;
        		buffer_counter++;
        		key_flag = 1;
        		break;

        	case XACTO_VALUE_PKT:
        		info("[%d:%d] Received value, size %d", fd, ntohl(request_pkt.serial), request_pkt.size);
        		packet_buffer[buffer_counter].packet = request_pkt;
        		packet_buffer[buffer_counter].data = request_data;
        		buffer_counter++;
        		value_flag = 1;
        		break;

        	case XACTO_COMMIT_PKT:
        		info("[%d:%d] COMMIT packet received", fd, ntohl(request_pkt.serial));
        		packet_buffer[buffer_counter].packet = request_pkt;
        		packet_buffer[buffer_counter].data = NULL;
        		buffer_counter++;
        		commit_flag = 1;
        		break;

        	case XACTO_REPLY_PKT:
        		break;

        	default:
        		error("Invalid request");
        		break;
        }

        // Request handler by flags
        if(put_flag){	// put request handle
        	if(key_flag && value_flag){
        		BLOB *key_blob_pt = blob_create((char *)packet_buffer[1].data, packet_buffer[1].packet.size);
        		KEY *kp = key_create(key_blob_pt);
        		BLOB *value_blob_pt = blob_create((char *)packet_buffer[2].data, packet_buffer[2].packet.size);

        		// Mapping the new key and value
        		if(store_put(new_transaction, kp, value_blob_pt) == TRANS_ABORTED){
        			error("store_put | Mapping key and value aborted");
        			break;
        		}

        		// Send reply
        		XACTO_PACKET response_packet = get_response_packet(packet_buffer[0].packet.serial);
        		if (proto_send_packet(fd, &response_packet, NULL) == -1) {
		        	error("proto_send_packet failed");
		            break;
		        }

		        // Deallocate data and reset flags
		        clear_packet_buffer(packet_buffer, buffer_counter);
		        get_flag = 0;
				put_flag = 0;
				key_flag = 0;
				value_flag = 0;
				commit_flag = 0;
				buffer_counter = 0;
        	}
        }else if(get_flag){		// Get request handle
        	if(key_flag){
        		BLOB *key_blob_pt = blob_create((char *)packet_buffer[1].data, packet_buffer[1].packet.size);
        		KEY *kp = key_create(key_blob_pt);
        		BLOB *result_blob;

        		// Search the matching key
        		if(store_get(new_transaction, kp, &result_blob) == TRANS_ABORTED){
        			error("store_get | finding by key aborted");
        			break;
        		}

        		// Send the reply
        		XACTO_PACKET response_packet = get_response_packet(packet_buffer[0].packet.serial);
        		if (proto_send_packet(fd, &response_packet, NULL) == -1) {
		        	error("proto_send_packet failed");
		            break;
		        }

		        // Send the found value
		        if (result_blob != NULL){
        			XACTO_PACKET result_packet = get_response_packet(packet_buffer[0].packet.serial);
        			result_packet.size = result_blob->size;
        			result_packet.type = XACTO_VALUE_PKT;
        			if (proto_send_packet(fd, &result_packet, result_blob->content) == -1) {
			        	error("proto_send_packet failed");
			            break;
			        }
		        }

		        // Deallocate data and reset flags
		        clear_packet_buffer(packet_buffer, buffer_counter);
		        get_flag = 0;
				put_flag = 0;
				key_flag = 0;
				value_flag = 0;
				commit_flag = 0;
				buffer_counter = 0;
        	}
        }else if(commit_flag){		// Commit request handle
        	trans_commit(new_transaction);

        	// Send reply
        	XACTO_PACKET commit_reply = get_response_packet(packet_buffer[0].packet.serial);
        	if(proto_send_packet(fd, &commit_reply, NULL) == -1){
        		error("proto_send_packet failed");
        		clear_packet_buffer(packet_buffer, buffer_counter);
        		free(packet_buffer);
        		return NULL;
        	}

        	// Ending client service
    		info("[%d] Ending client service", fd);
    		if (close(fd) == -1) {
			    error("Error closing socket");
			}
    		creg_unregister(client_registry, fd);
    		clear_packet_buffer(packet_buffer, buffer_counter);
    		free(packet_buffer);
    		return NULL;
        }else{
        	error("Invalid request");
        	break;
        }

    }//END of the while loop


    // Abort the transtion and end the client service
    info("[%d] Ending client service", fd);
    trans_abort(new_transaction);
    creg_unregister(client_registry, fd);
    clear_packet_buffer(packet_buffer, buffer_counter);
    free(packet_buffer);

	return NULL;
}

void clear_packet_buffer(PACKET_BUFFER *packet_buffer, int buffer_counter){
	for(int i = 0; i < buffer_counter; i++){
		if(packet_buffer[i].data != NULL){
			free(packet_buffer[i].data);
		}
	}
}