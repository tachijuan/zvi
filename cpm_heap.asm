; cpm_heap.asm  -- places _heap in bss_user (after all library BSS)
;
; Z88DK's CP/M linker script orders BSS sections as:
;   bss_compiler (SDCC user code)
;   bss_clib     (C library internals -- _buff, __exit_atexit_*, etc.)
;   bss_user     (deliberately reserved for user programs to extend BSS)
;
; cpm_heap.c defined _heap in bss_compiler, which is BEFORE the library's
; bss_clib segment.  Writing the gap buffer from _heap immediately
; corrupted _buff and __exit_atexit_*.  Placing _heap in bss_user ensures
; it always falls after __BSS_END_tail -- the first truly free TPA byte.
;
; The classic library's malloc-classic requires the external symbol _heap.
; This file satisfies that requirement correctly for the -compiler=sdcc build.

    SECTION bss_user
    PUBLIC  _heap
_heap:
