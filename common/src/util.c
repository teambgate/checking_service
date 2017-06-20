/*
* Copyright (C) 2017 Manh Tran
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/
#include <common/util.h>
#include <cherry/string.h>
#include <cherry/stdint.h>
#include <cherry/ctype.h>
#include <cherry/stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <cherry/unistd.h>

#define MAXPW 1024

static u8 __valids[] = {
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n',
        'o','p','q','r','s','t','u','v','w','x','y','z',
        '0','1','2','3','4','5','6','7','8','9','_','-'
};

int common_username_valid(char *name, size_t len)
{
        int i, j;
        u8 low;

        if(len <= 0) return 0;

        for_i(i, len) {
                low = tolower(name[i]);
                for_i(j, sizeof(__valids) / sizeof(__valids[0])) {
                        if(low == __valids[j]) {
                                goto success;
                        }
                }
                failed:;
                return 0;
                success:;
        }

        return 1;
}

void common_gen_random(char *s, size_t len) {
        for (int i = 0; i < len; ++i) {
                int randomChar = rand() % ( 26 + 26 + 10);
                if (randomChar < 26)
                s[i] = 'a' + randomChar;
                else if (randomChar < 26+26)
                s[i] = 'A' + randomChar - 26;
                else
                s[i] = '0' + randomChar - 26 - 26;
        }
        s[len] = 0;
}

int common_is_ip(char *s)
{
        struct sockaddr_in sa;
        int result = inet_pton(AF_INET, s, &(sa.sin_addr));
        return result != 0;
}

struct string *common_get_mac_address()
{
        struct string *result = string_alloc(0);
        struct ifreq ifr;
        struct ifconf ifc;
        char buf[1024];
        int success = 0;

        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock == -1) {
                goto finish;
        };

        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;
        if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
                close(sock);
                goto finish;
        }

        struct ifreq* it = ifc.ifc_req;
        const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

        for (; it != end; ++it) {
                strcpy(ifr.ifr_name, it->ifr_name);
                if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
                        if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                                        success = 1;
                                        break;
                                }
                        }
                } else {
                        /* handle error */
                }
        }
        close(sock);

        unsigned char mac_address[20];
        memset(mac_address, 0, 20);

        if (success) {
                int i;
                int index = 0;
                for (i = 0; i < 6; ++i) {
                        if(i > 0 && i < 6) {
                                sprintf(mac_address + index, ":");
                                index++;
                        }
                        sprintf(mac_address + index, "%.2X", (unsigned char) ifr.ifr_hwaddr.sa_data[i]);
                        index += 2;
                }
                string_cat(result, mac_address, strlen(mac_address));
        }
finish:;
        return result;
}

ssize_t common_getpasswd (char *pw, size_t sz, int mask, FILE *fp)
{
        if (!pw || !sz || !fp) return -1;       /* validate input   */
    #ifdef MAXPW
        if (sz > MAXPW) sz = MAXPW;
    #endif

        size_t idx = 0;         /* index, number of chars in read   */
        int c = 0;

        struct termios old_kbd_mode;    /* orig keyboard settings   */
        struct termios new_kbd_mode;

        if (tcgetattr (0, &old_kbd_mode)) { /* save orig settings   */
            fprintf (stderr, "%s() error: tcgetattr failed.\n", __func__);
            return -1;
        }   /* copy old to new */
        memcpy (&new_kbd_mode, &old_kbd_mode, sizeof(struct termios));

        new_kbd_mode.c_lflag &= ~(ICANON | ECHO);  /* new kbd flags */
        new_kbd_mode.c_cc[VTIME] = 0;
        new_kbd_mode.c_cc[VMIN] = 1;
        if (tcsetattr (0, TCSANOW, &new_kbd_mode)) {
            fprintf (stderr, "%s() error: tcsetattr failed.\n", __func__);
            return -1;
        }

        /* read chars from fp, mask if valid char specified */
        while (((c = fgetc (fp)) != '\n' && c != EOF && idx < sz - 1) ||
                (idx == sz - 1 && c == 127))
        {
            if (c != 127) {
                if (31 < mask && mask < 127)    /* valid ascii char */
                    fputc (mask, stdout);
                pw[idx++] = c;
            }
            else if (idx > 0) {         /* handle backspace (del)   */
                if (31 < mask && mask < 127) {
                    fputc (0x8, stdout);
                    fputc (' ', stdout);
                    fputc (0x8, stdout);
                }
                pw[--idx] = 0;
            }
        }
        pw[idx] = 0; /* null-terminate   */

        /* reset original keyboard  */
        if (tcsetattr (0, TCSANOW, &old_kbd_mode)) {
            fprintf (stderr, "%s() error: tcsetattr failed.\n", __func__);
            return -1;
        }

        if (idx == sz - 1 && c != '\n') /* warn if pw truncated */
            fprintf (stderr, " (%s() warning: truncated at %zu chars.)\n",
                    __func__, sz - 1);
        printf("\n");
        return idx; /* number of chars in passwd    */
}
