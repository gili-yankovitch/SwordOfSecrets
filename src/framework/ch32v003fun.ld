ENTRY( InterruptVector )

MEMORY
{
#if TARGET_MCU_LD == 0
	FLASH_ISRVEC (rx) : ORIGIN = 0x00000000, LENGTH = 192
	FLASH_TOP (rx) : ORIGIN = 0x000000c0 LENGTH = 3904 // 4K - 192
	FLASH (rx) : ORIGIN = 0x00001000, LENGTH = 12K
	RAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 2K
#elif TARGET_MCU_LD == 1
	#if MCU_PACKAGE == 1
		FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 64K
		RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
	#elif MCU_PACKAGE == 2
		FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 32K
		RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 10K
	#else
		#error "Unknown MCU package"
	#endif
#elif TARGET_MCU_LD == 2
	#if MCU_PACKAGE == 1
		FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 64K
		RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
	#elif MCU_PACKAGE == 2
		FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 32K
		RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 10K
	#elif MCU_PACKAGE == 3
		FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 128K
		RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 64K
	#else
		#error "Unknown MCU package"
	#endif
#elif TARGET_MCU_LD == 3
	#if MCU_PACKAGE == 1
		FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K
		RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 64K
	#elif MCU_PACKAGE == 2
		FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 128K
		RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 32K
	#else
		#error "Unknown MCU package"
	#endif
#else
	#error "Unknown MCU target"
#endif
}

SECTIONS
{
    .isrvec :
    {
        KEEP(*(.isrvec))
        KEEP(*(.isrvec.*))
    } >FLASH_ISRVEC AT>FLASH_ISRVEC

    .topflash :
    {
      KEEP(*(.topflash.text))         /* explicitly marked */
      KEEP(*(.topflash.rodata))
      KEEP(*(.topflash.*))       /* wildcard for sub-sections */
    } >FLASH_TOP AT>FLASH_TOP

    .main :
    {
        KEEP(*(.main))
        KEEP(*(.main.*))
    } >FLASH AT>FLASH
    
    .init :
    { 
      // _sinit = .;
      . = ALIGN(4);
      KEEP(*(SORT_NONE(.init)))
      . = ALIGN(4);
      _einit = .;
    } >FLASH AT>FLASH

    .text :
    {
      . = ALIGN(4);
      *(.text)
      *(.text.*)
      *(.rodata)
      *(.rodata*)
      *(.gnu.linkonce.t.*)
      . = ALIGN(4);
    } >FLASH AT>FLASH 

    // .topflash :
    // {
    //   KEEP(*(.topflash.text))         /* explicitly marked */
    //   KEEP(*(.topflash.rodata))
    //   KEEP(*(.topflash.*))       /* wildcard for sub-sections */
    // } >FLASH_TOP AT>FLASH_TOP


    .fini :
    {
      KEEP(*(SORT_NONE(.fini)))
      . = ALIGN(4);
    } >FLASH AT>FLASH

    PROVIDE( _etext = . );
    PROVIDE( _eitcm = . );  

    .preinit_array :
    {
      PROVIDE_HIDDEN (__preinit_array_start = .);
      KEEP (*(.preinit_array))
      PROVIDE_HIDDEN (__preinit_array_end = .);
    } >FLASH AT>FLASH 
  
    .init_array :
    {
      PROVIDE_HIDDEN (__init_array_start = .);
      KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
      KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
      PROVIDE_HIDDEN (__init_array_end = .);
    } >FLASH AT>FLASH 
  
    .fini_array :
    {
      PROVIDE_HIDDEN (__fini_array_start = .);
      KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
      KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
      PROVIDE_HIDDEN (__fini_array_end = .);
    } >FLASH AT>FLASH 
  
    .ctors :
    {
      /* gcc uses crtbegin.o to find the start of
         the constructors, so we make sure it is
         first.  Because this is a wildcard, it
         doesn't matter if the user does not
         actually link against crtbegin.o; the
         linker won't look for a file to match a
         wildcard.  The wildcard also means that it
         doesn't matter which directory crtbegin.o
         is in.  */
      KEEP (*crtbegin.o(.ctors))
      KEEP (*crtbegin?.o(.ctors))
      /* We don't want to include the .ctor section from
         the crtend.o file until after the sorted ctors.
         The .ctor section from the crtend file contains the
         end of ctors marker and it must be last */
      KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
      KEEP (*(SORT(.ctors.*)))
      KEEP (*(.ctors))
    } >FLASH AT>FLASH 
  
    .dtors :
    {
      KEEP (*crtbegin.o(.dtors))
      KEEP (*crtbegin?.o(.dtors))
      KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
      KEEP (*(SORT(.dtors.*)))
      KEEP (*(.dtors))
    } >FLASH AT>FLASH 

    .dalign :
    {
      . = ALIGN(4);
      PROVIDE(_data_vma = .);
    } >RAM AT>FLASH  

    .dlalign :
    {
      . = ALIGN(4); 
      PROVIDE(_data_lma = .);
    } >FLASH AT>FLASH

    .bootloader.data : 
    {
      . = ALIGN(4);
      *(.bootloader.data)
    } >RAM AT>FLASH

    .data :
    {
      . = ALIGN(4);
      *(.gnu.linkonce.r.*)
      *(.data .data.*)
      *(.gnu.linkonce.d.*)
      . = ALIGN(8);
      PROVIDE( __global_pointer$ = . + 0x800 );
      *(.sdata .sdata.*)
      *(.sdata2*)
      *(.gnu.linkonce.s.*)
      . = ALIGN(8);
      *(.srodata.cst16)
      *(.srodata.cst8)
      *(.srodata.cst4)
      *(.srodata.cst2)
      *(.srodata .srodata.*)
      . = ALIGN(4);
      PROVIDE( _edata = .);
    } >RAM AT>FLASH

    .bss :
    {
      . = ALIGN(4);
      PROVIDE( _sbss = .);
      *(.sbss*)
      *(.gnu.linkonce.sb.*)
      *(.bss*)
      *(.gnu.linkonce.b.*)    
      *(COMMON*)
      . = ALIGN(4);
      PROVIDE( _ebss = .);
    } >RAM AT>FLASH

    PROVIDE( _end = _ebss);
	PROVIDE( end = . );

	PROVIDE( _eusrstack = ORIGIN(RAM) + LENGTH(RAM));	
}



