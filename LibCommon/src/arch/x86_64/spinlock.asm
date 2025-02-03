; Copyright (©) 2024-2025  Frosty515
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

global spinlock_acquire
spinlock_acquire:
    lock bts QWORD [rdi], 0
    jc .spin_with_pause
    ret

.spin_with_pause:
    pause
    test QWORD [rdi], 1
    jnz .spin_with_pause
    jmp spinlock_acquire

global spinlock_release
spinlock_release:
    mov QWORD [rdi], 0
    ret
