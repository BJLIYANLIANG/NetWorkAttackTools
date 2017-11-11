#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <linux/if_packet.h>
#include <sys/time.h>

//#define IF_NAME "wlp0s20u1"


int createSocket(char *interface);

char *createBeacon(char *ssid, uint *size, uint16_t index);

int sendBeacon(int socket, char *beacon, uint size);

int main(int argc, char **argv) {
    printf("Send Start\n");
//    int s = createSocket(IF_NAME);
    int s = createSocket(argv[1]);
    uint16_t index = 0;
    while (1) {
        for (int i = 2; i < argc; i++) {
            uint size = 0;
            char *beacon = createBeacon(argv[i], &size, index);
            sendBeacon(s, beacon, size);
        }
        usleep(100000);
        index += 0x10;
//        break;
    }

    return 0;
}

char *createBeacon(char *ssid, uint *size, uint16_t index) {
    char *buff = malloc(1024);
    char radio_infor_head[] = "\x00\x00\x36\x00\x2f\x40\x40\xa0\x20\x08\x00\x00\x00\x00\x00\x00";

    uint8_t radio_infor_time[8] = {};
    struct timeval t;
    gettimeofday(&t, 0);
    uint64_t t_timestamp = ((uint64_t) t.tv_sec) * 1000000 + t.tv_usec;
    for (int i = 0; i < 8; i++)
        radio_infor_time[i] = (uint8_t) ((t_timestamp >> (i * 8)) & 0xFF);

    char radio_infor_tail[] = "\x02\x6c\x09\xa0\x00\xc4\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xd3\x28\x9e\x0e\x00\x00\x00\x00\x16\x00\x11\x03\xc9\x00";
//    char beacon_frame[] = "\x80\x00\x00\x00\xff\xff\xff\xff\xff\xff\x80\x89\x17\x5c\x52\x33\x80\x89\x17\x5c\x52\x33";
    char beacon_frame[] = "\x80\x00\x00\x00\xff\xff\xff\xff\xff\xff\x23\x33\x33\x33\x33\x33\x23\x33\x33\x33\x33\x33";

    char beacon_interval_and_capinfo[] = "\x64\x00\x21\x84\x00";
    char beacon_after[] = "\x01\x08\x82\x84\x8b\x0c\x12\x96\x18\x24\x03\x01\x01\x05\x04\x00\x01\x00\x00\x07\x06\x43\x4e\x49\x01\x0d\x14\x2a\x01\x00\x32\x04\x30\x48\x60\x6c\x2d\x1a\xec\x11\x03\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3d\x16\x01\x08\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xdd\x18\x00\x50\xf2\x02\x01\x01\x80\x00\x03\xa4\x00\x00\x27\xa4\x00\x00\x42\x43\x5e\x00\x62\x32\x2f\x00\xdd\x1e\x00\x90\x4c\x33\xec\x11\x03\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xdd\x1a\x00\x90\x4c\x34\x01\x08\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

    memcpy(buff + *size, radio_infor_head, sizeof(radio_infor_head));
    *size += sizeof(radio_infor_head) - 1;

    memcpy(buff + *size, radio_infor_time, sizeof(radio_infor_time));
    *size += sizeof(radio_infor_time);

    memcpy(buff + *size, radio_infor_tail, sizeof(radio_infor_tail));
    *size += sizeof(radio_infor_tail) - 1;

    memcpy(buff + *size, beacon_frame, sizeof(beacon_frame));
    *size += sizeof(beacon_frame) - 1;

    buff[*size] = (uint8_t) (index & 0xFF);
    *size += 1;
    buff[*size] = (uint8_t) ((index >> 8) & 0xFF);
    *size += 1;

    memcpy(buff + *size, radio_infor_time, sizeof(radio_infor_time));
    *size += sizeof(radio_infor_time);

    memcpy(buff + *size, beacon_interval_and_capinfo, sizeof(beacon_interval_and_capinfo));
    *size += sizeof(beacon_interval_and_capinfo) - 1;

    *(buff + *size) = (char) strlen(ssid);
    *size += sizeof(uint8_t);

    memcpy(buff + *size, ssid, strlen(ssid));
    *size += strlen(ssid);

    memcpy(buff + *size, beacon_after, sizeof(beacon_after));
    *size += sizeof(beacon_after) - 1;

    printf("Create %s done.\n", ssid);
    return buff;
}

int sendBeacon(int socket, char *beacon, uint size) {
    ssize_t send_size = write(socket, beacon, size);
    free(beacon);
    if (send_size < 0) {
        perror("Send GG");
        exit(-1);
    }
    printf("Send done. %zi\n", send_size);
}

int createSocket(char *interface) {
    int ss = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (ss < 0) {
        perror("Socket GG");
        exit(-1);
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, interface, sizeof(ifr.ifr_ifrn) - 1);
    if (ioctl(ss, SIOCGIFINDEX, &ifr) < 0) {
        perror("Ioctl GG");
        exit(-1);
    }

    struct sockaddr_ll ll;
    memset(&ll, 0, sizeof(ll));
    ll.sll_family = AF_PACKET;
    ll.sll_ifindex = ifr.ifr_ifindex;
    ll.sll_protocol = htons(ETH_P_ALL);
    if (bind(ss, (struct sockaddr *) &ll, sizeof(ll)) < 0) {
        perror("Bind GG");
        exit(-1);
    }

    struct packet_mreq pr;
    memset(&pr, 0, sizeof(pr));
    pr.mr_ifindex = ifr.ifr_ifindex;
    pr.mr_type = PACKET_MR_PROMISC;
    if (setsockopt(ss, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &pr, sizeof(pr)) < 0) {
        perror("Setsockopt GG");
        exit(-1);
    }

    return ss;
}