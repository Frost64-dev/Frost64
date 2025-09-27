; Copyright (©) 2025  Frosty515
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

global GetRegisterFromID

GetRegisterFromID: ; rdi=reg, rsi=error, rdx=data
    ; Check if reg is greater than IP, which is 41
    cmp dil, 41
    jg .error
    movzx eax, dil
    ret

.error:
    push rbp
    mov rbp, rsp
    mov rdi, rdx
    call rsi
    ; likely dead code from here
    pop rbp
    ret
