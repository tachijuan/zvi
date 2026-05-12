/*
 * zvi.c - Main entry point for ZVI (VI clone for CP/M)
 * Author: Juan Orlandini
 * License: MIT
 *
 * Usage: zvi [filename]
 */

#include <string.h>
#include <cpm.h>
#include "zio.h"
#include "zvi.h"
#include "zfmt.h"

extern void exit(int code);

#define CPM_CMD_TAIL ((unsigned char *)0x0080)
Editor ed;
char msg_insert[] = "-- INSERT --";

/* Output a NUL-terminated string to the CP/M console via BDOS function 2.
 * Used before term_init() so we bypass the (not-yet-set-up) terminal layer. */
static void con_str(const char *s)
{
    while (*s)
        bdos(CPM_WCON, (unsigned char)*s++);
}

static void usage(void)
{
    con_str("Usage: zvi [filename]\r\n");
    exit(1);
}

int main(void)
{
    int   file_arg;
    int   partial;
    ZFILE *preopen;

    /* --- Zero-initialise editor state --- */
    memset(&ed, 0, sizeof(ed));
    ed.scr_rows       = DEF_ROWS;
    ed.scr_cols       = DEF_COLS;
    ed.search_dir     = SEARCH_FWD;
    ed.undo.type      = UNDO_NONE;
    ed.cur_line       = 0;
    ed.cur_line_pos   = -1;  /* force full scan on first scr_cur_line() call */
    ed.cur_vrow       = -1;  /* force full scan on first scr_scroll_to_cursor() */

    file_arg = -1;

    /* --- Parse arguments manually from CP/M DMA at 0x80 --- */
    {
        unsigned char len = CPM_CMD_TAIL[0];
        unsigned char *p = &CPM_CMD_TAIL[1];
        char argbuf[PATH_MAX];
        int arglen = 0;
        
        while (len > 0 && *p == ' ') { p++; len--; } /* skip leading spaces */
        
        while (len > 0 && *p != ' ' && *p != '\0' && arglen < PATH_MAX - 1) {
            argbuf[arglen++] = *p++;
            len--;
        }
        argbuf[arglen] = '\0';
        
        if (arglen > 0) {
            if (argbuf[0] == '-') usage();
            strncpy(ed.filename, argbuf, PATH_MAX - 1);
            ed.filename[PATH_MAX - 1] = '\0';
            file_arg = 1; /* Mark as found */
        }
    }

    /*
     * Open the file BEFORE gb_init().  gb_init() allocates nearly the
     * entire heap, leaving no room for fopen()'s internal I/O buffer.
     * Opening first guarantees fopen() has the full heap available.
     */
    preopen = (ZFILE *)0;
    if (file_arg > 0) {
        preopen = z_fopen(ed.filename, "rb");
    }

    /* --- Initialise gap buffer (uses free TPA above our code) --- */
    if (!gb_init()) {
        if (preopen) z_fclose(preopen);
        {
            /* Show BDOS address so the user can diagnose memory constraints. */
            char msg[48];
            char *p = z_str(msg, "zvi: insufficient TPA (BDOS at 0x");
            p = z_hex4(p, (unsigned int)(((unsigned int)*((unsigned char *)0x0007) << 8)
                                         | (unsigned int)*((unsigned char *)0x0006)));
            z_str(p, ")\r\n");
            con_str(msg);
        }
        exit(1);
    }

    /* --- Load file using the pre-opened handle --- */
    if (file_arg >= 0) {
        if (preopen) {
            char *_p;
            partial = gb_load(ed.filename, preopen);
            _p = ed.status;
            *_p++ = '"'; _p = z_str(_p, ed.filename); _p = z_str(_p, "\" ");
            if (partial == 2) {
                _p = z_int(_p, gb_content_len());
                z_str(_p, " chars (partial)");
            } else {
                _p = z_int(_p, gb_content_len());
                z_str(_p, " chars");
            }
        } else {
            /* File does not exist -- start with empty buffer */
            char *_p = ed.status;
            *_p++ = '"'; _p = z_str(_p, ed.filename); z_str(_p, "\" [New File]");
        }
        ed.modified = 0;
    } else {
        strcpy(ed.status, "[No Name]");
    }

    /* --- Initialise terminal and run editor --- */
    term_init();
    edit_run();
    term_restore();

    gb_free();
    return 0;
}
