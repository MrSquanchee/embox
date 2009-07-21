/**
 * \file net.h
 *
 * \date Mar 11, 2009
 * \author anton
 */

#ifndef NET_H_
#define NET_H_

#include "net_device.h"
#include "eth.h"
#include "sock.h"

#define NET_INTERFACES_QUANTITY     0x4
#define IPV4_ADDR_LENGTH            0x4
#define NET_DEVICES_QUANTITY        0x4

enum sock_type {
        SOCK_STREAM     = 1,  /* TCP */
        SOCK_DGRAM      = 2,  /* UDP */
        SOCK_RAW        = 3,  /* IPPROTO_RAW */
};

/**
 * initialize protokol's stack
 * now only UDP ARP and so on
 * not support TCP protocol
 */
int net_init();

/**
 * set default config for net interfaces
 * default config load from ifconfig.inc
 *
 */
int net_config();

int netif_rx(net_packet *pack);

#endif /* NET_H_ */
