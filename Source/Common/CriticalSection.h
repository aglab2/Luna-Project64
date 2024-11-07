#pragma once

class CriticalSection
{
public:
	CriticalSection();
	~CriticalSection(void);

	void enter(void);
	void leave(void);

private:
	CriticalSection(const CriticalSection&);
	CriticalSection& operator=(const CriticalSection&);

	void * m_cs;
};

class CGuard
{
public:
	CGuard(CriticalSection& sectionName) : m_cs(sectionName)
	{
		m_cs.enter();
	}
	~CGuard()
	{
		m_cs.leave();
	}
private:
	CriticalSection& m_cs;
	CGuard(const CGuard& copy);
	CGuard &operator=(const CGuard& rhs);
};

class CUniqueLock
{
public:
	CUniqueLock(CriticalSection& sectionName) : m_cs(sectionName)
	{
		m_cs.enter();
	}
	~CUniqueLock()
	{
		if (m_locked)
			m_cs.leave();
	}

	void lock()
	{
		if (!m_locked)
		{
			m_cs.enter();
			m_locked = true;
		}
	}

	void unlock()
	{
		if (m_locked)
		{
			m_cs.leave();
			m_locked = false;
		}
	}

private:
	bool m_locked = true;
	CriticalSection& m_cs;
	CUniqueLock(const CUniqueLock& copy);
	CUniqueLock& operator=(const CUniqueLock& rhs);
};

#include <condition_variable>
#include <mutex>

class CEvent
{
public:
	void signal(void)
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_active = true;
		}
		m_cv.notify_all();
	}
	void wait(void)
	{
		if (m_active.load())
			return;

		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv.wait(lock, [this] { return m_active.load(); });
	}

private:
	std::atomic_bool m_active{ false };
	std::mutex m_mutex;
	std::condition_variable m_cv;
};
