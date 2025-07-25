#include "stdafx.h"
#include "SettingsType-UserNoteDatabase.h"

CIniFile * CSettingTypeUserNoteDatabase::m_UserNoteIniFile = nullptr;
std::string * CSettingTypeUserNoteDatabase::m_SectionIdent = nullptr;

CSettingTypeUserNoteDatabase::CSettingTypeUserNoteDatabase(const char * Name, uint32_t DefaultValue, bool DeleteOnDefault) :
    m_KeyName(Name),
    m_DefaultStr(""),
    m_DefaultValue(DefaultValue),
    m_DefaultSetting(Default_Constant),
    m_DeleteOnDefault(DeleteOnDefault)
{
}

CSettingTypeUserNoteDatabase::CSettingTypeUserNoteDatabase(const char * Name, bool DefaultValue, bool DeleteOnDefault) :
    m_KeyName(Name),
    m_DefaultStr(""),
    m_DefaultValue(DefaultValue),
    m_DefaultSetting(Default_Constant),
    m_DeleteOnDefault(DeleteOnDefault)
{
}

CSettingTypeUserNoteDatabase::CSettingTypeUserNoteDatabase(const char * Name, const char * DefaultValue, bool DeleteOnDefault) :
    m_KeyName(Name),
    m_DefaultStr(DefaultValue),
    m_DefaultValue(0),
    m_DefaultSetting(Default_Constant),
    m_DeleteOnDefault(DeleteOnDefault)
{
}

CSettingTypeUserNoteDatabase::CSettingTypeUserNoteDatabase(const char * Name, SettingID DefaultSetting, bool DeleteOnDefault) :
    m_KeyName(Name),
    m_DefaultStr(""),
    m_DefaultValue(0),
    m_DefaultSetting(DefaultSetting),
    m_DeleteOnDefault(DeleteOnDefault)
{
}

CSettingTypeUserNoteDatabase::~CSettingTypeUserNoteDatabase()
{
}

void CSettingTypeUserNoteDatabase::Initialize(void)
{
    WriteTrace(TraceAppInit, TraceDebug, "Start");

    m_UserNoteIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_Notes).c_str());

    g_Settings->RegisterChangeCB(Game_IniKey, nullptr, GameChanged);
    g_Settings->RegisterChangeCB(Cmd_BaseDirectory, nullptr, BaseDirChanged);

    m_SectionIdent = new stdstr(g_Settings->LoadStringVal(Game_IniKey));
    WriteTrace(TraceAppInit, TraceDebug, "Done");
}

void CSettingTypeUserNoteDatabase::CleanUp(void)
{
    g_Settings->UnregisterChangeCB(Cmd_BaseDirectory, nullptr, BaseDirChanged);
    g_Settings->UnregisterChangeCB(Game_IniKey, nullptr, GameChanged);
    if (m_UserNoteIniFile)
    {
        delete m_UserNoteIniFile;
        m_UserNoteIniFile = nullptr;
    }
    if (m_SectionIdent)
    {
        delete m_SectionIdent;
        m_SectionIdent = nullptr;
    }
}

void CALL CSettingTypeUserNoteDatabase::BaseDirChanged(void * /*Data */)
{
    if (m_UserNoteIniFile)
    {
        delete m_UserNoteIniFile;
        m_UserNoteIniFile = nullptr;
    }
    m_UserNoteIniFile = new CIniFile(g_Settings->LoadStringVal(SupportFile_Notes).c_str());
}

void CALL CSettingTypeUserNoteDatabase::GameChanged(void * /*Data */)
{
    if (m_SectionIdent)
    {
        *m_SectionIdent = g_Settings->LoadStringVal(Game_IniKey);
    }
}

bool CSettingTypeUserNoteDatabase::Load(uint32_t & Value) const
{
    return m_UserNoteIniFile->GetNumber(Section(), m_KeyName.c_str(), Value, Value);
}

bool CSettingTypeUserNoteDatabase::Load(uint32_t Index, bool & Value) const
{
    uint32_t temp_value = Value;
	if (Load(temp_value))
	{
		Value = temp_value != 0;
		return true;
	}
	LoadDefault(Index, Value);
	return false;
}

bool CSettingTypeUserNoteDatabase::Load(uint32_t Index, uint32_t & Value) const
{
    if (!Load(Value))
    {
        LoadDefault(Index, Value);
		return false;
	}
    return true;
}

bool CSettingTypeUserNoteDatabase::Load(uint32_t Index, std::string & Value) const
{
    stdstr temp_value;
    bool bRes = m_UserNoteIniFile->GetString(Section(), m_KeyName.c_str(), m_DefaultStr, temp_value);
    if (bRes)
    {
        Value = temp_value;
    }
    else
    {
        LoadDefault(Index, Value);
    }
    return bRes;
}

// Return the default values
void CSettingTypeUserNoteDatabase::LoadDefault(uint32_t /*Index*/, bool & Value) const
{
    if (m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultValue != 0;
        }
        else {
            g_Settings->LoadBool(m_DefaultSetting, Value);
        }
    }
}

void CSettingTypeUserNoteDatabase::LoadDefault(uint32_t /*Index*/, uint32_t & Value) const
{
    if (m_DefaultSetting != Default_None)
    {
        Value = m_DefaultSetting == Default_Constant ? m_DefaultValue : g_Settings->LoadDword(m_DefaultSetting);
    }
}

void CSettingTypeUserNoteDatabase::LoadDefault(uint32_t /*Index*/, std::string & Value) const
{
    if (m_DefaultSetting != Default_None)
    {
        if (m_DefaultSetting == Default_Constant)
        {
            Value = m_DefaultStr;
        }
        else {
            g_Settings->LoadStringVal(m_DefaultSetting, Value);
        }
    }
}

// Update the settings
void CSettingTypeUserNoteDatabase::Save(uint32_t /*Index*/, bool Value)
{
    if (m_DeleteOnDefault)
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);
    }

	m_UserNoteIniFile->SaveNumber(Section(), m_KeyName.c_str(), Value);
}

void CSettingTypeUserNoteDatabase::Save(uint32_t Index, uint32_t Value)
{
    if (m_DeleteOnDefault)
    {
        uint32_t defaultValue = 0;
        LoadDefault(Index, defaultValue);
        if (defaultValue == Value)
        {
            Delete(Index);
            return;
        }
    }
    m_UserNoteIniFile->SaveNumber(Section(), m_KeyName.c_str(), Value);
}

void CSettingTypeUserNoteDatabase::Save(uint32_t /*Index*/, const std::string & Value)
{
    m_UserNoteIniFile->SaveString(Section(), m_KeyName.c_str(), Value.c_str());
}

void CSettingTypeUserNoteDatabase::Save(uint32_t /*Index*/, const char * Value)
{
    m_UserNoteIniFile->SaveString(Section(), m_KeyName.c_str(), Value);
}

void CSettingTypeUserNoteDatabase::Delete(uint32_t /*Index*/)
{
    m_UserNoteIniFile->SaveString(Section(), m_KeyName.c_str(), nullptr);
}
