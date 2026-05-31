/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
	/* It used to be universally recommended to use 1M as a start offset,
	   as it was effectively guaranteed to be available under BIOS systems.
	   However, UEFI has made things more complicated, and experimental data
	   strongly suggests that 2M is a safer place to load. In 2016, a new
	   feature was introduced to the multiboot2 spec to inform bootloaders
	   that a kernel can be loaded anywhere within a range of addresses and
	   will be able to relocate itself to run from such a loader-selected
	   address, in order to give the loader freedom in selecting a span of
	   memory which is verified to be available by the firmware, in order to
	   work around this issue. This does not use that feature, so 2M was
	   chosen as a safer option than the traditional 1M. */
	. = 1M;

	/* First put the multiboot header, as it is required to be put very early
	   in the image or the bootloader won't recognize the file format.
	   Next we'll put the .text section. */
	.prologue_text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		build/arch/x86/boot/*.o(.prologue)
		build/arch/x86/boot/*.o(.text)
	}

	/* Read-only data. */
	.prologue_rodata BLOCK(4K) : ALIGN(4K)
	{
		build/arch/x86/boot/*.o(.rodata)
	}

	/* Read-write data (initialized) */
	.prologue_data BLOCK(4K) : ALIGN(4K)
	{
		build/arch/x86/boot/*.o(.data)
	}

	/* Read-write data (uninitialized) and stack */
	.prologue_bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		build/arch/x86/boot/*.o(.bss)
	}
	__prologue_end = ALIGN(4K);

	. += 0xC0000000;

	/* Virtual memory sections */
	.text BLOCK(4K): AT(ADDR(.text) - 0xC0000000) {
        __kernel_start = .;
		*(.text)
    }

    .rodata BLOCK(4K): AT(ADDR(.rodata) - 0xC0000000) {
		*(.rodata)
    }

    .data BLOCK(4K): AT(ADDR(.data) - 0xC0000000) {
		*(.data)
    }

    .bss BLOCK(4K): AT(ADDR(.bss) - 0xC0000000) {
        *(COMMON)
		*(.bss)
    }

	/* section kernel page global */
	.kernel_page BLOCK(4K) : AT(ADDR(.bss) - 0xC0000000)
	{
		*(.kpg)
	}

	__kernel_end = ALIGN(4);
    __kernel_heap_start = ALIGN(4);

	/* The compiler may produce other sections, by default it will put them in
	   a segment with the same name. Simply add stuff here as needed. */
}