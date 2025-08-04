#include "HighResTimeStamp.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <math.h>
#endif

#include <cstdint>

#ifdef _WIN32
uint64_t m_Freq;

void HighResTimeStamp::Init()
{
    LARGE_INTEGER value;
    QueryPerformanceFrequency(&value);
    m_Freq = value.QuadPart;
}
#endif
