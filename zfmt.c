/*
 * zfmt.c -- Tiny string-buffer builders to replace sprintf/fprintf.
 *
 * printf/sprintf in Z88DK's classic library costs ~1 000 bytes of linked
 * code: the core formatter, number-handling dispatch tables, and 64-bit
 * division helpers pulled in by %ld/%X support.  These three functions
 * cover every formatting need in this codebase.
 *
 * z_int() uses 16-bit division (% 10 / 10) which re-uses the already-
 * linked l_small_divu_16_16x16 routine; no extra library code is added.
 */

#include "zfmt.h"

/* Append string s to p, NUL-terminate, return pointer to the NUL. */
char *z_str(char *p, const char *s)
{
    while (*s)
        *p++ = *s++;
    *p = '\0';
    return p;
}

/*
 * Append the decimal representation of integer n to p.
 * Handles the full 16-bit signed range: -32768 .. 32767.
 */
char *z_int(char *p, int n)
{
    char tmp[7];   /* "-32768" + NUL */
    int  i = 0;

    if (n < 0) { *p++ = '-'; n = -n; }
    if (n == 0) {
        tmp[i++] = '0';
    } else {
        while (n > 0) { tmp[i++] = (char)('0' + n % 10); n /= 10; }
    }
    while (i > 0) *p++ = tmp[--i];
    *p = '\0';
    return p;
}

/*
 * Append a 4-digit uppercase hex string for unsigned int n.
 * Used only for the BDOS-address diagnostic in the OOM message.
 */
char *z_hex4(char *p, unsigned int n)
{
    static const char h[] = "0123456789ABCDEF";
    *p++ = h[(n >> 12) & 0xF];
    *p++ = h[(n >>  8) & 0xF];
    *p++ = h[(n >>  4) & 0xF];
    *p++ = h[ n        & 0xF];
    *p = '\0';
    return p;
}
