section .data
section .text
    bits 32
    global _start
    extern long_mode_start
_start:
    cli
    mov esp, stack_top

    call check_multiboot
    
    call check_cpuid
    call check_long_mode

    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:long_mode_start
    hlt
    ; goal: print syscall
check_multiboot:
    cmp eax, 0x36d76289 ; tf another magic number whats 920085129 doing here
    jne .no_multiboot
    ret
.no_multiboot:
    mov al, "M"
    jmp error
check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp ecx, eax
    je .no_cpuid
    ret

.no_cpuid:
    mov al, "C"
    jmp error

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret

.no_long_mode:
    mov al, "L"
    jmp error

setup_page_tables:
    mov eax, page_table_l3
    or eax, 0b11 ; present, writeable
    mov [page_table_l4], eax

    mov eax, page_table_l2
    or eax, 0b11 ; present, writeable
    mov [page_table_l3], eax

    mov ecx, 0
.loop:

    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011 ; present, writeable , huge page again.
    mov [page_table_l2 + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .loop

    ret

enable_paging:
    ; pass page table location to the cpu
    mov eax, page_table_l4
    mov cr3, eax
    ; enable PAE (physical adress extention)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    ; enable long mode
    mov ecx, 0xc0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret

error:
    ; hardcoded print of `ERROR: X ` where X is the code
    mov dword [0xb8000], 0x4F524F45
    mov dword [0xb8004], 0x4F4F4F52
    mov dword [0xb8008], 0x4F3A4F52
    mov word  [0xb800c], 0x4F20
    mov byte  [0xb800e], al
    mov byte  [0xb800f], 0x4F
    hlt

section .bss
    align 4096
    page_table_l4:
        resb 4096
    page_table_l3:
        resb 4096
    page_table_l2:
        resb 4096
    stack_bottom:
        resb 4096 * 4
    stack_top:
section .rodata
    gdt64:
        dq 0                                                    ; null
    .code_segment: equ $ - gdt64
        dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)     ; code
    .pointer:
        dw $ - gdt64 - 1
        dq gdt64
