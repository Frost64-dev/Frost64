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
global _x86_64_adc
global _x86_64_sub
global _x86_64_sbb
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
    setc al
    lahf ; AH = SF:ZF:0:AF:0:PF:1:CF
    seto cl ; CL = OF
    shr ah, 5
    or al, ah ; AL = CF, ZF, SF
    shl cl, 3 ; OF to bit 3
    or al, cl ; AL = CF, ZF, SF, OF
    mov r8b, BYTE [rdx] ; move flags to r8b
    and r8b, 0xF0 ; clear OF, SF, ZF, CF
    or r8b, al
    mov BYTE [rdx], r8b
%endmacro

; macro for OR/XOR/AND flags, which don't set CF/OF
%macro HANDLE_OR_FLAGS 0
    lahf ; AH = SF:ZF:0:AF:0:PF:1:CF
    shr ah, 5
    mov al, BYTE [rdx] ; move flags to al
    and al, 0xF0 ; clear OF, SF, ZF, CF
    or al, ah
    mov BYTE [rdx], al
%endmacro

; macro for SHL/SHR flags, where OF is unaffected
%macro HANDLE_SHIFT_FLAGS 0
    setc al
    lahf ; AH = SF:ZF:0:AF:0:PF:1:CF
    shr ah, 5
    or al, ah ; AL = CF, ZF, SF
    mov cl, BYTE [rdx] ; move flags to cl
    and cl, 0xF8 ; clear SF, ZF, CF
    or cl, al
    mov BYTE [rdx], cl
%endmacro

; macro for 1-operand instructions, using rsi instead of rdx
%macro HANDLE_1OP_FLAGS 0
    setc al
    lahf ; AH = SF:ZF:0:AF:0:PF:1:CF
    seto cl ; CL = OF
    shr ah, 5
    or al, ah ; AL = CF, ZF, SF
    shl cl, 3 ; OF to bit 3
    or al, cl ; AL = CF, ZF, SF, OF
    mov r8b, BYTE [rsi] ; move flags to r8b
    and r8b, 0xF0 ; clear OF, SF, ZF, CF
    or r8b, al
    mov BYTE [rsi], r8b
%endmacro

_x86_64_add:
    add rdi, rsi
    HANDLE_FLAGS

    mov rax, rdi
    ret

_x86_64_adc:
    ; Load incoming CF
    movzx r8, BYTE [rdx] ; move flags to r8
    and r8, 0xF1 ; clear OF, SF, ZF
    bt r8, 0 ; CF is bit 0

    adc rdi, rsi

    ; Now set flags
    setc al
    lahf ; AH = SF:ZF:0:AF:0:PF:1:CF
    seto cl ; CL = OF
    shr ah, 5
    or al, ah ; AL = CF, ZF, SF
    shl cl, 3 ; OF to bit 3
    or al, cl ; AL = CF, ZF, SF, OF
    and r8b, 0xF0 ; ensure lower nibble is clear, as CF could still be set
    or r8b, al
    mov BYTE [rdx], r8b

    mov rax, rdi
    ret

_x86_64_sub:
    sub rdi, rsi
    HANDLE_FLAGS

    mov rax, rdi
    ret

_x86_64_sbb:
    ; Load incoming CF
    movzx r8, BYTE [rdx] ; move flags to r8
    and r8, 0xF1 ; clear OF, SF, ZF
    bt r8, 0 ; CF is bit 0

    sbb rdi, rsi

    ; Now set flags
    setc al
    lahf ; AH = SF:ZF:0:AF:0:PF:1:CF
    seto cl ; CL = OF
    shr ah, 5
    or al, ah ; AL = CF, ZF, SF
    shl cl, 3 ; OF to bit 3
    or al, cl ; AL = CF, ZF, SF, OF
    and r8b, 0xF0 ; ensure lower nibble is clear, as CF could still be set
    or r8b, al
    mov BYTE [rdx], r8b

    mov rax, rdi
    ret

_x86_64_mul:
    mov rcx, rdx ; move pointer to flags to rcx

    and BYTE [rcx], 0xf4 ; clear CF, ZF, OF

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
    ret

_x86_64_div:
    mov rcx, rdx ; move divisor to rcx
    mov rdx, rsi ; move dividend to rdx:rax
    mov rax, rdi
.beforediv:
    div rcx
.afterdiv:
    ret

_x86_64_smul:
    mov rcx, rdx ; move pointer to flags to rcx

    and BYTE [rcx], 0xf4 ; clear CF, ZF, OF

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
    ret

_x86_64_sdiv:
    mov rcx, rdx ; move divisor to rcx
    mov rdx, rsi ; move dividend to rdx:rax
    mov rax, rdi
.beforediv:
    idiv rcx
.afterdiv:
    ret

_x86_64_or:
    or rdi, rsi
    HANDLE_OR_FLAGS

    mov rax, rdi
    ret

_x86_64_nor:
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
    ret

_x86_64_xor:
    xor rdi, rsi
    HANDLE_OR_FLAGS

    mov rax, rdi
    ret

_x86_64_xnor:
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
    ret

_x86_64_and:
    and rdi, rsi
    HANDLE_OR_FLAGS
    
    mov rax, rdi
    ret

_x86_64_nand:
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
    ret

_x86_64_not:
    not rdi
    mov rax, rdi
    ret

_x86_64_shl:
    mov cl, sil

    shl rdi, cl
    HANDLE_SHIFT_FLAGS

    mov rax, rdi
    ret

_x86_64_shr:
    mov cl, sil

    shr rdi, cl
    HANDLE_SHIFT_FLAGS

    mov rax, rdi
    ret

_x86_64_cmp:
    cmp rdi, rsi
    HANDLE_FLAGS
    ret

_x86_64_inc:
    add rdi, 1 ; x86 inc instruction doesn't affect CF, so use add
    HANDLE_1OP_FLAGS

    mov rax, rdi
    ret

_x86_64_dec:
    sub rdi, 1 ; x86 dec instruction doesn't affect CF, so use sub
    HANDLE_1OP_FLAGS

    mov rax, rdi
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