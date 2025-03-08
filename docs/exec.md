# Custom exec format

## Header

| Start | Size | Name | Description |
|-------|------|------|-------------|
| 0x00 | 8 | Magic | A magic number to identify the format |
| 0x08 | 8 | Ver | The version of the format |
| 0x10 | 1 | ABI | The ABI version of the format |
| 0x11 | 1 | Arch | The architecture of the format |
| 0x12 | 1 | Type | The type of the format |
| 0x13 | 1 | Flags | Various architecture specific flags |
| 0x14 | 4 | Align0 | Padding |
| 0x18 | 8 | FSecTS | The offset of the file section table |
| 0x20 | 8 | FSecNum | The number of file sections |

## File section table

The file section table has entries describing different sections within the file. The header of each file section table entry is as follows:

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0x00 | 8 | Offset | The offset of the section in the file |
| 0x08 | 8 | Size | The size of the section |
| 0x10 | 2 | Type | The type of the section |
| 0x12 | 1 | Flags | Various flags for the section |
| 0x13 | 5 | Align0 | Padding |

The type is expected to be one of the following:

- 0x00 - LOAD information
- 0x01 - Dynamic linking information
- 0x02 - Symbol table information
- 0x03 - Output segment information
- 0x04 - Debug information
- 0x05 - File storage table information
- 0x06 - General information

### LOAD information header

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0x18 | 8 | Count | The number of LOAD entries |

### Output segment information header

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0x18 | 8 | Count | The number of output segments |

### General information header

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0x18 | 8 | Entry | The entry point of the program |

## LOAD table

- Each entry within the LOAD table has a fixed size.
- Each entry is as follows:

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0x00 | 8 | FileOffset | The offset of the section in the file |
| 0x08 | 8 | FileSize | The size of the section in the file |
| 0x10 | 8 | MemOffset | The offset of the section in memory |
| 0x18 | 8 | MemSize | The size of the section in memory |
| 0x20 | 1 | Flags | Various flags for the section |
| 0x21 | 7 | Align0 | Padding |

The flags are in the following format:

| Bit | Name | Description |
|-----|------|-------------|
| 0 | Exec | The section is executable |
| 1 | Write | The section is writable |
| 2 | Read | The section is readable |
| 3-7 | Align0 | Padding |

## Output segment information

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0x00 | 8 | Offset | The offset of the segment in memory |
| 0x08 | 8 | Size | The size of the segment in memory |
| 0x10 | 32 | Name | The name of the segment |

- The name is expected to be a null-terminated string. Any remaining space is padded with null bytes.
