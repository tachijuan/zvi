#ifndef ZFMT_H
#define ZFMT_H

/*
 * zfmt.h -- Tiny string-buffer builders to replace sprintf/fprintf.
 *
 * The full printf subsystem in Z88DK's classic library costs roughly
 * 1 000 bytes of linked code (asm_printf, number formatters, 64-bit
 * division and comparison helpers pulled in by %ld/%X support).
 * We don't need any of that complexity: the three small functions below
 * cover every formatting need in this codebase.
 *
 * Convention: each z_* function appends text to *p, always
 * NUL-terminates, and returns a pointer to the trailing NUL so calls
 * can be chained:
 *
 *     char *p = buf;
 *     p = z_str(p, "\"");
 *     p = z_str(p, fname);
 *     p = z_int(p, count);
 *     z_str(p, " chars");
 */

extern char *z_str(char *p, const char *s);
extern char *z_int(char *p, int n);
extern char *z_hex4(char *p, unsigned int n);

/* Append single character c and NUL-terminate (macro for inlining). */
#define z_ch(p, c)  ((*(p) = (char)(c)), (*((p)+1) = '\0'), (p)+1)

/* Copy a literal string into buf; equivalent to strcpy(buf, lit). */
#define z_set(buf, lit)  z_str((buf), (lit))

#endif /* ZFMT_H */
