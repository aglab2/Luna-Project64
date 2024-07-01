#pragma once
#include <Common/path.h>
#include <Project64-core/Settings/SettingType/SettingsType-Base.h>
#include <shlobj.h>

class COREAPI CSettingTypeAppdataPath :
    public CSettingType
{
public:
    CSettingTypeAppdataPath(const char * Directory, const char * FileName);
    ~CSettingTypeAppdataPath();

    bool IndexBasedSetting (void) const { return false; }
    SettingType GetSettingType (void) const { return SettingType_AppdataPath; }
    bool IsSettingSet (void) const { return false; }

    //return the values
    bool Load (uint32_t /*Index*/, bool & /*Value*/) const { return false; };
    bool Load (uint32_t /*Index*/, uint32_t & /*Value*/) const { return false; };
    bool Load (uint32_t Index, std::string & Value) const;

    //return the default values
    void LoadDefault (uint32_t Index, bool & Value) const;
    void LoadDefault (uint32_t Index, uint32_t & Value) const;
    void LoadDefault (uint32_t Index, std::string & Value) const;

    //Update the settings
    void Save (uint32_t Index, bool Value);
    void Save (uint32_t Index, uint32_t Value);
    void Save (uint32_t Index, const std::string & Value);
    void Save (uint32_t Index, const char * Value);

    // Delete the setting
    void Delete (uint32_t Index );

private:
    CSettingTypeAppdataPath(void);
    CSettingTypeAppdataPath(const CSettingTypeAppdataPath&);
    CSettingTypeAppdataPath& operator=(const CSettingTypeAppdataPath&);

    static void RefreshSettings(void * _this);
    void BuildPath ( void );

    std::string m_FullPath;
    std::string m_Directory;
    std::string m_FileName;
};
