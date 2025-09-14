; Copyright (©) 2023-2024  Frosty515
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
global _x86_64_mul
global _x86_64_sub
global _x86_64_div
global _x86_64_or
global _x86_64_xor
global _x86_64_nor
global _x86_64_and
global _x86_64_nand
global _x86_64_not
global _x86_64_cmp
global _x86_64_inc
global _x86_64_dec
global _x86_64_shl
global _x86_64_shr

x86_64_convert_flags: ; di = CPU flags --> rax = flags
    xor rax, rax
    mov sil, dil
    and sil, 1
    or al, sil ; carry flag
    mov sil, dil
    and sil, 1<<6 | 1<<7
    shr sil, 5
    or al, sil ; zero and sign flags
    mov si, di
    and si, 1<<11
    shr si, 8 ; overflow flag
    or ax, si
    ret

_x86_64_add:
    push rbp
    mov rbp, rsp

    add rdi, rsi
    
    push rdi
    pushf
    pop rdi
    call x86_64_convert_flags
    mov QWORD [rdx], rax

    pop rax

    mov rsp, rbp
    pop rbp
    ret

_x86_64_mul:
    push rbp
    mov rbp, rsp

    imul rdi, rsi

    push rdi
    pushf
    pop rdi
    and rdi, ~1
    call x86_64_convert_flags
    mov QWORD [rdx], rax

    pop rax

    mov rsp, rbp
    pop rbp
    ret

_x86_64_sub:
    push rbp
    mov rbp, rsp

    sub rdi, rsi
    
    push rdi
    pushf
    pop rdi
    call x86_64_convert_flags
    mov QWORD [rdx], rax

    pop rax

    mov rsp, rbp
    pop rbp
    ret

_x86_64_div:
    push rbp
    mov rbp, rsp

    mov QWORD [rdx], 0

    xor rdx, rdx
    mov rax, rdi
    idiv rsi

    mov rsp, rbp
    pop rbp
    ret

_x86_64_or:
    push rbp
    mov rbp, rsp

    or rdi, rsi
    
    push rdi
    pushf
    pop rdi
    call x86_64_convert_flags
    mov QWORD [rdx], rax

    pop rax

    mov rsp, rbp
    pop rbp
    ret

_x86_64_xor:
    push rbp
    mov rbp, rsp

    xor rdi, rsi
    
    push rdi
    pushf
    pop rdi
    call x86_64_convert_flags
    mov QWORD [rdx], rax

    pop rax

    mov rsp, rbp
    pop rbp
    ret

_x86_64_nor:
    push rbp
    mov rbp, rsp

    push rdx
    call _x86_64_or
    mov rdi, rax
    pop rsi
    call _x86_64_not

    mov rsp, rbp
    pop rbp
    ret

_x86_64_and:
    push rbp
    mov rbp, rsp

    and rdi, rsi
    
    push rdi
    pushf
    pop rdi
    call x86_64_convert_flags
    mov QWORD [rdx], rax

    pop rax

    mov rsp, rbp
    pop rbp
    ret

_x86_64_nand:
    push rbp
    mov rbp, rsp

    push rdx
    call _x86_64_and
    mov rdi, rax
    pop rsi
    call _x86_64_not

    mov rsp, rbp
    pop rbp
    ret

_x86_64_not:
    push rbp
    mov rbp, rsp

    not rdi
    mov QWORD [rsi], 0
    mov rax, rdi

    mov rsp, rbp
    pop rbp
    ret

_x86_64_cmp:
    push rbp
    mov rbp, rsp

    cmp rdi, rsi
    
    pushf
    pop rdi
    call x86_64_convert_flags
    mov QWORD [rdx], rax

    mov rsp, rbp
    pop rbp
    ret

_x86_64_inc:
    push rbp
    mov rbp, rsp

    inc rdi

    push rdi
    pushf
    pop rdi
    call x86_64_convert_flags
    mov QWORD [rsi], rax

    pop rax

    mov rsp, rbp
    pop rbp
    ret

_x86_64_dec:
    push rbp
    mov rbp, rsp

    dec rdi

    push rdi
    pushf
    pop rdi
    call x86_64_convert_flags
    mov QWORD [rsi], rax

    pop rax

    mov rsp, rbp
    pop rbp
    ret

_x86_64_shl:
    push rbp
    mov rbp, rsp

    mov cl, sil

    shl rdi, cl
    push rdi

    pushf
    pop rdi
    call x86_64_convert_flags
    mov QWORD [rdx], rax

    pop rax

    mov rsp, rbp
    pop rbp
    ret

_x86_64_shr:
    push rbp
    mov rbp, rsp

    mov cl, sil

    shr rdi, cl
    push rdi

    pushf
    pop rdi
    call x86_64_convert_flags
    mov QWORD [rdx], rax

    pop rax

    mov rsp, rbp
    pop rbp
    ret
