#pragma once

#ifndef CALL
#ifdef _WIN32
#define CALL __cdecl
#else
#define CALL
#endif
#endif

class CNotificationSettings
{
    static void CALL StaticRefreshSettings(CNotificationSettings * _this)
    {
        _this->RefreshSettings();
    }

    void RefreshSettings(void);

    static bool m_bInFullScreen;

protected:
    CNotificationSettings();
    virtual ~CNotificationSettings();

    void RegisterNotifications(void);
    inline bool InFullScreen(void) const { return m_bInFullScreen; }
};
