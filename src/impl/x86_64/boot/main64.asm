section .data
section .text
    bits 64
    extern kernel_main
    global long_mode_start
long_mode_start:
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax ; IDK it needs 0 in all d segments to work i guess?
    
    call kernel_main
    hlt