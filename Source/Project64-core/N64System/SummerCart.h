#include <stddef.h>
#include <stdint.h>

#include <string>

class CSummerCart
{
public:
    static void MakeInitialImage();

    CSummerCart();
    ~CSummerCart();

    int ReadRegs(uint32_t address, uint32_t* value);
    int WriteRegs(uint32_t address, uint32_t value, uint32_t mask);

    bool Unlocked() const { return m_Unlock; }
    uint8_t* Buffer() { return m_Buffer; }

    static const std::wstring& SdPath() { return s_SdPath; }
    static const std::wstring& VhdPath() { return s_VhdPath; }

private:
    static std::wstring s_SdPath;
    static std::wstring s_VhdPath;

    FILE* m_SdFile;
    uint8_t m_Buffer[8192];
    long m_SdSize;
    uint32_t m_Status;
    uint32_t m_Data0;
    uint32_t m_Data1;
    uint32_t m_SdSector;
    char m_CfgRomWrite;
    char m_SdByteswap;
    char m_Unlock;
    char m_LockSeq;
    bool m_WarnedFailedToOpen = false;

    uint32_t Init();
    uint32_t Read();
    uint32_t Write();
    uint8_t* SDAddr(size_t size);
};
