# Design

## Instructions

First 2 bytes are for the instruction stuff, and data follows it

### Layout

1. 1-byte opcode
2. minimum 1-byte argument info for each argument

### Argument info layout

- bits 0-3 is the argument type
    - 0: register
    - 1: immediate
    - 2: MEM (base only, reg)
    - 3: MEM (base only, imm)
    - 4: MEM (base + offset, reg, reg)
    - 5: MEM (base + offset, reg, imm)
    - 6: MEM (base + offset, imm, reg)
    - 7: MEM (base + offset, imm, imm)
    - 8: MEM (base * index, reg, reg)
    - 9: MEM (base * index, reg, imm)
    - a: MEM (base * index + offset, reg, reg, reg)
    - b: MEM (base * index + offset, reg, reg, imm)
    - c: MEM (base * index + offset, reg, imm, reg)
    - d: MEM (base * index + offset, reg, imm, imm)
    - e-f: reserved
- bits 4-5 is for the size of the entire operation
    - 0: BYTE (8-bit)
    - 1: WORD (16-bit)
    - 2: DWORD (32-bit)
    - 3: QWORD (64-bit)
- bits 6-7 are for the size of the first immediate if it exists
    - 0: BYTE (8-bit)
    - 1: WORD (16-bit)
    - 2: DWORD (32-bit)
    - 3: QWORD (64-bit)
- bits 8-9 are for the size of the second immediate if it exists
    - 0: BYTE (8-bit)
    - 1: WORD (16-bit)
    - 2: DWORD (32-bit)
    - 3: QWORD (64-bit)
- bits 10-11 are reserved and should be 0

bits 8-11 are only present if the argument type requires it, otherwise they are omitted.
The highest bit of the Register ID is used to encode the sign of the offset register, where positive is 1, and negative is 0.

#### Argument info layouts for multiple operands

- 1 argument, basic data:
    - bits 0-7 are all that is needed
- 1 argument, complex data:
    - bits 0-11 are used
    - bits 12-15 are padding (0)
- 2 arguments, both basic data:
    - bits 0-7 for arg 1
    - bits 8-15 for arg 2
- 2 arguments, arg 1 basic data, arg 2 complex data:
    - bits 0-7 for arg 1
    - bits 8-11 padding (0)
    - bits 12-23 for arg 2
- 2 arguments, arg 1 complex data, arg 2 basic data:
    - bits 0-11 for arg 1
    - bits 12-15 padding (0)
    - bits 16-23 for arg 2
- 2 arguments, both complex data:
    - bits 0-11 for arg 1
    - bits 12-23 for arg 2
- 3 arguments, all basic data:
    - bits 0-7 for arg 1
    - bits 8-15 for arg 2
    - bits 16-23 for arg 3
- 3 arguments, arg 1 & 2 basic data, arg 3 complex data:
    - bits 0-7 for arg 1
    - bits 8-15 for arg 2
    - bits 16-19 padding (0)
    - bits 20-31 for arg 3
- 3 arguments, arg 1 basic data, arg 2 complex data, arg 3 basic data:
    - bits 0-7 for arg 1
    - bits 8-11 padding (0)
    - bits 12-23 for arg 2
    - bits 24-31 for arg 3
- 3 arguments, arg 1 complex data, arg 2 & 3 basic data

### Register ID

- 8-bit integer that identifies a specific register
- first 4 bits are type (0 is general purpose, 1 is stack, 2 is control & flags & instruction pointers)
- last 4 bits are the number

### Register numbers

- General purpose registers: it is simply the GPR number
- Stack: scp is 0, sbp is 1, stp is 2, rest are reserved
- Control, Flags & IPs: CR0-CR7 are numbers 0-7, STS is 8, IP is 9, rest are reserved

### Argument layout

- When there are 2 arguments, one being standard and the other being complex, there are an extra 4-bits of padding after the standard argument.
- On the case where arg 1 is standard, and arg 2 is complex, the padding for arg 1 has a copy of the first 4 bits of the complex argument. Only the arg type matters, the size is ignored.

### Opcode

#### Numbering

- Bits 0-3 are the offset
- Bits 4-6 are the group (0-1 for ALU, 2 for control flow, 3 for other, 4-7 are reserved)
- Bit 7 is reserved and should always be 0

#### ALU

| Offset | Group 0  | Group 1   |
|--------|----------|-----------|
| 0      | add      | inc       |
| 1      | sub      | dec       |
| 2      | mul      | (invalid) |
| 3      | div      | (invalid) |
| 4      | smul     | (invalid) |
| 5      | sdiv     | (invalid) |
| 6      | or       | (invalid) |
| 7      | nor      | (invalid) |
| 8      | xor      | (invalid) |
| 9      | xnor     | (invalid) |
| a      | and      | (invalid) |
| b      | nand     | (invalid) |
| c      | not      | (invalid) |
| d      | shl      | (invalid) |
| e      | shr      | (invalid) |
| f      | cmp      | (invalid) |



#### Control flow

| Name      | offset |
|-----------|--------|
| ret       | 0      |
| call      | 1      |
| jmp       | 2      |
| jc        | 3      |
| jnc       | 4      |
| jz        | 5      |
| jnz       | 6      |
| jl        | 7      |
| jnge      | 7      |
| jle       | 8      |
| jng       | 8      |
| jnl       | 9      |
| jge       | 9      |
| jnle      | a      |
| jg        | a      |
| (invalid) | b      |
| (invalid) | c      |
| (invalid) | d      |
| (invalid) | e      |
| (invalid) | f      |

#### Other

| Name      | offset |
|-----------|--------|
| mov       | 0      |
| nop       | 1      |
| hlt       | 2      |
| push      | 3      |
| pop       | 4      |
| pusha     | 5      |
| popa      | 6      |
| int       | 7      |
| lidt      | 8      |
| iret      | 9      |
| syscall   | a      |
| sysret    | b      |
| enteruser | c      |
| (invalid) | d      |
| (invalid) | e      |
| (invalid) | f      |
