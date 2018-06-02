#ifndef __XV6_TYPES_H__
#define __XV6_TYPES_H__

#define true 1
#define false 0

#ifndef __ASSEMBLER__

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint           pgtabe_t;

typedef char bool;

#endif    // #ifndef __ASSEMBLER__

#endif    // #ifndef __XV6_TYPES_H__
