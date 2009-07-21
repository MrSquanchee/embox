/**
 * \file ifdev.c
 * \date Jul 18, 2009
 * \author Anton Bondarev
 * \details realizing interface if_device struct (interface device)
 */
#include "types.h"
#include "common.h"
#include "net.h"
#include "if_device.h"

static IF_DEVICE ifs[NET_INTERFACES_QUANTITY];

static char status_handlers [NET_INTERFACES_QUANTITY];
static IF_DEVICE * find_free_handler(){
    int i;
    for(i = 0; i < NET_INTERFACES_QUANTITY; i ++){
        if (0 == status_handlers[i]){
            status_handlers[i] = 1;
            return &ifs[i];
        }
    }
    return NULL;
}
static int free_handler(IF_DEVICE * handler){
    int i;
    for(i = 0; i < NET_INTERFACES_QUANTITY; i ++){
        if ((1 == status_handlers[i]) && (&ifs[i] == handler)){
            status_handlers[i] = 0;
            return 0;
        }
    }
    return -1;
}
static int alloc_callback(IF_DEVICE *dev, unsigned int type,
                          ETH_LISTEN_CALLBACK callback) {
    int i;
    for (i = 0; i < array_len(dev->cb_info); i++) {
        if (0 == dev->cb_info[i].is_busy) {
            dev->cb_info[i].is_busy = 1;
            dev->cb_info[i].type = type;
            dev->cb_info[i].func = callback;
            return i;
        }
    }
    return -1;
}

int ifdev_init() {
    return 0;

}

net_device *ifdev_get_netdevice(void *handler) {
    IF_DEVICE *dev = (IF_DEVICE *) handler;
    if (NULL == dev) {
        LOG_ERROR("handler is NULL\n");
        return NULL;
    }
    return dev->net_dev;
}

int ifdev_listen(void *handler, unsigned short type,
                 ETH_LISTEN_CALLBACK callback) {
    IF_DEVICE *dev = (IF_DEVICE *) handler;
    if (NULL == dev) {
        return -1;
    }
    return alloc_callback(dev, type, callback);
}

int ifdev_find_by_ip(const unsigned char *ipaddr) {
    int i;
    for (i = 0; i < NET_INTERFACES_QUANTITY; i++) {
        if (0 == memcmp(ifs[i].ipv4_addr, ipaddr, 4)) {
            return 0;
        }
    }
    return -1;
}

void *ifdev_find_by_name(const char *if_name) {
    int i;
    for (i = 0; i < NET_INTERFACES_QUANTITY; i++) {
        TRACE("ifname %s, net_dev 0x%X\n", if_name, ifs[i].net_dev);
        if (0 == strncmp(if_name, ifs[i].net_dev->name,
                sizeof(ifs[i].net_dev->name))) {
            return &ifs[i];
        }
    }
    return NULL;
}

int ifdev_set_interface(char *name, char *ipaddr, char *macaddr) {
    int i;
    for (i = 0; i < NET_INTERFACES_QUANTITY; i++) {
        if (0 == strncmp(name, ifs[i].net_dev->name, array_len(ifs[i].net_dev->name))) {
            ifdev_set_ipaddr(&ifs[i], ipaddr);
            ifdev_set_macaddr(&ifs[i], macaddr);
            return i;
        }
    }
    return -1;
}

int ifdev_set_ipaddr(void *ifdev, const unsigned char *ipaddr) {
    if (NULL == ifdev)
        return -1;
    IF_DEVICE *dev = (IF_DEVICE *) ifdev;
    memcpy(dev->ipv4_addr, ipaddr, sizeof(dev->ipv4_addr));
    return 0;
}

int ifdev_set_macaddr(void *ifdev, const unsigned char *macaddr) {
    if (NULL == ifdev)
        return -1;
    IF_DEVICE *dev = (IF_DEVICE *) ifdev;
    memcpy(dev->net_dev->hw_addr, macaddr, dev->net_dev->addr_len);
    return 0;
}

unsigned char *ifdev_get_ipaddr(void *handler) {
    IF_DEVICE *dev = (IF_DEVICE *) handler;
    if (NULL == dev)
        return NULL;
    return dev->ipv4_addr;
}

static int rebuild_header(net_packet * pack) {
    if (NULL == pack) {
        return -1;
    }
    if (NULL == pack->sk || SOCK_RAW != pack->sk->sk_type) {
        if (NULL == arp_resolve_addr(pack, pack->nh.iph->daddr)) {
            LOG_WARN("Destanation host is unreachable\n");
            return -1;
        }
        memcpy(pack->mac.mach->src_addr, pack->netdev->hw_addr,
                sizeof(pack->mac.mach->src_addr));
        pack->mac.mach->type = pack->protocol;
        pack->len += sizeof(machdr);
        return 0;
    }
    return 0;
}

int ifdev_up(const char *iname){
    IF_DEVICE *ifhandler;
    if (NULL == (ifhandler = find_free_handler ())){
        TRACE("Error ifdev up: can't find find free handler\n");
        return -1;
    }
    if (NULL == (ifhandler->net_dev = find_net_device(iname))){
        TRACE("Error ifdev up: can't find net_device with name\n", iname);
        return -1;
    }
    ifhandler->net_dev->rebuild_header = rebuild_header;
    if (NULL == ifhandler->net_dev->open) {
        TRACE("Error ifdev up: can't find open function in net_device with name\n", iname);
        return -1;
    }
    return ifhandler->net_dev->open(ifhandler->net_dev);
}

int ifdev_down(const char *iname){
    IF_DEVICE *ifhandler;
    if (NULL == (ifhandler = ifdev_find_by_name(iname))){
            TRACE("Error ifdev down: can't find ifdev with name\n", iname);
            return -1;
        }
    if (NULL == (ifhandler->net_dev)){
            TRACE("Error ifdev down: can't find net_device with name\n", iname);
            return -1;
        }
    if (NULL == ifhandler->net_dev->stop) {
        TRACE("Error ifdev down: can't find stop function in net_device with name\n", iname);
        return -1;
    }
    free_handler(ifhandler);
    return ifhandler->net_dev->stop(ifhandler->net_dev);
}

/* iterator functions */
static int iterator_cnt;
IF_DEVICE * ifdev_get_fist_used(){
    for(iterator_cnt = 0; iterator_cnt < NET_INTERFACES_QUANTITY; iterator_cnt ++){
        if (0 == status_handlers[iterator_cnt]){
            status_handlers[iterator_cnt] = 1;
            return &ifs[iterator_cnt];
        }
    }
    return NULL;
}
IF_DEVICE * ifdev_get_next_used(){
    for(; iterator_cnt < NET_INTERFACES_QUANTITY; iterator_cnt ++){
        if (0 == status_handlers[iterator_cnt]){
            status_handlers[iterator_cnt] = 1;
            return &ifs[iterator_cnt];
        }
    }
    return NULL;
}
