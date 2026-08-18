; -----------------------------------------------------------------------------
; @file    bignum_init_from_string.asm
; @brief   Оптимизированная инициализация bignum_t с использованием LUT.
; @details System V ABI: rdi = dst, rsi = str, edx = base.
; -----------------------------------------------------------------------------
; SPDX-License-Identifier: MIT
; -----------------------------------------------------------------------------

default rel
section .text
    align 16
    global bignum_init_from_string

BIGNUM_CAPACITY                    equ 32
BIGNUM_WORD_SIZE                   equ 8
BIGNUM_OFFSET_LEN                  equ BIGNUM_CAPACITY * BIGNUM_WORD_SIZE
BIGNUM_INIT_FROM_STRING_SUCCESS                     equ 0
BIGNUM_INIT_FROM_STRING_ERROR_NULL_ARG              equ -1
BIGNUM_INIT_FROM_STRING_ERROR_OVERFLOW              equ -2
BIGNUM_INIT_FROM_STRING_ERROR_BAD_BASE              equ -3
BIGNUM_INIT_FROM_STRING_ERROR_EMPTY                 equ -4
BIGNUM_INIT_FROM_STRING_ERROR_PARSE                 equ -5

section .rodata
    align 16
    ; 256-byte LUT: ASCII byte -> digit value, or 0xFF for invalid input.
    digit_lut:
        times 48 db 0xFF
        db 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
        times 7 db 0xFF
        db 10, 11, 12, 13, 14, 15
        times 26 db 0xFF
        db 10, 11, 12, 13, 14, 15
        times 25 db 0xFF
        times 128 db 0xFF

    ; shift_lut: index = base, value = binary shift for bases 2, 8, 16.
    shift_lut:
        db 0, 0, 1, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 4

section .text
bignum_init_from_string:
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15

    mov     eax, BIGNUM_INIT_FROM_STRING_ERROR_NULL_ARG
    test    rdi, rdi
    jz      .return
    test    rsi, rsi
    jz      .return
    lea     r9, [rel digit_lut]

    mov     ebx, edx
    test    ebx, ebx
    jz      .base_auto
    cmp     ebx, 2
    jb      .bad_base
    cmp     ebx, 16
    ja      .bad_base
    mov     r8d, ebx
    jmp     .skip_spaces

.base_auto:
    mov     r8d, 10

.skip_spaces:
    movzx   eax, byte [rsi]
    cmp     al, ' '
    je      .space_next
    cmp     al, 9
    je      .space_next
    cmp     al, 10
    je      .space_next
    cmp     al, 11
    je      .space_next
    cmp     al, 12
    je      .space_next
    cmp     al, 13
    je      .space_next
    jmp     .check_sign

.space_next:
    inc     rsi
    jmp     .skip_spaces

.check_sign:
    cmp     byte [rsi], '+'
    je      .consume_sign
    cmp     byte [rsi], '-'
    jne     .check_prefix
.consume_sign:
    inc     rsi
.skip_spaces_after_sign:
    movzx   eax, byte [rsi]
    cmp     al, ' '
    je      .sign_space_next
    cmp     al, 9
    je      .sign_space_next
    cmp     al, 10
    je      .sign_space_next
    cmp     al, 11
    je      .sign_space_next
    cmp     al, 12
    je      .sign_space_next
    cmp     al, 13
    je      .sign_space_next
    jmp     .check_prefix

.sign_space_next:
    inc     rsi
    jmp     .skip_spaces_after_sign

.check_prefix:
    test    ebx, ebx
    jnz     .prefix_fixed_base

    cmp     byte [rsi], '0'
    jne     .after_prefix_check
    mov     al, [rsi + 1]
    cmp     al, 'b'
    je      .set_bin
    cmp     al, 'B'
    je      .set_bin
    cmp     al, 'o'
    je      .set_oct
    cmp     al, 'O'
    je      .set_oct
    cmp     al, 'x'
    je      .set_hex
    cmp     al, 'X'
    je      .set_hex
    jmp     .after_prefix_check

.prefix_fixed_base:
    cmp     r8d, 2
    je      .prefix_base2
    cmp     r8d, 8
    je      .prefix_base8
    cmp     r8d, 16
    je      .prefix_base16
    jmp     .after_prefix_check

.prefix_base2:
    cmp     byte [rsi], '0'
    jne     .after_prefix_check
    mov     al, [rsi + 1]
    cmp     al, 'b'
    je      .consume_bin
    cmp     al, 'B'
    je      .consume_bin
    jmp     .after_prefix_check

.prefix_base8:
    cmp     byte [rsi], '0'
    jne     .after_prefix_check
    mov     al, [rsi + 1]
    cmp     al, 'o'
    je      .consume_oct
    cmp     al, 'O'
    je      .consume_oct
    jmp     .after_prefix_check

.prefix_base16:
    cmp     byte [rsi], '0'
    jne     .after_prefix_check
    mov     al, [rsi + 1]
    cmp     al, 'x'
    je      .consume_hex
    cmp     al, 'X'
    je      .consume_hex
    jmp     .after_prefix_check

.set_bin:
    add     rsi, 2
    mov     r8d, 2
    jmp     .after_prefix_check
.set_oct:
    add     rsi, 2
    mov     r8d, 8
    jmp     .after_prefix_check
.set_hex:
    add     rsi, 2
    mov     r8d, 16
    jmp     .after_prefix_check
.consume_bin:
    add     rsi, 2
    mov     r8d, 2
    jmp     .after_prefix_check
.consume_oct:
    add     rsi, 2
    mov     r8d, 8
    jmp     .after_prefix_check
.consume_hex:
    add     rsi, 2
    mov     r8d, 16

.after_prefix_check:
    cmp     byte [rsi], 0
    je      .empty

    xor     r11d, r11d
    xor     r13d, r13d
.clear_loop:
    cmp     r13, BIGNUM_CAPACITY
    jae     .parse_loop
    mov     qword [rdi + r13 * BIGNUM_WORD_SIZE], 0
    inc     r13
    jmp     .clear_loop

.parse_loop:
    movzx   eax, byte [rsi]
    test    al, al
    jz      .success

    movzx   r10d, byte [r9 + rax]
    cmp     r10d, 0xFF
    je      .parse_error
    cmp     r10d, r8d
    jae     .parse_error

    cmp     r8d, 2
    je      .shift_by_1
    cmp     r8d, 8
    je      .shift_by_3
    cmp     r8d, 16
    je      .shift_by_4
    jmp     .multiply_loop

.shift_by_1:
    mov     r14d, 1
    jmp     .shift_begin
.shift_by_3:
    mov     r14d, 3
    jmp     .shift_begin
.shift_by_4:
    mov     r14d, 4
.shift_begin:
    xor     r12d, r12d
    xor     r13d, r13d
.shift_loop:
    cmp     r13, r11
    jae     .shift_done
    mov     rax, [rdi + r13 * BIGNUM_WORD_SIZE]
    mov     r15, rax
    mov     ecx, r14d
    shl     rax, cl
    or      rax, r12
    mov     ecx, 64
    sub     ecx, r14d
    shr     r15, cl
    mov     [rdi + r13 * BIGNUM_WORD_SIZE], rax
    mov     r12, r15
    inc     r13
    jmp     .shift_loop
.shift_done:
    jmp     .check_carry

.multiply_loop:
    xor     r12d, r12d
    xor     r13d, r13d
.mul_inner:
    cmp     r13, r11
    jae     .mul_done
    mov     rax, [rdi + r13 * BIGNUM_WORD_SIZE]
    mul     r8
    add     rax, r12
    adc     rdx, 0
    mov     [rdi + r13 * BIGNUM_WORD_SIZE], rax
    mov     r12, rdx
    inc     r13
    jmp     .mul_inner
.mul_done:
.check_carry:
    test    r12, r12
    jz      .add_digit
    cmp     r11, BIGNUM_CAPACITY
    jae     .overflow
    mov     [rdi + r11 * BIGNUM_WORD_SIZE], r12
    inc     r11

.add_digit:
    mov     r12, r10
    xor     r13d, r13d
.add_loop:
    test    r12, r12
    jz      .next_char
    cmp     r13, r11
    jae     .append_carry
    mov     rax, [rdi + r13 * BIGNUM_WORD_SIZE]
    add     rax, r12
    setc    dl
    movzx   r12d, dl
    mov     [rdi + r13 * BIGNUM_WORD_SIZE], rax
    inc     r13
    jmp     .add_loop

.append_carry:
    cmp     r11, BIGNUM_CAPACITY
    jae     .overflow
    mov     [rdi + r11 * BIGNUM_WORD_SIZE], r12
    inc     r11

.next_char:
    inc     rsi
    jmp     .parse_loop

.success:
    mov     [rdi + BIGNUM_OFFSET_LEN], r11
    xor     eax, eax
    jmp     .return

.bad_base:
    mov     eax, BIGNUM_INIT_FROM_STRING_ERROR_BAD_BASE
    jmp     .return
.empty:
    mov     eax, BIGNUM_INIT_FROM_STRING_ERROR_EMPTY
    jmp     .return
.parse_error:
    mov     eax, BIGNUM_INIT_FROM_STRING_ERROR_PARSE
    jmp     .return
.overflow:
    mov     eax, BIGNUM_INIT_FROM_STRING_ERROR_OVERFLOW

.return:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
