section .data
section .text
    bits 32
    global _start
    extern long_mode_start
_start:
    cli
    mov esp, stack_top
    push ebx            ; save multiboot info on stack
    
    call check_multiboot
    call check_cpuid

    call check_long_mode

    call setup_page_tables
    call enable_paging
    
    pop edi             ; restore to edi
    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:long_mode_start
.halt:
    hlt
    jmp .halt
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
    or eax, 0b11
    mov [page_table_l4], eax
    mov eax, page_table_l2
    or eax, 0b11
    mov [page_table_l3], eax
    mov eax, page_table_l1_0
    or eax, 0b11
    mov [page_table_l2], eax

    mov eax, page_table_l1_1
    or eax, 0b11
    mov [page_table_l2 + 8], eax

    mov eax, page_table_l1_2
    or eax, 0b11
    mov [page_table_l2 + 16], eax

    mov eax, page_table_l1_3
    or eax, 0b11
    mov [page_table_l2 + 24], eax
    mov ecx, 0
.loop:
    mov eax, 0x1000
    mul ecx
    or eax, 0b11
    cmp ecx, 512
    jl .t0
    cmp ecx, 1024
    jl .t1
    cmp ecx, 1536
    jl .t2
    jmp .t3
.t0:
    mov [page_table_l1_0 + ecx * 8], eax
    jmp .next
.t1:
    lea edx, [ecx - 512]
    mov [page_table_l1_1 + edx * 8], eax
    jmp .next
.t2:
    lea edx, [ecx - 1024]
    mov [page_table_l1_2 + edx * 8], eax
    jmp .next
.t3:
    lea edx, [ecx - 1536]
    mov [page_table_l1_3 + edx * 8], eax
.next:
    inc ecx
    cmp ecx, 2048
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
    global stack_top
    align 4096
    page_table_l4:
        resb 4096
    page_table_l3:
        resb 4096
    page_table_l2:
        resb 4096
    page_table_l1_0:
        resb 4096
    page_table_l1_1:
        resb 4096
    page_table_l1_2:
        resb 4096
    page_table_l1_3:
        resb 4096
    stack_bottom:
        resb 4096 * 4
    stack_top:
section .rodata
    global gdt64
    gdt64:
        dq 0
        .code_segment: equ $ - gdt64
            dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
        .data_segment: equ $ - gdt64
            dq (1 << 44) | (1 << 47) | (1 << 41)
        .tss: equ $ - gdt64
            dq 0
            dq 0
        .user_data: equ $ - gdt64
            dq (1 << 44) | (1 << 47) | (1 << 41) | (3 << 45)
        .user_code: equ $ - gdt64
            dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) | (3 << 45)
        .pointer:
            dw $ - gdt64 - 1
            dq gdt64

