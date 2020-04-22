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

#endif /* ping_h */
