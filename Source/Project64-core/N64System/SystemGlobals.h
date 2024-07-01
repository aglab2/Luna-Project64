#pragma once

#include <Project64-core/CoreApi.h>

class CSettings;
extern COREAPI CSettings * g_Settings;

class CN64System;
extern COREAPI CN64System * g_System;
extern COREAPI CN64System * g_BaseSystem;
extern COREAPI CN64System * g_SyncSystem;

class CRecompiler;
extern CRecompiler * g_Recompiler;

class CMipsMemoryVM;
extern COREAPI CMipsMemoryVM * g_MMU; // Memory of the N64

class CTLB;
extern COREAPI CTLB * g_TLB; // TLB unit

class CRegisters;
extern COREAPI CRegisters * g_Reg; // Current register set attached to the g_MMU

class CPlugins;
extern COREAPI CPlugins * g_Plugins;

class CN64Rom;
extern COREAPI CN64Rom * g_Rom;      // The current ROM that this system is executing, it can only execute one file at the time
extern COREAPI CN64Rom * g_DDRom;    // 64DD IPL ROM

class CN64Disk;
extern COREAPI CN64Disk * g_Disk;     // 64DD disk

class CAudio;
extern COREAPI CAudio * g_Audio;

class CSystemTimer;
extern CSystemTimer * g_SystemTimer;

__interface CTransVaddr;
extern CTransVaddr * g_TransVaddr;

class CSystemEvents;
extern CSystemEvents * g_SystemEvents;

extern int32_t * g_NextTimer;
extern uint32_t * g_TLBLoadAddress;
extern uint32_t * g_TLBStoreAddress;

__interface CDebugger;
extern COREAPI CDebugger * g_Debugger;

extern uint8_t ** g_RecompPos;

class CMempak;
extern CMempak * g_Mempak;

class CRandom;
extern CRandom * g_Random;

class CEnhancements;
extern COREAPI CEnhancements * g_Enhancements;

class CSummerCart;
extern CSummerCart * g_SummerCart;

class InputDelayer;
extern InputDelayer * g_InputDelayer;

extern bool g_InputDelayerIncompatibleSettingsNotified;
