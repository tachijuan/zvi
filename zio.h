#ifndef ZIO_H
#define ZIO_H

/*
 * zio.h - Minimal bespoke file I/O for ZVI over CP/M BDOS
 *
 * This completely replaces <stdio.h> to bypass Z88DK's ~3KB POSIX
 * emulation and stdio layer. Provides buffered byte-by-byte access
 * and simple random access on 128-byte CP/M records.
 */

#ifndef EOF
#define EOF (-1)
#endif

typedef struct {
    unsigned char fcb[36];     /* CP/M File Control Block */
    unsigned char buf[128];    /* Sector buffer for 1 sector */
    int buf_len;               /* Number of valid bytes in buf */
    int buf_pos;               /* Current read/write index (0..128) */
    long offset;               /* Logical byte offset in file */
    int is_write;              /* 1 if opened for writing, 0 for reading */
} ZFILE;

/* Open a file. mode must be "rb" or "wb" */
extern ZFILE *z_fopen(const char *name, const char *mode);

/* Close a file and flush buffer if writing */
extern int z_fclose(ZFILE *f);

/* Read the next byte. Returns EOF on end of file. Handles CP/M 0x1A EOF. */
extern int z_fgetc(ZFILE *f);

/* Write a byte. */
extern int z_fputc(int c, ZFILE *f);

/* Seek to an absolute byte offset from the beginning (whence = 0). */
extern int z_fseek(ZFILE *f, long offset, int whence);

/* Return the current byte offset. */
extern long z_ftell(ZFILE *f);

/* Delete a file. */
extern int z_remove(const char *name);

/* Rename a file. */
extern int z_rename(const char *oldname, const char *newname);

#endif /* ZIO_H */
