#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "mcached.h"

#define PORT 11211
#define MAX_THREADS 1000

typedef struct table {

    UT_hash_handle hh;
} table;

//table only holds one value per key: replace if existing
table cache_table;

int main(int argc, char **argv) {

    cache_table = NULL;

    //read port num

    //read num of worker threads

    // create a socket
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); /* host network byte order */

    if (bind(srv, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("bind");
        close(srv);
        return EXIT_FAILURE;
    }
    if (listen(srv, 1) < 0) {
        perror("listen");
        close(srv);
        return EXIT_FAILURE;
    }

    int client = accept(srv, NULL, NULL);
    if (client < 0) {
        perror("accept");
        close(srv);
        return EXIT_FAILURE;
    }

    char buffer[sizeof(memcache_req_header_t)] = {0};
    memcache_req_header_t *header = (memcache_req_header_t *)buffer;

    int n;
    n = recv(client, header, sizeof(memcache_req_header_t), MSG_WAITALL);
    if(n != sizeof(memcache_req_header_t)) {
        perror("cannot read header");
        close(client);
        close(srv);
        return EXIT_FAILURE;
    }
    //check validity of header

    uint8_t *key, *value;
    uint32_t body_len = ntohl(header->total_body_length);
    uint16_t key_len = ntohs(header->key_length);
    
    if (body_len > 0) {
        uint8_t *body = malloc(body_len);
        n = recv(client, body, body_len, MSG_WAITALL);
        if (n != body_len) {
            perror("cannot read body");
            close(client);
            close(srv);
            return EXIT_FAILURE;
        }
        key = body;
        value = body + key_len;
    }

    uint8_t opcode = header->opcode;
    printf("header opcode: %d\n", opcode);

    close(client);
    close(srv);
    return EXIT_SUCCESS;
}

/*
//return 1 if found, 0 otherwise
int get(const uint8_t *key) {

}

//add key if DNE, otherwise replace
int add(const uint8_t *key) {

}

int delete(const uint8_t *key) {

}
*/
//worker thread function
//reads data from connected client
//reads request header and checks validity
//performs operation: GET, SET/ADD, DELETE, VERSION, OUTPUT