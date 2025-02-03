/*
Copyright (©) 2024-2025  Frosty515

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _SPINLOCK_H
#define _SPINLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long spinlock_t;

#define spinlock_init(lock) (*(lock) = 0)
#define spinlock_new(name) spinlock_t name = 0

void spinlock_acquire(spinlock_t* lock);
void spinlock_release(spinlock_t* lock);

#ifdef __cplusplus
}
#endif

#endif /* _SPINLOCK_H */