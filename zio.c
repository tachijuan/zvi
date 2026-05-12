#include <cpm.h>
#include "zvi.h"
#include "zfmt.h"
#include "zio.h"

/* Pre-allocate a small pool of file structs to avoid malloc dependency.
 * ZVI never has more than 2 files open simultaneously (during gb_save). */
#define MAX_FILES 2
static ZFILE s_zfiles[MAX_FILES];
static int   s_zfiles_used[MAX_FILES] = {0, 0};

/* Initialize an FCB with a parsed 8.3 filename. Returns 1 if valid. */
static int parse_filename(unsigned char *fcb, const char *name)
{
    int i;
    const char *p = name;
    char c;

    /* Zero out the entire FCB (36 bytes) */
    for (i = 0; i < 36; i++) fcb[i] = 0;

    /* Drive letter prefix (e.g. "B:") */
    if (p[0] && p[1] == ':') {
        c = p[0];
        if (c >= 'a' && c <= 'z') c -= 32;
        if (c >= 'A' && c <= 'P') fcb[0] = c - 'A' + 1;
        else return 0; /* invalid drive */
        p += 2;
    } else {
        fcb[0] = 0; /* default drive */
    }

    /* Filename (up to 8 chars) */
    for (i = 1; i <= 8; i++) fcb[i] = ' ';
    for (i = 1; i <= 8 && *p && *p != '.'; i++, p++) {
        c = *p;
        if (c >= 'a' && c <= 'z') c -= 32;
        fcb[i] = c;
    }

    /* Skip extra filename chars */
    while (*p && *p != '.') p++;

    /* Extension (up to 3 chars) */
    for (i = 9; i <= 11; i++) fcb[i] = ' ';
    if (*p == '.') {
        p++;
        for (i = 9; i <= 11 && *p; i++, p++) {
            c = *p;
            if (c >= 'a' && c <= 'z') c -= 32;
            fcb[i] = c;
        }
    }

    return 1;
}

ZFILE *z_fopen(const char *name, const char *mode)
{
    ZFILE *f = 0;
    int i;

    /* Find a free ZFILE slot */
    for (i = 0; i < MAX_FILES; i++) {
        if (!s_zfiles_used[i]) {
            f = &s_zfiles[i];
            s_zfiles_used[i] = 1;
            break;
        }
    }
    if (!f) return 0; /* Too many open files */

    if (!parse_filename(f->fcb, name)) {
        s_zfiles_used[i] = 0;
        return 0;
    }

    f->buf_len = 0;
    f->buf_pos = 0;
    f->offset = 0L;

    if (mode[0] == 'w') {
        f->is_write = 1;
        bdos(19, (unsigned)f->fcb); /* Delete existing */
        if (bdos(22, (unsigned)f->fcb) == 255) { /* Make file */
            s_zfiles_used[i] = 0;
            return 0;
        }
    } else {
        f->is_write = 0;
        if (bdos(15, (unsigned)f->fcb) == 255) { /* Open file */
            s_zfiles_used[i] = 0;
            return 0;
        }
    }

    return f;
}

int z_fclose(ZFILE *f)
{
    int i;
    if (!f) return EOF;

    if (f->is_write) {
        if (f->buf_pos > 0) {
            /* Pad the rest of the sector with Ctrl-Z (EOF) */
            while (f->buf_pos < 128) f->buf[f->buf_pos++] = 0x1A;
            bdos(26, (unsigned)f->buf); /* Set DMA */
            bdos(21, (unsigned)f->fcb); /* Write Sequential */
        }
    }

    bdos(16, (unsigned)f->fcb); /* Close file */

    /* Mark as free */
    for (i = 0; i < MAX_FILES; i++) {
        if (f == &s_zfiles[i]) {
            s_zfiles_used[i] = 0;
            break;
        }
    }
    return 0;
}

int z_fgetc(ZFILE *f)
{
    if (f->is_write) return EOF;

    if (f->buf_pos >= f->buf_len) {
        bdos(26, (unsigned)f->buf); /* Set DMA */
        if (bdos(20, (unsigned)f->fcb) != 0) { /* Read Sequential */
            return EOF; /* End of file or error */
        }
        f->buf_len = 128;
        f->buf_pos = 0;
    }

    f->offset++;
    return f->buf[f->buf_pos++];
}

int z_fputc(int c, ZFILE *f)
{
    if (!f->is_write) return EOF;

    f->buf[f->buf_pos++] = (unsigned char)c;
    f->offset++;

    if (f->buf_pos >= 128) {
        bdos(26, (unsigned)f->buf); /* Set DMA */
        if (bdos(21, (unsigned)f->fcb) != 0) { /* Write Sequential */
            return EOF; /* Disk full or error */
        }
        f->buf_pos = 0;
    }
    return c;
}

int z_fseek(ZFILE *f, long offset, int whence)
{
    long record;

    /* We only support SEEK_SET (whence == 0) */
    if (whence != 0 || f->is_write) return -1;
    if (offset < 0L) offset = 0L;

    record = offset / 128L;
    f->offset = record * 128L;

    /* Set Random Record (r0, r1, r2) */
    f->fcb[33] = (unsigned char)(record & 0xFF);         /* r0 */
    f->fcb[34] = (unsigned char)((record >> 8) & 0xFF);  /* r1 */
    f->fcb[35] = (unsigned char)((record >> 16) & 0xFF); /* r2 */

    bdos(26, (unsigned)f->buf); /* Set DMA */
    if (bdos(33, (unsigned)f->fcb) != 0) { /* Read Random */
        f->buf_len = 0;
        f->buf_pos = 0;
        return -1;
    }

    f->buf_len = 128;
    f->buf_pos = (int)(offset % 128L);
    f->offset = offset;

    /* CP/M's Read Random (33) doesn't advance the internal current record (cr).
     * It leaves cr and ex pointing to the record that was just read.
     * This means the NEXT Read Sequential (20) will read the EXACT SAME record again,
     * causing duplicated 128-byte blocks and file corruption in the editor.
     * We MUST issue a Read Sequential right now to advance the FCB.
     * It will overwrite f->buf with the exact same data, but correctly advance cr/ex. */
    bdos(20, (unsigned)f->fcb);

    return 0;
}

long z_ftell(ZFILE *f)
{
    return f->offset;
}

int z_remove(const char *name)
{
    unsigned char fcb[36];
    if (!parse_filename(fcb, name)) return -1;
    bdos(19, (unsigned)fcb);
    return 0;
}

int z_rename(const char *oldname, const char *newname)
{
    unsigned char fcb[36];
    if (!parse_filename(fcb, oldname)) return -1;
    /* Rename expects old name in first 16 bytes, new name in second 16 bytes */
    if (!parse_filename(fcb + 16, newname)) return -1;
    /* The second parse_filename writes to drive byte at offset 16 */
    /* CP/M expects the new filename at offset 17, and ignores the drive byte at 16 */
    bdos(23, (unsigned)fcb);
    return 0;
}

void exit(int code)
{
    (void)code;
    bdos(0, 0);
}
