; Copyright (©) 2023-2026  Frosty515
; 
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
; 
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.

[bits 64]

global _x86_64_add
global _x86_64_sub
global _x86_64_mul
global _x86_64_div
global _x86_64_smul
global _x86_64_sdiv
global _x86_64_or
global _x86_64_nor
global _x86_64_xor
global _x86_64_xnor
global _x86_64_and
global _x86_64_nand
global _x86_64_not
global _x86_64_shl
global _x86_64_shr
global _x86_64_cmp
global _x86_64_inc
global _x86_64_dec

; macro for flags
%macro HANDLE_FLAGS 0
    setc BYTE [rdx]
    setz al
    sets sil
    seto cl
    shl al, 1
    shl sil, 2
    shl cl, 3
    or BYTE [rdx], al
    or BYTE [rdx], sil
    or BYTE [rdx], cl
%endmacro

; macro for OR/XOR/AND flags, which don't set CF/OF
%macro HANDLE_OR_FLAGS 0
    setz al
    sets sil
    shl al, 1
    shl sil, 2
    or BYTE [rdx], al
    or BYTE [rdx], sil
%endmacro

; macro for SHL/SHR flags, where OF is unaffected
%macro HANDLE_SHIFT_FLAGS 0
    setc BYTE [rdx]
    setz al
    sets sil
    shl al, 1
    shl sil, 2
    or BYTE [rdx], al
    or BYTE [rdx], sil
%endmacro

; macro for 1-operand instructions, using rsi instead of rdx
%macro HANDLE_1OP_FLAGS 0
    setc BYTE [rsi]
    setz al
    sets cl
    seto dl
    shl al, 1
    shl cl, 2
    shl dl, 3
    or BYTE [rsi], al
    or BYTE [rsi], cl
    or BYTE [rsi], dl
%endmacro

_x86_64_add:
    push rbp
    mov rbp, rsp

    and BYTE [rdx], 0xf0

    add rdi, rsi
    HANDLE_FLAGS

    mov rax, rdi

    mov rsp, rbp
    pop rbp
    ret

_x86_64_sub:
    push rbp
    mov rbp, rsp

    and BYTE [rdx], 0xf0

    sub rdi, rsi
    HANDLE_FLAGS

    mov rax, rdi

    mov rsp, rbp
    pop rbp
    ret

_x86_64_mul:
    push rbp
    mov rbp, rsp

    and BYTE [rcx], 0xf4 ; clear CF, ZF, OF

    mov rcx, rdx ; move pointer to flags to rcx

    mov rax, rdi
    mul rsi ; rdx:rax = rax * rsi
    ; carry and overflow flags are set on x86
    setc BYTE [rcx]
    seto dil
    ; need to manually set zero flag
    mov rsi, rax
    or rsi, rdx
    setz sil
    shl sil, 1
    shl dil, 3
    or BYTE [rcx], sil
    or BYTE [rcx], dil

    mov rsp, rbp
    pop rbp
    ret

_x86_64_div:
    push rbp
    mov rbp, rsp

    mov rcx, rdx ; move divisor to rcx
    mov rdx, rsi ; move dividend to rdx:rax
    mov rax, rdi
.beforediv:
    div rcx
.afterdiv:

    mov rsp, rbp
    pop rbp
    ret

_x86_64_smul:
    push rbp
    mov rbp, rsp

    and BYTE [rcx], 0xf4 ; clear CF, ZF, OF

    mov rcx, rdx ; move pointer to flags to rcx

    mov rax, rdi
    imul rsi ; rdx:rax = rax * rsi
    ; carry and overflow flags are set on x86
    setc BYTE [rcx]
    seto dil
    ; need to manually set zero flag
    mov rsi, rax
    or rsi, rdx
    setz sil
    shl sil, 1
    shl dil, 3
    or BYTE [rcx], sil
    or BYTE [rcx], dil

    mov rsp, rbp
    pop rbp
    ret

_x86_64_sdiv:
    push rbp
    mov rbp, rsp

    mov rcx, rdx ; move divisor to rcx
    mov rdx, rsi ; move dividend to rdx:rax
    mov rax, rdi
.beforediv:
    idiv rcx
.afterdiv:

    mov rsp, rbp
    pop rbp
    ret

_x86_64_or:
    push rbp
    mov rbp, rsp

    and BYTE [rdx], 0xf0

    or rdi, rsi
    HANDLE_OR_FLAGS

    mov rax, rdi

    mov rsp, rbp
    pop rbp
    ret

_x86_64_nor:
    push rbp
    mov rbp, rsp

    and BYTE [rdx], 0xf0 ; clear CF and OF, not don't set them

    or rdi, rsi
    not rdi ; doesn't affect flags
    setnz al ; invert zero flag
    bt rdi, 63 ; test sign flag
    setc sil ; carry flag acts as sign flag here
    shl al, 1
    shl sil, 2
    or BYTE [rdx], al
    or BYTE [rdx], sil

    mov rax, rdi

    mov rsp, rbp
    pop rbp
    ret

_x86_64_xor:
    push rbp
    mov rbp, rsp

    and BYTE [rdx], 0xf0

    xor rdi, rsi
    HANDLE_OR_FLAGS

    mov rax, rdi

    mov rsp, rbp
    pop rbp
    ret

_x86_64_xnor:
    push rbp
    mov rbp, rsp

    and BYTE [rdx], 0xf0 ; clear CF and OF, not don't set them

    xor rdi, rsi
    not rdi ; doesn't affect flags
    setnz al ; invert zero flag
    bt rdi, 63 ; test sign flag
    setc sil ; carry flag acts as sign flag here
    shl al, 1
    shl sil, 2
    or BYTE [rdx], al
    or BYTE [rdx], sil

    mov rax, rdi

    mov rsp, rbp
    pop rbp
    ret

_x86_64_and:
    push rbp
    mov rbp, rsp

    and BYTE [rdx], 0xf0

    and rdi, rsi
    HANDLE_OR_FLAGS
    
    mov rax, rdi

    mov rsp, rbp
    pop rbp
    ret

_x86_64_nand:
    push rbp
    mov rbp, rsp

    and BYTE [rdx], 0xf0 ; clear CF and OF, not don't set them

    and rdi, rsi
    not rdi ; doesn't affect flags
    setnz al ; invert zero flag
    bt rdi, 63 ; test sign flag
    setc sil ; carry flag acts as sign flag here
    shl al, 1
    shl sil, 2
    or BYTE [rdx], al
    or BYTE [rdx], sil

    mov rax, rdi

    mov rsp, rbp
    pop rbp
    ret

_x86_64_not:
    push rbp
    mov rbp, rsp

    not rdi
    mov rax, rdi

    mov rsp, rbp
    pop rbp
    ret

_x86_64_shl:
    push rbp
    mov rbp, rsp

    and BYTE [rdx], 0xf8

    mov cl, sil

    shl rdi, cl
    HANDLE_SHIFT_FLAGS

    mov rax, rdi

    mov rsp, rbp
    pop rbp
    ret

_x86_64_shr:
    push rbp
    mov rbp, rsp

    and BYTE [rdx], 0xf8

    mov cl, sil

    shr rdi, cl
    HANDLE_SHIFT_FLAGS

    mov rax, rdi

    mov rsp, rbp
    pop rbp
    ret

_x86_64_cmp:
    push rbp
    mov rbp, rsp

    and BYTE [rdx], 0xf0

    cmp rdi, rsi
    HANDLE_FLAGS

    mov rsp, rbp
    pop rbp
    ret

_x86_64_inc:
    push rbp
    mov rbp, rsp

    and BYTE [rsi], 0xf0

    add rdi, 1
    HANDLE_1OP_FLAGS

    mov rax, rdi

    mov rsp, rbp
    pop rbp
    ret

_x86_64_dec:
    push rbp
    mov rbp, rsp

    and BYTE [rsi], 0xf0

    sub rdi, 1
    HANDLE_1OP_FLAGS

    mov rax, rdi

    mov rsp, rbp
    pop rbp
    ret

section .data

global _x86_64_div_beforediv
global _x86_64_div_afterdiv
_x86_64_div_beforediv: dq _x86_64_div.beforediv
_x86_64_div_afterdiv: dq _x86_64_div.afterdiv
global _x86_64_sdiv_beforediv
global _x86_64_sdiv_afterdiv
_x86_64_sdiv_beforediv: dq _x86_64_sdiv.beforediv
_x86_64_sdiv_afterdiv: dq _x86_64_sdiv.afterdiv