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

typedef struct hash_node {
    uint8_t *key, *val;
    uint16_t key_len;
    uint32_t val_len;
    UT_hash_handle hh;
} hash_node;

typedef struct thread_args {
    int client;
    int thread_num;
    int port;
} thread_args;

void process_request(int client, memcache_req_header_t *header, uint8_t *key, uint8_t *value, uint16_t key_len, uint32_t val_len);

//table only holds one value per key: replace if existing
hash_node *cache_table;

int main(int argc, char **argv) {

    cache_table = NULL;

    //read port num
    int port = atoi(argv[1]);
    //read num of worker threads
    //int num_threads = strtol(argv[2]);

    // create a socket
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(srv);
        return EXIT_FAILURE;
    }
    if (listen(srv, 128) < 0) {
        perror("listen");
        close(srv);
        return EXIT_FAILURE;
    }

    //queue client requests: terminate when no more clients
    while(1) {
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
        while(n != 0) {
            if(n != sizeof(memcache_req_header_t)) {
                perror("cannot read header");
                close(client);
                close(srv);
                return EXIT_FAILURE;
            }
            uint8_t *key, *value;
            uint32_t body_len = ntohl(header->total_body_length);
            uint16_t key_len = ntohs(header->key_length);
            uint32_t val_len = body_len - key_len;
            
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

            process_request(client, header, key, value, key_len, val_len);

            //read next request
            n = recv(client, header, sizeof(memcache_req_header_t), MSG_WAITALL);
        }
        close(client);
    }
    
    close(srv);
    return EXIT_SUCCESS;
}

void process_request(int client, memcache_req_header_t *header, uint8_t *key, uint8_t *value, uint16_t key_len, uint32_t val_len) {
    header->magic = 0x81;
    hash_node *ptr;
    HASH_FIND(hh, cache_table, key, key_len, ptr);
    uint8_t opcode = header->opcode;

    printf("key_len: %d, val_len: %u\n", key_len, val_len);

    switch(opcode) {
        case CMD_GET:
            puts("GET");
            //item not found
            if(ptr == NULL) {
                header->vbucket_id = htons(RES_NOT_FOUND);
                header->key_length = 0;
                header->total_body_length = 0;
                write(client, header, sizeof(memcache_req_header_t));
            }
            //item found
            else {
                header->vbucket_id = RES_OK;
                header->key_length = 0;
                header->total_body_length = htonl(ptr->val_len);
                write(client, header, sizeof(memcache_req_header_t));
                write(client, ptr->val, ptr->val_len);
            }
            break;
        case CMD_SET:
            puts("SET");
            hash_node *new_node = (hash_node*)malloc(sizeof(hash_node));
            new_node->key = key;
            new_node->val = value;
            new_node->key_len = key_len;
            new_node->val_len = val_len;
            if(ptr != NULL) {
                //remove existing value
                HASH_DEL(cache_table, ptr);
                free(ptr->key);
                free(ptr);
            }
            //add new value
            HASH_ADD_KEYPTR(hh, cache_table, key, key_len, new_node);
            header->vbucket_id = RES_OK;
            header->key_length = 0;
            header->total_body_length = 0;
            write(client, header, sizeof(memcache_req_header_t));
            break;
        case CMD_ADD:
            printf("ADD\n");
            if(ptr == NULL) {
                hash_node *new_node = (hash_node*)malloc(sizeof(hash_node));
                new_node->key = key;
                new_node->val = value;
                new_node->key_len = key_len;
                new_node->val_len = val_len;
                HASH_ADD_KEYPTR(hh, cache_table, key, key_len, new_node);
                header->vbucket_id = RES_OK;
            }
            else {
                //printf("vbucket id: %u\n", htons(RES_NOT_FOUND));
                header->vbucket_id = htons(RES_NOT_FOUND);
            }
            header->key_length = 0;
            header->total_body_length = 0;
            write(client, header, sizeof(memcache_req_header_t));
            break;
        case CMD_DELETE:
            puts("DELETE");
            if(ptr == NULL) {
                //printf("vbucket id: %u\n", htons(RES_NOT_FOUND));
                header->vbucket_id = htons(RES_NOT_FOUND);
            }
            else {
                HASH_DEL(cache_table, ptr);
                free(ptr->key);
                free(ptr);
                header->vbucket_id = RES_OK;
            }
            header->key_length = 0;
            header->total_body_length = 0;
            write(client, header, sizeof(memcache_req_header_t));
            break;
        case CMD_VERSION:
            puts("VERSION");
            header->vbucket_id = RES_OK;
            header->key_length = 0;
            header->total_body_length = htonl(strlen("C-Memcached 1.0"));
            const uint8_t *msg = "C-Memcached 1.0";
            write(client, header, sizeof(memcache_req_header_t));
            int test = write(client, msg, 15);
            printf("bytes written: %d\n", test);
            break;
        case CMD_OUTPUT:
            puts("OUTPUT");
            struct timespec time;
            for (hash_node *node = cache_table; node != NULL; node = node->hh.next) {
                if (clock_gettime(CLOCK_REALTIME, &time) != 0) {
                    perror("clock_gettime");
                    exit(EXIT_FAILURE);
                }
                printf("0x%lx:0x%lx:", time.tv_sec, time.tv_nsec);
                if (node->key_len != 0) {
                    printf("0x");
                    int j = 0;
                    while (j < node->key_len) {
                        printf("%x",  ( *(node->key + j)));
                        j++;
                    }
                    printf(":");
                }
                if (node->val_len != 0) {
                    printf("0x");
                    int j = 0;
                    while (j < node->val_len) {
                        printf("%02x", *(node->val + j));
                        j++;
                    }
                }
                printf("\n");
            }
            header->vbucket_id = RES_OK;
            header->key_length = 0;
            header->total_body_length = 0;
            write(client, header, sizeof(memcache_req_header_t));
            break;
        default:
            perror("invalid opcode");
            exit(EXIT_FAILURE);
    }
}

//worker thread function
/*void* worker_thread(void *arg) {
    struct thread_args targs = *(struct thread_args *)arg;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(targs.port);
    inet_pton(AF_INET, targs.server_ip, &server.sin_addr);

    int thread_num = targs.thread_num;

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
        pthread_mutex_lock(&pmutex);
        printf("Thread %d; ", thread_num);
        printf("FAILURE: Couldn't connect to server.\n");
        pthread_mutex_unlock(&pmutex);
        exit(-1);
    }
}*/