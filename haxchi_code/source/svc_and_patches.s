
#define BAT_SETUP_HOOK_ADDR         0xFFF1D624
#define BAT_SET_NOP_ADDR_1          0xFFF06B6C
#define BAT_SET_NOP_ADDR_2          0xFFF06BF8
#define BAT_SET_NOP_ADDR_3          0xFFF003C8
#define BAT_SET_NOP_ADDR_4          0xFFF003CC
#define BAT_SET_NOP_ADDR_5          0xFFF1D70C
#define BAT_SET_NOP_ADDR_6          0xFFF1D728
#define BAT_SET_NOP_ADDR_7          0xFFF1D82C
#define BAT_SET_NOP_ADDR_8          0xFFEE11C4
#define BAT_SET_NOP_ADDR_9          0xFFEE11C8
#define BAT_SETUP_HOOK_ENTRY        0x00800000
#define BAT4U_VAL                   0x008000FF
#define BAT4L_VAL                   0x30800012

#define SET_4_TO_ADDR(addr)        \
    lis 3, addr@h              ;   \
    ori 3, 3, addr@l          ;   \
    stw 4, 0(3)               ;   \
    dcbf 0, 3                  ;   \
    icbi 0, 3                  ;

     .globl Syscall_0x25	# KernelCopyData
Syscall_0x25:
    li 0, 0x2500
    sc
    blr

     .globl Syscall_0x36	# Kernel_Patches
Syscall_0x36:
    li 0, 0x3600
    sc
    blr
     .globl Syscall_0x37	# kernel_memcpy
Syscall_0x37:
    li 0, 0x3700
    sc
    blr
     .globl Syscall_0x26    # kernel_memcpy
Syscall_0x26:
    li 0, 0x2600
    sc
    blr

    .globl Syscall_0x12 	# OSDisableInterrupts
Syscall_0x12:
	li 0, 0x1200
	sc
	blr

    .globl Syscall_0x14		# OSEnableAndClearInterrupts
Syscall_0x14:
	li 0, 0x1400
	sc
	blr

    .globl KernelPatches
KernelPatches:

    # store the old DBAT0
    mfdbatu 5, 0
    mfdbatl 6, 0

    # memory barrier
    eieio
    isync

    # setup DBAT0 for access to kernel code memory
    lis 3, 0xFFF0
    ori 3, 3, 0x0002
    mtdbatu 0, 3
    lis 3, 0xFFF0
    ori 3, 3, 0x0032
    mtdbatl 0, 3

    # memory barrier
    eieio
    isync

    # SaveAndResetDataBATs_And_SRs hook setup, but could be any BAT function though
    # just chosen because its simple
    lis 3, BAT_SETUP_HOOK_ADDR@h
    ori 3, 3, BAT_SETUP_HOOK_ADDR@l

    # make the kernel setup our section in IBAT4 and
    # jump to our function to restore the replaced instructions
    lis 4, 0x3ce0      				#   lis r7, BAT4L_VAL@h
    ori 4, 4, BAT4L_VAL@h
    stw 4, 0x00(3)
    lis 4, 0x60e7      				#   ori r7, r7, BAT4L_VAL@l
    ori 4, 4, BAT4L_VAL@l
    stw 4, 0x04(3)
    lis 4, 0x7cf1      				#   mtspr 561, r7
    ori 4, 4, 0x8ba6
    stw 4, 0x08(3)
    lis 4, 0x3ce0      				#   lis r7, BAT4U_VAL@h
    ori 4, 4, BAT4U_VAL@h
    stw 4, 0x0C(3)
    lis 4, 0x60e7      				#   ori r7, r7, BAT4U_VAL@l
    ori 4, 4, BAT4U_VAL@l
    stw 4, 0x10(3)
    lis 4, 0x7cf0      				#   mtspr 560, r7
    ori 4, 4, 0x8ba6
    stw 4, 0x14(3)
    lis 4, 0x7c00      				#   eieio
    ori 4, 4, 0x06ac
    stw 4, 0x18(3)
    lis 4, 0x4c00      				#   isync
    ori 4, 4, 0x012c
    stw 4, 0x1C(3)
    lis 4, 0x7ce8     				    #   mflr r7
    ori 4, 4, 0x02a6
    stw 4, 0x20(3)
    lis 4, (BAT_SETUP_HOOK_ENTRY | 0x48000003)@h      #   bla BAT_SETUP_HOOK_ENTRY
    ori 4, 4, (BAT_SETUP_HOOK_ENTRY | 0x48000003)@l
    stw 4, 0x24(3)

    # flush and invalidate the replaced instructions
    lis 3, (BAT_SETUP_HOOK_ADDR & ~31)@h
    ori 3, 3, (BAT_SETUP_HOOK_ADDR & ~31)@l
    dcbf 0, 3
    icbi 0, 3
    lis 3, ((BAT_SETUP_HOOK_ADDR + 0x20) & ~31)@h
    ori 3, 3, ((BAT_SETUP_HOOK_ADDR + 0x20) & ~31)@l
    dcbf 0, 3
    icbi 0, 3
    sync

    # setup IBAT4 for core 1 at this position (not really required but wont hurt)
    # IBATL 4
    lis 3, BAT4L_VAL@h
    ori 3, 3, BAT4L_VAL@l
    mtspr 561, 3

    # IBATU 4
    lis 3, BAT4U_VAL@h
    ori 3, 3, BAT4U_VAL@l
    mtspr 560, 3

    # memory barrier
    eieio
    isync

    lis 4, 0x6000

    SET_4_TO_ADDR(BAT_SET_NOP_ADDR_1)
    SET_4_TO_ADDR(BAT_SET_NOP_ADDR_2)
    SET_4_TO_ADDR(BAT_SET_NOP_ADDR_3)
    SET_4_TO_ADDR(BAT_SET_NOP_ADDR_4)
    SET_4_TO_ADDR(BAT_SET_NOP_ADDR_5)
    SET_4_TO_ADDR(BAT_SET_NOP_ADDR_6)
    SET_4_TO_ADDR(BAT_SET_NOP_ADDR_7)

    eieio
    isync

    lis 3, 0xFFEE
    ori 3, 3, 0x0002
    mtdbatu 0, 3
    lis 3, 0xFFEE
    ori 3, 3, 0x0032
    mtdbatl 0, 3

    eieio
    isync

    lis 4, 0x6000
    SET_4_TO_ADDR(BAT_SET_NOP_ADDR_8)
    SET_4_TO_ADDR(BAT_SET_NOP_ADDR_9)


    # memory barrier
    eieio
    isync

    # restore DBAT 0 and return from interrupt
    mtdbatu 0, 5
    mtdbatl 0, 6

    eieio
    isync

    blr