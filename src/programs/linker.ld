OUTPUT_FORMAT(binary)
SECTIONS
{
    . = 0;

    .header :
    {
        LONG(start);
        LONG(update);
    }

    .text :
    {
        *(.text*)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data*)
        *(COMMON)
        *(.bss*)
    }
}
