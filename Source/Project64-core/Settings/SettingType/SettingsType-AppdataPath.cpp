#include "stdafx.h"
#include "SettingsType-AppdataPath.h"

CSettingTypeAppdataPath::CSettingTypeAppdataPath(const char * Directory, const char * FileName) :
    m_Directory(Directory),
    m_FileName(FileName)
{
    BuildPath();
    g_Settings->RegisterChangeCB(Cmd_AppdataDirectory, this, RefreshSettings);
}

CSettingTypeAppdataPath::~CSettingTypeAppdataPath(void)
{
    g_Settings->UnregisterChangeCB(Cmd_AppdataDirectory, this, RefreshSettings);
}

bool CSettingTypeAppdataPath::Load(uint32_t /*Index*/, std::string & value) const
{
    value = m_FullPath;
    return true;
}

// Return the default values
void CSettingTypeAppdataPath::LoadDefault(uint32_t /*Index*/, bool & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeAppdataPath::LoadDefault(uint32_t /*Index*/, uint32_t & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeAppdataPath::LoadDefault(uint32_t /*Index*/, std::string & /*Value*/) const
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeAppdataPath::Save(uint32_t /*Index*/, bool /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeAppdataPath::Save(uint32_t /*Index*/, uint32_t /*Value*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeAppdataPath::Save(uint32_t /*Index*/, const std::string & Value)
{
    m_Directory = "";
    m_FileName = Value;
    BuildPath();
}

void CSettingTypeAppdataPath::Save(uint32_t /*Index*/, const char * Value)
{
    m_Directory = "";
    m_FileName = Value;
    BuildPath();
}

void CSettingTypeAppdataPath::Delete(uint32_t /*Index*/)
{
    g_Notify->BreakPoint(__FILE__, __LINE__);
}

void CSettingTypeAppdataPath::BuildPath(void)
{
    CPath FullPath(g_Settings->LoadStringVal(Cmd_AppdataDirectory).c_str(), "");
    FullPath.AppendDirectory(m_Directory.c_str());
    FullPath.SetNameExtension(m_FileName.c_str());
    m_FullPath = (const char*)FullPath;
}

void CSettingTypeAppdataPath::RefreshSettings(void * _this)
{
    ((CSettingTypeAppdataPath *)_this)->BuildPath();
}
