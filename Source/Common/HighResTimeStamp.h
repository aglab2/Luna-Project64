#pragma once
#include <stdint.h>

class HighResTimeStamp
{
public:
    HighResTimeStamp & SetToNow (void);
    uint64_t GetMicroSeconds(void);
    void SetMicroSeconds(uint64_t MicroSeconds);

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

#ifdef _WIN32
    static void Init();
#endif

private:
    uint64_t m_time = 0;
};
