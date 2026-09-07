#include "windows.h"
#include "stdio.h"

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

AGENT_DATA64 g_AgentData = { 0 };

UINT64
__fastcall
ReadPhysicalQword(
    UINT64 Pa
)
{
    return *(UINT64*)(Pa + g_AgentData.DirectMapBase);
}

VOID
__fastcall
WritePhysicalQword(
    UINT64 Pa,
    UINT64 Qword
)
{
    *(UINT64*)(Pa + g_AgentData.DirectMapBase) = Qword;

    return;
}

VOID
main(
    VOID
) 
//
// Example.
//
{
    g_AgentData.Signature = AGENT_DATA_SIGNATURE64;
    g_AgentData.Signature2 = AGENT_DATA_SIGNATURE64;

    printf_s("SBAgent start...\n");

    printf_s("Waiting for SB signature...\n");

    while (g_AgentData.Signature != AGENT_DATA_SB_SIGNATURE64 &&
        g_AgentData.Signature2 != AGENT_DATA_SB_SIGNATURE64) {
        Sleep(1000);
    }

    printf_s("SB signature found.\n");

    printf_s("Direct map has been received.\n");

    printf_s("PML4 dump...\n");
     
    for (UINT16 i = 0; i < 512; i++) {
        printf_s(
            "[PXE 0x%02X]: 0x%I64X\n",
            i,
            ReadPhysicalQword(g_AgentData.DirectoryTableBase + (i * 8)));
    }

    printf_s("Removing direct map...\n");

    WritePhysicalQword(
        g_AgentData.DirectoryTableBase + (g_AgentData.DirectMapPxeIdx * 8),
        0);

    printf_s("10 minute delay before exiting...\n");

    Sleep(600000);

    return;
}