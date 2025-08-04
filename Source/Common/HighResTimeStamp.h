#pragma once
#include <stdint.h>
#include <Windows.h>

extern uint64_t m_Freq;

class HighResTimeStamp
{
public:
    HighResTimeStamp& SetToNow(void)
    {
        LARGE_INTEGER value;
        QueryPerformanceCounter(&value);
        m_time = value.QuadPart;
        return *this;
    }

    uint64_t GetMicroSeconds(void) const
    {
        return (m_time * 1000000ULL) / m_Freq;
    }

    uint64_t Get100NanoSeconds(void) const
    {
        return (m_time * 10000000ULL) / m_Freq;
	}

    void SetMicroSeconds(uint64_t MicroSeconds)
    {
        m_time = (MicroSeconds * m_Freq) / 1000000ULL;
    }


    HighResTimeStamp operator+(const HighResTimeStamp& Other) const
    {
        HighResTimeStamp Result;
        Result.m_time = m_time + Other.m_time;
        return Result;
    }

    HighResTimeStamp operator-(const HighResTimeStamp& Other) const
    {
        HighResTimeStamp Result;
		Result.m_time = m_time - Other.m_time;
		return Result;
    }
    HighResTimeStamp& operator+=(const HighResTimeStamp& Other)
	{
		m_time += Other.m_time;
		return *this;
	}

    HighResTimeStamp& operator-=(const HighResTimeStamp& Other)
    {
        m_time -= Other.m_time;
        return *this;
    }

    HighResTimeStamp operator*(const uint64_t Multiplier) const
    {
        HighResTimeStamp Result;
        Result.m_time = m_time * Multiplier;
        return Result;
	}

    bool operator<(const HighResTimeStamp& Other) const
    {
        return m_time < Other.m_time;
	}

    uint64_t Raw() const
    {
		return m_time;
    }

#ifdef _WIN32
    static void Init();
#endif

private:
    uint64_t m_time = 0;
};
