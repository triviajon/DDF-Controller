#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>

#define INTERFACE_NAME "enp2s0"
#define INTERFACE_NAME_SIZE 6
#define ERROR_OUT -1
#define DATA_BYTES 81  /* 27*3 */
#define LED_INDEX_BYTES 2
#define NUM_LEDS 440  /* 8*55 */

int main() {
    int socket_fd;
    
    struct ifreq interface_id;
    struct ifreq interface_mac;

    const uint8_t HEADER_BYTES = sizeof(struct ether_header);
    const uint8_t FRAME_BYTES = HEADER_BYTES + LED_INDEX_BYTES + DATA_BYTES;
    char frame_buffer[FRAME_BYTES];

    struct ether_header *eth_head = (struct ether_header *) frame_buffer;
    struct sockaddr_ll socket_address;

    uint32_t led_color = 0x00660000;
    uint32_t led_bit_mask = 1;
    uint16_t led_index = 430;  // [0, NUM_LEDS - 1]

    memset(frame_buffer, 0, FRAME_BYTES);

    if ((socket_fd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
        perror("Error [socket]");
        return ERROR_OUT;
    }

    /* Get interface index from name */
    memset(&interface_id, 0, sizeof(struct ifreq));
    strncpy(interface_id.ifr_name, INTERFACE_NAME, INTERFACE_NAME_SIZE);
    if (ioctl(socket_fd, SIOCGIFINDEX, &interface_id) < 0) {
        perror("Error [SIOCGIFINDEX]");
        return ERROR_OUT;
    }

    /* Get sender MAC address */
    memset(&interface_mac, 0, sizeof(struct ifreq));
    strncpy(interface_mac.ifr_name, INTERFACE_NAME, INTERFACE_NAME_SIZE);
    if (ioctl(socket_fd, SIOCGIFHWADDR, &interface_mac) < 0) {
        perror("Error [SIOCGIFHWADDR]");
        return ERROR_OUT;
    }
    
    /* Construct Ethernet header */
    for (uint8_t i = 0; i < 6; ++i) {
        eth_head->ether_shost[i] = ((uint8_t *) &interface_mac.ifr_hwaddr.sa_data)[i];
        eth_head->ether_dhost[i] = 0xff;  /* Broadcast MAC address */

        socket_address.sll_addr[i] = 0xff;  /* And same for sll_addr */
     }
    eth_head->ether_type = htons(ETH_P_IP);

    socket_address.sll_ifindex = interface_id.ifr_ifindex;
    socket_address.sll_halen = ETH_ALEN;

    while (1) {
        /* Set LED index */
        frame_buffer[HEADER_BYTES] = (uint8_t) (led_index & 0x00ff);
        frame_buffer[HEADER_BYTES + 1] = (uint8_t) (led_index >> 8);

        /* Set packet data */
        for (uint16_t i = HEADER_BYTES + LED_INDEX_BYTES; i < FRAME_BYTES; ++i) {
            frame_buffer[i] = 0x00;
        }

        /* Send packet */
        /* printf("Sending packet with LED index %d...\n", led_index); */
        if (sendto(socket_fd, frame_buffer, FRAME_BYTES, 0, (struct sockaddr *) &socket_address, sizeof(struct sockaddr_ll)) < 0) {
            perror("Error [sendto]");
            return ERROR_OUT;
        }

        ++led_index;
        if (led_index == NUM_LEDS) {
            led_index = 0;
        }

        usleep(25);
    }

    return 0;
}

