# Makefile for ZVI - VI clone for CP/M (Z88DK/SDCC build)
# Author: Juan Orlandini
# License: MIT
#
# Build:
#   make
#
# The Z88DK toolchain is bundled in the ./z88dk subdirectory so the
# build is fully self-contained.  No external Z88DK installation is needed.
#
# Compiler flags:
#   +cpm            Target CP/M 2.2 / 3.0
#   -compiler=sdcc  Use the SDCC Z80 back-end
#   --opt-code-size Optimise for smallest code (important in 64 KB)
#
# Notable fixes applied during porting from HiTech-C:
#
#   1. term.c now includes <cpm.h> instead of a raw "extern int bios(...)"
#      declaration.  The header wraps bios() / bdos() with __ZPROTO3 /
#      __z88dk_callee so SDCC's stack-based calling convention is correctly
#      translated to the Z80 register convention. Without this wrapper the
#      bios() call reads garbage registers and jumps to a random BIOS entry
#      -> immediate crash on the first character output.
#
#   2. cpm_heap.asm places _heap in the *bss_user* section, which Z88DK's
#      CP/M linker script guarantees follows all library BSS (bss_clib,
#      bss_driver, etc.).  The old cpm_heap.c put _heap in bss_compiler
#      (SDCC BSS), which is *before* library BSS.  The gap buffer then
#      wrote straight over _buff and __exit_atexit_* -> crash.
#
#   3. remove() / rename() are declared in <stdio.h> with proper __LIB__
#      attributes; the manual extern prototypes in gap.c were removed.
#
# Output: zvi.com

# ---- Z88DK root: bundled in ./z88dk ----------------------------------------
Z88DK   := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))z88dk
export Z88DK
export ZCCCFG := $(Z88DK)/lib/config
# Full path makes Make itself find zcc; PATH export lets zcc's child tools
# (z88dk-z80asm, z88dk-link, sdcc) be found by the recipe shell.
export PATH   := $(Z88DK)/bin:$(PATH)
CC      = $(Z88DK)/bin/zcc
# ----------------------------------------------------------------------------
CFLAGS = +cpm -compiler=sdcc --opt-code-size -SO3 -I$(Z88DK)/include -pragma-define:CRT_DISABLE_STDIO=1 -pragma-define:CRT_ENABLE_COMMANDLINE=0 -pragma-define:CLIB_OPEN_MAX=0

# cpm_heap.asm MUST be last so _heap lands in bss_user after all library BSS.
SRCS = zvi.c gap.c term.c screen.c emove.c edit.c erepeat.c ex.c zfmt.c zio.c cpm_heap.asm

zvi.com: $(SRCS) zvi.h
	$(CC) $(CFLAGS) -m $(SRCS) -o zvi -create-app

# Verbose build (shows every zcc sub-invocation):
verbose: $(SRCS) zvi.h
	$(CC) $(CFLAGS) -v $(SRCS) -o zvi -create-app

clean:
	rm -f zvi.com zvi.map zvi_BSS.bin zvi_CODE.bin
	rm -f $(filter-out cpm_heap.asm,$(wildcard *.asm)) *.o *.lst *.sym *.err
