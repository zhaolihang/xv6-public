//
// assembler macros to create x86 segments
//

#define ASM_SEG_NULL \
    .word 0, 0;      \
    .byte 0, 0, 0, 0

// The 0xC0 means the limit is in 4096-byte units
// and (for executable segments) 32-bit mode.
#define ASM_MAKE_SEG(type, base, lim)                 \
    .word(((lim) >> 12) & 0xffff), (( base )&0xffff); \
    .byte(((base) >> 16) & 0xff), (0x90 | (type)), (0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)

#define APP_SEG_TYPE_X 0x8    // Executable segment
#define APP_SEG_TYPE_E 0x4    // Expand down (non-executable segments)
#define APP_SEG_TYPE_C 0x4    // Conforming code segment (executable only)
#define APP_SEG_TYPE_W 0x2    // Writeable (non-executable segments)
#define APP_SEG_TYPE_R 0x2    // Readable (executable segments)
#define APP_SEG_TYPE_A 0x1    // Accessed
