/* Simple linker script for the JOS kernel.
   See the GNU ld 'info' manual ("info ld") to learn the syntax. */

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(entry)

SECTIONS
{
	/* Link the kernel at this address: "." means the current address */
	/* Must be equal to VA_KERNAL_LINKED_BASE */
	. = 0x80100000;
	
	/*see http://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_mono/ld.html#IDX305 keyworld [AT ( ldadr )] */
	/* 代码段起始位置load to 物理内存0x100000地址处 */
	.text : AT(0x100000) {
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}

	PROVIDE(text_end = .);	/* Define the 'text_end' symbol to this value */

	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	/* Include debugging information in kernel memory */
	.stab : {
		PROVIDE(__STAB_BEGIN__ = .);
		*(.stab);
		PROVIDE(__STAB_END__ = .);
		BYTE(0)		/* Force the linker to allocate space
				   for this section */
	}

	.stabstr : {
		PROVIDE(__STABSTR_BEGIN__ = .);
		*(.stabstr);
		PROVIDE(__STABSTR_END__ = .);
		BYTE(0)		/* Force the linker to allocate space
				   for this section */
	}

	/* Adjust the address for the data segment to the next page */
	. = ALIGN(0x1000);

	/* Conventionally, Unix linkers provide pseudo-symbols
	 * text_end, data_end, and kernel_end, at the kernel_end of the text, data, and bss.
	 * For the kernel mapping, we need the address at the beginning
	 * of the data section, but that's not one of the conventional
	 * symbols, because the convention started before there was a
	 * read-only rodata section between text and data. */
	PROVIDE(data_start = .);

	/* The data segment */
	.data : {
		*(.data)
	}

	PROVIDE(data_end = .);

	.bss : {
		*(.bss)
	}

	PROVIDE(kernel_end = .);

	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
	}
}
