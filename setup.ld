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
        PROVIDE( _kernel_start = . );
        .init.text :
        {
            code = .;
            *(.multiboot)
            *(.init.text)
            . = ALIGN(4096); 
        }
        .init.data : AT(phys + (data - code))
        { 
            data = .;
            *(.init.data)

            . = ALIGN(4096); 
        }
        .init.bss : AT(phys + bss - code)
        { 
            bss = .;
            *(.init.bss)
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
        PROVIDE( _kernel_end = . );
        
        /DISCARD/ : { *(.comment) *(.eh_frame) }
}
