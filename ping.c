
#include "ping.h"
////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////// Struct ///////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
struct Ping {
    ////////////////////////////// Data  ///////////////////////////
    char sendpacket[PACKET_SIZE];
    char recvpacket[PACKET_SIZE];

    int sockfd;
    int datalen;// = 56;
    int nsend; // = 0;
    int nreceived;// = 0;

    pid_t pid;

    struct sockaddr_in dest_addr;
    struct sockaddr_in sour_addr;
    struct timeval tvrecv;
    
    //////////////// Helper Function (Pointers) ///////////////////////////
    struct Ping* (*PingCtor)(struct sockaddr_in *sour_addr); // ctor
    unsigned short (*cal_chksum)(unsigned short *addr, int len);
    int (*helper_pack)(int pack_no);
    void (*send_packet)(void);
    void (*recv_packet)(void);
    int (*helper_unpack)(char *buf, int len);
    void (*calc_time)(struct timeval *out, struct timeval *in);
    void (*start)(struct Ping *p);
};
////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// Struct Functions ///////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

// infinitely receiving packets
void listening() {}

// infinitely sending packets
void sending() {}

//main loop func for Ping
void start(struct Ping *p) {
    
    pthread_t send_th;
    pthread_create(&send_th, NULL, (void *)sending, NULL);
    
    pthread_t listen_th;
    pthread_create(&listen_th, NULL, (void *)listening, NULL);
    
    pthread_join(send_th, NULL);
    pthread_join(listen_th, NULL);
    
}



//ctor for Ping
struct Ping ping(char* hostname) {
    struct Ping p;
    
    // init data
//    p.sour_addr = *sour_addr;
    p.datalen = 56;
    p.nsend = 0;
    p.nreceived = 0;
    
    // assign member function pointers
    p.send_packet = &sending;
    p.recv_packet = &listening;
    p.start = &start;
    
    // init socket
//    struct sockaddr_in * sour_addr;
//    struct sockaddr_in * dest_addr;
    // get sour_addr
    // get dest_addr from argv[1]
    struct hostent *host;
    struct protoent *protocol;
    unsigned long in_addr = 0;
    //int sockfd;
    int buff_sz = 1024; // tmp 1024
    // get host && set socket
    protocol = getprotobyname("icmp");
    if (protocol == NULL) {
        perror("failed to get protocol");
        exit(1);
    }
    p.sockfd = socket(AF_INET, SOCK_RAW, protocol->p_proto);
    if (p.sockfd < 0) {
        perror("failed to init socket");
        exit(1);
    }
    int res = setsockopt(p.sockfd, SOL_SOCKET, SO_RCVBUF, &buff_sz, sizeof(buff_sz));
    if (res < 0) {
        perror("failed to set socket");
        exit(1);
    }
    memset(&p.dest_addr, 0, sizeof(struct sockaddr_in)); // set mem to 0
    p.dest_addr.sin_family = AF_INET;
    
    // TODO: if input is IP, get hostname by IP
    // TODO: set dest_addr
    
    return p;
}

////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////// Main ///////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
    
    if (argc < 2) {
        perror("not enough argument");
        return -1;
    }

    struct Ping p = ping(argv[1]);
    p.start(&p);
    
    return 0;
}
