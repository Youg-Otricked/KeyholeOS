section .multiboot_header
    header_start:
        ; wtf does multi-boot 2 use a magic number gtfo
        dd 0xe85250d6 ; bruh. Da hell is 3897708758 multiboot2?
        ; architecture info
        dd 0 ; protected mode for some reason i386
        ; header length
        dd header_end - header_start ; header length. Ok so this actually makes sense YAY!
        ; checksum
        dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))
        ; information request tag
        dw 1
        dw 0
        dd 12
        dd 6
        ; padding for alignment
        dd 0
        ; end tag
        dw 0 ; W
        dw 0 ; T
        dw 8 ; F
        ; does this mean
    header_end: