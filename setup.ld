/*
 * setup.ld
 *
 * Linker script for the i386 setup code
 */
OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

phys = 0x00100000;

SECTIONS
{
        . = 0x100000;
        .text :
        {
            code = .;
            *(.multiboot)
            *(.text)            
            *(.rodata)
            . = ALIGN(4096); 
        }
        .data : AT(phys + (data - code))
        { 
            data = .;
            *(.data)

            . = ALIGN(4096); 
        }
        .bss : AT(phys + bss - code)
        { 
            bss = .;
            *(.bss)
            . = ALIGN(4096); 
        }
        .stab :
        {
                *(.stab)
                . = ALIGN(4096);
        }
        .stabstr :
        {
                *(.stabstr)
                . = ALIGN(4096);
        }
        
        /DISCARD/ : { *(.comment) *(.eh_frame) }
}
