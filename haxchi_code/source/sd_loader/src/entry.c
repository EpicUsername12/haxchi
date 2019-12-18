#include <gctypes.h>
#include "elf_abi.h"
#include "entry.h"
#include "../../common.h"
#include "elf_stuff.h"

extern void my_PrepareTitle_hook(void);
void KernelCopyData(uint32_t addr, uint32_t src, uint32_t len);
static int LiWaitOneChunk(uint32_t *iRemainingBytes, const char *filename, int fileType);
void Syscall_0x25(uint32_t addr, uint32_t src, uint32_t len);
static uint32_t hook_LiWaitOneChunk;
static uint32_t addrphys_LiWaitOneChunk;

void my_PrepareTitle(CosAppXmlInfo *xmlKernelInfo)
{
	if(ELF_DATA_ADDR == MEM_AREA_TABLE->address)
	{
		xmlKernelInfo->max_size = RPX_MAX_SIZE;
		xmlKernelInfo->max_codesize = RPX_MAX_CODE_SIZE;
		//! setup our hook to LiWaitOneChunk for RPX loading
		hook_LiWaitOneChunk = ((u32)LiWaitOneChunk) | 0x48000002;
		KernelCopyData(addrphys_LiWaitOneChunk, (u32) &hook_LiWaitOneChunk, 4);
		asm volatile("icbi 0, %0" : : "r" (OS_SPECIFICS->addr_LiWaitOneChunk & ~31));
	}
	else if((MAIN_ENTRY_ADDR == 0xC001C0DE) && (*(u32*)xmlKernelInfo->rpx_name == 0x66666c5f)) // ffl_
	{
		//! restore original LiWaitOneChunk instruction as our RPX is done
		MAIN_ENTRY_ADDR = 0xDEADC0DE;
		KernelCopyData(addrphys_LiWaitOneChunk, (u32)&OS_SPECIFICS->orig_LiWaitOneChunkInstr, 4);
		asm volatile("icbi 0, %0" : : "r" (OS_SPECIFICS->addr_LiWaitOneChunk & ~31));
	}
}

/* OS Cache functions */
void (*DCFlushRange)(void *buffer, uint32_t length);
void (*DCInvalidateRange)(void *buffer, uint32_t length);
void (*ICInvalidateRange)(void *buffer, uint32_t length);
 
/* OS Screen Functions */
int (*OSScreenPutFontEx)(int bufferNum, unsigned int posX, unsigned int line, const char* buffer);
int (*OSScreenClearBufferEx)(int bufferNum, u32 val);
void (*OSScreenInit)(void);
int (*OSScreenGetBufferSizeEx)(int bufferNum);
int (*OSScreenSetBufferEx)(int bufferNum, void* addr);
int (*OSScreenFlipBuffersEx)(int bufferNum);
 
/* OS misc functions */
void(*__os_snprintf)(char *new, int len, char *format, ...);
void (*OSSleepTicks)(unsigned long long int number_of_ticks);
void (*SYSRelaunchTitle)(int a, char **);
void (*OSExit)(int code);
int (*VPADRead)(int controller, VPADData *buffer, unsigned int num, int *error);
void *(*OSEffectiveToPhysical)(void *in_addr);
 
/* OS memory functions */
void*(*MEMAllocFromDefaultHeapEx)(int size, int align);
void (*MEMFreeToDefaultHeap)(void *ptr);
 
/* OS filesystem functions */
int (*FSInit)(void);
int (*FSAddClientEx)(void *client, int zero, int err_h);
int (*FSDelClient)(void *client);
void (*FSInitCmdBlock)(void *cmd_block);
int (*FSGetMountSource)(void *client, void *cmd_block, int type, void *source, int err);
int (*FSMount)(void *client, void *cmd_block, void *source, const char *path, uint32_t bytes, int err);
int (*FSUnmount)(void *client, void *cmd_block, const char *target, int err);
int (*FSOpenFile)(void *client, void *cmd_block, const char *path, const char *mode, int *fd, int err);
int (*FSGetStatFile)(void *client, void *cmd_block, int fd, void *buffer, int err);
int (*FSReadFile)(void *client, void *cmd_block, void *buf, int size, int count, int fd, int flags, int err);
int (*FSCloseFile)(void *client, void *cmd_block, int fd, int err);

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

static int LiWaitOneChunk(uint32_t *iRemainingBytes, const char *filename, int fileType)
{
	unsigned int result;
	register int core_id;
	int remaining_bytes = 0;

	int sgFileOffset;
	int sgBufferNumber;
	int *sgBounceError;
	int *sgGotBytes;
	int *sgTotalBytes;
	int *sgIsLoadingBuffer;
	int *sgFinishedLoadingBuffer;

	asm volatile("mfspr %0, 0x3EF" : "=r" (core_id));
	uint32_t gDynloadInitialized = *(volatile uint32_t*)(OS_SPECIFICS->addr_gDynloadInitialized + (core_id << 2));

	loader_globals_550_t *loader_globals = (loader_globals_550_t*)(0xEFE19E80);

	sgBufferNumber = loader_globals->sgBufferNumber;
	sgFileOffset = loader_globals->sgFileOffset;
	sgBounceError = &loader_globals->sgBounceError;
	sgGotBytes = &loader_globals->sgGotBytes;
	sgTotalBytes = &loader_globals->sgTotalBytes;
	sgFinishedLoadingBuffer = &loader_globals->sgFinishedLoadingBuffer;
	sgIsLoadingBuffer = NULL;

	if(gDynloadInitialized != 0) {
		result = OS_SPECIFICS->LiWaitIopCompleteWithInterrupts(0x2160EC0, &remaining_bytes);

	}
	else {
		result = OS_SPECIFICS->LiWaitIopComplete(0x2160EC0, &remaining_bytes);
	}
	s_mem_area *mem_area = MEM_AREA_TABLE;
	if((ELF_DATA_ADDR == mem_area->address) && (fileType == 0))
	{
		unsigned int load_address = (sgBufferNumber == 1) ? 0xF6000000 : (0xF6000000 + 0x00400000);
		unsigned int load_addressPhys = (sgBufferNumber == 1) ? 0x1B000000 : (0x1B000000 + 0x00400000);

		remaining_bytes = ELF_DATA_SIZE - sgFileOffset;
		if (remaining_bytes > 0x400000)
			remaining_bytes = 0x400000;

		DCFlushRange((void*)load_address, remaining_bytes);

		u32 rpxBlockPos = 0;
		u32 done = 0;
		u32 mapOffset = 0;

		while((done < (u32)sgFileOffset) && mem_area)
		{
			if((done + mem_area->size) > (u32)sgFileOffset)
			{
				mapOffset = sgFileOffset - done;
				done = sgFileOffset;
			}
			else
			{
				done += mem_area->size;
				mem_area = mem_area->next;
			}
		}

		while((done < ELF_DATA_SIZE) && (rpxBlockPos < 0x400000) && mem_area)
		{
			u32 address = mem_area->address + mapOffset;
			u32 blockSize = ELF_DATA_SIZE - done;

			if(blockSize > (0x400000 - rpxBlockPos))
			{
				blockSize = 0x400000 - rpxBlockPos;
			}
			if((mapOffset + blockSize) >= mem_area->size)
			{
				blockSize = mem_area->size - mapOffset;
				mapOffset = -blockSize;
				mem_area = mem_area->next;
			}

			Syscall_0x25(load_addressPhys + rpxBlockPos, address, blockSize);
			done += blockSize;
			rpxBlockPos += blockSize;
			mapOffset += blockSize;
		}

		DCInvalidateRange((void*)load_address, remaining_bytes);

		if((u32)(sgFileOffset + remaining_bytes) == ELF_DATA_SIZE)
		{
			ELF_DATA_ADDR = 0xDEADC0DE;
			ELF_DATA_SIZE = 0;
			MAIN_ENTRY_ADDR = 0xC001C0DE;
		}
		result = 0;
	}
	if(sgBounceError) {
		*sgBounceError = result;
	}

	if(sgFinishedLoadingBuffer)
	{
		unsigned int zeroBitCount = 0;
		asm volatile("cntlzw %0, %0" : "=r" (zeroBitCount) : "r"(*sgFinishedLoadingBuffer));
		*sgFinishedLoadingBuffer = zeroBitCount >> 5;
	}
	else if(sgIsLoadingBuffer)
	{
		*sgIsLoadingBuffer = 0;
	}

	if(result == 0)
	{
		if(iRemainingBytes) {
			if(sgGotBytes) {
				*sgGotBytes = remaining_bytes;
			}

			*iRemainingBytes = remaining_bytes;

			if(sgTotalBytes) {
				*sgTotalBytes += remaining_bytes;
			}
		}
	}

	return result;
}

void KernelCopyData(uint32_t addr, uint32_t src, uint32_t len)
{
	/*
	 * Setup a DBAT access with cache inhibited to write through and read directly from memory
	 */
	unsigned int dbatu0, dbatl0, dbatu1, dbatl1;
	// save the original DBAT value
	asm volatile("mfdbatu %0, 0" : "=r" (dbatu0));
	asm volatile("mfdbatl %0, 0" : "=r" (dbatl0));
	asm volatile("mfdbatu %0, 1" : "=r" (dbatu1));
	asm volatile("mfdbatl %0, 1" : "=r" (dbatl1));

	uint32_t target_dbatu0 = 0;
	uint32_t target_dbatl0 = 0;
	uint32_t target_dbatu1 = 0;
	uint32_t target_dbatl1 = 0;

	uint32_t *dst_p = (uint32_t*)addr;
	uint32_t *src_p = (uint32_t*)src;

	// we only need DBAT modification for addresses out of our own DBAT range
	// as our own DBAT is available everywhere for user and supervisor
	// since our own DBAT is on DBAT5 position we don't collide here
	if(addr < 0x00800000 || addr >= 0x01000000)
	{
		target_dbatu0 = (addr & 0x00F00000) | 0xC0000000 | 0x3FF;
		target_dbatl0 = (addr & 0xFFF00000) | 0x32;
		asm volatile("mtdbatu 0, %0" : : "r" (target_dbatu0));
		asm volatile("mtdbatl 0, %0" : : "r" (target_dbatl0));
		dst_p = (uint32_t*)((addr & 0xFFFFFF) | 0xC0000000);
	}
	if(src < 0x00800000 || src >= 0x01000000)
	{
		target_dbatu1 = (src & 0x00F00000) | 0xB0000000 | 0x1FF;
		target_dbatl1 = (src & 0xFFF00000) | 0x32;

		asm volatile("mtdbatu 1, %0" : : "r" (target_dbatu1));
		asm volatile("mtdbatl 1, %0" : : "r" (target_dbatl1));
		src_p = (uint32_t*)((src & 0xFFFFFF) | 0xB0000000);
	}

	asm volatile("eieio; isync");

	unsigned int i;
	for(i = 0; i < len; i += 4)
	{
		// if we are on the edge to next chunk
		if((target_dbatu0 != 0) && (((uint32_t)dst_p & 0x00F00000) != (target_dbatu0 & 0x00F00000)))
		{
			target_dbatu0 = ((addr + i) & 0x00F00000) | 0xC0000000 | 0x1F;
			target_dbatl0 = ((addr + i) & 0xFFF00000) | 0x32;
			dst_p = (uint32_t*)(((addr + i) & 0xFFFFFF) | 0xC0000000);

			asm volatile("eieio; isync");
			asm volatile("mtdbatu 0, %0" : : "r" (target_dbatu0));
			asm volatile("mtdbatl 0, %0" : : "r" (target_dbatl0));
			asm volatile("eieio; isync");
		}
		if((target_dbatu1 != 0) && (((uint32_t)src_p & 0x00F00000) != (target_dbatu1 & 0x00F00000)))
		{
			target_dbatu1 = ((src + i) & 0x00F00000) | 0xB0000000 | 0x1F;
			target_dbatl1 = ((src + i) & 0xFFF00000) | 0x32;
			src_p = (uint32_t*)(((src + i) & 0xFFFFFF) | 0xB0000000);

			asm volatile("eieio; isync");
			asm volatile("mtdbatu 1, %0" : : "r" (target_dbatu1));
			asm volatile("mtdbatl 1, %0" : : "r" (target_dbatl1));
			asm volatile("eieio; isync");
		}

		*dst_p = *src_p;

		++dst_p;
		++src_p;
	}

	/*
	 * Restore original DBAT value
	 */
	asm volatile("eieio; isync");
	asm volatile("mtdbatu 0, %0" : : "r" (dbatu0));
	asm volatile("mtdbatl 0, %0" : : "r" (dbatl0));
	asm volatile("mtdbatu 1, %0" : : "r" (dbatu1));
	asm volatile("mtdbatl 1, %0" : : "r" (dbatl1));
	asm volatile("eieio; isync");
}

static const char *HBL_ELF_PATH = "/vol/external01/wiiu/apps/homebrew_launcher/homebrew_launcher.elf";
 
int wait_for_vpad_input()
{
	VPADData vpad;
	int err_h;
 
	while(1)
	{
		VPADRead(0, &vpad, 1, &err_h);
		if(vpad.btn_hold != 0)
		{
			break;
		}
	}
 
	return vpad.btn_hold;
}
 
void ScreenInit()
{
	OSScreenInit();
	framebuffer_drc_size = OSScreenGetBufferSizeEx(1);
	framebuffer_drc = (u32*)0xF4000000;
	OSScreenSetBufferEx(1, framebuffer_drc);
}
 
void ScreenClear()
{
	OSScreenClearBufferEx(1, 0x808080FF);
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

uint32_t *entrypoint = (uint32_t*)0x1005E040;
void (*memcpy)(void *, void *, int);

uint32_t _main(int argc, char **argv)
{

	  /* Values for WiiU FW 5.53 (55X) */
	int (*const OSDynLoad_Acquire)(const char* lib_name, int* out_addr) = (void*)COREINIT(0x0200DFB4);
	int (*const OSDynLoad_FindExport)(int lib_handle, int flags, const char* name, void* out_addr) = (void*)COREINIT(0x0200F428);
 
	int coreinit_handle, vpad_handle;
	OSDynLoad_Acquire("coreinit.rpl", &coreinit_handle);
	OSDynLoad_Acquire("vpad.rpl", &vpad_handle);
 
	OSSleepTicks = (void*)COREINIT(0x0202634C);
	SYSRelaunchTitle = (void*)COREINIT(0x02019A80);
	OSExit = (void*)COREINIT(0x020009C4);

	OSDynLoad_FindExport(vpad_handle, 0, "VPADRead", &VPADRead);

	uint32_t *fptr;

	OSDynLoad_FindExport(coreinit_handle, 1, "MEMAllocFromDefaultHeapEx", &fptr);
	MEMAllocFromDefaultHeapEx = (void * (*)(int, int))*fptr;

	OSDynLoad_FindExport(coreinit_handle, 1, "MEMFreeToDefaultHeap", &fptr);
	MEMFreeToDefaultHeap = (void (*)(void *))*fptr;
 
	DCFlushRange = (void*)COREINIT(0x02007B88);
	DCInvalidateRange = (void*)COREINIT(0x02007B5C);
	ICInvalidateRange = (void*)COREINIT(0x02007CB0);

	OSScreenPutFontEx = (void*)COREINIT(0x0201F114);
	OSScreenClearBufferEx = (void*)COREINIT(0x0201EC90);
	OSScreenInit = (void*)COREINIT(0x0201EA80);
	OSScreenGetBufferSizeEx = (void*)COREINIT(0x0201EB1C);
	OSScreenSetBufferEx = (void*)COREINIT(0x0201EB34);
	OSScreenFlipBuffersEx = (void*)COREINIT(0x0201EBD0);
	__os_snprintf = (void*)COREINIT(0x02012D60);
	OSEffectiveToPhysical = (void*)COREINIT(0x02003120);

	memcpy = (void*)COREINIT(0x02019BC8);
 
	FSInit = (void*)COREINIT(0x0204CA10);
	FSAddClientEx = (void*)COREINIT(0x0204CC44);
	FSDelClient = (void*)COREINIT(0x0204D050);
	FSInitCmdBlock = (void*)COREINIT(0x0204D29C);
	FSGetMountSource = (void*)COREINIT(0x0205326C);
	FSMount = (void*)COREINIT(0x0205335C);
	FSUnmount = (void*)COREINIT(0x020533D4);
	FSOpenFile = (void*)COREINIT(0x020535C4);
	FSGetStatFile = (void*)COREINIT(0x02053BE8);
	FSReadFile = (void*)COREINIT(0x02053750);
	FSCloseFile = (void*)COREINIT(0x020536D0);

	ScreenInit();
	ScreenClear();

	char buf[30];
	__os_snprintf(buf, 30, "Starting.. (%016llx)", *(uint64_t*)0x10013C10);
	print(buf);

	kern_write((void*)(OS_SPECIFICS->addr_KernSyscallTbl1 + (0x25 * 4)), (unsigned int)KernelCopyData);
	kern_write((void*)(OS_SPECIFICS->addr_KernSyscallTbl2 + (0x25 * 4)), (unsigned int)KernelCopyData);
	kern_write((void*)(OS_SPECIFICS->addr_KernSyscallTbl3 + (0x25 * 4)), (unsigned int)KernelCopyData);
	kern_write((void*)(OS_SPECIFICS->addr_KernSyscallTbl4 + (0x25 * 4)), (unsigned int)KernelCopyData);
	kern_write((void*)(OS_SPECIFICS->addr_KernSyscallTbl5 + (0x25 * 4)), (unsigned int)KernelCopyData);

	addrphys_LiWaitOneChunk = (uint32_t)OSEffectiveToPhysical((void*)OS_SPECIFICS->addr_LiWaitOneChunk);

	u32 addr_my_PrepareTitle_hook = ((u32)my_PrepareTitle_hook) | 0x48000003;
	DCFlushRange(&addr_my_PrepareTitle_hook, 4);

	Syscall_0x25(OS_SPECIFICS->addr_PrepareTitle_hook, (uint32_t)OSEffectiveToPhysical(&addr_my_PrepareTitle_hook), 4);

	if(MAIN_ENTRY_ADDR != 0xC001C0DE)
    {
		if(ELF_DATA_ADDR != 0xDEADC0DE && ELF_DATA_SIZE > 0)
		{
			uint8_t* ELF_FileBuffer = MEMAllocFromDefaultHeapEx(ELF_DATA_SIZE, 4);

			memcpy(ELF_FileBuffer, (uint8_t*)ELF_DATA_ADDR, ELF_DATA_SIZE);
			MAIN_ENTRY_ADDR = load_elf_image(ELF_FileBuffer);
			MEMFreeToDefaultHeap(ELF_FileBuffer);

			ELF_DATA_ADDR = 0xDEADC0DE;
	        ELF_DATA_SIZE = 0;
		}
		if(MAIN_ENTRY_ADDR == 0xDEADC0DE || MAIN_ENTRY_ADDR == 0)
		{

			if(HBL_CHANNEL)
				return *entrypoint;

			void *FSClient = MEMAllocFromDefaultHeapEx(FS_CLIENT_BUFFER_SIZE, 4);
			void *FSCmd = MEMAllocFromDefaultHeapEx(FS_CMD_BLOCK_SIZE, 4);

			FSInit();
			FSInitCmdBlock(FSCmd);
			FSAddClientEx(FSClient, 0, -1);

			char tempPath[0x300];
			char mountPath[0x100];
			int ret = 0;

			uint8_t *ELF_FileBuffer;
			int ELF_FileHandle;
			FSStat ELF_FileStat;
			uint32_t done = 0;

			const char *error_text = "\n\nCouldn't mount the SD Card\nExiting in 2 seconds\n(Reload Mii Maker to open HBL again)";

			while (FSGetMountSource(FSClient, FSCmd, 0, tempPath, -1) != 0) {

				ScreenClear();
				print("-3");
				print(error_text);

				os_sleep(2);

				FSDelClient(FSClient);
				// FSShutdown();
				MEMFreeToDefaultHeap(FSClient);
				MEMFreeToDefaultHeap(FSCmd);
				return *entrypoint;
			}

			while(FSMount(FSClient, FSCmd, tempPath, mountPath, 128, -1) != 0) {

				ScreenClear();
				print("-4");
				print(error_text);

				os_sleep(2);

				FSDelClient(FSClient);
				// FSShutdown();
				MEMFreeToDefaultHeap(FSClient);
				MEMFreeToDefaultHeap(FSCmd);

				return *entrypoint;

			}

			ret = FSOpenFile(FSClient, FSCmd, HBL_ELF_PATH, "r", &ELF_FileHandle, -1);
			if(ret != 0)
			{
				print("Couldn't open the file");
				os_sleep(2);

				return *entrypoint;
			}

			ELF_FileStat.size = 0;
			FSGetStatFile(FSClient, FSCmd, ELF_FileHandle, &ELF_FileStat, -1);

			ELF_FileBuffer = (uint8_t*)MEMAllocFromDefaultHeapEx((ELF_FileStat.size + 0x3F) & ~0x3F, 0x40);
			while(done < ELF_FileStat.size)
			{
				int readBytes = FSReadFile(FSClient, FSCmd, ELF_FileBuffer + done, 1, ELF_FileStat.size - done, ELF_FileHandle, 0, -1);
				if(readBytes <= 0) {
					break;
				}
				done += readBytes;
			}

			FSCloseFile(FSClient, FSCmd, ELF_FileHandle, -1);
			FSUnmount(FSClient, FSCmd, mountPath, -1);
			FSDelClient(FSClient);

			MEMFreeToDefaultHeap(FSClient);
			MEMFreeToDefaultHeap(FSCmd);

			MAIN_ENTRY_ADDR = load_elf_image(ELF_FileBuffer);
			MEMFreeToDefaultHeap(ELF_FileBuffer);
		}
		
		int rc = ((int (*)(int, char **))MAIN_ENTRY_ADDR)(argc, argv);

		if(rc != -3)
		{
			SD_LOADER_FORCE_HBL = 1;
			MAIN_ENTRY_ADDR = 0xDEADC0DE;
			SYSRelaunchTitle(argc, argv);
			OSExit(0);
		}

		return *entrypoint;
	

	}

	return *entrypoint;
}
