#ifndef IMPORTS_H
#define IMPORTS_H

#include "types.h"
#include "constants.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* stolen from dynamic_libs, thanks Maschell lmaooo */
#define BUS_SPEED                       248625000
#define SECS_TO_TICKS(sec)              (((unsigned long long)(sec)) * (BUS_SPEED/4))
#define MILLISECS_TO_TICKS(msec)        (SECS_TO_TICKS(msec) / 1000)
#define MICROSECS_TO_TICKS(usec)        (SECS_TO_TICKS(usec) / 1000000)
#define os_usleep(usecs)                OSSleepTicks(MICROSECS_TO_TICKS(usecs))
#define os_sleep(secs)                  OSSleepTicks(SECS_TO_TICKS(secs))

#define COREINIT(x) (x - 0xFE3C00) // From IDA

#define BSS_PHYS_OFFSET (0x50000000 - 0x10000000)
#define BSS_VA2PA(a) ((void*)(((u32)a) + BSS_PHYS_OFFSET))
#define BSS_PA2VA(a) ((void*)(((u32)a) - BSS_PHYS_OFFSET))

extern void* _bss_start;
extern void* _bss_end;

extern void (*OSShutdown)(int hi_there);
extern void (*OSSleepTicks)(unsigned long long int number_of_ticks);

extern void* (*MEMAllocFromDefaultHeapEx)(int size, int align);
extern void  (*MEMFreeToDefaultHeap)(void *ptr);
extern void*(*OSAllocFromSystem)(int size, int align);
extern void (*OSFreeToSystem)(void *ptr);
extern void (*CopyMemory)(void *dest, void *source, unsigned int size);
extern void (*SetMemory)(void *dest, unsigned int value, unsigned int size);
extern void*(*OSEffectiveToPhysical)(void* addr);
extern void (*DCFlushRange)(void *buffer, uint32_t length);
extern void (*ICInvalidateRange)(void *buffer, uint32_t length);

extern int(*IM_Open)();
extern int(*IM_Close)(int fd);
extern int(*IM_SetDeviceState)(int fd, void *mem, int state, int a, int b);

extern bool (*OSCreateThread)(void *thread, void *entry, int argc, void *args, uint32_t stack, uint32_t stack_size, int priority, uint16_t attr);
extern int  (*OSResumeThread)(void *thread);
extern void (*OSExitThread)(int c);
extern void (*OSJoinThread)(void *thread, int *ret_val);
extern int  (*OSIsThreadTerminated)(void *thread);
extern void (*OSYieldThread)(void);

extern void (*OSSavesDone_ReadyToRelease)(void);
extern void (*OSReleaseForeground)(void);

extern int (*_SYSLaunchTitleWithStdArgsInNoSplash)(uint64_t tid, void *ptr);
extern uint64_t (*_SYSGetSystemApplicationTitleId)(int sysApp);

extern int (*OSScreenPutFontEx)(int bufferNum, unsigned int posX, unsigned int line, const char* buffer);
extern int (*OSScreenClearBufferEx)(int bufferNum, u32 val);
extern void (*OSScreenInit)(void);
extern int (*OSScreenGetBufferSizeEx)(int bufferNum);
extern int (*OSScreenSetBufferEx)(int bufferNum, void* addr);
extern int (*OSScreenFlipBuffersEx)(int bufferNum);

extern void(*OSExit)(int code);

/* dynamic_libs/vpad_functions.h */
#define BUTTON_A        0x8000
#define BUTTON_B        0x4000
#define BUTTON_X        0x2000
#define BUTTON_Y        0x1000
#define BUTTON_LEFT     0x0800
#define BUTTON_RIGHT    0x0400
#define BUTTON_UP       0x0200
#define BUTTON_DOWN     0x0100
#define BUTTON_ZL       0x0080
#define BUTTON_ZR       0x0040
#define BUTTON_L        0x0020
#define BUTTON_R        0x0010
#define BUTTON_PLUS     0x0008
#define BUTTON_MINUS    0x0004
#define BUTTON_HOME     0x0002
#define BUTTON_SYNC     0x0001


typedef struct OSContext {
    char tag[8];    // Context Identifiert
    u32 gpr[32];    // general purpose registers (r0, r1, r2 etc..)
    u32 cr;
    u32 lr;
    u32 ctr;
    u32 xer;
    u32 srr0;
    u32 srr1;
    u32 ex0;
    u32 ex1;
    u32 exception_type;
    u32 reserved;
    double fpscr;
    double fpr[32];
    u16 spinLockCount;
    u16 state;
    u32 gqr[8];
    u32 pir;
    double psf[32];
    u64 coretime[3];
    u64 starttime;
    u32 error;
    u32 attributes;
    u32 pmc1;
    u32 pmc2;
    u32 pmc3;
    u32 pmc4;
    u32 mmcr0;
    u32 mmcr1;
} OSContext;

typedef struct OSThread {

    OSContext context;
    u32 txtTag;
    u8 state;
    u8 attr;
    short threadId;
    int suspend;
    int priority;
    char _[0x394 - 0x330 - 8];
    u64 thread_link;
    void *stackBase;
    void *stackEnd;
    void *entryPoint;
    char _3A0[0x6A0 - 0x3A0];
} OSThread;

typedef struct
{
    float x,y;
} Vec2D;

typedef struct
{
    uint16_t x, y;               /* Touch coordinates */
    uint16_t touched;            /* 1 = Touched, 0 = Not touched */
    uint16_t validity;           /* 0 = All valid, 1 = X invalid, 2 = Y invalid, 3 = Both invalid? */
} VPADTPData;
 
typedef struct
{
    uint32_t btn_hold;           /* Held buttons */
    uint32_t btn_trigger;        /* Buttons that are pressed at that instant */
    uint32_t btn_release;        /* Released buttons */
    Vec2D lstick, rstick;        /* Each contains 4-byte X and Y components */
    char unknown1c[0x52 - 0x1c]; /* Contains accelerometer and gyroscope data somewhere */
    VPADTPData tpdata;           /* Normal touchscreen data */
    VPADTPData tpdata1;          /* Modified touchscreen data 1 */
    VPADTPData tpdata2;          /* Modified touchscreen data 2 */
    char unknown6a[0xa0 - 0x6a];
    uint8_t volume;
    uint8_t battery;             /* 0 to 6 */
    uint8_t unk_volume;          /* One less than volume */
    char unknowna4[0xac - 0xa4];
} VPADData;

extern int (*VPADRead)(int controller, VPADData *buffer, unsigned int num, int *error);

extern void (*GX2Init)(void *arg);
extern void (*GX2Shutdown)(void);
extern void (*GX2SetSemaphore)(uint64_t *sem, int action);
extern void (*GX2Flush)(void);
extern int  (*GX2DirectCallDisplayList)(void* dl_list, uint32_t size);
extern int  (*GX2GetMainCoreId)(void);

void InitFuncPtrs();

extern void(*__os_snprintf)(char *new, int len, char *format, ...);
extern int (*const OSDynLoad_Acquire)(const char* lib_name, int* out_addr);
extern int (*const OSDynLoad_FindExport)(int lib_handle, int flags, const char* name, void* out_addr);

#endif