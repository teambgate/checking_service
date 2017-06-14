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
#include <arpa/inet.h>

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
