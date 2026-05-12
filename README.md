# ZVI - VI Clone for CP/M

**Version 2.0**

A lightweight VI-compatible editor for CP/M 2.2 and CP/M 3.0, built with the Z88DK SDCC toolchain. Uses a gap buffer for efficient editing and ANSI escape sequences
for terminal control. Implements most of the basic movement and editing commands
including the `.` operator for repeat. Also has a single level undo.

**Author:** Juan Orlandini  
**License:** MIT

---

## Building

### Requirements

- Z88DK (specifically the SDCC compiler) - included in the repository!
- CP/M 2.2 or CP/M 3.0 system with at least 48K TPA

### Build Steps (Linux/macOS host)

The Z88DK toolchain is bundled within the repository in the `z88dk` directory, so no external installation is required.

Simply run:

```
make
```

This will compile the C source files and link them using SDCC into the optimized `ZVI.COM` binary.

Transfer `ZVI.COM` to your CP/M system via XMODEM, Z-Modem, or disk image.

---

## Usage

```
ZVI [filename]
```

- `filename` — file to open (created if it does not exist)

---

## Supported Commands

### Normal Mode — Movement

| Key        | Action                              |
|------------|-------------------------------------|
| `h` / `←`  | Move left one character             |
| `l` / `→`  | Move right one character            |
| `j` / `↓`  | Move down one line                  |
| `k` / `↑`  | Move up one line                    |
| `Enter`    | Move to first non-blank of next line |
| `w`        | Forward to start of next word       |
| `b`        | Backward to start of previous word  |
| `e`        | Forward to end of word              |
| `0`        | Move to beginning of line           |
| `^`        | Move to first non-blank of line     |
| `$`        | Move to end of line                 |
| `G`        | Go to last line (or line N with count) |
| `gg`       | Go to first line (or line N: `5gg`) |
| `Ctrl-F`   | Scroll forward one page; cursor lands in the middle of the new page. No-op if already at the end of the file. |
| `Ctrl-B`   | Scroll backward one page; cursor lands in the middle of the new page. No-op if already at the beginning of the file. |
| `Ctrl-D`   | Scroll forward half page            |
| `Ctrl-U`   | Scroll backward half page           |

### Normal Mode — Insert / Append

| Key | Action                                    |
|-----|-------------------------------------------|
| `i` | Insert before cursor                      |
| `a` | Append after cursor                       |
| `I` | Insert at beginning of line               |
| `A` | Append at end of line                     |
| `o` | Open new line below, enter insert mode    |
| `O` | Open new line above, enter insert mode    |
| `s` | Substitute character(s) (delete + insert) |
| `S` | Substitute entire line                    |

### Normal Mode — Delete / Change

| Key  | Action                              |
|------|-------------------------------------|
| `x`  | Delete character under cursor       |
| `X`  | Delete character before cursor      |
| `dd` | Delete current line                 |
| `dw` | Delete word forward                 |
| `db` | Delete word backward                |
| `d$` | Delete to end of line               |
| `d0` | Delete to beginning of line         |
| `dG` | Delete to end of file               |
| `D`  | Delete to end of line (same as `d$`)|
| `cc` | Change current line                 |
| `cw` | Change word                         |
| `c$` | Change to end of line               |
| `C`  | Change to end of line (same as `c$`)|
| `r`  | Replace single character            |
| `J`  | Join line below to current line     |
| `~`  | Toggle case of character            |

### Normal Mode — Yank and Put

| Key  | Action                                        |
|------|-----------------------------------------------|
| `yy` | Yank (copy) current line                      |
| `Y`  | Yank current line (same as `yy`)              |
| `yw` | Yank word                                     |
| `y$` | Yank to end of line                           |
| `p`  | Put (paste) after cursor / below current line |
| `P`  | Put before cursor / above current line        |

### Normal Mode — Search

| Key  | Action                         |
|------|--------------------------------|
| `/`  | Search forward for pattern     |
| `?`  | Search backward for pattern    |
| `n`  | Repeat last search             |
| `N`  | Repeat last search in reverse  |

### Normal Mode — Character Search (current line)

| Key      | Action                                              |
|----------|-----------------------------------------------------|
| `f{c}`   | Move to next occurrence of character `c` on line    |
| `F{c}`   | Move to previous occurrence of character `c` on line|
| `;`      | Repeat last `f` or `F` in the same direction        |
| `,`      | Repeat last `f` or `F` in the opposite direction    |

Both repeat commands accept a count prefix (e.g. `3;` skips to the third next match).

### Normal Mode — Miscellaneous

| Key      | Action                        |
|----------|-------------------------------|
| `.`      | Repeat last change            |
| `u`      | Undo last change              |
| `:`      | Enter ex command mode         |
| `Ctrl-L` | Redraw screen                 |

### Insert Mode

| Key        | Action                              |
|------------|-------------------------------------|
| (any char) | Insert character                    |
| `Enter`    | Insert newline                      |
| `Backspace`| Delete previous character           |
| `Ctrl-H`   | Delete previous character           |
| `Ctrl-W`   | Delete previous word                |
| `Ctrl-U`   | Delete to start of line             |
| `↑↓←→`    | Move cursor (stay in insert mode)   |
| `ESC`      | Return to normal mode               |

### Ex Commands

| Command      | Action                                      |
|--------------|---------------------------------------------|
| `:w`         | Write (save) current file                   |
| `:w filename`| Write to named file                         |
| `:q`         | Quit (fails if unsaved changes)             |
| `:q!`        | Quit without saving                         |
| `:wq`        | Write and quit                              |
| `:wq!`       | Write and quit (force)                      |
| `:x`         | Write if modified, then quit                |
| `:x!`        | Write and quit (force)                      |
| `:e filename`| Abandon current buffer and edit named file  |
| `:e! filename`| Abandon modified buffer and edit named file |
| `:r filename`| Read file and insert after current line     |
| `:N`         | Go to line number N                         |
| `:$`         | Go to last line                             |

---

## Count Prefix

Most commands accept a numeric count prefix:

- `5j` — move down 5 lines
- `3w` — move forward 3 words
- `2dd` — delete 2 lines
- `10G` — go to line 10

---

## Status Line

The status line shows:

```
"filename" [+] L<current>/<total>
```

- `[+]` appears when the buffer has unsaved changes
- `<current>` is the current line number (1-based)
- `<total>` is the total line count in the loaded buffer

When a file is too large to fit entirely in memory, a `+` is appended to the
total to indicate that more content exists beyond the loaded window:

```
"filename" [+] L42/300+
```

Whenever ZVI reads the next chunk of a large file from disk, it briefly shows
`[Loading...]` on the status line.  The indicator is replaced by the normal
status display as soon as the screen is refreshed after the load completes.

---

## Large File Support

ZVI can open and edit files larger than available RAM. At startup it allocates
the largest single block available (typically ~20 KB on a CP/M system) and
loads as much of the file as fits, recording the byte offset where loading
stopped.

As you scroll forward with `j`, `Ctrl-D`, or `Ctrl-F`, ZVI automatically loads
the next chunk from the tail. When the buffer is full, an equal number of bytes
are silently discarded from the beginning of the buffer to make room. The
in-memory content is therefore a sliding window over the file.

On every `:w` save:

1. Any bytes scrolled off the front (before the current window) are copied from
   the original file.
2. The in-memory (edited) content is written in `CR+LF` format.
3. The unloaded tail (after the current window) is appended from the original
   file.
4. A `Ctrl-Z` (0x1A) terminator is written last.

When saving to the same filename that holds the tail, ZVI writes to a temporary
**Seamless Large File Edits:** ZVI now supports robust, seamless edits for files of any size. If you modify text and then scroll past the gap buffer boundaries (e.g., scrolling far forward, or jumping backwards with `Ctrl-B` / `G`), ZVI will automatically dump your unsaved changes to a temporary swap file (`ZVISWAP.TMP`). 
- When this happens, `[Auto-saving to swap...]` will briefly flash on your status bar. 
- The `[+]` indicator will remain on the status line to remind you that the swap file is active and contains uncommitted edits. 
- When you execute `:w`, ZVI seamlessly reconstructs your file from the swap data and your original file. 
- ZVI will prevent you from quitting (`:q`) or loading a new file (`:e`) if you have uncommitted swap data, unless you force it with `!`.

---

## Terminal Requirements

ZVI uses ANSI/VT100 escape sequences. It defaults to 80 columns × 24 rows,
which is the standard CP/M terminal size.

At startup ZVI always queries the terminal for its actual dimensions by
sending `ESC[999;999H` (cursor to extreme bottom-right) followed by
`ESC[6n` (ANSI cursor-position report). The response is read byte-by-byte
via BIOS CONIN, bypassing BDOS canonical buffering. If the terminal does
not respond within the polling timeout, ZVI silently falls back to the
80 × 24 defaults — it will not hang. No recompilation flag is needed.

Compatible terminals: VT100, VT220, xterm, ANSI.SYS, and most modern
terminal emulators connected via a serial port.

---

## File Format

ZVI reads files in binary mode, stripping bare `CR` characters on load.
On save, each `LF` is written as `CR+LF` per CP/M convention, and the
file is terminated with `Ctrl-Z` (0x1A) per CP/M file format rules.

---

## Performance

ZVI is designed for 9600 baud serial terminals. All screen updates are sized to
the minimum needed for the operation:

| Operation | Output sent |
|-----------|-------------|
| `h`, `l`, `0`, `^`, `$`, `f`, `F`, `;`, `,` | Cursor reposition only — no text redrawn, no status bar update |
| `j`, `k`, `Enter`, `w`, `b`, `e`, `/`, `?`, `n`, `N`, `gg`, `nG` | Terminal scroll + one new line (~53 bytes) when viewport shifts by one row, or cursor reposition only when no scroll needed — status bar not updated |
| `r` replace character | Single visual row redrawn |
| `x`, `X`, `D`, `~`, `s`, `S`, `C` | Current logical line redrawn |
| `J`, `o`, `O`, `p`, `P`, `u`, `dw`, `dd`, `cw`, Enter in insert mode | Rows from cursor to bottom redrawn (rows above cursor skipped) |
| `G`, `Ctrl-F`, `Ctrl-B`, `Ctrl-D`, `Ctrl-U`, `:` commands | Full screen redrawn |

The "cursor to bottom" tier is the key optimization for editing operations: on
a 24-row terminal with the cursor near the middle, it sends roughly half the
bytes of a full screen refresh (~600 bytes vs ~1200 bytes at 9600 baud ≈ 0.5
seconds saved per keystroke).

The status bar line counter is not refreshed on every `j`/`k`/`Enter`/`nG`
keypress — it updates on the next edit, search, page scroll, or `Ctrl-L`. The
total line count is cached after the first computation and invalidated only when
the buffer changes, so the status bar update itself is cheap when it does occur.

---

## Known Limitations

- Single-level undo only (`u` undoes the most recent change)
- Edits to large files are limited to the in-memory window (see Large File Support above)
- No visual/block selection mode
- No macro recording/playback
- No window splitting

---

## License

MIT License. Copyright (c) Juan Orlandini.
