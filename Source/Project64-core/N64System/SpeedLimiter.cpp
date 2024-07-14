#include "stdafx.h"
#include "Project64-core/N64System/SpeedLimiter.h"

#include <Common/Util.h>

#ifdef _WIN32
#pragma comment(lib, "winmm.lib")

#include <timeapi.h>

typedef LONG(NTAPI* pSetTimerResolution)(ULONG RequestedResolution, BOOLEAN Set, PULONG ActualResolution);
typedef LONG(NTAPI* pQueryTimerResolution)(PULONG MinimumResolution, PULONG MaximumResolution, PULONG CurrentResolution);

static void setAccurateTimerResolution()
{
    timeBeginPeriod(1);

    LONG status;
    pSetTimerResolution setFunction;
    pQueryTimerResolution queryFunction;
    ULONG minResolution, maxResolution, actualResolution;
    const HINSTANCE hLibrary = LoadLibrary(L"NTDLL.dll");
    if (hLibrary == NULL)
    {
        return;
    }

    queryFunction = (pQueryTimerResolution)GetProcAddress(hLibrary, "NtQueryTimerResolution");
    if (queryFunction == NULL)
    {
        return;
    }

    queryFunction(&minResolution, &maxResolution, &actualResolution);
    {
        wchar_t line[256];
        wsprintf(line, L"Win32 Timer Resolution:\n\tMinimum Value:\t%u\n\tMaximum Value:\t%u\n\tActual Value:\t%u\n\n", minResolution, maxResolution, actualResolution);
        OutputDebugString(line);
    }

    setFunction = (pSetTimerResolution)GetProcAddress(hLibrary, "NtSetTimerResolution");
    if (setFunction == NULL)
    {
        return;
    }

    status = setFunction(maxResolution, TRUE, &actualResolution);
    OutputDebugString(status == 0 ? L"Timer Resolution Set\n" : L"Timer Resolution Not Set\n");
}

static void unsetAccurateTimerResolution()
{
	timeEndPeriod(1);
}
#endif

const uint32_t CSpeedLimiter::m_DefaultSpeed = 60;

CSpeedLimiter::CSpeedLimiter() :
m_Frames(0),
m_Speed(m_DefaultSpeed),
m_BaseSpeed(m_DefaultSpeed)
{
#ifdef _WIN32
    setAccurateTimerResolution();
    m_Timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (!m_Timer)
        m_Timer = CreateWaitableTimer(nullptr, FALSE, nullptr);
#endif
}

CSpeedLimiter::~CSpeedLimiter()
{
#ifdef _WIN32
    CloseHandle(m_Timer);
    unsetAccurateTimerResolution();
#endif
}

void CSpeedLimiter::SetHertz(uint32_t Hertz)
{
    m_Speed = Hertz;
    m_BaseSpeed = Hertz;
    FixSpeedRatio();
}

void CSpeedLimiter::FixSpeedRatio()
{
    m_MicroSecondsPerFrame = 1000000 / m_Speed;
    m_Frames = 0;
}

struct RecordedFrame
{
    uint64_t calculatedTime;
    HighResTimeStamp lastTime;
    HighResTimeStamp currentTime;
    uint32_t lastFrames;
    char reset;
};

#define MS_RESET_TIME 70

bool CSpeedLimiter::Timer_Process(uint32_t * FrameRate)
{
    if (-2 == m_Frames)
    {
        m_Frames = -1;
        return false;
    }
    if (-1 == m_Frames)
    {
        m_Frames = 0;
        m_LastTime.SetToNow();
    }

    struct RecordedFrame record {};
    record.lastFrames = m_Frames++;
    record.lastTime = m_LastTime;
    HighResTimeStamp CurrentTime;
    CurrentTime.SetToNow();
    record.currentTime = CurrentTime;

    // Calculate time that should have elapsed for this frame
    uint64_t LastTime = m_LastTime.GetMicroSeconds(), CurrentTimeValue = CurrentTime.GetMicroSeconds();
    if (LastTime == 0)
    {
        m_Frames = 0;
        m_LastTime = CurrentTime;
        return true;
    }

    uint64_t CalculatedTime;
    record.calculatedTime = (CalculatedTime = LastTime + ((uint64_t) m_MicroSecondsPerFrame * m_Frames));
    bool reset = CurrentTimeValue - LastTime >= 1000000;

    if (CurrentTimeValue < CalculatedTime)
    {
        int32_t time = (int)(CalculatedTime - CurrentTimeValue);
        if (time > 0)
        {
#ifdef _WIN32
            LARGE_INTEGER left;
            // TODO: This is slightly inaccurate, use 100ns instead of us
            left.QuadPart = -10 * time;
            SetWaitableTimer(m_Timer, &left, 0, nullptr, nullptr, false);
            WaitForSingleObject(m_Timer, INFINITE);
#else
            pjutil::Sleep((time / 1000) + 1);
#endif
        }
        // Refresh current time
        CurrentTime.SetToNow();
        CurrentTimeValue = CurrentTime.GetMicroSeconds();
    }
    else
    {
        // this is a new code - if we are falling very behind, try to reset the timer
        uint64_t time = CurrentTimeValue - CalculatedTime;
        reset = time > MS_RESET_TIME * 1000;
    }

    record.reset = reset;

#ifdef DEBUG_ENABLE_TIMER_TRACING
    if (sFramesLeftToRecord)
    {
        sRecordedFrames.push_back(record);
        sFramesLeftToRecord--;
    }
    else if (!sRecordedFrames.empty())
    {
        FILE* fd = fopen(getRecordsPath(), "w");
        if (fd)
        {
            for (const auto& record : sRecordedFrames)
            {
                fprintf(fd, "calc=%lf cur=%ld diff=%lf time=%ld frames=%ld%s\n", record.calculatedTime, record.currentTime, record.calculatedTime - record.currentTime, record.lastTime, record.lastFrames, record.reset ? " reset" : "");
            }
            fclose(fd);
        }
        sRecordedFrames.clear();
    }
#endif

    if (reset)
    {
        // Output FPS
        if (FrameRate != nullptr) { *FrameRate = m_Frames; }
        m_Frames = 0;
        m_LastTime = CurrentTime;
        return true;
    }
    return false;
}

void CSpeedLimiter::AlterSpeed( const ESpeedChange SpeedChange )
{
	int32_t SpeedFactor = 1;
	if (SpeedChange == DECREASE_SPEED) { SpeedFactor = -1; }

	if (m_Speed >= m_DefaultSpeed)
	{
		m_Speed += 10 * SpeedFactor;
	}
	else if (m_Speed >= 15)
	{
		m_Speed += 5 * SpeedFactor;
	}
	else if ((m_Speed > 1 && SpeedChange == DECREASE_SPEED) || SpeedChange == INCREASE_SPEED)
	{
		m_Speed += 1 * SpeedFactor;
	}

	SpeedChanged(m_Speed);
	FixSpeedRatio();
}

void CSpeedLimiter::SetSpeed(int Speed)
{
    if (Speed < 1)
    {
        Speed = 1;
    }
    m_Speed = Speed;
    SpeedChanged(m_Speed);
    FixSpeedRatio();
}

int CSpeedLimiter::GetSpeed(void) const
{
    return m_Speed;
}

int CSpeedLimiter::GetBaseSpeed(void) const
{
    return m_BaseSpeed;
}

void CSpeedLimiter::AdjustTime(HighResTimeStamp diff)
{
    m_LastTime += diff;
}

void CSpeedLimiter::Reset()
{
    m_Frames = -2;
}
