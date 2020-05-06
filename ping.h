//
//  ping.h
//  ping
//
//  Created by Yiran Si on 4/21/20.
//  Copyright © 2020 Yiran Si. All rights reserved.
//

#ifndef ping_h
#define ping_h

#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>



#define MAX_PACKETS_NUM 4096
#define PACKET_SIZE     4096

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
/////////////////////////////// Helper Functions ///////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
/* Substract packet recv time from packet sent time */
void calc_time_diff(struct timeval * sent, struct timeval *recv) {
    // 1 sec = 1000000 usec
    // make convertion if necessary
    if ((sent->tv_usec -= recv->tv_usec) < 0) {
        --sent->tv_sec;
        sent->tv_usec += 1000000;
    }
    sent->tv_sec -= recv->tv_sec;
}

/* Calculate checksum */
//A checksum is a small-sized datum derived from a block of digital data for the purpose of detecting errors that may have been introduced during its transmission or storage. By themselves, checksums are often used to verify data integrity but are not relied upon to verify data authenticity.
unsigned short cal_chksum(unsigned short * addr, int len) {
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;
    while (nleft > 1) {
        sum += *w;
        w++;
        nleft -= 2;
    }
    if (nleft == 1) {
        *(unsigned char *) (&answer) = *(unsigned char *)w;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

/* Helper function for send_packet
   Calculate packet size */
int pack(struct Ping * p) {
    int sz;
    struct icmp *icmp;
    icmp = (struct icmp*) p->sendpacket;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_cksum = 0;
    icmp->icmp_seq = p->nsend;
    icmp->icmp_id = p->pid;
    sz = 8 + p->datalen;
    
    // update cur time of icmp pack
    gettimeofday((struct timeval*)icmp->icmp_data, NULL);
    
    icmp->icmp_cksum = cal_chksum((unsigned short*)icmp, sz);
    
    
    memcpy(p->sendpacket, (char * )icmp, sizeof(icmp));
    printf("packing:  sizeof(icmp): %d, sz: %d\n", sizeof(icmp), sz);
    return sz;
}

int unpack(struct Ping *p, int len) {
    int ip_header;
    struct ip *ip;
    struct icmp *icmp;
    struct timeval *tvsend;
    double rtt; // round travel time
    char * buff = (char *) p->recvpacket;
    
    ip = (struct ip*) buff;
    ip_header = ip->ip_hl << 2; //?
    icmp = (struct icmp*) (buff + ip_header);
    len -= ip_header;
    
    if (len < 8) {
        perror("ICMP packets length less than 8\n");
        return -1;
    }
    if ((icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == p->pid)) {
        // get time stamp when packet was sent
        tvsend = (struct timeval *) icmp->icmp_data;
        // calc time interval between send and recv packets
        calc_time_diff(&p->tvrecv, tvsend);
        rtt = (p->tvrecv.tv_sec * 1000) + (tvsend->tv_usec / 1000);
        
        // print states for received packets
        printf("pck received from %s: sz=%d bytes icmp_seq=%u ttl=%d rtt=%0.3f ms\n", inet_ntoa(p->sour_addr.sin_addr), len, icmp->icmp_seq, ip->ip_ttl, rtt);
        return 1;
    }
    // if not echo reply or pid doesn't match
    return -1;
}



////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// Struct Functions ///////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

// infinitely receiving packets
void sending(struct Ping *p) {
    int count = 0;
    int packetsize;
    while (1) {
        count++;
        printf("sending %d \n", count);
        
        p->nsend++;
        packetsize = pack(p);
        
        if (sendto(p->sockfd, p->sendpacket, packetsize, 0, (struct sockaddr*) &p->dest_addr, sizeof(p->dest_addr)) < 0) {
            perror("failed sendto");
            exit(1);
        }
        // send packet periotically
        sleep(2);
        
        //for testing only: break infinite while loop if sent && received MAX_PACKETS_NUM packets
        if (p->nsend >= MAX_PACKETS_NUM && p->nreceived >= MAX_PACKETS_NUM) {
             break;
        }
    } // end while (1)
}

// infinitely sending packets
void listening(struct Ping *p) {
    int n;
    unsigned int fromlen;
    extern int errno;
    
    fromlen = sizeof(p->sour_addr);
    int count = 0;
    while (1) {
        count++;
        printf("listening %d \n", count);
        if ((n == recvfrom(p->sockfd, p->recvpacket, sizeof(p->recvpacket), 0, (struct sockaddr *) &(p->sour_addr), &fromlen)) < 0) {
            if (errno == EINTR) continue;
            perror("failed recvfrom\n");
            continue;
        }
        printf("listening passed recvfrom \n");
    
        // update timeval for recved packet
        gettimeofday(&(p->tvrecv), NULL);
        
        unpack(p, n);
        
        p->nreceived++;
        
        // stats for loss
        double loss = (double)(p->nsend - p->nreceived) / (double)p->nsend;
        printf("%d packets sent, %d received , %0.3f%% lost\n\n", p->nsend, p->nreceived, loss * 100);
        
        sleep(2);
        
        //for testing only: break infinite while loop if sent && received MAX_PACKETS_NUM packets
        if (p->nsend >= MAX_PACKETS_NUM && p->nreceived >= MAX_PACKETS_NUM) {
             break;
        }
    } // end while (1)
    
    
    
}

//main loop func for Ping
void start(struct Ping *p) {
    
    pthread_t send_th;
    pthread_create(&send_th, NULL, (void *)sending, (void *)p);
    
    pthread_t listen_th;
    pthread_create(&listen_th, NULL, (void *)listening, (void *)p);
    
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
    struct hostent *host = NULL;
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
    
    // TODO: set dest_addr
    p.dest_addr.sin_addr.s_addr = inet_addr(hostname);
    // TODO: if input is IP, get hostname by IP
    in_addr = inet_addr(hostname);
    if (in_addr == INADDR_NONE) {
        host = gethostbyname(hostname);
        if (host == NULL) {
            perror("failed to get host by name");
            exit(1);
        }
        memcpy((char *)&p.dest_addr.sin_addr, (char *)host->h_addr, host->h_length);
    } else {

    }
    
    
    printf("PING: %s IP: %s\n\n", hostname, inet_ntoa(p.dest_addr.sin_addr));
    
    return p;
}

#endif /* ping_h */
