#ifndef ENTRY__H
#define ENTRY__H

int sy = 0;
#define COREINIT(x) (x - 0xFE3C00) // From IDA
 
#define FS_CLIENT_BUFFER_SIZE   (5888) /* 0x1700 */
#define FS_CMD_BLOCK_SIZE       (2688) /* 0xA80 */
 
#define FS_STATUS_OK                0 /* Everything looks good */
 
#define FS_STATUS_BASE              (0)
 
#define FS_STATUS_CANCELED          (FS_STATUS_BASE - 1)
#define FS_STATUS_END               (FS_STATUS_BASE - 2)
 
#define FS_STATUS_MAX               (FS_STATUS_BASE - 3)
#define FS_STATUS_ALREADY_OPEN      (FS_STATUS_BASE - 4)
#define FS_STATUS_EXISTS            (FS_STATUS_BASE - 5)
#define FS_STATUS_NOT_FOUND         (FS_STATUS_BASE - 6)
#define FS_STATUS_NOT_FILE          (FS_STATUS_BASE - 7)
#define FS_STATUS_NOT_DIR           (FS_STATUS_BASE - 8)
#define FS_STATUS_ACCESS_ERROR      (FS_STATUS_BASE - 9)
#define FS_STATUS_PERMISSION_ERROR  (FS_STATUS_BASE -10)
#define FS_STATUS_FILE_TOO_BIG      (FS_STATUS_BASE -11)
#define FS_STATUS_STORAGE_FULL      (FS_STATUS_BASE -12)
#define FS_STATUS_JOURNAL_FULL      (FS_STATUS_BASE -13)
#define FS_STATUS_UNSUPPORTED_CMD   (FS_STATUS_BASE -14)
 
#define FS_STATUS_MEDIA_NOT_READY   (FS_STATUS_BASE -15)
#define FS_STATUS_INVALID_MEDIA     (FS_STATUS_BASE -16)
#define FS_STATUS_MEDIA_ERROR       (FS_STATUS_BASE -17)
#define FS_STATUS_DATA_CORRUPTED    (FS_STATUS_BASE -18)
#define FS_STATUS_WRITE_PROTECTED   (FS_STATUS_BASE -19)
 
#define FS_STATUS_FATAL_ERROR       (FS_STATUS_BASE -1024)
 
#define FS_RET_NO_ERROR         0x0000
#define FS_RET_MAX              0x0001
#define FS_RET_ALREADY_OPEN     0x0002
#define FS_RET_EXISTS           0x0004
#define FS_RET_NOT_FOUND        0x0008
#define FS_RET_NOT_FILE         0x0010
#define FS_RET_NOT_DIR          0x0020
#define FS_RET_ACCESS_ERROR     0x0040
#define FS_RET_PERMISSION_ERROR 0x0080
#define FS_RET_FILE_TOO_BIG     0x0100
#define FS_RET_STORAGE_FULL     0x0200
#define FS_RET_UNSUPPORTED_CMD  0x0400
#define FS_RET_JOURNAL_FULL     0x0800
 
#define FS_ERROR_BASE                   (-196608)           /* 0xFFFD0000 */
#define FS_ERROR_LIB_NOT_INIT           (FS_ERROR_BASE - 1)
#define FS_ERROR_INVALID_CLIENT_HANDLE  (FS_ERROR_BASE -37) /* ??? */
 
#define FS_SOURCETYPE_EXTERNAL          0
#define FS_SOURCETYPE_HFIO              1
 
/* stolen from dynamic_libs, thanks Maschell lmaooo */
#define BUS_SPEED                       248625000
#define SECS_TO_TICKS(sec)              (((unsigned long long)(sec)) * (BUS_SPEED/4))
#define MILLISECS_TO_TICKS(msec)        (SECS_TO_TICKS(msec) / 1000)
#define MICROSECS_TO_TICKS(usec)        (SECS_TO_TICKS(usec) / 1000000)
#define os_usleep(usecs)                OSSleepTicks(MICROSECS_TO_TICKS(usecs))
#define os_sleep(secs)                  OSSleepTicks(SECS_TO_TICKS(secs))
 
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
 
typedef struct
{
    uint32_t flag;
    uint32_t permission;
    uint32_t owner_id;
    uint32_t group_id;
    uint32_t size;
    uint32_t alloc_size;
    uint64_t quota_size;
    uint32_t ent_id;
    uint64_t ctime;
    uint64_t mtime;
    uint8_t attributes[48];
} __attribute__((packed)) FSStat;
 
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

// original structure in the kernel that is originally 0x1270 long
typedef struct
{
    uint32_t version_cos_xml;           // version tag from cos.xml
    uint64_t os_version;                // os_version from app.xml
    uint64_t title_id;                  // title_id tag from app.xml
    uint32_t app_type;                  // app_type tag from app.xml
    uint32_t cmdFlags;                  // unknown tag as it is always 0 (might be cmdFlags from cos.xml but i am not sure)
    char rpx_name[0x1000];              // rpx name from cos.xml
    uint32_t unknown2;                  // 0x050B8304 in mii maker and system menu (looks a bit like permissions complex that got masked!?)
    uint32_t unknown3[63];              // those were all zeros, but its probably connected with unknown2
    uint32_t max_size;                  // max_size in cos.xml which defines the maximum amount of memory reserved for the app
    uint32_t avail_size;                // avail_size or codegen_size in cos.xml (seems to mostly be 0?)
    uint32_t codegen_size;              // codegen_size or avail_size in cos.xml (seems to mostly be 0?)
    uint32_t codegen_core;              // codegen_core in cos.xml (seems to mostly be 1?)
    uint32_t max_codesize;              // max_codesize in cos.xml
    uint32_t overlay_arena;             // overlay_arena in cos.xml
    uint32_t unknown4[59];              // all zeros it seems
    uint32_t default_stack0_size;       // not sure because always 0 but very likely
    uint32_t default_stack1_size;       // not sure because always 0 but very likely
    uint32_t default_stack2_size;       // not sure because always 0 but very likely
    uint32_t default_redzone0_size;     // not sure because always 0 but very likely
    uint32_t default_redzone1_size;     // not sure because always 0 but very likely
    uint32_t default_redzone2_size;     // not sure because always 0 but very likely
    uint32_t exception_stack0_size;     // from cos.xml, 0x1000 on mii maker
    uint32_t exception_stack1_size;     // from cos.xml, 0x1000 on mii maker
    uint32_t exception_stack2_size;     // from cos.xml, 0x1000 on mii maker
    uint32_t sdk_version;               // from app.xml, 20909 (0x51AD) on mii maker
    uint32_t title_version;             // from app.xml, 0x32 on mii maker
    /*
    // ---------------------------------------------------------------------------------------------------------------------------------------------
    // the next part might be changing from title to title?! I don't think its important but nice to know maybe....
    // ---------------------------------------------------------------------------------------------------------------------------------------------
    char mlc[4];                        // string "mlc" on mii maker and sysmenu
    uint32_t unknown5[7];               // all zeros on mii maker and sysmenu
    uint32_t unknown6_one;              // 0x01 on mii maker and sysmenu
    // ---------------------------------------------------------------------------------------------------------------------------------------------
    char ACP[4];                        // string "ACP" on mii maker and sysmenu
    uint32_t unknown7[15];              // all zeros on mii maker and sysmenu
    uint32_t unknown8_5;                // 0x05 on mii maker and sysmenu
    uint32_t unknown9_zero;             // 0x00 on mii maker and sysmenu
    uint32_t unknown10_ptr;             // 0xFF23DD0C pointer on mii maker and sysmenu
    // ---------------------------------------------------------------------------------------------------------------------------------------------
    char UVD[4];                        // string "UVD" on mii maker and sysmenu
    uint32_t unknown11[15];             // all zeros on mii maker and sysmenu
    uint32_t unknown12_5;               // 0x05 on mii maker and sysmenu
    uint32_t unknown13_zero;            // 0x00 on mii maker and sysmenu
    uint32_t unknown14_ptr;             // 0xFF23EFC8 pointer on mii maker and sysmenu
    // ---------------------------------------------------------------------------------------------------------------------------------------------
    char SND[4];                        // string "SND" on mii maker and sysmenu
    uint32_t unknown15[15];             // all zeros on mii maker and sysmenu
    uint32_t unknown16_5;               // 0x05 on mii maker and sysmenu
    uint32_t unknown17_zero;            // 0x00 on mii maker and sysmenu
    uint32_t unknown18_ptr;             // 0xFF23F014 pointer on mii maker and sysmenu
    // ---------------------------------------------------------------------------------------------------------------------------------------------
    uint32_t unknown19;                 // 0x02 on miimaker, 0x0F on system menu
    */
    // after that only zeros follow
} __attribute__((packed)) CosAppXmlInfo;

typedef struct _loader_globals_t
{
    int sgIsLoadingBuffer;
    int sgFileType;
    int sgProcId;
    int sgGotBytes;
    int sgFileOffset;
    int sgBufferNumber;
    int sgBounceError;
    char sgLoadName[0x1000];
} __attribute__((packed)) loader_globals_t;

typedef struct _loader_globals_550_t
{
    int sgFinishedLoadingBuffer;
    int sgFileType;
    int sgProcId;
    int sgGotBytes;
    int sgTotalBytes;
    int sgFileOffset;
    int sgBufferNumber;
    int sgBounceError;
    char sgLoadName[0x1000];
} __attribute__((packed)) loader_globals_550_t;
 
uint32_t* framebuffer_drc;
uint32_t framebuffer_drc_size;

#endif