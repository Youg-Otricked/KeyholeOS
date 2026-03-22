section .data
section .text
    bits 64
    extern kernel_main
    global long_mode_start
    global load_idt
    extern interrupt_handler
    global outb
    global inb
    
outb:
    xor rdx, rdx
    mov dx, di
    xor rax, rax
    mov al, sil
    out dx, al
    ret
; inb: rdi = port, returns byte in rax
inb:
    mov dx, di
    in al, dx
    movzx rax, al
    ret
load_idt:
    lidt [rdi]    ; rdi = first argument = pointer to idtp
    sti           ; enable interrupts
    ret
isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp
    call interrupt_handler
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16       ; remove interrupt number + error code
    iretq

%macro isr_no_err 1 ; generates no code macros
global isr%1
isr%1:
    push 0
    push %1
    jmp isr_common
%endmacro

%macro isr_err 1 ; generates code macros
global isr%1
isr%1:
    push %1
    jmp isr_common
%endmacro

isr_no_err 0
isr_no_err 1
isr_no_err 2
isr_no_err 3
isr_no_err 4
isr_no_err 5
isr_no_err 6
isr_no_err 7
isr_err    8
isr_no_err 9
isr_err    10
isr_err    11
isr_err    12
isr_err    13
isr_err    14
isr_no_err 15
isr_no_err 16
isr_err    17
isr_no_err 18
isr_no_err 19
isr_no_err 20
isr_err    21
; Hardware
isr_no_err 32
isr_no_err 33
isr_no_err 34
isr_no_err 35
isr_no_err 36
isr_no_err 37
isr_no_err 38
isr_no_err 39
isr_no_err 40
isr_no_err 41
isr_no_err 42
isr_no_err 43
isr_no_err 44
isr_no_err 45
isr_no_err 46
isr_no_err 47
long_mode_start:
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax ; IDK it needs 0 in all d segments to work i guess?
    call kernel_main
.halt:
    hlt
    jmp .halt
