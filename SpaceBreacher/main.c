#include "ntifs.h"
#include "ntddk.h"
#include "intrin.h"

#pragma warning(push)
#pragma warning(disable: 4201)

typedef struct _MMPTE_HARDWARE {
    UINT64 Valid : 1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    UINT64 Dirty1 : 1;
#else
#ifdef CONFIG_SMP
    UINT64 Writable : 1;
#else
    UINT64 Write : 1;
#endif
#endif
    UINT64 Owner : 1;
    UINT64 WriteThrough : 1;
    UINT64 CacheDisable : 1;
    UINT64 Accessed : 1;
    UINT64 Dirty : 1;
    UINT64 LargePage : 1;
    UINT64 Global : 1;
    UINT64 CopyOnWrite : 1;
    UINT64 Prototype : 1;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    UINT64 Write : 1;
    UINT64 PageFrameNumber : 36;
    UINT64 reserved1 : 4;
#else
#ifdef CONFIG_SMP
    UINT64 Write : 1;
#else
    UINT64 reserved0 : 1;
#endif
    UINT64 PageFrameNumber : 28;
    UINT64 reserved1 : 12;
#endif
    UINT64 SoftwareWsIndex : 11;
    UINT64 NoExecute : 1;
} MMPTE_HARDWARE;

typedef struct _MI_ACTIVE_PFN {
    union {
        struct {
            UINT64 Tradable : 1;
            UINT64 NonPagedBuddy : 43;
            UINT64 Spare : 20;
        } Leaf;
        struct {
            UINT64 Tradable : 1;
            UINT64 NonPagedBuddy : 31;
            UINT64 UsedPageTableEntries : 10;
            UINT64 WsleAge : 3;
            UINT64 OldestWsleLeafEntries : 10;
            UINT64 OldestWsleLeafAge : 3;
            UINT64 Spare : 6;
        } PageTable;
        UINT64 EntireActiveField;
    };
} MI_ACTIVE_PFN;

typedef struct _MIPFNFLINK {
    union {
        SLIST_ENTRY* pNextSlistPfn;
        VOID* pNext;
        struct {
            UINT64 Flink : 40;
            UINT64 NodeFlinkLow : 24;
        };
        UINT64 EntireField;
        MI_ACTIVE_PFN Active;
    };
} MIPFNFLINK;

typedef struct _MIPFNBLINK {
    union {
        struct {
            UINT64 Blink : 40;
            UINT64 NodeBlinkLow : 19;
            UINT64 TbFlushStamp : 3;
            UINT64 PageBlinkDeleteBit : 1;
            UINT64 PageBlinkLockBit : 1;
        };
        struct {
            UINT64 ShareCount : 62;
            UINT64 PageShareCountDeleteBit : 1;
            UINT64 PageShareCountLockBit : 1;
        };
        INT64 EntireField;
        struct {
            UINT64 LockNotUsed : 62;
            UINT64 DeleteBit : 1;
            UINT64 LockBit : 1;
        };
    };
} MIPFNBLINK;

typedef struct _MMPFNENTRY1 {
    UINT8 PageLocation : 3;
    UINT8 WriteInProgress : 1;
    UINT8 Modified : 1;
    UINT8 ReadInProgress : 1;
    UINT8 CacheAttribute : 2;
} MMPFNENTRY1;

typedef struct _MMPFNENTRY3 {
    UINT8 Priority : 3;
    UINT8 OnProtectedStandby : 1;
    UINT8 InPageError : 1;
    UINT8 SystemChargedPage : 1;
    UINT8 RemovalRequested : 1;
    UINT8 ParityError : 1;
} MMPFNENTRY3;

typedef struct _MI_PFN_FLAGS {
    union {
        struct {
            UINT16 ReferenceCount;
            UINT8 PageLocation : 3;
            UINT8 WriteInProgress : 1;
            UINT8 Modified : 1;
            UINT8 ReadInProgress : 1;
            UINT8 CacheAttribute : 2;
            UINT8 Priority : 3;
            UINT8 OnProtectedStandby : 1;
            UINT8 InPageError : 1;
            UINT8 SystemChargedPage : 1;
            UINT8 RemovalRequested : 1;
            UINT8 ParityError : 1;
        };
        UINT32 EntireField;
    };
} MI_PFN_FLAGS;

typedef struct _MI_PFN_FLAGS5 {
    union {
        UINT32 EntireField;
        struct {
            UINT32 NodeBlinkHigh : 21;
            UINT32 NodeFlinkMiddle : 11;
        } StandbyList;
        struct {
            UINT8 ModifiedListBucketIndex : 4;
        } MappedPageList;
        struct {
            UINT32 PageTableBlinkLow : 16;
            UINT32 PageTableBuddyHigh : 10;
            UINT32 PageTableLinked : 1;
            UINT32 AnchorLargePageSize : 2;
            UINT32 Spare1 : 3;
        } Active;
    };
} MI_PFN_FLAGS5;

typedef struct _MI_PFN_FLAGS4 {
    union {
        struct {
            UINT64 PteFrame : 40;
            UINT64 ResidentPage : 1;
            UINT64 ResidentPageContainsBadPages : 1;
            UINT64 Unused1 : 1;
            UINT64 Partition : 10;
            UINT64 FileOnly : 1;
            UINT64 PfnExists : 1;
            UINT64 NodeFlinkHigh : 5;
            UINT64 PageIdentity : 3;
            UINT64 PrototypePte : 1;
        };
        UINT64 EntireField;
    };
} MI_PFN_FLAGS4;

typedef struct _MMPFN {
    union {
        LIST_ENTRY ListEntry;
        RTL_BALANCED_NODE TreeNode;
        struct {
            MIPFNFLINK u1;
            union {
                MMPTE_HARDWARE* pPteAddress;
                UINT64 PteLong;
            };
            MMPTE_HARDWARE OriginalPte;
        };
    };
    volatile MIPFNBLINK u2;
    union {
        struct {
            UINT16 ReferenceCount;
            MMPFNENTRY1 e1;
        };
        struct {
            struct {
                UINT16 ReferenceCount;
            } e2;
            UINT8 MmPfnPad0[1];
            MMPFNENTRY3 e3;
        };
        volatile MI_PFN_FLAGS e4;
    } u3;
    MI_PFN_FLAGS5 u5;
    MI_PFN_FLAGS4 u4;
} MMPFN;

typedef struct _DBGKD_DEBUG_DATA_HEADER64 {
    LIST_ENTRY64 List;
    UINT32 OwnerTag;
    UINT32 Size;
} DBGKD_DEBUG_DATA_HEADER64;

#define ULPTR64 UINT64

typedef struct _KDDEBUGGER_DATA64 {
    DBGKD_DEBUG_DATA_HEADER64 Header;
    UINT64 KernBase;
    ULPTR64 BreakpointWithStatus;
    UINT64 SavedContext;
    USHORT ThCallbackStack;
    USHORT NextCallback;
    USHORT FramePointer;
    USHORT PaeEnabled : 1;
    ULPTR64 KiCallUserMode;
    UINT64 KeUserCallbackDispatcher;
    ULPTR64 PsLoadedModuleList;
    ULPTR64 PsActiveProcessHead;
    ULPTR64 PspCidTable;
    ULPTR64 ExpSystemResourcesList;
    ULPTR64 ExpPagedPoolDescriptor;
    ULPTR64 ExpNumberOfPagedPools;
    ULPTR64 KeTimeIncrement;
    ULPTR64 KeBugCheckCallbackListHead;
    ULPTR64 KiBugcheckData;
    ULPTR64 IopErrorLogListHead;
    ULPTR64 ObpRootDirectoryObject;
    ULPTR64 ObpTypeObjectType;
    ULPTR64 MmSystemCacheStart;
    ULPTR64 MmSystemCacheEnd;
    ULPTR64 MmSystemCacheWs;
    ULPTR64 MmPfnDatabase;
    ULPTR64 MmSystemPtesStart;
    ULPTR64 MmSystemPtesEnd;
    ULPTR64 MmSubsectionBase;
    ULPTR64 MmNumberOfPagingFiles;
    ULPTR64 MmLowestPhysicalPage;
    ULPTR64 MmHighestPhysicalPage;
    ULPTR64 MmNumberOfPhysicalPages;
    ULPTR64 MmMaximumNonPagedPoolInBytes;
    ULPTR64 MmNonPagedSystemStart;
    ULPTR64 MmNonPagedPoolStart;
    ULPTR64 MmNonPagedPoolEnd;
    ULPTR64 MmPagedPoolStart;
    ULPTR64 MmPagedPoolEnd;
    ULPTR64 MmPagedPoolInformation;
    UINT64 MmPageSize;
    ULPTR64 MmSizeOfPagedPoolInBytes;
    ULPTR64 MmTotalCommitLimit;
    ULPTR64 MmTotalCommittedPages;
    ULPTR64 MmSharedCommit;
    ULPTR64 MmDriverCommit;
    ULPTR64 MmProcessCommit;
    ULPTR64 MmPagedPoolCommit;
    ULPTR64 MmExtendedCommit;
    ULPTR64 MmZeroedPageListHead;
    ULPTR64 MmFreePageListHead;
    ULPTR64 MmStandbyPageListHead;
    ULPTR64 MmModifiedPageListHead;
    ULPTR64 MmModifiedNoWritePageListHead;
    ULPTR64 MmAvailablePages;
    ULPTR64 MmResidentAvailablePages;
    ULPTR64 PoolTrackTable;
    ULPTR64 NonPagedPoolDescriptor;
    ULPTR64 MmHighestUserAddress;
    ULPTR64 MmSystemRangeStart;
    ULPTR64 MmUserProbeAddress;
    ULPTR64 KdPrintCircularBuffer;
    ULPTR64 KdPrintCircularBufferEnd;
    ULPTR64 KdPrintWritePointer;
    ULPTR64 KdPrintRolloverCount;
    ULPTR64 MmLoadedUserImageList;

#if (NTDDI_VERSION >= NTDDI_WINXP)
    ULPTR64 NtBuildLab;
    ULPTR64 KiNormalSystemCall;
#endif

    /* NOTE: Documented as "NT 5.0 hotfix (QFE) addition". */
#if (NTDDI_VERSION >= NTDDI_WIN2KSP4)
    ULPTR64 KiProcessorBlock;
    ULPTR64 MmUnloadedDrivers;
    ULPTR64 MmLastUnloadedDriver;
    ULPTR64 MmTriageActionTaken;
    ULPTR64 MmSpecialPoolTag;
    ULPTR64 KernelVerifier;
    ULPTR64 MmVerifierData;
    ULPTR64 MmAllocatedNonPagedPool;
    ULPTR64 MmPeakCommitment;
    ULPTR64 MmTotalCommitLimitMaximum;
    ULPTR64 CmNtCSDVersion;
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
    ULPTR64 MmPhysicalMemoryBlock;
    ULPTR64 MmSessionBase;
    ULPTR64 MmSessionSize;
    ULPTR64 MmSystemParentTablePage;
#endif

#if (NTDDI_VERSION >= NTDDI_WS03)
    ULPTR64 MmVirtualTranslationBase;
    UINT16 OffsetKThreadNextProcessor;
    UINT16 OffsetKThreadTeb;
    UINT16 OffsetKThreadKernelStack;
    UINT16 OffsetKThreadInitialStack;
    UINT16 OffsetKThreadApcProcess;
    UINT16 OffsetKThreadState;
    UINT16 OffsetKThreadBStore;
    UINT16 OffsetKThreadBStoreLimit;
    UINT16 SizeEProcess;
    UINT16 OffsetEprocessPeb;
    UINT16 OffsetEprocessParentCID;
    UINT16 OffsetEprocessDirectoryTableBase;
    UINT16 SizePrcb;
    UINT16 OffsetPrcbDpcRoutine;
    UINT16 OffsetPrcbCurrentThread;
    UINT16 OffsetPrcbMhz;
    UINT16 OffsetPrcbCpuType;
    UINT16 OffsetPrcbVendorString;
    UINT16 OffsetPrcbProcStateContext;
    UINT16 OffsetPrcbNumber;
    UINT16 SizeEThread;
    ULPTR64 KdPrintCircularBufferPtr;
    ULPTR64 KdPrintBufferSize;
    ULPTR64 KeLoaderBlock;
    UINT16 SizePcr;
    UINT16 OffsetPcrSelfPcr;
    UINT16 OffsetPcrCurrentPrcb;
    UINT16 OffsetPcrContainedPrcb;
    UINT16 OffsetPcrInitialBStore;
    UINT16 OffsetPcrBStoreLimit;
    UINT16 OffsetPcrInitialStack;
    UINT16 OffsetPcrStackLimit;
    UINT16 OffsetPrcbPcrPage;
    UINT16 OffsetPrcbProcStateSpecialReg;
    UINT16 GdtR0Code;
    UINT16 GdtR0Data;
    UINT16 GdtR0Pcr;
    UINT16 GdtR3Code;
    UINT16 GdtR3Data;
    UINT16 GdtR3Teb;
    UINT16 GdtLdt;
    UINT16 GdtTss;
    UINT16 Gdt64R3CmCode;
    UINT16 Gdt64R3CmTeb;
    ULPTR64 IopNumTriageDumpDataBlocks;
    ULPTR64 IopTriageDumpDataBlocks;
#endif

#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULPTR64 VfCrashDataBlock;
    ULPTR64 MmBadPagesDetected;
    ULPTR64 MmZeroedPageSingleBitErrorsDetected;
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
    ULPTR64 EtwpDebuggerData;
    UINT16 OffsetPrcbContext;
#endif

#if (NTDDI_VERSION >= NTDDI_WIN8)
    UINT16 OffsetPrcbMaxBreakpoints;
    UINT16 OffsetPrcbMaxWatchpoints;
    UINT32 OffsetKThreadStackLimit;
    UINT32 OffsetKThreadStackBase;
    UINT32 OffsetKThreadQueueListEntry;
    UINT32 OffsetEThreadIrpList;
    UINT16 OffsetPrcbIdleThread;
    UINT16 OffsetPrcbNormalDpcState;
    UINT16 OffsetPrcbDpcStack;
    UINT16 OffsetPrcbIsrStack;
    UINT16 SizeKDPC_STACK_FRAME;
#endif

#if (NTDDI_VERSION >= NTDDI_WINBLUE) // NTDDI_WIN81.
    UINT16 OffsetKPriQueueThreadListHead;
    UINT16 OffsetKThreadWaitReason;
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
    UINT16 Padding;
    ULPTR64 PteBase;
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)
    ULPTR64 RetpolineStubFunctionTable;
    UINT32 RetpolineStubFunctionTableSize;
    UINT32 RetpolineStubOffset;
    UINT32 RetpolineStubSize;
#endif
} KDDEBUGGER_DATA64;

typedef struct _PHYSICAL_MEMORY_RUN64 {
    UINT64 BasePage;
    UINT64 PageCount;
} PHYSICAL_MEMORY_RUN64;

typedef struct _PHYSICAL_MEMORY_DESCRIPTOR64 {
    UINT32 NumberOfRuns;
    UINT8 PhysicalMemoryDescriptorPad0[4];
    UINT64 NumberOfPages;
    PHYSICAL_MEMORY_RUN64 Run[1];
} PHYSICAL_MEMORY_DESCRIPTOR64;

typedef union _DUMP_FILE_ATTRIBUTES {
    struct {
        UINT32 BitField;
    } DUMMYSTRUCTNAME;
    UINT32 Attributes;
} DUMP_FILE_ATTRIBUTES;

typedef struct _DUMP_HEADER64 {
    UINT32 Signature;
    UINT32 ValidDump;
    UINT32 MajorVersion;
    UINT32 MinorVersion;
    UINT64 DirectoryTableBase;
    UINT64 PfnDataBase;
    UINT64 PsLoadedModuleList;
    UINT64 PsActiveProcessHead;
    UINT32 MachineImageType;
    UINT32 NumberProcessors;
    UINT32 BugCheckCode;
    UINT8 DumpHeaderPad0[4];
    UINT64 BugCheckParameter1;
    UINT64 BugCheckParameter2;
    UINT64 BugCheckParameter3;
    UINT64 BugCheckParameter4;
    INT8 VersionUser[32];
    KDDEBUGGER_DATA64 *pKdDebuggerDataBlock;
    union {
        PHYSICAL_MEMORY_DESCRIPTOR64 PhysicalMemoryBlock;
        UINT8 PhysicalMemoryBlockBuffer[700];
    } DUMMYUNIONNAME;
    UINT8 ContextRecord[3000];
    UINT8 DumpHeaderPad1[4];
    EXCEPTION_RECORD64 Exception;
    UINT32 DumpType;
    UINT8 DumpHeaderPad2[4];
    INT64 RequiredDumpSpace;
    INT64 SystemTime;
    INT8 Comment[128];
    INT64 SystemUpTime;
    UINT32 MiniDumpFields;
    UINT32 SecondaryDataState;
    UINT32 ProductType;
    UINT32 SuiteMask;
    UINT32 WriterStatus;
    UINT8 Reserved0;
    UINT8 KdSecondaryVersion;
    UINT8 Reserved1[2];
    DUMP_FILE_ATTRIBUTES Attributes;
    UINT32 BootId;
    UINT8 Reserved2[4008];
} DUMP_HEADER64;

#define DUMP_BLOCK_SIZE 0x40000

typedef struct _AGENT_DATA64 {
    UINT64 Signature;
    UINT64 Reserved0;
    UINT64 Signature2;
    UINT64 DirectMapBase;
    UINT64 DirectMapPxeIdx;
    UINT64 DirectoryTableBase;
} AGENT_DATA64;

#define AGENT_DATA_SIGNATURE64 0xCCFC2B26AFA9AB55UI64
#define AGENT_DATA_SB_SIGNATURE64 0xFDDD4692BEB83017UI64
     
#pragma warning(pop)

extern
UINT32
FASTCALL
KeCapturePersistentThreadState(
    OUT CONTEXT *pContext,
    IN  PKTHREAD pThread,
    IN  UINT32 BugCheckCode,
    IN  UINT32 BugCheckParameter1,
    IN  UINT32 BugCheckParameter2,
    IN  UINT32 BugCheckParameter3,
    IN  UINT32 BugCheckParameter4,
    OUT DUMP_HEADER64 *pDumpHeader
);

#define DBG 0

#define DbgLog(Format, ...)    \
do {                        \
    if (DBG) {              \
        DbgPrintEx(         \
        DPFLTR_SYSTEM_ID,   \
        DPFLTR_ERROR_LEVEL, \
        Format,             \
        ##__VA_ARGS__);     \
    }                       \
} while (FALSE)

#define DBG_BREAK                          \
do {                                       \
    if (DBG) {                             \
       DbgLog("DebugBreak.\n");            \
                                           \
       DbgLog("FilePath: %s\n", __FILE__); \
                                           \
       DbgLog("FileLine: %d\n", __LINE__); \
                                           \
        __debugbreak();                    \
    }                                      \
} while (FALSE)

typedef UINT32 SB_STATUS;
#define SB_SUCCESS 0xFFFFFFFFUI32
#define SB_ABORTED 0x00000000UI32
#define SB_ERROR(Status) \
((Status) != SB_SUCCESS)

#define MMU_LONG_PAGING_LEVELS 4

#define PTE_BASE_IDX 0
#define PDE_BASE_IDX 1
#define PPE_BASE_IDX 2 
#define PXE_BASE_IDX 3

#define SYSTEM_VA_CANONICAL_MASK 0xFFFF000000000000UI64

#define EPROCESS_DTB_OFFSET 0x28 
#define EPOROCESS_ACTIVEPROCESSLINKS_OFFSET 0x1D8
#define EPROCESS_PEB_OFFSET 0x2E0
#define EPROCESS_IMAGEFILENAME_OFFSET 0x338

#define PEB_LDR_OFFSET 0x18

#define PEB_LDR_DATA_INLOADORDERMODULELIST_OFFSET 0x10

#define LDR_DATA_TABLE_ENTRY_DLLBASE_OFFSET 0x30
#define LDR_DATA_TABLE_ENTRY_SIZEOFIMAGE_OFFSET 0x40
#define LDR_DATA_TABLE_ENTRY_BASEDLLNAME_OFFSET 0x58

SB_STATUS
FASTCALL
SBStart(
    VOID
)
{
    CONTEXT *pThreadContext = NULL;
    DUMP_HEADER64 *pDumpHeader = NULL;

    KDDEBUGGER_DATA64 *pKdDataBlock = NULL;
    BOOLEAN FoundKdDataBlock = FALSE;

    UINT64 SelfRefPxeIdx = 0;

    MMPFN *pPfnDatabase = NULL;
    UINT64 PteBase[MMU_LONG_PAGING_LEVELS] = { 0 };

    VOID *pCurrentProcess = NULL;
    VOID *pCurrentPeb = NULL;
    VOID *pCurrentPebLdr = NULL;
    VOID *pCurrentLdrEntry = NULL;

    VOID *pAgentModuleBase = NULL;
    UINT32 SoAgentModule = 0;
    BOOLEAN FoundAgentModule = FALSE;

    AGENT_DATA64 *pAgentData = NULL;
    BOOLEAN FoundAgentData = FALSE;

    KAPC_STATE ThreadApcState = { 0 };
    BOOLEAN IsSwitchedSpace = FALSE;

    PEPROCESS pAgentProcess = NULL;

    UINT64 AgentPml4Va = 0;
    MMPTE_HARDWARE *pAgentPml4VaPte = NULL;
    UINT64 SaveAgentPml4VaPfn = 0;

    UINT64 AgentDirectMapPxeIdx = 0;
    BOOLEAN FoundAgentDirectMapPxeIdx = FALSE;

    MMPTE_HARDWARE *pDirectMapPdpt = NULL;
    UINT64 DirectMapPdptPfn = 0;

    UINT64 AgentCr3 = 0;

    BOOLEAN IsAborted = TRUE;

    if (!(pThreadContext =
        (CONTEXT*)
        ExAllocatePool2(
            POOL_FLAG_NON_PAGED,
            sizeof(CONTEXT),
            '768Q'))) {
        DBG_BREAK;
        goto aborted;
    }

    if (!(pDumpHeader =
        (DUMP_HEADER64*)
        ExAllocatePool2(
            POOL_FLAG_NON_PAGED,
            DUMP_BLOCK_SIZE,
            '768Q'))) {
        DBG_BREAK;
        goto aborted;
    }

    if (!(KeCapturePersistentThreadState(
        pThreadContext,
        NULL,
        0,
        0,
        0,
        0,
        0,
        pDumpHeader))) {
        DBG_BREAK;
        goto aborted;
    }

    pKdDataBlock =
        (KDDEBUGGER_DATA64*)
        (((UINT8*)pDumpHeader) + sizeof(DUMP_HEADER64));

    for (UINT64 i = 0; i < (DUMP_BLOCK_SIZE - sizeof(DUMP_HEADER64)); i++) {
        if ((pKdDataBlock->Header.OwnerTag == 'GBDK') &&
            (pKdDataBlock->MmPageSize == PAGE_SIZE)) {
            FoundKdDataBlock = TRUE;
            break;
        }
        pKdDataBlock =
            (KDDEBUGGER_DATA64*)
            (((UINT8*)pKdDataBlock) + i);
    }

    if (!FoundKdDataBlock) {
        DBG_BREAK;
        goto aborted;
    }
    
    pPfnDatabase = (MMPFN*)(*(VOID**)pKdDataBlock->MmPfnDatabase);

    PteBase[PTE_BASE_IDX] = pKdDataBlock->PteBase;

    SelfRefPxeIdx = (pKdDataBlock->PteBase & ~SYSTEM_VA_CANONICAL_MASK);

    for (UINT8 i = PDE_BASE_IDX; i < MMU_LONG_PAGING_LEVELS; i++) {
        PteBase[i] = (PteBase[i - 1] | (SelfRefPxeIdx >> (9 * i)));
    }
    
    pCurrentProcess =
        ((LIST_ENTRY*)(((UINT8*)PsInitialSystemProcess) +
            EPOROCESS_ACTIVEPROCESSLINKS_OFFSET))->Flink;
    pCurrentProcess =
        ((UINT8*)pCurrentProcess) -
        EPOROCESS_ACTIVEPROCESSLINKS_OFFSET;
    do {
        if (!(memcmp(
            (CONST VOID*)
            (((UINT8*)pCurrentProcess) + EPROCESS_IMAGEFILENAME_OFFSET),
            "SBAgent.exe",
            12)) &&
            (pCurrentPeb =
                *(VOID**)((UINT8*)pCurrentProcess + EPROCESS_PEB_OFFSET))) {
            KeStackAttachProcess(
                pCurrentProcess,
                &ThreadApcState);
            IsSwitchedSpace = TRUE;
            pCurrentPebLdr =
                *(VOID**)(((UINT8*)pCurrentPeb) + PEB_LDR_OFFSET);
            pCurrentLdrEntry =
                ((LIST_ENTRY*)(((UINT8*)pCurrentPebLdr) +
                    PEB_LDR_DATA_INLOADORDERMODULELIST_OFFSET))->Flink;
            do {
                if (!(memcmp(
                    ((UNICODE_STRING*)
                    ((UINT8*)pCurrentLdrEntry +
                        LDR_DATA_TABLE_ENTRY_BASEDLLNAME_OFFSET))->Buffer,
                    L"SBAgent.exe",
                    24))) {
                    FoundAgentModule = TRUE;
                    break;
                }
                pCurrentLdrEntry = ((LIST_ENTRY*)pCurrentLdrEntry)->Flink;
            } while ((UINT64)pCurrentLdrEntry !=
                (((UINT64)pCurrentPebLdr) +
                    PEB_LDR_DATA_INLOADORDERMODULELIST_OFFSET));
            if (FoundAgentModule) {
                pAgentModuleBase = *(VOID**)((UINT8*)pCurrentLdrEntry +
                    LDR_DATA_TABLE_ENTRY_DLLBASE_OFFSET);
                SoAgentModule = *(UINT32*)((UINT8*)pCurrentLdrEntry +
                    LDR_DATA_TABLE_ENTRY_SIZEOFIMAGE_OFFSET);
                break;
            }
            KeUnstackDetachProcess(
                &ThreadApcState
            );
            IsSwitchedSpace = FALSE;
        }  
        pCurrentProcess = 
            ((LIST_ENTRY*)(((UINT8*)pCurrentProcess) +
            EPOROCESS_ACTIVEPROCESSLINKS_OFFSET))->Flink;
        pCurrentProcess =
            ((UINT8*)pCurrentProcess) - 
            EPOROCESS_ACTIVEPROCESSLINKS_OFFSET;
    } while ((UINT64)pCurrentProcess != (UINT64)PsInitialSystemProcess);

    if (!FoundAgentModule) {
        DBG_BREAK;
        goto aborted;
    }

    pAgentProcess = pCurrentProcess;

    pAgentData = (AGENT_DATA64*)pAgentModuleBase;

    for (UINT64 i = 0; i < (((UINT64)SoAgentModule) - 8); i++) {
        if (pAgentData->Signature == AGENT_DATA_SIGNATURE64 &&
            pAgentData->Signature2 == AGENT_DATA_SIGNATURE64) {
            FoundAgentData = TRUE;
            break;
        }
        pAgentData = (AGENT_DATA64*)((UINT8*)pAgentData + 1);
    }

    if (!FoundAgentData) {
        DBG_BREAK;
        goto aborted;
    }

    if (!(AgentPml4Va =
        (UINT64)
        MmAllocateContiguousMemory(
            PAGE_SIZE,
            (PHYSICAL_ADDRESS){MAXUINT64}))) {
        DBG_BREAK;
        goto aborted;
    }

    if (!(pDirectMapPdpt =
        (MMPTE_HARDWARE*)
        MmAllocateContiguousMemory(
            PAGE_SIZE,
            (PHYSICAL_ADDRESS){MAXUINT64}))) {
        DBG_BREAK;
        goto aborted;
    }

    AgentPml4Va &= ~SYSTEM_VA_CANONICAL_MASK;

    for (UINT8 i = MMU_LONG_PAGING_LEVELS,
        i2 = PXE_BASE_IDX; i; i--, i2--) {
        pAgentPml4VaPte =
            (MMPTE_HARDWARE*)
            (((AgentPml4Va >> (9 * i)) & 0xFFFFFFFFFFF8) | PteBase[i2]);
        if (!pAgentPml4VaPte->Valid) {
            DBG_BREAK;
            goto aborted;
        } else if (pAgentPml4VaPte->LargePage) {
            break;
        }
    }

    AgentPml4Va |= SYSTEM_VA_CANONICAL_MASK;

    for (UINT16 i = 0; i < 512; i++) {
        *((UINT64*)(pDirectMapPdpt + i)) = 0;
        (pDirectMapPdpt + i)->Valid = 1;
        (pDirectMapPdpt + i)->Dirty1 = 1;
        (pDirectMapPdpt + i)->Owner = 1;
        (pDirectMapPdpt + i)->LargePage = 1;
        (pDirectMapPdpt + i)->PageFrameNumber = (i << 18);
        (pDirectMapPdpt + i)->NoExecute = 0;
    }

    if (!(DirectMapPdptPfn = 
        ((MmGetPhysicalAddress(pDirectMapPdpt)).QuadPart))) {
        DBG_BREAK;
        goto aborted;
    }

    DirectMapPdptPfn >>= PAGE_SHIFT;

    _InterlockedIncrement16(
        (volatile SHORT*)&(pPfnDatabase +
            DirectMapPdptPfn)->u3.ReferenceCount);

    AgentCr3 =
        *(UINT64*)(((UINT8*)pAgentProcess) +
            EPROCESS_DTB_OFFSET);

    _disable();

    SaveAgentPml4VaPfn = pAgentPml4VaPte->PageFrameNumber;

    pAgentPml4VaPte->PageFrameNumber = (AgentCr3 >> PAGE_SHIFT);

    __invlpg((VOID*)AgentPml4Va);

    for (UINT8 i = 255, i2 = 0; i2 < 256; i--, i2++) {
        if (!*(((UINT64*)AgentPml4Va) + i)) {
            AgentDirectMapPxeIdx = i;
            FoundAgentDirectMapPxeIdx = TRUE;
            break;
        }
    }

    if (!FoundAgentDirectMapPxeIdx) {
        pAgentPml4VaPte->PageFrameNumber = SaveAgentPml4VaPfn;
        __invlpg((VOID*)AgentPml4Va);
        _enable();
        DBG_BREAK;
        goto aborted;
    }

    AgentPml4Va += 
        (AgentDirectMapPxeIdx * sizeof(MMPTE_HARDWARE));

    *((UINT64*)AgentPml4Va) = 0;
    ((MMPTE_HARDWARE*)AgentPml4Va)->Valid = 1;
    ((MMPTE_HARDWARE*)AgentPml4Va)->Dirty1 = 1;
    ((MMPTE_HARDWARE*)AgentPml4Va)->Owner = 1;
    ((MMPTE_HARDWARE*)AgentPml4Va)->PageFrameNumber = DirectMapPdptPfn;
    ((MMPTE_HARDWARE*)AgentPml4Va)->NoExecute = 1;

    AgentPml4Va -=
        (AgentDirectMapPxeIdx * sizeof(MMPTE_HARDWARE));

    pAgentPml4VaPte->PageFrameNumber = SaveAgentPml4VaPfn;

    __invlpg((VOID*)AgentPml4Va);

    __writecr0(__readcr0() & ~0x10000);

    pAgentData->Signature = AGENT_DATA_SB_SIGNATURE64;
    pAgentData->Signature2 = AGENT_DATA_SB_SIGNATURE64;
    pAgentData->DirectMapBase = (AgentDirectMapPxeIdx << 39);
    pAgentData->DirectMapPxeIdx = AgentDirectMapPxeIdx;
    pAgentData->DirectoryTableBase = (AgentCr3 & ~(0x0FFF));

    __writecr0(__readcr0() | 0x10000);

    _enable();

    IsAborted = FALSE;

aborted:

    if (IsSwitchedSpace) {
        KeUnstackDetachProcess(
            &ThreadApcState
        );
        IsSwitchedSpace = FALSE;
    }

    if (pThreadContext) {
        RtlSecureZeroMemory(
            pThreadContext,
            sizeof(CONTEXT));
        ExFreePoolWithTag(
            pThreadContext,
            '768Q');
    }

    if (pDumpHeader) {
        RtlSecureZeroMemory(
            pDumpHeader,
            DUMP_BLOCK_SIZE);
        ExFreePoolWithTag(
            pDumpHeader,
            '768Q');
    }

    if (AgentPml4Va) {
        MmFreeContiguousMemory((VOID*)AgentPml4Va);
    }

    if (IsAborted) {
        if (pDirectMapPdpt) {
            if (DirectMapPdptPfn) {
                _InterlockedDecrement16(
                    (volatile SHORT*)&(pPfnDatabase +
                        DirectMapPdptPfn)->u3.ReferenceCount);
            }
            MmFreeContiguousMemory(pDirectMapPdpt);
        }
        return SB_ABORTED;
    }

    return SB_SUCCESS;
}

