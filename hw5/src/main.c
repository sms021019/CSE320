#include "debug.h"
#include "client_registry.h"
#include "transaction.h"
#include "store.h"
#include "csapp.h"
#include "server.h"
#include "protocol.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>


static void terminate(int status);
int isInputValid(int argc, char* argv[]);
void sighupHandler(int sigNum);

CLIENT_REGISTRY *client_registry;
char* hostname;
char* port;
int qflag = 0;


int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.

    int listenfd, *clientfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  // Enough space for any address
    pthread_t tid;

    if(argc < 2 || isInputValid(argc, argv) == -1){
        fprintf(stderr, "wrong input");
        exit(1);
    }



    struct sigaction sa;
    sa.sa_handler = sighupHandler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    // Set up the handler for SIGHUP
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    // Perform required initializations of the client_registry,
    // transaction manager, and object store.
    client_registry = creg_init();
    trans_init();
    store_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function xacto_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    listenfd = open_listenfd(port);
    if (listenfd < 0) {
        perror("Failed to open listening socket");
        exit(1);
    }

    while (1) {
        clientfdp = malloc(sizeof(int));  // Allocate space for client socket descriptor
        clientlen = sizeof(struct sockaddr_storage);
        *clientfdp = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);


        if (*clientfdp < 0) {
            perror("Failed to accept connection");
            free(clientfdp);
            continue;
        }
        // XACTO_PACKET temp_packet;
        // proto_recv_packet(*clientfdp, &temp_packet, NULL);
        // while(1) sleep(1);
        // Create a thread to handle the client request
        if (pthread_create(&tid, NULL, xacto_client_service, clientfdp) != 0) {
            perror("Failed to create thread");
            free(clientfdp);
            continue;
        }

        // Detach the thread to handle its cleanup after finishing
        pthread_detach(tid);
    }

    // Close listening socket (never reached in this example)
    close(listenfd);
    terminate(EXIT_FAILURE);
    return 0;

}

int stringToInt(const char *str) {
    int result = 0;

    // Iterate through each character of the string
    for (int i = 0; str[i] != '\0'; ++i) {
        // Check for non-numeric characters
        if (!isdigit(str[i])) {
            // Non-numeric character found, stop processing
            return -1;
        }

        // Convert character to digit and add to the result
        int digit = str[i] - '0';
        result = result * 10 + digit;
    }

    return result;
}

int isInputValid(int argc, char* argv[]){
    int pflag = 0;
    int hflag = 0;
    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "-p") == 0){
            if(pflag == 1) return -1;
            if(argv[i+1] != NULL){
                port = argv[i+1];
                pflag = 1;
                i++;
                continue;
            }else{
                return -1;
            }
        }
        else if(strcmp(argv[i], "-h") == 0){
            if(hflag == 1) return -1;
            if(argv[i+1] != NULL){
                hostname = argv[i+1];
                hflag = 1;
                i++;
                continue;
            }else{
                return -1;
            }
        }else if(strcmp(argv[i], "-q") == 0){
            if(qflag == 1) return -1;
            qflag = 1;
        }else{
            return -1;
        }
    }

    if(pflag != 1) return -1;
    return 0;
}

void sighupHandler(int sigNum){
    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);
    
    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    // Finalize modules.
    creg_fini(client_registry);
    trans_fini();
    store_fini();

    debug("Xacto server terminating");
    exit(status);
}
