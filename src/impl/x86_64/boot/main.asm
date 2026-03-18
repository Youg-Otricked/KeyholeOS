section .data
section .text
    bits 32
    global _start
_start:
    ; goal so far: print `OK`
    mov dword [0xb8000], 0x2f4b2f4f
    hlt
