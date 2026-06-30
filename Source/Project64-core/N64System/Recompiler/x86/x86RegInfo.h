#pragma once

#if defined(__i386__) || defined(_M_IX86)
#include <Project64-core/N64System/Recompiler/RegBase.h>
#include <Project64-core/N64System/Recompiler/x86/x86ops.h>
#include <Project64-core/N64System/Mips/Register.h>
#include <Project64-core/Settings/DebugSettings.h>

class CX86RegInfo :
    public CRegBase,
    private CDebugSettings,
    private CX86Ops,
    private CSystemRegisters
{
public:
    // Enums
    enum REG_MAPPED
    {
        NotMapped = 0,
        GPR_Mapped = 1,
        Temp_Mapped = 2,
        Stack_Mapped = 3,
    };

    enum FPU_STATE
    {
        FPU_Any = -1,
        FPU_Unknown = 0,
        FPU_Dword = 1,
        FPU_Qword = 2,
        FPU_Float = 3,
        FPU_Double = 4,
    };

public:
    enum XMM_FPR_FORMAT
    {
        XMM_FMT_UNKNOWN = 0,
        XMM_FMT_FLOAT = 1,
        XMM_FMT_DOUBLE = 2,
    };

    CX86RegInfo();
    CX86RegInfo(const CX86RegInfo&);
    ~CX86RegInfo();

    CX86RegInfo& operator=(const CX86RegInfo&);

    bool operator==(const CX86RegInfo& right) const;
    bool operator!=(const CX86RegInfo& right) const;

    static REG_STATE ConstantsType(int64_t Value);

    void BeforeCallDirect(void);
    void AfterCallDirect(void);

    void   FixSseRoundModelDefault();
    void   FixRoundModel(FPU_ROUND RoundMethod);
    void   ChangeFPURegFormat(int32_t Reg, FPU_STATE OldFormat, FPU_STATE NewFormat, FPU_ROUND RoundingModel);
    void   Load_FPR_ToTop(int32_t Reg, int32_t RegToLoad, FPU_STATE Format);
    bool   RegInStack(int32_t Reg, FPU_STATE Format);
    x86XmmReg Map_FPR_ToXmm(int32_t Reg, FPU_STATE Format);
    x86XmmReg Map_FPR_ToXmmWrite(int32_t Reg, FPU_STATE Format);
    void   FlushXmmCache();
    void   UnMap_AllFPRs();
    void   UnMap_FPR(int32_t Reg, bool WriteBackValue);
    x86FpuValues StackPosition(int32_t Reg);

    x86Reg FreeX86Reg();
    x86Reg Free8BitX86Reg();
    void   Map_GPR_32bit(int32_t MipsReg, bool SignValue, int32_t MipsRegToLoad);
    void   Map_GPR_64bit(int32_t MipsReg, int32_t MipsRegToLoad);
    x86Reg Get_MemoryStack() const;
    x86Reg Map_MemoryStack(x86Reg Reg, bool bMapRegister, bool LoadValue = true);
    x86Reg Map_TempReg(x86Reg Reg, int32_t MipsReg, bool LoadHiWord);
    void   ProtectGPR(uint32_t Reg);
    void   UnProtectGPR(uint32_t Reg);
    void   ResetX86Protection();
    x86Reg UnMap_TempReg();
    void   UnMap_GPR(uint32_t Reg, bool WriteBackValue);
    bool   UnMap_X86reg(x86Reg Reg);
    void   WriteBackRegisters();

    CX86Ops::x86Reg   GetMipsRegMapLo(int32_t Reg) const { return m_RegMapLo[Reg]; }
    CX86Ops::x86Reg   GetMipsRegMapHi(int32_t Reg) const { return m_RegMapHi[Reg]; }

    uint32_t             GetX86MapOrder(x86Reg Reg) const { return m_x86reg_MapOrder[Reg]; }
    bool              GetX86Protected(x86Reg Reg) const { return m_x86reg_Protected[Reg]; }
    REG_MAPPED        GetX86Mapped(x86Reg Reg) const { return m_x86reg_MappedTo[Reg]; }

    void              SetMipsRegMapLo(int32_t GetMipsReg, x86Reg Reg)      { m_RegMapLo[GetMipsReg] = Reg; }
    void              SetMipsRegMapHi(int32_t GetMipsReg, x86Reg Reg)      { m_RegMapHi[GetMipsReg] = Reg; }

    void              SetX86MapOrder(x86Reg Reg, uint32_t Order)         { m_x86reg_MapOrder[Reg] = Order; }
    void              SetX86Protected(x86Reg Reg, bool Protected)      { m_x86reg_Protected[Reg] = Protected; }
    void              SetX86Mapped(x86Reg Reg, REG_MAPPED Mapping)  { m_x86reg_MappedTo[Reg] = Mapping; }

    int32_t & StackTopPos() { return m_Stack_TopPos; }
    int32_t & FpuMappedTo(int32_t Reg) { return m_x86fpu_MappedTo[Reg]; }
    FPU_STATE & FpuState(int32_t Reg) { return m_x86fpu_State[Reg]; }
    FPU_ROUND & FpuRoundingModel(int32_t Reg) { return m_x86fpu_RoundingModel[Reg]; }

private:
    x86Reg UnMap_8BitTempReg();
    XMM_FPR_FORMAT ToXmmFormat(FPU_STATE Format) const;
    int32_t AllocXmmSlot();
    void TouchXmmSlot(int32_t Slot);
    void ClearXmmSlot(int32_t Slot);

    // r4k
    x86Reg      m_RegMapHi[32];
    x86Reg      m_RegMapLo[32];

    REG_MAPPED  m_x86reg_MappedTo[10];
    uint32_t    m_x86reg_MapOrder[10];
    bool        m_x86reg_Protected[10];

    // FPU
    int32_t     m_Stack_TopPos;
    int32_t     m_x86fpu_MappedTo[8];
    FPU_STATE   m_x86fpu_State[8];
    bool        m_x86fpu_StateChanged[8];
    FPU_ROUND   m_x86fpu_RoundingModel[8];

    int32_t     m_xmm_MappedToFpr[8];
    XMM_FPR_FORMAT m_xmm_Format[8];
    uint32_t    m_xmm_Age[8];
    uint32_t    m_xmm_AgeCounter;
    int32_t     m_fpr_MappedToXmm[32];
    XMM_FPR_FORMAT m_fpr_XmmFormat[32];

    static uint32_t m_fpuControl;
    static uint32_t m_mxcsrControl;
};
#endif
