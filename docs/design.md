# The Frost64 Architecture

## Registers

- 16 64-bit general purpose registers (GPRs) named r0-r15
- 3 64-bit stack related registers. See [Stack](#stack-info)
- 1 64-bit status register that is read-only. named STS. See [Status Layout](#status-layout) for more info
- 4 64-bit control registers named CR0-CR8, see [Control Registers Layout](#control-registers-layout) for more info
- 1 64-bit instruction register, named IP. It is read-only.

### Control Registers Layout

#### CR0

| Bit  | Name     | Description            |
|------|----------|------------------------|
| 0    | PE       | Protected mode enabled |
| 1    | PG       | Paging enabled         |
| 2-3  | PGS      | Base page size         |
| 4-5  | PTL      | Page table levels      |
| 6-63 | RESERVED | Reserved               |

#### CR1

- Note: CR1 is reserved in 64-bit real mode

| Bit  | Name   | Description                               |
|------|--------|-------------------------------------------|
| 0-63 | SFLAGS | Supervisor flags on supervisor mode entry |

#### CR2

- Note: CR2 is reserved in 64-bit real mode

| Bit  | Name   | Description                              |
|------|--------|------------------------------------------|
| 0-63 | SENTRY | Supervisor `I0` on supervisor mode entry |

#### CR3

- Note: CR3 is reserved in 64-bit real mode

| Bit  | Name | Description                               |
|------|------|-------------------------------------------|
| 0-63 | PT   | Page table address (must be page aligned) |

#### CR4-CR7

| Bit  | Name     | Description |
|------|----------|-------------|
| 0-63 | RESERVED | Reserved    |

### Status Layout

| Bit     | Name     | Description    |
|---------|----------|----------------|
| 0       | CF       | Carry flag     |
| 1       | ZF       | Zero flag      |
| 2       | SF       | Sign flag      |
| 3       | OF       | Overflow flag  |
| 4       | IF       | Interrupt flag |
| 5-63    | RESERVED | Reserved       |

## Stack Info

- grows upwards.
- register `sbp` for base of the stack address (index 0)
- register `scp` for current stack address (index 1)
- register `stp` for top of the stack address (index 2)

## Exceptions

- The first 32 interrupts are reserved for exceptions.
- Currently, there are 9 exceptions defined as follows:

| Number | Name                      | Error Code Size in QWORDs | Description                                                        |
|--------|---------------------------|---------------------------|--------------------------------------------------------------------|
| 0      | Divide by zero            | 0                         | Thrown when a divide by zero is attempted                          |
| 1      | Phys Mem Violation        | 1                         | Thrown when an invalid physical memory address is accessed         |
| 2      | Unhandled Interrupt       | 1                         | Thrown when an interrupt is raised but not handled                 |
| 3      | Invalid Instruction       | 0                         | Thrown when an invalid instruction is executed                     |
| 4      | Stack Violation           | 1                         | Thrown when the stack is out of bounds or unaligned                |
| 5      | User Mode Violation       | 0                         | Thrown when a supervisor mode instruction is executed in user mode |
| 6      | Supervisor Mode Violation | 0                         | Thrown when a user mode instruction is executed in supervisor mode |
| 7      | Paging Violation          | 2                         | Thrown when a paging violation occurs                              |
| 8      | Integer Overflow          | 0                         | Thrown when an integer overflow occurs                             |

### Error Info

- The error code is pushed to the stack after the other arguments.
- It must be popped off the stack before the `iret` instruction is called.
- The error code can be 0-2 QWORDs in size.

#### Phys Mem Violation

- The error code is the physical address that was attempted to be accessed.

#### Unhandled Interrupt

- The error code is the interrupt number that was unable to be handled.

#### Stack Violation

- The error code is in the following format:

| Offset in bits | Name     | Description                                         |
|----------------|----------|-----------------------------------------------------|
| 0              | UNDER    | Did the current pointer go under the bottom pointer |
| 1              | OVER     | Did the current pointer go over the top pointer     |
| 2              | ALIGN    | Was the stack pointer not 8-byte aligned            |
| 3-63           | RESERVED | Reserved                                            |

#### Paging Violation

- The error code is broken up into 2 QWORDs.
- First QWORD is the virtual address that was attempted to be accessed.
- Second QWORD is the error code in the following format:

| Offset in bits | Name       | Description                                          |
|----------------|------------|------------------------------------------------------|
| 0              | PRESENT    | Was the page present                                 |
| 1              | READ       | Was the page attempted to be read                    |
| 2              | WRITE      | Was the page attempted to be written                 |
| 3              | EXECUTE    | Was the page attempted to be executed                |
| 4              | USER       | Was the page attempted to be accessed from user mode |
| 5              | RSVD_WRITE | Was a reserved bit in the data structures set        |
| 6-63           | RESERVED   | Reserved                                             |

## Calling convention info

### On call

#### Prior to call instruction

1. registers r0-r7 are saved to the stack (starting with r0, finishing with r7) if their values are of importance
2. arguments are placed on the stack right to left

#### Performed during the call instruction

1. I1 is placed on the stack
2. return address is placed in I1
3. function is jumped to

#### At start of new function

1. sbp is saved to the stack
2. scp is saved in sbp

### On return

#### Prior to ret instruction

1. any return value is in r0 and optionally r1 as well. (low 64-bits are in r0, high 64-bits are in r1)
2. scp is restored from sbp
3. sbp is popped off the stack

#### Performed during ret instruction

1. Value in I1 is moved to I0, but not executed yet
2. I1 is restored from the stack
3. CPU continues executing

#### After ret instruction

1. arguments are removed from stack
2. register r0-r7 are restored from the stack (starting with r7, finishing with r0) if their values were saved

## Operation modes

- Default mode: 64-bit real mode
- Extra mode: 64-bit protected mode

### 64-bit real mode

- All memory is accessible by everything
- All registers are accessible by everything (IP, and STS are read-only)
- All valid instructions always work

### 64-bit protected mode

- 2 levels of access: user, supervisor.
- bit 0 of CR0 is set to 1 to enable protected mode.
- defaults to supervisor mode.
- Interrupts behave differently. See [Protected mode interrupts](#protected-mode-interrupts) for more info.
- Paging support. See [Paging](#paging) for more info.

#### Switching privilege levels

On supervisor mode entry, the contents of `STS` is swapped with the contents of `CR1`, and `IP` will be set to `CR2`. `r14` will be set to the address of the instruction after the `syscall` instruction. The old value of `r14` is not saved. `r15` will be set to the user mode stack. Supervisor mode is responsible for saving `sbp`, `stp`, and `r0`-`r13` if they are of importance. Supervisor mode is also responsible for getting its own stack. After all this, the values of `r14` and `r15` would have been overridden. If they are of importance, they should be saved prior to the `syscall` instruction.

On supervisor mode exit, the contents of `CR1` is swapped with the contents of `STS`, and `I0` will be set to `r14`. `scp` will be set to `r15`. The old value of `IP` is not saved. Supervisor mode is responsible for restoring `sbp`, `stp`, and `r0`-`r13` if they were saved.

On user mode entry (different from supervisor mode exit), `STS` is cleared. `IP` will be set to the first argument of the instruction. All other registers are untouched.

## Paging

- Total of 9 possible modes.
- There are 3 different page size options set in PGS: 4KiB (0), 16KiB (1), 64KiB (2).
- Each page table level takes up 10 bits of the address space.
- There can be 3-5 page table levels. This is set in PTL: 3 levels is 0, 4 levels is 1, 5 levels is 2.
- A page size of 64KiB is not an option for 5 levels of page tables. An `INVALID_INSTRUCTION` exception will be generated if this is attempted to be enabled.
- At each level, there is the option to specify if the page table is the lowest level. This allows for larger pages to save space in the page tables.

### Enabling paging

- Bit 1 of CR0 is set to 1 to enable paging
- Bit 2-4 of CR0 is set to the desired paging mode. This must be set before or at the same time as enabling paging.
- CR3 must be set to the page aligned physical address of the highest page table level

### Page table entries

- 64-bit entries
- Follows the following format:

| Bit      | Name     | Description                                                                                                 |
|----------|----------|-------------------------------------------------------------------------------------------------------------|
| 0        | Present  | Is this entry present                                                                                       |
| 1        | Read     | Is this entry readable                                                                                      |
| 2        | Write    | Is this entry writable                                                                                      |
| 3        | Execute  | Is this entry executable                                                                                    |
| 4        | User     | Is this entry accessible in user mode                                                                       |
| 5        | Lowest   | Is this the lowest level of the page table                                                                  |
| 6-(PS-1) | Reserved | Must be 0, PS is Page Size                                                                                  |
| PS-63    | Address  | Physical address of the next page table or the physical address of the page shifted to the right by PS bits |

## Instructions

### Size

- Described as `SIZE` in the following instructions.
- Can be one of the following:

| Name  | Description    |
|-------|----------------|
| BYTE  | 8-bit integer  |
| WORD  | 16-bit integer |
| DWORD | 32-bit integer |
| QWORD | 64-bit integer |

### Flags

- The flags (in the `STS` register) are set by the ALU instructions depending on the result of the operation.

### Stack

#### push

- `push SIZE src` pushes the value of SIZE at `src` to the stack, incrementing `scp` by 8.
- `src` can be a register, memory address (simple or complex), or an immediate.

#### pop

- `pop SIZE dst` pops the value of SIZE from the stack to `dst`, decrementing `scp` by 8.
- `dst` can be a register or memory address (simple or complex).

#### pusha

- `pusha` pushes all general purpose registers to the stack starting with r15, and finishing with r0.

#### popa

- `popa` pops all general purpose registers to the stack starting with r0, and finishing with r15.

### ALU

#### add

- `add SIZE dst, src` adds the value of `src` to the value of `dst` and stores the result in `dst`.
- `dst` can be a register or memory address (simple or complex).
- `src` can be a register, memory address (simple or complex), or an immediate.

#### sub

- `sub SIZE dst, src` subtracts the value of `src` from the value of `dst` and stores the result in `dst`.
- `dst` can be a register or memory address (simple or complex).
- `src` can be a register, memory address (simple or complex), or an immediate.

#### mul

- `mul SIZE dst2, dst1, src` multiplies the value of `src` by the value of `dst1` and stores the result in `dst2:dst1`.
- In other words, `dst2:dst1 = dst1 * src`.
- `dst2` and `dst1` can be a register or memory address (simple or complex).
- `src` can be a register, memory address (simple or complex), or an immediate.
- This is an unsigned operation.

#### div

- `div SIZE dst2, dst1, src` divides the value of `dst2:dst1` by the value of `src`.
- `src1` and `src2` can be a register or memory address (simple or complex), or an immediate.
- The quotient is stored in `dst1`, and the remainder is stored in `dst2`.
- In other words, `dst1 = dst2:dst1 / src`, and `dst2 = dst2:dst1 % src`.
- If the value of `src` is 0, a divide by zero exception is thrown. If the result is too big to store in `dst1`, an integer overflow exception is thrown.
- This is an unsigned operation.

#### smul

- `smul SIZE dst2, dst1, src` multiplies the value of `src` by the value of `dst1` and stores the result in `dst2:dst1`.
- In other words, `dst2:dst1 = dst1 * src`.
- `dst2` and `dst1` can be a register or memory address (simple or complex).
- `src` can be a register, memory address (simple or complex), or an immediate.
- This is a signed operation.

#### sdiv

- `sdiv SIZE dst2, dst1, src` divides the value of `dst2:dst1` by the value of `src`.
- `src1` and `src2` can be a register or memory address (simple or complex), or an immediate.
- The quotient is stored in `dst1`, and the remainder is stored in `dst2`.
- In other words, `dst1 = dst2:dst1 / src`, and `dst2 = dst2:dst1 % src`.
- If the value of `src` is 0, a divide by zero exception is thrown. If the result is too big to store in `dst1`, an integer overflow exception is thrown.
- This is a signed operation.

#### or

- `or SIZE dst, src` performs a bitwise OR operation on the value of `dst` and the value of `src` and stores the result in `dst`.
- `dst` can be a register or memory address (simple or complex).
- `src` can be a register, memory address (simple or complex), or an immediate.

#### nor

- `nor SIZE dst, src` performs a bitwise NOR operation on the value of `dst` and the value of `src` and stores the result in `dst`.
- `dst` can be a register or memory address (simple or complex).
- `src` can be a register, memory address (simple or complex), or an immediate.

#### xor

- `xor SIZE dst, src` performs a bitwise XOR operation on the value of `dst` and the value of `src` and stores the result in `dst`.
- `dst` can be a register or memory address (simple or complex).
- `src` can be a register, memory address (simple or complex), or an immediate.

#### xnor

- `xnor SIZE dst, src` performs a bitwise XNOR operation on the value of `dst` and the value of `src` and stores the result in `dst`.
- `dst` can be a register or memory address (simple or complex).
- `src` can be a register, memory address (simple or complex), or an immediate.

#### and

- `and SIZE dst, src` performs a bitwise AND operation on the value of `dst` and the value of `src` and stores the result in `dst`.
- `dst` can be a register or memory address (simple or complex).
- `src` can be a register, memory address (simple or complex), or an immediate.

#### nand

- `nand SIZE dst, src` performs a bitwise NAND operation on the value of `dst` and the value of `src` and stores the result in `dst`.
- `dst` can be a register or memory address (simple or complex).
- `src` can be a register, memory address (simple or complex), or an immediate.

#### not

- `not SIZE dst` performs a bitwise NOT operation on the value of `dst` and stores the result in `dst`.
- `dst` can be a register or memory address (simple or complex).

#### shl

- `shl SIZE dst, src` shifts the value of `dst` to the left by the value of `src` and stores the result in `dst`.
- `dst` can be a register or memory address (simple or complex).
- `src` can be a register, memory address (simple or complex), or an immediate.

#### shr

- `shr SIZE dst, src` shifts the value of `dst` to the right by the value of `src` and stores the result in `dst`.
- `dst` can be a register or memory address (simple or complex).
- `src` can be a register, memory address (simple or complex), or an immediate.

#### cmp

- `cmp SIZE src1, src2` compares the value of `src2` with the value of `src1`.
- `src2` can be a register or memory address (simple or complex).
- `src1` can be a register, memory address (simple or complex), or an immediate.
- It is equivalent to `sub SIZE src2, src1` without storing the result.

#### inc

- `inc SIZE dst` increments the value of `dst` by 1 and stores the result in `dst`.
- `dst` can be a register or memory address (simple or complex).
- Equivalent to `add SIZE dst, 1`.

#### dec

- `dec SIZE dst` decrements the value of `dst` by 1 and stores the result in `dst`.
- `dst` can be a register or memory address (simple or complex).
- Equivalent to `sub SIZE dst, 1`.

### Program flow

#### ret

- `ret` returns from a function.
- It pops the return address from the stack and jumps to it.

#### call

- `call SIZE address` calls the function at `address`.
- `address` can be a register, memory address (simple or complex), or an immediate.
- It pushes the return address to the stack and jumps to the function.

#### jmp

- `jmp SIZE address` jumps to the function at `address`.
- `address` can be a register, memory address (simple or complex), or an immediate.

#### jc

- `jc SIZE address` jumps to the function at `address` if the carry flag is set.
- `address` can be a register, memory address (simple or complex), or an immediate.
- Equivalent to a `nop` if the carry flag is not set.

#### jnc

- `jnc SIZE address` jumps to the function at `address` if the carry flag is not set.
- `address` can be a register, memory address (simple or complex), or an immediate.
- Equivalent to a `nop` if the carry flag is set.

#### jz

- `jz SIZE address` jumps to the function at `address` if the zero flag is set.
- `address` can be a register, memory address (simple or complex), or an immediate.
- Equivalent to a `nop` if the zero flag is not set.

#### jnz

- `jnz SIZE address` jumps to the function at `address` if the zero flag is not set.
- `address` can be a register, memory address (simple or complex), or an immediate.
- Equivalent to a `nop` if the zero flag is set.

#### jl

- `jl SIZE address` jumps to the function at `address` if the sign flag is not equal to the overflow flag.
- `address` can be a register, memory address (simple or complex), or an immediate.
- Equivalent to a `nop` if the condition is not met.

#### jle

- `jle SIZE address` jumps to the function at `address` if the sign flag is not equal to the overflow flag or the zero flag is set.
- `address` can be a register, memory address (simple or complex), or an immediate.
- Equivalent to a `nop` if the condition is not met.

#### jnl

- `jnl SIZE address` jumps to the function at `address` if the sign flag is equal to the overflow flag.
- `address` can be a register, memory address (simple or complex), or an immediate.
- Equivalent to a `nop` if the condition is not met.

#### jnle

- `jnle SIZE address` jumps to the function at `address` if the sign flag is equal to the overflow flag and the zero flag is not set.
- `address` can be a register, memory address (simple or complex), or an immediate.
- Equivalent to a `nop` if the condition is not met.

#### jg

- `jg SIZE address` jumps to the function at `address` if the sign flag is equal to the overflow flag and the zero flag is not set.
- `address` can be a register, memory address (simple or complex), or an immediate.
- Equivalent to a `nop` if the condition is not met.
- This instruction is an alias for `jnle`.

#### jge

- `jge SIZE address` jumps to the function at `address` if the sign flag is equal to the overflow flag.
- `address` can be a register, memory address (simple or complex), or an immediate.
- Equivalent to a `nop` if the condition is not met.
- This instruction is an alias for `jnl`.

#### jng

- `jng SIZE address` jumps to the function at `address` if the sign flag is not equal to the overflow flag or the zero flag is set.
- `address` can be a register, memory address (simple or complex), or an immediate.
- Equivalent to a `nop` if the condition is not met.
- This instruction is an alias for `jle`.

#### jnge

- `jnge SIZE address` jumps to the function at `address` if the sign flag is not equal to the overflow flag.
- `address` can be a register, memory address (simple or complex), or an immediate.
- Equivalent to a `nop` if the condition is not met.
- This instruction is an alias for `jl`.

### Interrupts

#### int

- `int number` raises an interrupt with the number `number`.
- `number` can be a register, memory address (simple or complex), or an immediate.
- More info can be found at [Interrupts info](#interrupts-info).

#### lidt

- `lidt address` loads the Interrupt Descriptor Table (IDT) from `address`.
- `address` can be a register, memory address (simple or complex), or an immediate.
- More info can be found at [Interrupts info](#interrupts-info).

#### iret

- `iret` returns from an interrupt.
- More info can be found at [Interrupts info](#interrupts-info).

### Other

#### mov

- `mov SIZE dst, src` moves the value of `src` to `dst`.
- `dst` can be a register or memory address (simple or complex).
- `src` can be a register, memory address (simple or complex), or an immediate.

#### nop

- `nop` does nothing for that instruction cycle.
- Any arguments are ignored.

#### hlt

- `hlt` freezes the CPU in its current state.

### Protected mode Instructions

#### syscall

- `syscall` enters supervisor mode.
- More info can be found at [Switching privilege levels](#switching-privilege-levels).

#### sysret

- `sysret` returns to user mode.
- This instruction is not intended to enter user mode the first time.
- More info can be found at [Switching privilege levels](#switching-privilege-levels).

#### enteruser

- `enteruser SIZE address` enters user mode at `address`.
- `address` can be a register, memory address (simple or complex), or an immediate.
- This instruction is intended to be for entering user mode the first time.
- More info can be found at [Switching privilege levels](#switching-privilege-levels).

## Interrupts info

Has a register called IDTR which contains the address of a table called the Interrupt Descriptor Table (IDT) which contains the addresses of interrupt handlers. It is 256 entries long. The format of an entry is as follows:

| Bit  | Name    | Description                  |
|------|---------|------------------------------|
| 0    | Present | Is this entry present        |
| 1-7  | Flags   | Reserved                     |
| 8-71 | Address | Address of interrupt handler |

### Interrupt calling

1. I0, flags register and an error code are sent as arguments
2. CPU calls address in relevant IDT entry using `call` instruction
3. relevant value in flags register is set so instructions know the CPU is in interrupt mode.

### Interrupt returning

1. remove arguments from stack
2. call `ret` instruction

### Protected mode interrupts

- Bit 1 of the IDT entry is set to 1 if the interrupt is able to be raised from user mode.
- If an interrupt is raised from user mode that isn't configured to be able to, a `USER_MODE_VIOLATION` exception is thrown.
- Interrupts are **always** handled in kernel mode.

## Assembly syntax

### Labels

- Labels are defined by a string followed by a colon
- Example:

```asm
foo:
    nop
```

### Sub-labels

- Sub-labels are defined by a period followed by a string followed by a colon
- Sub-labels are only accessible within the scope of the label they are defined in
- Example:

```asm
foo:
    nop
.bar:
    nop
```

### Comments

- Single line comments are defined by a semicolon followed by a string
- Multi-line comments are defined by a `/*` followed by a string and ending with a `*/`

### Includes

- `%include "path/to/file.asm"` to include another assembly file.
- They are relative to the file being assembled.

### Define macros

- `%define name value` to define a macro.
- Does not support arguments.
- The value goes until the end of the line.
- Multi-line macros are unsupported.

### Directives

- `db` to define a byte
- `dw` to define a word (2 bytes)
- `dd` to define a double-word (4 bytes)
- `dq` to define a quad-word (8 bytes)
- `org` to set the origin of the program counter. Can only be set once. Regardless of where the in the program it is specified in, it will be set to the first instruction.
- `ascii` to define a string
- `asciiz` to define a null-terminated string
- `align` to align the current position in the program to a multiple of a number. The number must be a power of 2. It fills the space with `nop` instructions.
- Example usage:

```x86asm
org 0xF0000000
db 0x00
align 8
ascii "Hello, world!"
asciiz "Hello, world!"
```

### String literals

- Strings are defined by double quotes.
- They are only valid in the `ascii` and `asciiz` directives.
- They support the following escape sequences:

| Sequence | Description                            |
|----------|----------------------------------------|
| `\"`     | `"`                                    |
| `\'`     | `'`                                    |
| `\\`     | `\`                                    |
| `\n`     | Newline                                |
| `\r`     | Carriage return                        |
| `\t`     | Tab                                    |
| `\0`     | Null byte                              |
| `\xHH`   | Hexadecimal byte (upper or lower case) |

### Character literals

- Characters are defined by single quotes.
- They are converted to a number, so can be used anywhere.
- They support the same escape sequences as string literals, so see [the string literals section](#string-literals) for more info.

### Memory addresses

- 2 forms
- form 1: `[literal]` where literal is a 64-bit integer (also known as `MEMORY`).
- form 2: `[base*index+offset]`, where any of the 3 can be a register or an immediate of any size (also known as `COMPLEX`).
- In form 2, index or offset can be excluded. If the offset is a register, it can be positive or negative. Not all combinations are valid. These are all valid combinations:
    - `[reg]`
    - `[reg+reg]`
    - `[reg+imm]`
    - `[imm+reg]`
    - `[imm-reg]`
    - `[imm+imm]`
    - `[reg*reg]`
    - `[reg*imm]`
    - `[reg*reg+reg]`
    - `[reg*reg-reg]`
    - `[reg*reg+imm]`
    - `[reg*imm+reg]`
    - `[reg*imm-reg]`
    - `[reg*imm+imm]`

### Constant folding

- Constant folding is supported in the assembler to an extent.
- Unary `+` and `-` operations are not supported. Otherwise, most other operators are supported.
- Operations including labels or sub-labels are not supported, but operations including macros are supported.
- Expressions are evaluated left to right, with standard operator precedence, similar to languages like C.
- Expressions must be enclosed in parentheses.

## Devices

### Memory mapped I/O bus device

- 1 64-bit Memory mapped I/O bus device in the last 256 bytes of the BIOS address space.
- All accesses are 8-byte aligned regardless of the size of the access, allowing for up to 32 ports.
- This is the current port layout:

| Offset | Size in QWORDS | Name     | Description      |
|--------|----------------|----------|------------------|
| 0      | 1              | COMMAND  | Command register |
| 1      | 1              | STATUS   | Status register  |
| 2      | 4              | DATA     | Data register    |
| 6      | 26             | RESERVED | Reserved         |

- The command register is used to send commands to the device.
- The status register is used to get the status of the device. Bit 0 is set to 1 when the current command is complete, and bit 1 is set to 1 when there is an error.
- The data register is used to send data to the device or get data from the device.

#### Commands

| Command | Description           |
|---------|-----------------------|
| 0       | Get bus info          |
| 1       | Get device info       |
| 2       | Set device info       |
| 3       | Get Interrupt mapping |
| 4       | Set Interrupt mapping |

##### Get bus info

- 0 arguments
- Data register contains the following:

| Offset | Width | Name    | Description                  |
|--------|-------|---------|------------------------------|
| 0      | 8     | DEVICES | Number of devices on the bus |

##### Get device info

- 1 argument: the index of the device. Not to be confused with the device ID.
- Data register contains the following:

| Offset | Width | Name          | Description                         |
|--------|-------|---------------|-------------------------------------|
| 0      | 8     | ID            | Device ID                           |
| 8      | 8     | Base Address  | Base physical address of the device |
| 16     | 8     | Size          | Size of the device in bytes         |
| 24     | 8     | INT Count     | Number of interrupts the device has |

##### Set device info

- Arguments are as follows:

| Offset | Width | Name         | Description                         |
|--------|-------|--------------|-------------------------------------|
| 0      | 8     | ID           | Device ID                           |
| 8      | 8     | Base Address | Base physical address of the device |

##### Get Interrupt mapping

- Arguments are as follows:

| Offset | Width | Name | Description      |
|--------|-------|------|------------------|
| 0      | 8     | ID   | Device ID        |
| 8      | 8     | INT  | Interrupt index  |

- Data register contains the software interrupt number (SINT) that the hardware interrupt is mapped to.

##### Set Interrupt mapping

- Arguments are as follows:

| Offset | Width | Name | Description               |
|--------|-------|------|---------------------------|
| 0      | 8     | ID   | Device ID                 |
| 8      | 8     | INT  | Interrupt index           |
| 16     | 1     | SINT | Software interrupt number |

### Console device

- There is a console I/O device taking up 16 ports by default
- A raw read/write of a byte will read/write to the console via stdin/stderr respectively
- Any other sized read/write will be ignored

### Video device

- There is a video I/O device taking up 3 ports by default

#### Video device registers

| Port | Name    | Description      |
|------|---------|------------------|
| 0    | COMMAND | Command register |
| 1    | DATA    | Data register    |
| 2    | STATUS  | Status register  |

#### Video device commands

| Command | Description     |
|---------|-----------------|
| 0       | Initialise      |
| 1       | Get screen info |
| 2       | Get mode        |
| 3       | Set mode        |

##### Initialise

- 0 arguments
- STATUS register is set to 0 if there is no error.

##### Get screen info

- 1 argument: address to store the screen info.
- Returns a non-zero value in the STATUS register if there is an error. The buffer is as follows:

| Offset | Width | Name    | Description                           |
|--------|-------|---------|---------------------------------------|
| 0      | 4     | WIDTH   | Native width of the screen in pixels  |
| 4      | 4     | HEIGHT  | Native height of the screen in pixels |
| 8      | 2     | HZ      | Refresh rate of the screen in Hz      |
| 10     | 2     | BPP     | Maximum bits per pixel                |
| 12     | 2     | MODES   | Number of modes available             |
| 14     | 2     | CURMODE | Current mode                          |

##### Get mode

- 1 argument: address of the input data. The buffer is as follows:
  
| Offset | Width | Name     | Description                  |
|--------|-------|----------|------------------------------|
| 0      | 8     | ADDRESS  | Address to store output data |
| 8      | 2     | MODE     | Mode to get                  |
| 10     | 6     | RESERVED | Reserved                     |

- Returns a non-zero value in the STATUS register if there is an error. The buffer is as follows:

| Offset | Width | Name   | Description                      |
|--------|-------|--------|----------------------------------|
| 0      | 4     | WIDTH  | Width of the screen in pixels    |
| 4      | 4     | HEIGHT | Height of the screen in pixels   |
| 8      | 2     | BPP    | Bits per pixel                   |
| 10     | 4     | PITCH  | Pitch of the screen in bytes     |
| 14     | 2     | HZ     | Refresh rate of the screen in Hz |

##### Set mode

- 1 argument: address of the mode info.
- Returns a non-zero value in the STATUS register if there is an error. The buffer is as follows:

| Offset | Width | Name     | Description                |
|--------|-------|----------|----------------------------|
| 0      | 8     | ADDRESS  | Address of the framebuffer |
| 8      | 2     | MODE     | Mode to set                |
| 10     | 6     | RESERVED | Reserved                   |

### Storage device

- There is a storage I/O device taking up 3 ports by default
- The storage device is a block device. All reads and writes are in 512-byte blocks (or sectors).
- Status register is read-only.
- It can only handle 1 command at a time.

#### Storage device registers

| Port | Name    | Description      |
|------|---------|------------------|
| 0    | COMMAND | Command register |
| 1    | STATUS  | Status register  |
| 2    | DATA    | Data register    |

##### Status register

| Bit  | Name     | Description          |
|------|----------|----------------------|
| 0    | EN       | Device enabled       |
| 1    | ERR      | Error flag           |
| 2    | RDY      | Ready flag           |
| 3    | TRN      | Transfer in progress |
| 3    | INTE     | Interrupts enabled   |
| 4    | INTP     | Interrupt pending    |
| 5-63 | RESERVED | Reserved             |

#### Storage device commands

| Command | Description     |
|---------|-----------------|
| 0       | Configure       |
| 1       | Get device info |
| 2       | Read            |
| 3       | Write           |

##### Configure

- 1 argument, QWORD of the config flags as follows:

| Bit  | Name     | Description        |
|------|----------|--------------------|
| 0    | EN       | Enable device      |
| 1    | INTE     | Enable interrupts  |
| 2-63 | RESERVED | Reserved           |

##### Get device info (storage)

- 1 argument - address to store the device info.
- The following data is stored at the given address:

| Offset | Width | Name    | Description                                         |
|--------|-------|---------|-----------------------------------------------------|
| 0      | 8     | SIZE    | Raw size of the device, aligned to 512-byte blocks  |
| 8      | 8     | BLOCKS  | Number of 512-byte blocks the device has            |

##### Read

- Data register contains address to store the following:

| Offset | Width | Name    | Description                        |
|--------|-------|---------|------------------------------------|
| 0      | 8     | LBA     | Logical block address to read from |
| 8      | 8     | COUNT   | Number of blocks to read           |
| 16     | 8     | PRLS    | Physical region list start address |
| 24     | 8     | PRLNC   | Physical region list node count    |
| 32     | 8     | FLAGS   | Flags                              |

- The flags are as follows:

| Bit  | Name | Description                   |
|------|------|-------------------------------|
| 0    | INT  | Raise interrupt on completion |
| 1-63 | RSVD | Reserved                      |

- STATUS.TRN is set then the command has been validated and the actual transfer is in progress.
- STATUS.RDY is set then the command is completely done and the transfer is complete.
- If STATUS.TRN does not get set, then the command was invalid.

##### Write

- Data register contains address to store the following:

| Offset | Width | Name    | Description                        |
|--------|-------|---------|------------------------------------|
| 0      | 8     | LBA     | Logical block address to write to  |
| 8      | 8     | COUNT   | Number of blocks to write          |
| 16     | 8     | PRLS    | Physical region list start address |
| 24     | 8     | PRLNC   | Physical region list node count    |
| 32     | 8     | FLAGS   | Flags                              |

- The flags are as follows:

| Bit  | Name | Description                   |
|------|------|-------------------------------|
| 0    | INT  | Raise interrupt on completion |
| 1-63 | RSVD | Reserved                      |

- STATUS.TRN is set then the command has been validated and the actual transfer is in progress.
- STATUS.RDY is set then the command is completely done and the transfer is complete.
- If STATUS.TRN does not get set, then the command was invalid.

#### Physical region list

- A list is made of multiple nodes of arbitrary size.
- Each node has a total of 2 QWORDs of metadata. One at the start and one at the end.
- Each item in a node is 2 QWORDS. An item has the following format:

| Offset | Name  | Description                        |
|--------|-------|------------------------------------|
| 0      | PADDR | Physical address of the data block |
| 8      | COUNT | Number of 512-byte blocks          |

- Each node then has the following format:

| Offset        | Name  | Description                       |
|---------------|-------|-----------------------------------|
| 0             | COUNT | Number of items in the node       |
| 8-(COUNT*8-1) | ITEMS | List of items in the node         |
| COUNT*8       | NEXT  | Physical address of the next node |

- The next node is 0 if there is no next node.

## The BIOS

- The BIOS has a dedicated memory region from 0xF000'0000 to 0xFFFF'FEFF.
- The IP register is set to 0xF000'0000 on boot.

## Memory layout

- This is the current memory layout of the system:

| Start                 | End                   | Description      |
|-----------------------|-----------------------|------------------|
| 0x0000'0000'0000'0000 | 0x0000'0000'EFFF'FFFF | Low User memory  |
| 0x0000'0000'F000'0000 | 0x0000'0000'FFFF'FEFF | BIOS             |
| 0x0000'0000'FFFF'FF00 | 0x0000'0000'FFFF'FFFF | I/O bus          |
| 0x0000'0001'0000'0000 | 0xFFFF'FFFF'FFFF'FFFF | High User memory |

- The BIOS region should never be accessed outside the BIOS.
