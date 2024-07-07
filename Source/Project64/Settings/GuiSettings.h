#pragma once

class CGuiSettings
{
protected:
	CGuiSettings();
	virtual ~CGuiSettings();
	
	static inline bool bCPURunning ( void) { return m_bCPURunning; }
	static inline bool bAutoSleep  ( void) { return m_bAutoSleep;  }
	static inline bool bMinimizedSleep(void) { return m_bMinimizedSleep; }
	static inline bool bDarkTheme(void) { return m_bDarkTheme; }
	static inline bool bGlobalCheats(void) { return m_bGlobalCheats; }

private:
	static void RefreshSettings (void *);

	static bool m_bCPURunning;
	static bool m_bAutoSleep;
	static bool m_bMinimizedSleep;
	static bool m_bDarkTheme;
	static bool m_bGlobalCheats;
	
	static int  m_RefCount;

};
