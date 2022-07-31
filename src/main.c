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
#include <sys/time.h>
#include <math.h>

#include "global_defines.h"
#include "gif.h"

#define DO_ETH 1
#define INTERFACE_NAME "enp2s0"
#define CHUNKS 27
#define LED_CHANNELS 3
#define LED_BITS 24  /* LED_CHANNELS*8 */
#define CHUNK_ROWS 8
#define CHUNK_COLS 55
#define LED_INDEX_BYTES 2
#define DATA_BYTES 81  /* CHUNKS*LED_CHANNELS */
#define CHUNK_LEDS 440  /* CHUNK_ROWS*CHUNK_COLS */
#define HEADER_BYTES 14  /* sizeof(struct ether_header) = 6+6+2 */
#define FRAME_BYTES 97  /* HEADER_BYTES+LED_INDEX_BYTES+DATA_BYTES */
#define BRIGHTNESS_SCALE 0.5

uint8_t frame_buffer[FRAME_BYTES];  /* One Ethernet frame contains data for one LED per chunk  */
uint8_t color_frame[LED_ROWS][LED_COLS][LED_CHANNELS];  /* Colors for full dance floor */

void color_frame_to_eth(uint16_t led_index) {
    /* 
     * Fill frame_buffer with color_frame data at given led_index
     * color_frame format: GRB, GRB, GRB, ... for each row and column of LEDs
     * frame_buffer format: G bit 1, chunk 1; G bit 1, chunk 2; ...; B bit 8,i chunk 27
     */

    uint8_t chunk_led_row = led_index / CHUNK_COLS;
    uint8_t chunk_led_col = led_index % CHUNK_COLS;

    /* led_row and led_col are chunk indices above but with chunk offsets applied */
    uint8_t led_row = 0;
    uint8_t led_col = 0;

    uint8_t i, j, k, l;
    uint16_t frame_buffer_bit_index = 8 * (HEADER_BYTES + LED_INDEX_BYTES);

    memset(frame_buffer + HEADER_BYTES + LED_INDEX_BYTES, 0, DATA_BYTES);

    for (i = 0; i < LED_CHANNELS; ++i) {
        /* For each bit of the current channel */
        for (j = 0; j < 8; ++j) {
            /* For each row of chunks */
            for (k = 0; k < 9; ++k) {
                led_row = chunk_led_row + k * CHUNK_ROWS;
                /* For each column of chunks */
                for (l = 0; l < 3; ++l) {
                    led_col = chunk_led_col + l * CHUNK_COLS;

                    /* Select bit j from color_frame, place in chunk_index bit of current_bits */
                    frame_buffer[frame_buffer_bit_index / 8] |= (
                        ((color_frame[led_row][led_col][i] >> (7 - j)) & 1U) <<
                        frame_buffer_bit_index % 8
                    );

                    ++frame_buffer_bit_index;
                }
            }
        }
    }
}

double get_millis(struct timeval *tv) {
    gettimeofday(tv, 0);
    return tv->tv_sec * 1000 + tv->tv_usec / 1000.0; 
}

void load_gif_frame(struct gif *gif, struct frame *frame) {
    /* Copy frame data into color_frame */
    
    uint8_t i, j;
    uint16_t pixel_counter = 0;
    uint8_t ct_index;

    for (i = 0; i < LED_ROWS; ++i) {
        for (j = 0; j < LED_COLS; ++j) {
            ct_index = frame->ct_indices[pixel_counter++];
        
            if (frame->has_transparency && ct_index == frame->transparent_index) {
                /* Transparent pixel retains same color */
                continue;
            }

            if (gif->has_gct && ct_index == gif->bg_index) {
                /* Grab background color from GCT */
                color_frame[i][j][0] = gif->gct[ct_index][1];  /* G */
                color_frame[i][j][1] = gif->gct[ct_index][0];  /* R */
                color_frame[i][j][2] = gif->gct[ct_index][2];  /* B */
            }
            else {
                color_frame[i][j][0] = frame->ct[ct_index][1];
                color_frame[i][j][1] = frame->ct[ct_index][0];
                color_frame[i][j][2] = frame->ct[ct_index][2];
            }
        }
    }
}

int prep_gif(struct gif *gif, const char *filename) {
    /* Load GIF and copy first frame data into color_frame */

    uint8_t i, j;
    uint16_t pixel_counter = 0;
    uint8_t ct_index;
    struct frame *frame;
    uint8_t bg_r, bg_g, bg_b;

    if (gif_load(gif, filename) == ERROR_OUT) {
        return ERROR_OUT;
    }

    frame = (struct frame *) dyn_arr_get(&(gif->frames), 0);

    if (gif->has_gct) {
        bg_r = gif->gct[gif->bg_index][0] * BRIGHTNESS_SCALE;
        bg_g = gif->gct[gif->bg_index][1] * BRIGHTNESS_SCALE;
        bg_b = gif->gct[gif->bg_index][2] * BRIGHTNESS_SCALE;
    }
    else {
        bg_r = 0;
        bg_g = 0;
        bg_b = 0;
    }

    for (i = 0; i < LED_ROWS; ++i) {
        for (j = 0; j < LED_COLS; ++j) {
            ct_index = frame->ct_indices[pixel_counter++];
            if ((frame->has_transparency && ct_index == frame->transparent_index) ||
                (gif->has_gct && ct_index == gif->bg_index)
            ) {
                /* Transparent pixel or background pixel both set to bg color */
                color_frame[i][j][0] = bg_g * BRIGHTNESS_SCALE;
                color_frame[i][j][1] = bg_r * BRIGHTNESS_SCALE;
                color_frame[i][j][2] = bg_b * BRIGHTNESS_SCALE;
            }
            else {
                color_frame[i][j][0] = frame->ct[ct_index][1] * BRIGHTNESS_SCALE;
                color_frame[i][j][1] = frame->ct[ct_index][0] * BRIGHTNESS_SCALE;
                color_frame[i][j][2] = frame->ct[ct_index][2] * BRIGHTNESS_SCALE;
            }
        }
    }

    return SUCC_OUT;
}

void print_color_frame() {
    uint8_t i, j;
    uint8_t r, g, b;
    uint32_t formatted_color;

    printf("Frame:\n");
    for (i = 0; i < LED_ROWS; ++i) {
        printf("    ");
        for (j = 0; j < LED_COLS; ++j) {
            g = color_frame[i][j][0];
            r = color_frame[i][j][1];
            b = color_frame[i][j][2];
            formatted_color = (((uint32_t) r) << 16);
            formatted_color |= (((uint32_t) g) << 8);
            formatted_color |= (uint32_t) b;
            printf("0x%X ", formatted_color);
        }
        printf("\n");
    }
}

int main() {
    int socket_fd;
    
    struct ifreq interface_id;
    struct ifreq interface_mac;

    struct ether_header *eth_head = (struct ether_header *) frame_buffer;
    struct sockaddr_ll socket_address;

    uint16_t i;

    struct timeval tv;
    double millis;

    double last_frame_millis = 0;
    uint16_t frame_index = 0;
    struct frame *current_frame;

    struct gif gif;

    memset(frame_buffer, 0, FRAME_BYTES);

    if ((socket_fd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
        perror("Error [socket]");
        return ERROR_OUT;
    }

    /* Get interface index from name */
    memset(&interface_id, 0, sizeof(struct ifreq));
    strncpy(interface_id.ifr_name, INTERFACE_NAME, IFNAMSIZ);
    if (ioctl(socket_fd, SIOCGIFINDEX, &interface_id) < 0) {
        perror("Error [SIOCGIFINDEX]");
        return ERROR_OUT;
    }

    /* Get sender MAC address */
    memset(&interface_mac, 0, sizeof(struct ifreq));
    strncpy(interface_mac.ifr_name, INTERFACE_NAME, IFNAMSIZ);
    if (ioctl(socket_fd, SIOCGIFHWADDR, &interface_mac) < 0) {
        perror("Error [SIOCGIFHWADDR]");
        return ERROR_OUT;
    }
    
    /* Construct Ethernet header */
    for (i = 0; i < 6; ++i) {
        eth_head->ether_shost[i] = ((uint8_t *) &interface_mac.ifr_hwaddr.sa_data)[i];
        eth_head->ether_dhost[i] = 0xff;  /* Broadcast MAC address */

        socket_address.sll_addr[i] = 0xff;  /* And same for sll_addr */
     }
    eth_head->ether_type = htons(ETH_P_IP);

    socket_address.sll_ifindex = interface_id.ifr_ifindex;
    socket_address.sll_halen = ETH_ALEN;

    /* Load GIF file */
    gif_init(&gif);
    if (prep_gif(&gif, "img/sample2.gif") == ERROR_OUT) {
        return ERROR_OUT;
    }
    current_frame = (struct frame *) dyn_arr_get(&(gif.frames), 0);

    #if DO_ETH
        while (1) {
            millis = get_millis(&tv);

            if (millis - last_frame_millis >= current_frame->delay * 10) {
                /* Switch to next frame */
                ++frame_index;
                /*
                printf(
                    "Frame period target: %d, hit: %f\n",
                    current_frame->delay * 10,
                    millis - last_frame_millis
                );
                */
                if (frame_index == gif.frames.length) {
                    frame_index = 0;
                }
                current_frame = (struct frame *) dyn_arr_get(&(gif.frames), frame_index);
                last_frame_millis = millis;
                load_gif_frame(&gif, current_frame);
            }
     
            /* Send Ethernet packets for each LED */
            for (i = 0; i < CHUNK_LEDS; ++i) {
                /* Set LED index */
                frame_buffer[HEADER_BYTES] = (uint8_t) (i & 0x00ff);
                frame_buffer[HEADER_BYTES + 1] = (uint8_t) (i >> 8);

                /* Set packet data */
                color_frame_to_eth(i);

                /* Send packet */
                if (sendto(
                        socket_fd,
                        frame_buffer,
                        FRAME_BYTES,
                        0,
                        (struct sockaddr *) &socket_address,
                        sizeof(struct sockaddr_ll)
                    ) < 0
                ) {
                    perror("Error [sendto]");
                    return ERROR_OUT;
                }

                usleep(10);
            }
        }
    #endif

    gif_free(&gif);
    close(socket_fd);

    return SUCC_OUT;
}

