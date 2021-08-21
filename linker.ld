ENTRY(_start) 
/* Entry point defined in meminit.asm file*/
SECTIONS {

    /* Contains the  multiboot header and must be in the first 32KB of file */

    .boot :
    {
        *(.multiboot_header)
    }

   
    /*
        The init code without address relocation
    */
 
    .meminit ALIGN(4096):
    {
        *(.meminit)
    }

    /*
        From now on, the sections will have addresses starting from 0xC0100000
        and will be loadex in physical memory starting from address 0x10000 which makes it 
        trivial to convert from Virtual to Physical when Paging will be enabled
        (just remove 0xC0000000 from VA to get the PA)
    */
    . = 0xC0100000;

    /*
        We can define symbols to use in our C code, this denotes the beginning of kernel
    */

    kimage_start = .;

    /*
        Sections will be aligned at 4k (section starting address will always end in 000)
        The physical address in memory will be (whatever address the linker gave - 0xC0000000)

        This is what the final ELF file will look like.
        Notice that VMA - 0XC000000 = LMA

        Sections:
            Idx Name         Size      VMA       LMA       File off  Algn
            0 .boot         00000020  00000000  00000000  00001000  2**0
                            CONTENTS, ALLOC, LOAD, READONLY, DATA
            1 .rodata       000011a2  00000020  00000020  00001020  2**5
                            CONTENTS, ALLOC, LOAD, READONLY, DATA
            2 .eh_frame     00000aa8  000011c4  000011c4  000021c4  2**2
                            CONTENTS, ALLOC, LOAD, READONLY, DATA
            3 .meminit      0000600c  00002000  00002000  00003000  2**12
                            CONTENTS, ALLOC, LOAD, CODE
            4 .text         00003029  c0100000  00100000  0000a000  2**4
                            CONTENTS, ALLOC, LOAD, READONLY, CODE
            5 .data         000001a0  c0104000  00104000  0000e000  2**5
                            CONTENTS, ALLOC, LOAD, DATA
            6 .bss          00005d0c  c0105000  00105000  0000e1a0  2**12

                                        (...)

    */

    .text ALIGN (0x1000): AT(ADDR(.text) - 0xC0000000)
    {
        kentry_point = .;
        *(.text)
       
    }

    .files ALIGN (0x1000): AT(ADDR(.files) - 0xC0000000)
    {
        files_start = .;
        *(.files)
        files_end = .;


    }

    .data ALIGN (0x1000): AT(ADDR(.data) - 0xC0000000)
    {
        *(.data)
    }


    

    .bss ALIGN (0x1000): AT(ADDR(.bss) - 0xC0000000)
    {
        *(.bss)
    }



    /*
        Kernel size = kimag_end - kimage_start;
    */
    kimage_end = .;
} 
