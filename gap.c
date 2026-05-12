/*
 * gap.c - Gap buffer implementation for ZVI
 * Author: Juan Orlandini
 * License: MIT
 *
 * The gap buffer stores file content as:
 *   [text before gap][GAP bytes][text after gap]
 * Insertions move the gap to the cursor and fill from the front.
 * Deletions simply expand the gap.
 */

#include "zio.h"
#include <string.h>
#include "zvi.h"
#include "zfmt.h"

/*
 * 'heap' is defined in cpm_heap.asm (placed in bss_user, which follows
 * all library BSS).  Its address is the first free byte of TPA -- used
 * directly as the gap buffer base without calling malloc.
 */
extern char heap;

extern Editor ed;

/* HI-TECH C V3.09 does not include memmove -- provide our own.
 * Kept as a static helper to avoid any calling-convention dependency
 * on the compiler's own memmove. */
static void gb_memmove(char *dst, char *src, int len)
{
    if (dst < src) {
        while (len-- > 0) *dst++ = *src++;
    } else if (dst > src) {
        dst += len; src += len;
        while (len-- > 0) *--dst = *--src;
    }
}

/*
 * Initialise the gap buffer using all available CP/M TPA free memory.
 *
 * Standard CP/M idiom (CP/M 2.2 Technical Reference §1.4):
 *
 *   CP/M page zero layout:
 *     0x0005: JMP <BDOS entry>      (3-byte instruction)
 *     0x0006: low  byte of BDOS entry address
 *     0x0007: high byte of BDOS entry address
 *
 * The free TPA lies between the end of our loaded program (anchored by
 * the 'heap' symbol in cpm_heap.c, which is the last BSS byte) and the
 * BDOS entry point less a stack reserve.
 *
 * This works on any CP/M machine -- 64 K, 56 K, 48 K -- because the
 * BDOS address is always accurately reported by the system, regardless
 * of how much RAM is actually installed.
 *
 * Returns 1 on success, 0 if there is less than 4 KB of free TPA.
 */
#define STACK_RESERVE  512u   /* bytes reserved for the runtime stack */

int gb_init(void)
{
    unsigned int buf_start;   /* first free TPA byte (= &heap)        */
    unsigned int bdos_lo;     /* low  byte of BDOS entry address       */
    unsigned int bdos_hi;     /* high byte of BDOS entry address       */
    unsigned int bdos_addr;   /* full BDOS entry address               */
    unsigned int buf_end;     /* last usable byte (BDOS - stack guard) */
    int          buf_size;

    /* Read BDOS address from CP/M page zero. */
    bdos_lo   = (unsigned int) *((unsigned char *)0x0006);
    bdos_hi   = (unsigned int) *((unsigned char *)0x0007);
    bdos_addr = (bdos_hi << 8) | bdos_lo;

    /* Guard: BDOS must be above our program. */
    if (bdos_addr < STACK_RESERVE) return 0;
    buf_end   = bdos_addr - STACK_RESERVE;

    /* Buffer starts at the first byte of free TPA. */
    buf_start = (unsigned int)(void *)&heap;

    if (buf_end <= buf_start) return 0;

    buf_size = (int)(buf_end - buf_start);
    if (buf_size < 4096) return 0;   /* not enough to be useful */

    /* Cap so that gap-buffer int offsets never overflow on 16-bit SDCC. */
    if (buf_size > BUF_MAX + GAP_MIN)
        buf_size = BUF_MAX + GAP_MIN;

    ed.gb.buf    = (char *)buf_start;
    ed.gb.size   = buf_size;
    ed.gb.gstart = 0;
    ed.gb.gend   = buf_size;
    return 1;
}

void gb_free(void)
{
    /* Buffer was carved directly from TPA, not malloc'd. */
    ed.gb.buf = (char *)0;
}

/* Logical content length (excludes the gap). */
int gb_content_len(void)
{
    return ed.gb.size - (ed.gb.gend - ed.gb.gstart);
}

/*
 * Return the character at logical position pos.
 * Returns -1 if pos is out of range.
 */
int gb_char_at(int pos)
{
    int len = gb_content_len();
    if (pos < 0 || pos >= len)
        return -1;
    if (pos < ed.gb.gstart)
        return (unsigned char)ed.gb.buf[pos];
    return (unsigned char)ed.gb.buf[ed.gb.gend + (pos - ed.gb.gstart)];
}

/*
 * Move the gap so that gstart == pos.
 * This is the core operation: O(n) move of characters.
 */
static void gb_move_gap(int pos)
{
    int gap_len = ed.gb.gend - ed.gb.gstart;
    if (pos == ed.gb.gstart)
        return;
    if (pos < ed.gb.gstart) {
        /* move gap left: shift text right */
        int move = ed.gb.gstart - pos;
        gb_memmove(ed.gb.buf + ed.gb.gend - move,
                ed.gb.buf + pos,
                move);
        ed.gb.gstart = pos;
        ed.gb.gend   = pos + gap_len;
    } else {
        /* move gap right: shift text left */
        int move = pos - ed.gb.gstart;
        gb_memmove(ed.gb.buf + ed.gb.gstart,
                ed.gb.buf + ed.gb.gend,
                move);
        ed.gb.gstart = pos;
        ed.gb.gend   = pos + gap_len;
    }
}

/*
 * Insert len bytes from text at logical position pos.
 * Returns 1 on success, 0 if the buffer is full (gap exhausted).
 */
int gb_insert(int pos, char *text, int len)
{
    int i, nl_added = 0;

    for (i = 0; i < len; i++) {
        if (text[i] == '\n') nl_added++;
    }

    if (ed.line_cnt_cached > 0) ed.line_cnt_cached += nl_added;

    if (ed.cur_line_pos >= 0) {
        if (pos <= ed.cur_line_pos) {
            ed.cur_line_pos += len;
            ed.cur_line += nl_added;
        }
    }

    if (pos != ed.cur_pos || nl_added > 0) {
        ed.cur_vrow = -1;
    }

    for (i = 0; i < len; i++) {
        if (ed.gb.gend - ed.gb.gstart < 1)
            return 0;   /* buffer is pre-allocated; cannot grow */
        gb_move_gap(pos + i);
        ed.gb.buf[ed.gb.gstart] = text[i];
        ed.gb.gstart++;
    }
    return 1;
}

/*
 * Delete len bytes starting at logical position pos.
 * Returns 1 on success.
 */
int gb_delete(int pos, int len)
{
    int i, clen, nl_del = 0;

    clen = gb_content_len();
    if (pos < 0 || pos >= clen)
        return 0;
    if (pos + len > clen)
        len = clen - pos;

    for (i = 0; i < len; i++) {
        if (gb_char_at(pos + i) == '\n') nl_del++;
    }

    if (ed.line_cnt_cached > 0) ed.line_cnt_cached -= nl_del;

    if (ed.cur_line_pos >= 0) {
        if (pos + len <= ed.cur_line_pos) {
            ed.cur_line_pos -= len;
            ed.cur_line -= nl_del;
        } else if (pos <= ed.cur_line_pos) {
            ed.cur_line_pos = -1;
        }
    }

    if (pos != ed.cur_pos || nl_del > 0) {
        ed.cur_vrow = -1;
    }

    gb_move_gap(pos);
    /* Expand gap right to consume deleted chars */
    ed.gb.gend += len;
    return 1;
}

/*
 * Show the [Loading...] indicator on the status line.
 * The caller's subsequent scr_refresh() will overwrite it.
 */
static void show_loading(void)
{
    term_goto(ed.scr_rows - 1, 0);
    term_clreol();
    term_reverse();
    term_puts("[Loading...]");
    term_normal();
    term_flush();
}

/*
 * Inner read loop shared by gb_load and gb_reload_from.
 * Reads chars from f into the gap buffer until EOF/^Z or the buffer fills.
 * On partial fill: records tail offset; if filename != NULL also sets
 * tail_file.  Always closes f before returning.
 * Returns 1 (complete) or 2 (partial — buffer filled before EOF).
 */
static int gb_fill(ZFILE *f, char *filename)
{
    int  c, prev_cr, full;
    long pos;
    char tmp[1];

    prev_cr = 0;
    while ((c = z_fgetc(f)) != EOF) {
        if (c == 0x0D) { prev_cr = 1; continue; }
        if (c == 0x1A) break;
        full = (gb_content_len() >= ed.gb.size - GAP_MIN);
        if (full) {
            pos = z_ftell(f) - 1L;
            if (c == 0x0A && prev_cr) pos -= 1L;
            ed.tail_offset = pos;
            if (filename) {
                strncpy(ed.tail_file, filename, PATH_MAX - 1);
                ed.tail_file[PATH_MAX - 1] = '\0';
            }
            z_fclose(f);
            return 2;
        }
        prev_cr = 0;
        tmp[0] = (char)c;
        gb_insert(gb_content_len(), tmp, 1);
    }
    z_fclose(f);
    return 1;
}

/*
 * Load a file into the gap buffer.
 * If fp != NULL use that already-open handle (pre-opened before gb_init()
 * so the heap is still free for fopen's buffer); otherwise fopen filename.
 * Strips bare CR characters to normalise line endings.
 * Returns 1 on success, 0 on open error, 2 on partial load (file too large).
 */
int gb_load(char *filename, ZFILE *fp)
{
    if (!fp) {
        fp = z_fopen(filename, "rb");
        if (!fp) return 0;
    }
    ed.gb.gstart    = 0;
    ed.gb.gend      = ed.gb.size;
    ed.win_start    = 0L;
    ed.win_line_offset = 0;
    ed.tail_offset  = 0L;
    ed.tail_file[0] = '\0';
    return gb_fill(fp, filename);
}

/*
 * Discard n characters from the head of the buffer, advancing win_start
 * to track the file-byte offset.  Adjusts cur_pos and top_pos.
 * The discard is capped so the cursor position is never dropped.
 * Returns the number of characters actually discarded.
 */
static int gb_discard_head(int n)
{
    int i, c, discard;
    long new_win;

    if (n <= 0) return 0;
    discard = n;
    if (discard > ed.cur_pos) discard = ed.cur_pos;
    if (discard <= 0) return 0;

    /* Each '\n' in the buffer was '\r\n' in the file, so count extra bytes */
    new_win = ed.win_start;
    for (i = 0; i < discard; i++) {
        c = gb_char_at(i);
        if (c == '\n') {
            new_win++;
            ed.win_line_offset++;
        }
        new_win++;
    }
    gb_delete(0, discard);
    ed.win_start = new_win;
    ed.cur_pos  -= discard;
    ed.top_pos  -= discard;
    if (ed.top_pos < 0) ed.top_pos = 0;
    if (ed.cur_pos < 0) ed.cur_pos = 0;
    return discard;
}

/*
 * Load up to n characters from the tail file into the end of the buffer.
 * If the buffer is already full, discards an equal number from the head first.
 * Returns the number of characters loaded, or 0 if the tail is exhausted.
 */
int gb_load_more(int n)
{
    ZFILE *f;
    int   c, loaded, need;
    char  tmp[1];

    if (ed.tail_offset == 0L || !ed.tail_file[0]) return 0;

    /* Make room: if content + n would exceed capacity, discard from head */
    need = gb_content_len() - (ed.gb.size - GAP_MIN - n);
    if (need > 0)
        if (gb_discard_head(need) == 0) return 0;

    show_loading();

    f = z_fopen(ed.tail_file, "rb");
    if (!f) return 0;
    z_fseek(f, ed.tail_offset, 0);

    loaded = 0;
    while (loaded < n && gb_content_len() < ed.gb.size - GAP_MIN) {
        c = z_fgetc(f);
        if (c == EOF || c == 0x1A) { ed.tail_offset = 0L; break; }
        if (c == 0x0D) continue;
        tmp[0] = (char)c;
        if (!gb_insert(gb_content_len(), tmp, 1)) break;
        loaded++;
    }
    if (ed.tail_offset != 0L)
        ed.tail_offset = z_ftell(f);

    z_fclose(f);
    return loaded;
}

/*
 * Reload the buffer starting from a given byte offset in tail_file.
 * Clears all buffer content and loads fresh content from that offset.
 * Resets cur_pos, top_pos, win_start, tail_offset, and display caches.
 * Shows [Loading...] on the status line while reading.
 * Returns 1 on success, 0 on failure (file not open, seek failed, etc.).
 */
int gb_reload_from(long offset)
{
    ZFILE *f;

    if (!ed.tail_file[0]) return 0;
    if (offset < 0L) offset = 0L;

    f = z_fopen(ed.tail_file, "rb");
    if (!f) return 0;

    show_loading();

    /* Compute how many lines precede this offset so absolute line numbers stay correct */
    ed.win_line_offset = 0;
    if (offset > 0L) {
        long pos = 0L;
        int c;
        while (pos < offset && (c = z_fgetc(f)) != EOF) {
            if (c == '\n') ed.win_line_offset++;
            pos++;
        }
    }

    if (z_fseek(f, offset, 0) != 0) { z_fclose(f); return 0; }

    /* Clear buffer and reset all window and display tracking. */
    ed.gb.gstart       = 0;
    ed.gb.gend         = ed.gb.size;
    ed.win_start       = offset;
    ed.tail_offset     = 0L;
    ed.cur_pos         = 0;
    ed.top_pos         = 0;
    ed.cur_vrow        = -1;
    ed.cur_line_pos    = -1;
    ed.line_cnt_cached = 0;
    /* tail_file unchanged — same source file */
    return gb_fill(f, (char *)0);
}

/*
 * Save the buffer to filename.
 * Writes LF as CR+LF (CP/M convention) and terminates with ^Z.
 * If the file was partially loaded (tail_offset > 0), the unloaded tail
 * is appended from tail_file after the in-memory content, so no data is
 * lost even when editing files larger than BUF_MAX.
 *
 * When saving to the same file that holds the tail, we write to a temp
 * file (ZVITMP.TMP) first, then replace the original with the temp so
 * that we are never reading from and writing to the same file at once.
 *
 * After a successful save, tail_file and tail_offset are updated to
 * point to the correct location in the newly written file so that
 * repeated saves remain correct.
 *
 * Returns 1 on success, 0 on error.
 */
/* Write the original-file head (bytes before the buffer window) to f. */
static void gb_write_head(ZFILE *f)
{
    ZFILE *tf;
    long  pos;
    int   c;
    if (ed.win_start <= 0L || !ed.tail_file[0]) return;
    tf = z_fopen(ed.tail_file, "rb");
    if (!tf) return;
    pos = 0L;
    while (pos < ed.win_start) {
        c = z_fgetc(tf);
        if (c == EOF || c == 0x1A) break;
        z_fputc(c, f);
        pos++;
    }
    z_fclose(tf);
}

/* Write in-memory buffer to f (LF -> CR+LF); return byte count written. */
static long gb_write_buf(ZFILE *f)
{
    long  written;
    int   i, len, c;
    written = ed.win_start;
    len = gb_content_len();
    for (i = 0; i < len; i++) {
        c = gb_char_at(i);
        if (c == '\n') { z_fputc(0x0D, f); written++; }
        z_fputc(c, f);
        written++;
    }
    return written;
}

/* Append the unloaded tail from the original source file to f. */
static void gb_write_tail(ZFILE *f)
{
    ZFILE *tf;
    int   c;
    if (ed.tail_offset <= 0L || !ed.tail_file[0]) return;
    tf = z_fopen(ed.tail_file, "rb");
    if (!tf) return;
    z_fseek(tf, ed.tail_offset, 0);
    while ((c = z_fgetc(tf)) != EOF && c != 0x1A)
        z_fputc(c, f);
    z_fclose(tf);
}

/*
 * Save the buffer to filename.
 * For large files, writes head + buffer + tail so no data is lost.
 * Uses a temp file when saving back to the same file that holds the tail.
 * Returns 1 on success, 0 on error.
 */
int gb_save(char *filename)
{
    ZFILE *f;
    int   using_tmp;
    long  new_tail;
    char  tmp_name[16];

    using_tmp = 0;
    if (ed.tail_file[0] && (ed.win_start > 0L || ed.tail_offset > 0L) &&
        strcmp(filename, ed.tail_file) == 0) {
        strcpy(tmp_name, "ZVITMP.TMP");
        f = z_fopen(tmp_name, "wb");
        using_tmp = 1;
    } else {
        f = z_fopen(filename, "wb");
    }
    if (!f) return 0;

    gb_write_head(f);
    new_tail = gb_write_buf(f);
    gb_write_tail(f);
    z_fputc(0x1A, f);
    z_fclose(f);

    if (using_tmp) { z_remove(filename); z_rename(tmp_name, filename); }

    if (ed.tail_file[0] && (ed.win_start > 0L || ed.tail_offset > 0L)) {
        strncpy(ed.tail_file, filename, PATH_MAX - 1);
        ed.tail_file[PATH_MAX - 1] = '\0';
        ed.tail_offset = new_tail;
    }
    return 1;
}

/*
 * Find beginning of line logically containing pos.
 */
int find_bol(int pos)
{
    while (pos > 0 && gb_char_at(pos - 1) != '\n') pos--;
    return pos;
}

/*
 * Find end of line logically containing pos.
 */
int find_eol(int pos)
{
    int size = gb_content_len();
    while (pos < size && gb_char_at(pos) != '\n') pos++;
    return pos;
}
