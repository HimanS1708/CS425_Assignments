#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>


#ifndef SERVER_IP
#define SERVER_IP "127.0.0.1"  // Server IP address
#endif

#ifndef SERVER_PORT
#define SERVER_PORT 12345      // Server listening port
#endif

const int CLIENT_PORT = 1234;           // Source port used by this client
const int MAX_RETRIES = 3;              // Retry attempts for receiving SYN-ACK
const int TIMEOUT_SECONDS = 2;          // Socket timeout duration (recv)
const int MAX_PACKET_SIZE = 4096;       // Packet buffer size
const int RECV_BUFFER_SIZE = 65536;     // Buffer for receiving packets

//TCP Header required for checksum calculation
struct Header {
    uint32_t src_addr;
    uint32_t dest_addr;
    uint8_t placeholder;
    uint8_t protocol;
    uint16_t tcp_length;
};

// Calculates the TCP checksum over the TCP header and data, including the head-header
uint16_t compute_checksum(uint16_t *data, int length) {
    long sum = 0;

    while (length > 1) {
        sum += *data++;
        length -= 2;
    }

    if (length == 1) {
        uint16_t last_byte = 0;
        *((uint8_t*)&last_byte) = *(uint8_t*)data;
        sum += last_byte;
    }

    // Fold to 16 bits
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    uint16_t answer = (uint16_t)~sum;

    return answer;
}

class TCPHandshakeClient {
private:
    int sock;                                     // Raw socket descriptor
    sockaddr_in server_addr{};                    // Server address info
    char packet[MAX_PACKET_SIZE]{};               // Outgoing packet buffer
    char checksum_buffer[MAX_PACKET_SIZE]{};      // Temporary buffer for checksum calc
    char recv_buffer[RECV_BUFFER_SIZE]{};         // Incoming packet buffer

    // Prepares an IP header for the outgoing packet
    void prepare_ip_header(iphdr *ip_header, uint32_t src_ip, uint32_t dest_ip) {
        ip_header->ihl = 5;                                         // IP header length (20 bytes)
        ip_header->version = 4;                                     // IPv4
        ip_header->tos = 0;                                         // Type of Service
        ip_header->tot_len = htons(sizeof(iphdr) + sizeof(tcphdr)); // Total length
        ip_header->id = htons(54321);                               // Identification (arbitrary)
        ip_header->frag_off = 0;                                    // No fragmentation
        ip_header->ttl = 64;                                        // Time-to-live
        ip_header->protocol = IPPROTO_TCP;                          // Protocol = TCP
        ip_header->saddr = src_ip;                                  // Source IP address
        ip_header->daddr = dest_ip;                                 // Destination IP address
    }

    // Constructs and sends a SYN packet to initiate handshake
    bool send_syn() {
        iphdr *ip_header = (iphdr *)packet;
        tcphdr *tcp_header = (tcphdr *)(packet + sizeof(iphdr));
        Header head{};

        // Fill headers
        prepare_ip_header(ip_header, inet_addr("127.0.0.1"), server_addr.sin_addr.s_addr);

        tcp_header->source = htons(CLIENT_PORT);
        tcp_header->dest = htons(SERVER_PORT);
        tcp_header->seq = htonl(200);              // Required by server
        tcp_header->ack_seq = 0;
        tcp_header->doff = 5;                      // Data offset (5 x 4 = 20 bytes)
        tcp_header->syn = 1;                       // Set SYN flag
        tcp_header->ack = 0;
        tcp_header->window = htons(5840);
        tcp_header->check = 0;

        // Build head-header
        head.src_addr = inet_addr("127.0.0.1");
        head.dest_addr = server_addr.sin_addr.s_addr;
        head.placeholder = 0;
        head.protocol = IPPROTO_TCP;
        head.tcp_length = htons(sizeof(tcphdr));

        // Copy for checksum calc
        memcpy(checksum_buffer, &head, sizeof(Header));
        memcpy(checksum_buffer + sizeof(Header), tcp_header, sizeof(tcphdr));
        tcp_header->check = compute_checksum((uint16_t *)checksum_buffer, sizeof(Header) + sizeof(tcphdr));

        // Send SYN packet
        if (sendto(sock, packet, ntohs(ip_header->tot_len), 0, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("[!] Failed to send SYN packet");
            return false;
        }

        std::cout << "[+] Sent SYN packet (SEQ=200) to " << SERVER_IP << "\n";
        return true;
    }

    // Constructs and sends final ACK packet
    bool send_ack(uint32_t server_seq) {
        iphdr *ip_header = (iphdr *)packet;
        tcphdr *tcp_header = (tcphdr *)(packet + sizeof(iphdr));
        Header head{};

        prepare_ip_header(ip_header, inet_addr("127.0.0.1"), server_addr.sin_addr.s_addr);

        tcp_header->source = htons(CLIENT_PORT);
        tcp_header->dest = htons(SERVER_PORT);
        tcp_header->seq = htonl(600);              // Arbitrary, consistent
        tcp_header->ack_seq = htonl(server_seq + 1); // ACK = server SEQ + 1
        tcp_header->doff = 5;
        tcp_header->syn = 0;
        tcp_header->ack = 1;
        tcp_header->window = htons(5840);
        tcp_header->check = 0;

        // Compute checksum
        head.src_addr = inet_addr("127.0.0.1");
        head.dest_addr = server_addr.sin_addr.s_addr;
        head.placeholder = 0;
        head.protocol = IPPROTO_TCP;
        head.tcp_length = htons(sizeof(tcphdr));

        memcpy(checksum_buffer, &head, sizeof(Header));
        memcpy(checksum_buffer + sizeof(Header), tcp_header, sizeof(tcphdr));
        tcp_header->check = compute_checksum((uint16_t *)checksum_buffer, sizeof(Header) + sizeof(tcphdr));

        if (sendto(sock, packet, ntohs(ip_header->tot_len), 0, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("[!] Failed to send ACK packet");
            return false;
        }

        std::cout << "[+] Sent ACK packet (SEQ=600, ACK=" << server_seq + 1 << ")\n";
        return true;
    }

    // Waits for SYN-ACK from server and extracts server's SEQ number
    bool receive_syn_ack(uint32_t &server_seq_out) {
        int attempts = 0;

        while (attempts < MAX_RETRIES) {
            int size = recv(sock, recv_buffer, RECV_BUFFER_SIZE, 0);
            if (size < 0) {
                std::cerr << "[!] Timeout receiving SYN-ACK, retrying (" << attempts + 1 << "/" << MAX_RETRIES << ")...\n";
                attempts++;
                send_syn();  // Resend SYN
                continue;
            }

            iphdr *recv_ip = (iphdr *)recv_buffer;
            tcphdr *recv_tcp = (tcphdr *)(recv_buffer + recv_ip->ihl * 4);

            if (recv_tcp->source == htons(SERVER_PORT) &&
                recv_tcp->dest == htons(CLIENT_PORT) &&
                recv_tcp->syn == 1 && recv_tcp->ack == 1 &&
                ntohl(recv_tcp->ack_seq) == 201) { // Expecting ACK=200+1

                server_seq_out = ntohl(recv_tcp->seq);
                std::cout << "[+] Received SYN-ACK (SEQ=" << server_seq_out
                          << ", ACK=" << ntohl(recv_tcp->ack_seq) << ")\n";
                return true;
            }
        }

        std::cerr << "[!] Failed to receive valid SYN-ACK after retries.\n";
        return false;
    }

public:
    // Constructor: sets up raw socket and socket options
    TCPHandshakeClient() {
        sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
        if (sock < 0) {
            perror("[!] Socket creation failed");
            exit(EXIT_FAILURE);
        }

        // Set timeout for recv()
        timeval timeout = {TIMEOUT_SECONDS, 0};
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        // Tell OS we're building IP headers manually
        int opt = 1;
        setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt));

        // Set up server address
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    }

    // Destructor: close socket
    ~TCPHandshakeClient() {
        close(sock);
    }

    // High-level method to perform full 3-way handshake
    bool perform_handshake() {
        std::cout << "[+] Initiating TCP 3-way handshake...\n";

        if (!send_syn())
            return false;

        uint32_t server_seq;
        if (!receive_syn_ack(server_seq))
            return false;

        if (!send_ack(server_seq))
            return false;

        std::cout << "[+] TCP 3-way handshake completed successfully.\n";
        return true;
    }
};

int main() {
    std::cout << "[CS425 A3] TCP Handshake Client Starting...\n";

    TCPHandshakeClient client;
    if (!client.perform_handshake()) {
        std::cerr << "[!] Handshake failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
